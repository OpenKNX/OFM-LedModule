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
        _currentSense[i] = Adafruit_INA238();

        if (_currentSense[i].begin(currentInaAddr[i], &OPENKNX_GPIO_WIRE))
        {
            logDebugP("KNX INA238 setup done with address %u", currentInaAddr[i]);

            _currentSense[i].setShunt(LEDMODULE_INA_SHUNT, 3);
            _currentSense[i].setAveragingCount(INA2XX_COUNT_16);
            _currentSense[i].setMode(INA2XX_MODE_CONTINUOUS);
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
    _powerSupplyVoltage = (float)voltageAnalog / 4095 * (float)3.3 * (float)LEDMODULE_VOLTAGE_MEASURE_FACTOR * 1000; // mV

    if (ParamLED_PowerSupplyVoltageChangeSend)
    {
        float voltageDifference = abs(_lastVoltageSent - _powerSupplyVoltage);
        if (voltageDifference > 0)
        {
            if (voltageDifference >= _lastVoltageSent * ParamLED_PowerSupplyVoltageMinChangePercent / 100.0f &&
                voltageDifference >= ParamLED_PowerSupplyVoltageMinChangeAbsolute)
            {
                KoLED_PowerSupplyVoltage.value(_powerSupplyVoltage, DPT_Value_Volt);
                _lastVoltageSent = _powerSupplyVoltage;
            }
            else
                KoLED_PowerSupplyVoltage.valueNoSend(_powerSupplyVoltage, DPT_Value_Volt);
        }

        if (ParamLED_PowerSupplyVoltageCyclicTimeMS > 0 && delayCheck(_voltageSendTimer, ParamLED_PowerSupplyVoltageCyclicTimeMS))
        {
            KoLED_PowerSupplyVoltage.objectWritten();
            _lastVoltageSent = _powerSupplyVoltage;
            _voltageSendTimer = delayTimerInit();
        }
    }

    if (KoLED_PowerSupplyRelayStatus.value(DPT_Switch))
    {
        bool powerNeeded = false;
        for (int i = 0; i < numChannels; i++)
        {
            if (getLevel(i) > 0)
            {
                powerNeeded = true;
                break;
            }
        }

        if (powerNeeded)
            _powerShutdownTimer = 0;
        else
        {
            if (_powerShutdownTimer == 0)
                _powerShutdownTimer = delayTimerInit();
            else if (delayCheck(_powerShutdownTimer, ParamLED_PowerSupplyRelayOffDelayTimeMS))
            {
                if (delayCheck(_powerSupplyLastRequest, POWER_SUPPLY_REQUEST_DELAY))
                {
                    KoLED_PowerSupplyRelay.value(false, DPT_Switch);
                    _powerSupplyLastRequest = delayTimerInit();
                    _powerShutdownTimer = 0;
                }
            }
        }
    }
#endif
}

void HWDimmer::processCurrentSense()
{
#ifdef LEDMODULE_CURRENT_ADDR
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        _currentValues[i] = _currentSense[i].getCurrent_mA();
        _voltageValues[i] = _currentSense[i].getBusVoltage_V();
        _temperatureValues[i] = _currentSense[i].readDieTemp();

        // if (delayCheck(_currentSenseDebugTimer, 1000))
        //     logDebugP("Channel %d: %.2f mA, Voltage: %.2f V", i, _currentSenseValues[i], _currentVoltageValues[i]);
    }

    if (delayCheck(_currentSenseDebugTimer, 1000))
    {
        logDebugP("Channel %d: %.2f mA, Voltage: %.2f V, Temperature: %.2f °C", 0, _currentValues[0], _voltageValues[0], _temperatureValues[0]);
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

float HWDimmer::getVoltage(uint8_t channel)
{
#ifdef LEDMODULE_CURRENT_ADDR
    return _voltageValues[channel];
#else
    return 0.0f;
#endif
}

bool HWDimmer::powerSupplyAvailableOrRequest()
{
    if (!ParamLED_PowerSupplyRelay)
        return true;

    if (!KoLED_PowerSupplyRelayStatus.value(DPT_Switch))
    {
        if (delayCheck(_powerSupplyLastRequest, POWER_SUPPLY_REQUEST_DELAY))
        {
            KoLED_PowerSupplyRelay.value(true, DPT_Switch);
            _powerSupplyLastRequest = delayTimerInit();
        }
    }

    _powerShutdownTimer = 0;

    bool available = _powerSupplyVoltage >= ParamLED_PowerSupplyRelayMinVoltage * 1000;
    return available && KoLED_PowerSupplyRelayStatus.value(DPT_Switch);
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

            setLevel(levels[i], i);
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
 * @brief Set level of selected channel to value
 *
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmer::setLevelInternal(uint16_t level, uint8_t channel)
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
            tmpLevel = ParamLED_FrontControlManualBrightness_ * VALUE_KNX_MULTIPLY;
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