#include "HWDimmer.h"

/**
 * @brief Construct a new HWDimmer::HWDimmer object
 *
 * @param numChannels
 */
HWDimmer::HWDimmer(uint8_t numChannels)
{
    this->numChannels = numChannels;
    levels = new uint16_t[numChannels]{0};

#ifdef LEDMODULE_CURRENT_ADDR
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        _currentSense[i] = new INA238(currentInaAddr[i], &OPENKNX_GPIO_WIRE);

        if (_currentSense[i]->begin())
        {
            logDebugP("KNX INA238 setup done with address %u", currentInaAddr[i]);

            _currentSense[i]->setMaxCurrentShunt(3, LEDMODULE_INA_SHUNT);
            _currentSense[i]->setAverage(INA238_16_SAMPLES);
            _currentSense[i]->setMode(INA238_MODE_CONT_TEMP_BUS_SHUNT);
        }
        else
            logDebugP("KNX INA238 not found at address %u", currentInaAddr[i]);
    }
#endif
}

/**
 * @brief Destroy the HWDimmer::HWDimmer object
 *
 */
HWDimmer::~HWDimmer()
{
    delete[] levels;
#ifdef LEDMODULE_CURRENT_ADDR
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        if (_currentSense[i] != nullptr)
            delete _currentSense[i];
    }
#endif
}

/**
 * @brief Loop function to update status output on a regular basis
 *
 */
void HWDimmer::loop()
{
    checkPowerSupply();

    processCurrentSense();

    processFrontInput();
    processFrontOutput();
}

void HWDimmer::checkPowerSupply()
{
#ifdef LEDMODULE_VOLTAGE_MEASURE_PIN
    int voltageAnalog = analogRead(LEDMODULE_VOLTAGE_MEASURE_PIN);
    _powerSupplyVoltage = (float)voltageAnalog / 4095 * (float)3.3 * (float)LEDMODULE_VOLTAGE_MEASURE_FACTOR;

    // Supply voltage is a continuously measured value (never in a fade); scale 1000 keeps
    // millivolt resolution so the absolute threshold (in mV) and percent threshold apply directly.
    StatusOutput::sendValue<float>(KoLED_PowerSupplyVoltage, DPT_Value_Electric_Potential,
                                   ParamLED_PowerSupplyVoltageChangeSend, false, _powerSupplyVoltage,
                                   _statusVoltage, ParamLED_PowerSupplyVoltageCyclicTimeMS,
                                   ParamLED_PowerSupplyVoltageMinChangePercent, ParamLED_PowerSupplyVoltageMinChangeAbsolute,
                                   STATUS_SEND_RATE_MS, true, 1000.0f);
#endif

    if (!ParamLED_PowerSupplyRelay)
        return;

#ifndef LEDMODULE_VOLTAGE_MEASURE_PIN
    // without voltage measurement the relay status is the only indicator: read it once after startup,
    // so a relay that was already switched on before the dimmer started is known
    if (!_powerSupplyStatusRead && openknx.afterStartupDelay())
    {
        KoLED_PowerSupplyRelayStatus.requestObjectRead();
        _powerSupplyStatusRead = true;
    }
#endif

    bool powerNeeded = false;
    for (int i = 0; i < numChannels; i++)
    {
        if (getLevel(i) > 0 || _currentManualMode[i])
        {
            powerNeeded = true;
            break;
        }
    }

    // switch off only once (not repeated, so a failed actor or missing status does not flood the bus),
    // and only if the relay was requested by the dimmer or is switched on
    if (powerNeeded || _powerSupplyRequest == PowerSupplyRequest::Off ||
        (_powerSupplyRequest == PowerSupplyRequest::Unknown && !powerSupplyAvailable()))
    {
        _powerShutdownTimer = 0;
        return;
    }

    if (_powerShutdownTimer == 0)
        _powerShutdownTimer = delayTimerInit();
    else if (delayCheck(_powerShutdownTimer, ParamLED_PowerSupplyRelayOffDelayTimeMS) &&
             delayCheck(_powerSupplyLastRequest, POWER_SUPPLY_REQUEST_DELAY))
    {
        KoLED_PowerSupplyRelay.value(false, DPT_Switch);
        _powerSupplyRequest = PowerSupplyRequest::Off;
        _powerSupplyLastRequest = delayTimerInit();
        _powerShutdownTimer = 0;
    }
}

/**
 * @brief Power supply is available: measured voltage reached the configured minimum or, without
 * voltage measurement, the relay status is on
 */
bool HWDimmer::powerSupplyAvailable()
{
#ifdef LEDMODULE_VOLTAGE_MEASURE_PIN
    return _powerSupplyVoltage >= ParamLED_PowerSupplyRelayMinVoltage;
#else
    return KoLED_PowerSupplyRelayStatus.value(DPT_Switch);
#endif
}

void HWDimmer::processCurrentSense()
{
#ifdef LEDMODULE_CURRENT_ADDR
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        if (_currentSense[i] != nullptr)
        {
            _currentValues[i] = _currentSense[i]->getCurrent();
            _voltageValues[i] = _currentSense[i]->getBusVoltage();
            _temperatureValues[i] = _currentSense[i]->getTemperature();
        }
    }

    if (delayCheck(_currentSenseDebugTimer, 1000))
    {
        logDebugP("Channel %d: %.2f mA, Voltage: %.2f V, Temperature: %.2f °C", 0, _currentValues[0] / 1000.0f, _voltageValues[0], _temperatureValues[0]);
        _currentSenseDebugTimer = delayTimerInit();
    }
#endif
}

float HWDimmer::getTemperature(uint8_t channel)
{
#ifdef LEDMODULE_CURRENT_ADDR
    return _temperatureValues[channel];
#else
    return 0.0f;
#endif
}

float HWDimmer::getTemperatureAvg()
{
#ifdef LEDMODULE_CURRENT_ADDR
    float totalTemp = 0.0f;
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
        totalTemp += _temperatureValues[i];

    return totalTemp / LEDMODULE_MAX_LIGHT_CHANNELS;
#else
    return 0.0f;
#endif
}

float HWDimmer::getCurrent(uint8_t channel)
{
#ifdef LEDMODULE_CURRENT_ADDR
    return _currentValues[channel];
#else
    return 0.0f;
#endif
}

float HWDimmer::getTotalCurrent()
{
#ifdef LEDMODULE_CURRENT_ADDR
    float totalCurrent = 0.0f;
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
        totalCurrent += _currentValues[i];

    return totalCurrent;
#else
    return 0.0f;
#endif
}

float HWDimmer::getVoltage(uint8_t channel)
{
#ifdef LEDMODULE_CURRENT_ADDR
    return _voltageValues[channel];
#else
    return 0.0f;
#endif
}

float HWDimmer::getTotalPower()
{
#ifdef LEDMODULE_CURRENT_ADDR
    float totalPower = 0.0f;
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
        totalPower += _currentValues[i] * _voltageValues[i];

    return totalPower;
#else
    return 0.0f;
#endif
}

bool HWDimmer::powerSupplyAvailableOrRequest()
{
    if (!ParamLED_PowerSupplyRelay)
        return true;

    _powerShutdownTimer = 0;

    // after an off request the voltage/status may not have dropped yet, so the relay is requested again
    if (_powerSupplyRequest != PowerSupplyRequest::Off && powerSupplyAvailable())
    {
        // relay is on (requested or switched on externally), so switch it off after the off delay later on
        _powerSupplyRequest = PowerSupplyRequest::On;
        return true;
    }

    // new power demand: limited number of requests, so a failed actor does not flood the bus
    if (_powerSupplyRequest != PowerSupplyRequest::On)
    {
        _powerSupplyRequest = PowerSupplyRequest::On;
        _powerSupplyRequestCount = 0;
    }

    if (_powerSupplyRequestCount < POWER_SUPPLY_REQUEST_COUNT &&
        (_powerSupplyRequestCount == 0 || delayCheck(_powerSupplyLastRequest, POWER_SUPPLY_REQUEST_DELAY)))
    {
        KoLED_PowerSupplyRelay.value(true, DPT_Switch);
        _powerSupplyRequestCount++;
        _powerSupplyLastRequest = delayTimerInit();
    }

#ifdef LEDMODULE_VOLTAGE_MEASURE_PIN
    // keep waiting without further requests: dimming starts as soon as the voltage is present
    return false;
#else
    // no status received after all requests: dim anyway, as the relay might have switched without
    // sending its status, and nothing else could tell otherwise
    return _powerSupplyRequestCount >= POWER_SUPPLY_REQUEST_COUNT &&
           delayCheck(_powerSupplyLastRequest, POWER_SUPPLY_REQUEST_DELAY);
#endif
}

void HWDimmer::processFrontInput()
{
#ifdef LEDMODULE_FRONT_PLATE_USED
    if (!ParamLED_FrontControlInput)
        return;

    for (int i = 0; i < numChannels; i++)
    {
        bool buttonPressed = openknx.gpio.digitalRead(OPENKNX_LED_GPIO_INPUT_OFFSET + i) == OPENKNX_LED_GPIO_INPUT_ACTIVE_ON;

        if (buttonPressed &&
            delayCheck(_currentManualModeLastChange[i], FRONT_INPUT_DEBOUNCE))
        {
            _currentManualMode[i] = !_currentManualMode[i];
            _currentManualModeLastChange[i] = delayTimerInit();
            logDebugP("processFrontInput: manual mode button toggle (_currentManualMode[%u]: %u)", i, _currentManualMode[i]);

            if (_currentManualMode[i])
                powerSupplyAvailableOrRequest();

            setLevelInternal(getLevel(i), i);
        }
    }
#endif
}

void HWDimmer::processFrontOutput()
{
#ifdef LEDMODULE_FRONT_PLATE_USED
    if (!ParamLED_FrontControlOutput)
        return;

    for (int i = 0; i < numChannels; i++)
    {
        bool ledOn = getLevel(i) > 0 ? true : false;
        openknx.gpio.digitalWrite(OPENKNX_LED_GPIO_OUTPUT_OFFSET + i, ledOn ? OPENKNX_LED_GPIO_OUTPUT_ACTIVE_ON : !OPENKNX_LED_GPIO_OUTPUT_ACTIVE_ON);
    }
#endif
}

/**
 * @brief Sets level of selected channel to value
 *
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmer::setLevel(uint16_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if (updateLevelValue(level, channel))
        setLevelInternal(getLevel(channel), channel);
    
    return isValidChannel;
}

/**
 * @brief Updates level value of selected channel to value
 *
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmer::updateLevelValue(uint16_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if (channel < numChannels)
    {
        levels[channel] = level;
        isValidChannel = true;
    }
    else
    {
        logErrorP("Invalid channel: %d", channel);
    }
    return isValidChannel;
}

/**
 * @brief Get level of selected channel to value
 *
 * @param channel selected channel
 * @return uint8_t current level
 */
uint16_t HWDimmer::getLevel(uint8_t channel)
{
    uint16_t tmpLevel = 0;
    if (channel < numChannels)
    {
        if (_currentManualMode[channel])
            tmpLevel = scale(LedBrightness::parameterToLevel(ParamLED_FrontControlManualBrightness), Linear);
        else
            tmpLevel = levels[channel];
    }

    return tmpLevel;
}

/**
 * @brief Prefix of this module when using OpenKNX log functions
 *
 * @return std::string Prefix
 */
std::string HWDimmer::logPrefix()
{
    return "HWDimmer";
}