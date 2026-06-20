#include "LedModule.h"
#include "LedModuleConfig.h"
#include "OpenKNX.h"

#include "Colors.h"

const std::string LedModule::name() { return LEDMODULE_HARDWARE_NAME; }

const std::string LedModule::version()
{
    // hides the module in the version output on the console, because the firmware
    // version is sufficient.
    return "";
}

void LedModule::setup(bool configured)
{
    logInfoP("Init:");
    logIndentUp();

#if defined(LEDMODULE_DIMMER_PCA9685)
    logInfoP("LEDMODULE_DIMMER_PCA9685");
    _pDimmer = new HWDimmerPCA(HWDimmerPCA::PCAType::PCA9685, LEDMODULE_PCA_ADDR,
                               LEDMODULE_PCA_PWMFREQUENCY);
    logInfoP("LEDMODULE_DIMMER_PCA9685 SET");
#else
    #if defined(LEDMODULE_DIMMMER_RP2040)
    _pDimmer =
        new HWDimmerRP2040(dimPins, LEDMODULE_MAX_LIGHT_CHANNELS,
                           (ParamLED_PwmFrequency + 1) * PWM_FREQUENCY_FACTOR);
    logInfoP("LEDMODULE_DIMMER_RP2040");
    #else
        #if defined(LEDMODULE_DIMMMER_WS)
    _pDimmer = new HWDimmerWS(HWDimmerWS::WSType::WS2811, LEDMODULE_WS_PIN,
                              LEDMOPDULE_WS_NUM_LEDS);
    logInfoP("LEDMODULE_DIMMER_WS");
        #else // create dummy driver to have dimmer initialized
    dimmer = new HWDimmer(1);
    logErrorP("Unknown PWM driver %s ('RP2040' and 'PCA9685PW' are supported)",
              LEDMODULE_PWMDRIVER);
        #endif
    #endif
#endif
    logIndentDown();

    if (!configured)
    {
        logInfoP("Setup: not configured");
        return;
    }

    logInfoP("Setup0:");
    logIndentUp();
    setupCustomFlash();
    setupFrontPlate();
    setupTemperatureSensor();
    setupVoltageMeasurement();
    setupConstantCurrentMode();
    setupChannels();
    logIndentDown();
}

void LedModule::setupChannels()
{
    logDebugP("Setting up channels");
    logIndentUp();

    memset(_SC_HWChannels, 0xFF, LED_SC_ChannelCount);
    memset(_TW_HWChannels, 0xFF, LED_TW_ChannelCount * 2);
    memset(_RGB_HWChannels, 0xFF, LED_RGB_ChannelCount * 3);
    memset(_RGBW_HWChannels, 0xFF, LED_RGBW_ChannelCount * 4);
    memset(_RGBTW_HWChannels, 0xFF, LED_RGBTW_ChannelCount * 5);

    for (uint8_t _channelIndex = 0; _channelIndex < LED_ChannelCount;
         _channelIndex++)
    {
        switch (ParamLED_CH_Lighttype)
        {
            case LightType::Single:
                _SC_HWChannels[ParamLED_CH_SC_Light - 1][0] = _channelIndex;
                break;

            case LightType::TunableWhite:
                if (ParamLED_CH_TW_Function == 1)
                    _TW_HWChannels[ParamLED_CH_TW_Light - 1][1] = _channelIndex;
                else if (ParamLED_CH_TW_Function == 2)
                    _TW_HWChannels[ParamLED_CH_TW_Light - 1][0] = _channelIndex;
                break;

            case LightType::RGB:
                if (ParamLED_CH_RGB_Function == 1)
                    _RGB_HWChannels[ParamLED_CH_RGB_Light - 1][0] = _channelIndex;
                else if (ParamLED_CH_RGB_Function == 2)
                    _RGB_HWChannels[ParamLED_CH_RGB_Light - 1][1] = _channelIndex;
                else if (ParamLED_CH_RGB_Function == 3)
                    _RGB_HWChannels[ParamLED_CH_RGB_Light - 1][2] = _channelIndex;
                break;

            // TODO: Add other light type implementations here
            case LightType::RGBW:
                if (ParamLED_CH_RGBW_Function == 1)
                    _RGBW_HWChannels[ParamLED_CH_RGBW_Light - 1][0] = _channelIndex;
                else if (ParamLED_CH_RGBW_Function == 2)
                    _RGBW_HWChannels[ParamLED_CH_RGB_Light - 1][1] = _channelIndex;
                else if (ParamLED_CH_RGBW_Function == 3)
                    _RGBW_HWChannels[ParamLED_CH_RGBW_Light - 1][2] = _channelIndex;
                else if (ParamLED_CH_RGBW_Function == 4)
                    _RGBW_HWChannels[ParamLED_CH_RGBW_Light - 1][3] = _channelIndex;
                break;
            case LightType::RGBTW:
                if (ParamLED_CH_RGBTW_Function == 1)
                    _RGBTW_HWChannels[ParamLED_CH_RGBTW_Light - 1][0] = _channelIndex;
                else if (ParamLED_CH_RGBTW_Function == 2)
                    _RGBTW_HWChannels[ParamLED_CH_RGBTW_Light - 1][1] = _channelIndex;
                else if (ParamLED_CH_RGBTW_Function == 3)
                    _RGBTW_HWChannels[ParamLED_CH_RGBTW_Light - 1][2] = _channelIndex;
                else if (ParamLED_CH_RGBTW_Function == 4)
                    _RGBTW_HWChannels[ParamLED_CH_RGBTW_Light - 1][3] = _channelIndex;
                else if (ParamLED_CH_RGBTW_Function == 5)
                    _RGBTW_HWChannels[ParamLED_CH_RGBTW_Light - 1][4] = _channelIndex;
                break;
            case LightType::CentralObject:
                _CO_HWChannels[ParamLED_CH_CO_Light - 1][0] = _channelIndex;
                break;

            default:
                break;
        }
    }
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount)
        {
            _singleChannels[ch] = new SingleChannel(ch, _pDimmer, _SC_HWChannels[ch]);
            _CO_Active_SC[ch][0] = _singleChannels[ch]->getCO1();
            _CO_Active_SC[ch][1] = _singleChannels[ch]->getCO2();
            _CO_Active_SC[ch][2] = _singleChannels[ch]->getCO3();
        }
        if (ch < LED_TW_ChannelCount)
        {
            _twChannels[ch] = new TWChannel(ch, _pDimmer, _TW_HWChannels[ch]);
            _CO_Active_TW[ch][0] = _twChannels[ch]->getCO1();
            _CO_Active_TW[ch][1] = _twChannels[ch]->getCO2();
            _CO_Active_TW[ch][2] = _twChannels[ch]->getCO3();
        }
        if (ch < LED_RGB_ChannelCount)
        {
            _rgbChannels[ch] = new RGBChannel(ch, _pDimmer, _RGB_HWChannels[ch]);
            _CO_Active_RGB[ch][0] = _rgbChannels[ch]->getCO1();
            _CO_Active_RGB[ch][1] = _rgbChannels[ch]->getCO2();
            _CO_Active_RGB[ch][2] = _rgbChannels[ch]->getCO3();
        }
        if (ch < LED_RGBW_ChannelCount)
        {
            _rgbwChannels[ch] = new RGBWChannel(ch, _pDimmer, _RGBW_HWChannels[ch]);
            _CO_Active_RGBW[ch][0] = _rgbwChannels[ch]->getCO1();
            _CO_Active_RGBW[ch][1] = _rgbwChannels[ch]->getCO2();
            _CO_Active_RGBW[ch][2] = _rgbwChannels[ch]->getCO3();
        }
        if (ch < LED_RGBTW_ChannelCount)
        {
            _rgbtwChannels[ch] = new RGBTWChannel(ch, _pDimmer, _RGBTW_HWChannels[ch]);
            _CO_Active_RGBTW[ch][0] = _rgbtwChannels[ch]->getCO1();
            _CO_Active_RGBTW[ch][1] = _rgbtwChannels[ch]->getCO2();
            _CO_Active_RGBTW[ch][2] = _rgbtwChannels[ch]->getCO3();
        }
        if (ch < LED_CO_ChannelCount)
            _coChannels[ch] = new COChannel(ch, _pDimmer, _CO_HWChannels[ch]);
    }

    logDebugP("Channel setup finished.");
    logIndentDown();
}

void LedModule::setupTemperatureSensor()
{
    #ifdef OPENKNX_GPIO_WIRE
        #if OPENKNX_LED_TEMPSENS_WIRE == OPENKNX_GPIO_WIRE
            return;
        #endif
    #endif
    #ifdef OPENKNX_LED_TEMPSENS_WIRE
    OPENKNX_LED_TEMPSENS_WIRE.setSDA(OPENKNX_LED_TEMPSENS_PIN_SDA);
    OPENKNX_LED_TEMPSENS_WIRE.setSCL(OPENKNX_LED_TEMPSENS_PIN_SCL);
    OPENKNX_LED_TEMPSENS_WIRE.begin();
    _temperature.begin();
    #endif
    #ifdef OPENKNX_LED_TEMPSENS_TYPE_TMP100
    _temperature.setConfiguration(0x60);    // change to 12bit mode for compatibility with PTC2075 lib
    #endif
}

void LedModule::setupFrontPlate()
{
#ifdef LEDMODULE_FRONT_PLATE_USED
    logDebugP("Front plate used");
    for (uint8_t i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        openknx.gpio.pinMode(0x0100 + i, OUTPUT, true,
                             !OPENKNX_LED_GPIO_OUTPUT_ACTIVE_ON);
        openknx.gpio.pinMode(0x0200 + i, INPUT);
    }
#endif
}

void LedModule::setupVoltageMeasurement()
{
#ifdef LEDMODULE_VOLTAGE_MEASURE_PIN
    pinMode(LEDMODULE_VOLTAGE_MEASURE_PIN, INPUT);
    analogReadResolution(12);
    logDebugP("Voltage Measurement: PIN: %u, resolution: %u bits",
              LEDMODULE_VOLTAGE_MEASURE_PIN, 12);
#endif
}

void LedModule::setupConstantCurrentMode()
{
#ifdef LEDMODULE_DIM_TYPE_PIN
    pinMode(LEDMODULE_DIM_TYPE_PIN, OUTPUT);
    digitalWrite(LEDMODULE_DIM_TYPE_PIN,
                 ParamLED_DimmerCcType); // HIGH = PWM-only, LOW = hybrid
    logDebugP("Constant Current: PIN: %u, mode: %u", LEDMODULE_DIM_TYPE_PIN,
              ParamLED_DimmerCcType);
#endif
}

void LedModule::setupCustomFlash()
{
    logDebugP("initialize ledModule flash");
    OpenKNX::Flash::Driver _ledStorage;
#ifdef ARDUINO_ARCH_ESP32
    _ledStorage.init("ledModule");
#else
    // TODO: Reactivate flash storage when values are know and functionality is
    // needed _ledStorage.init("ledModule", LEDMODULE_FLASH_OFFSET,
    // LEDMODULE_FLASH_SIZE);
#endif

    // logTraceP("write ledModule data");
    // _ledStorage.writeByte(0, 0x11);
    // _ledStorage.writeWord(1, 0xFFFF);
    // _ledStorage.writeInt(3, 6666666);
    // _ledStorage.commit();

    // logDebugP("read ledModule data");
    // logIndentUp();
    // logHexDebugP(_ledStorage.flashAddress(), 7);
    // logDebugP("byte: %02X", _ledStorage.readByte(0)); // UINT8
    // logDebugP("word: %i", _ledStorage.readWord(1));   // UINT16
    // logDebugP("int: %i", _ledStorage.readInt(3));     // UINT32
    // logIndentDown();
}

void LedModule::loop(bool configured)
{
    if (delayCheck(_timer1, 5100))
    {
        logDebugP("Loop0");
        _timer1 = millis();
    }

    if (knx.configured())
    {
        if (delayCheck(_timerCheckConnection, 500))
        {
            // If PWM side of the Adum1251 has no power and power returns, the PWM lib
            // is not initialized
            if (!_pDimmer->checkConnection())
            {
                if (_doResetPwm)
                    _pDimmer->reconnect();
            }
            else
                _doResetPwm = true;

            _timerCheckConnection = millis();
        }

        for (size_t i = 0; i < LED_SC_ChannelCount; i++)
            _singleChannels[i]->loop();

        for (size_t i = 0; i < LED_TW_ChannelCount; i++)
            _twChannels[i]->loop();

        for (size_t i = 0; i < LED_RGB_ChannelCount; i++)
            _rgbChannels[i]->loop();

        for (size_t i = 0; i < LED_RGBW_ChannelCount; i++)
            _rgbwChannels[i]->loop();

        for (size_t i = 0; i < LED_RGBTW_ChannelCount; i++)
            _rgbtwChannels[i]->loop();

        _pDimmer->loop();

        float temperature = TEMPERATURE_INVALID;
#ifdef OPENKNX_LED_TEMPSENS_ADDR
        temperature = _temperature.getTemperature();
#elifdef LEDMODULE_CURRENT_ADDR
        temperature = _pDimmer->getTemperatureAvg();
#endif
        if (temperature > TEMPERATURE_INVALID)
            StatusOutput::sendValue<float>(
                KoLED_Temperature, DPT_Value_Temp, ParamLED_TemperatureChangeSend, false,
                temperature, _statusTemperature, ParamLED_TemperatureCyclicTimeMS,
                ParamLED_TemperatureMinChangePercent, ParamLED_TemperatureMinChangeAbsolute,
                STATUS_SEND_RATE_MS);

        float totalCurrent = _pDimmer->getTotalCurrent();
        StatusOutput::sendValue<float>(
            KoLED_TotalCurrent, DPT_Value_Electric_Current, ParamLED_TotalCurrentSend, false,
            totalCurrent, _statusTotalCurrent, ParamLED_TotalCurrentSendCyclicTimeMS,
            0, ParamLED_TotalCurrentSendMinChangeAbsolute, STATUS_SEND_RATE_MS);

        float totalPower = _pDimmer->getTotalPower();
        StatusOutput::sendValue<float>(
            KoLED_TotalPower, DPT_Value_Power, ParamLED_TotalPowerSend, false,
            totalPower, _statusTotalPower, ParamLED_TotalPowerSendCyclicTimeMS,
            0, ParamLED_TotalPowerSendMinChangeAbsolute, STATUS_SEND_RATE_MS);
    }
}

#ifdef OPENKNX_DUALCORE

void LedModule::setup1(bool configured)
{
    delay(1000);
    logInfoP("Setup1");
}

void LedModule::loop1(bool configured)
{
    if (delayCheck(_timer2, 7200))
    {
        logInfoP("Loop1");
        _timer2 = millis();
    }
}
#endif

void LedModule::processInputKo(GroupObject& ko)
{
    // logDebugP("proc.Ko GA%04X", ko.asap());
    // logHexDebugP(ko.valueRef(), ko.valueSize());

    uint16_t asap = ko.asap();
    uint16_t channelnumber = 0;

    if (asap >= LED_SC_KoBlockOffset && asap < LED_TW_KoBlockOffset)
    {
        channelnumber = (asap - LED_SC_KoBlockOffset) / LED_SC_KoBlockSize;
        logDebugP("SC %d", channelnumber);
        _singleChannels[channelnumber]->processInputKo(ko);
    }
    else if (asap >= LED_TW_KoBlockOffset && asap < LED_RGB_KoBlockOffset)
    {
        channelnumber = (asap - LED_TW_KoBlockOffset) / LED_TW_KoBlockSize;
        logDebugP("TW %d", channelnumber);
        _twChannels[channelnumber]->processInputKo(ko);
    }
    else if (asap >= LED_RGB_KoBlockOffset &&
             asap < (LED_RGB_KoBlockOffset +
                     LED_RGB_ChannelCount * LED_RGB_KoBlockSize))
    {
        channelnumber = (asap - LED_RGB_KoBlockOffset) / LED_RGB_KoBlockSize;
        logDebugP("RGB %d", channelnumber);
        _rgbChannels[channelnumber]->processInputKo(ko);
    }
    else if (asap >= LED_RGBW_KoBlockOffset &&
             asap < (LED_RGBW_KoBlockOffset +
                     LED_RGBW_ChannelCount * LED_RGBW_KoBlockSize))
    {
        channelnumber = (asap - LED_RGBW_KoBlockOffset) / LED_RGBW_KoBlockSize;
        logDebugP("RGBW %d", channelnumber);
        _rgbwChannels[channelnumber]->processInputKo(ko);
    }
    else if (asap >= LED_RGBTW_KoBlockOffset &&
             asap < (LED_RGBTW_KoBlockOffset +
                     LED_RGBTW_ChannelCount * LED_RGBTW_KoBlockSize))
    {
        channelnumber = (asap - LED_RGBTW_KoBlockOffset) / LED_RGBTW_KoBlockSize;
        logDebugP("RGBTW %d", channelnumber);
        _rgbtwChannels[channelnumber]->processInputKo(ko);
    }
    else if (asap >= LED_CO_KoBlockOffset &&
             asap < (LED_CO_KoBlockOffset +
                     LED_CO_ChannelCount * LED_CO_KoBlockSize))
    {
        channelnumber = (asap - LED_CO_KoBlockOffset) / LED_CO_KoBlockSize;
        logDebugP("CO %d", channelnumber);
        _coChannels[channelnumber]->processInputKo(ko);
    }
}

void LedModule::showHelp()
{
    openknx.console.printHelpLine("led info", "Print ledModule configuration");
    openknx.console.printHelpLine("led state", "Print ledModule status");
    openknx.console.printHelpLine("led lut", "Print LED LUT");
    openknx.console.printHelpLine("led test mode", "Simple hardware test mode");
}

bool LedModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 3) != "led")
        return false;

    if (cmd.length() == 8 && cmd.substr(4, 4) == "info")
    {
        logInfoP("======================== Information "
                 "===========================================");
        logIndentUp();
        logInfoP("LED MODULE INFORMATION");
        logInfoP("PWM driver:              %s", LEDMODULE_PWMDRIVER);
#ifdef LEDMODULE_DIMMER_PCA9685
        logInfoP("1Wire SDA:              %s", LEDMODULE_WIRE_SDA);
        logInfoP("1Wire SCL:              %s", LEDMODULE_WIRE_SCL);
#endif
#ifdef LEDMODULE_DIMMMER_RP2040
        std::string tmp = "RP2040 Dimming Pins: ";
        for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
        {
            tmp.append(std::to_string(dimPins[i]));
            tmp.append(", ");
        }
        logInfoP("%s", tmp.c_str());
#endif
        logInfoP("HW Channel configuration:");
        logInfoP("CH\tTYPE\tSC\tTW\tFunc\tRGB\tFunc\tRGBW\tFunc\tRGBTW\tFunc");
        for (int _channelIndex = 0; _channelIndex < LED_ChannelCount;
             _channelIndex++)
            logInfoP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", _channelIndex,
                     ParamLED_CH_Lighttype, ParamLED_CH_SC_Light,
                     ParamLED_CH_TW_Light, ParamLED_CH_TW_Function,
                     ParamLED_CH_RGB_Light, ParamLED_CH_RGB_Function,
                     ParamLED_CH_RGBW_Light, ParamLED_CH_RGBW_Function,
                     ParamLED_CH_RGBTW_Light, ParamLED_CH_RGBTW_Function);
        for (int i = 0; i < LED_SC_ChannelCount; i++)
            logInfoP("SC %d: CH: %d CO1: %d CO2: %d CO3: %d",
                     i, _SC_HWChannels[i][0],
                     _singleChannels[i]->getCO1(), _singleChannels[i]->getCO2(), _singleChannels[i]->getCO3());
        for (int i = 0; i < LED_TW_ChannelCount; i++)
            logInfoP("TW %d: Cold: %d, Warm: %d CO1: %d CO2: %d CO3: %d",
                     i, _TW_HWChannels[i][0], _TW_HWChannels[i][1],
                     _twChannels[i]->getCO1(), _twChannels[i]->getCO2(), _twChannels[i]->getCO3());
        for (int i = 0; i < LED_RGB_ChannelCount; i++)
            logInfoP("RGB %d: Red: %d, Green: %d, Blue: %d CO1: %d CO2: %d CO3: %d",
                     i, _RGB_HWChannels[i][0], _RGB_HWChannels[i][1], _RGB_HWChannels[i][2],
                     _rgbChannels[i]->getCO1(), _rgbChannels[i]->getCO2(), _rgbChannels[i]->getCO3());
        for (int i = 0; i < LED_RGBW_ChannelCount; i++)
            logInfoP("RGBW %d: Red: %d, Green: %d, Blue: %d, White: %d CO1: %d CO2: %d CO3: %d",
                     i, _RGBW_HWChannels[i][0], _RGBW_HWChannels[i][1], _RGBW_HWChannels[i][2], _RGBW_HWChannels[i][3],
                     _rgbwChannels[i]->getCO1(), _rgbwChannels[i]->getCO2(), _rgbwChannels[i]->getCO3());
        for (int i = 0; i < LED_RGBTW_ChannelCount; i++)
            logInfoP("RGBTW %d: Red: %d, Green: %d, Blue: %d, WarmWhite: %d, CoolWhite: %d CO1: %d CO2: %d CO3: %d",
                     i, _RGBTW_HWChannels[i][0], _RGBTW_HWChannels[i][1], _RGBTW_HWChannels[i][2], _RGBTW_HWChannels[i][3], _RGBTW_HWChannels[i][4],
                     _rgbtwChannels[i]->getCO1(), _rgbtwChannels[i]->getCO2(), _rgbtwChannels[i]->getCO3());
        logIndentDown();
        logInfoP("-----------------------------------------------------------------"
                 "---------------");
        return true;
    }

    if (cmd.length() == 9 && cmd.substr(4, 5) == "state")
    {
        logInfoP("======================== Information "
                 "===========================================");
        logInfoP("LED MODULE STATE INFORMATION");
        for (int i = 0; i < LED_ChannelCount; i++)
            logInfoP("CH%d: %d", i, _pDimmer->getLevel(i));
        logInfoP("-----------------------------------------------------------------"
                 "---------------");
        return true;
    }

    if (cmd.length() == 7 && cmd.substr(4, 3) == "lut")
    {
        logInfoP("======================== Information "
                 "===========================================");
        logInfoP("LED MODULE LUT INFORMATION");

        //_pDimmer->outputLUT();
        // HWDimmer::outputLUT();

        logInfoP("-----------------------------------------------------------------"
                 "---------------");
        return true;
    }

    if (cmd.length() == 13 && cmd.substr(4, 9) == "test mode")
    {
        openknx.watchdog.deactivate();
        _pDimmer->runTestMode();
        return true;
    }

    return false;
}

// If KNX power goes off, activate emergency light
void LedModule::savePower()
{
    for (uint8_t i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        // TODO_KPA: Move to channel
        //  uint8_t value = knx.paramByte(LED_SceneEmergency_LEDCH0 + i );
        //  pwm.setPWM(i, 0, value);
    }
}

SingleChannel* LedModule::getSingleChannel(uint8_t channel)
{
    if (channel >= LED_SC_ChannelCount)
    {
        logErrorP("Invalid single channel requested: %d", channel);
        return nullptr;
    }

    return _singleChannels[channel];
}

TWChannel* LedModule::getTWChannel(uint8_t channel)
{
    if (channel >= LED_TW_ChannelCount)
    {
        logErrorP("Invalid tunable white channel requested: %d", channel);
        return nullptr;
    }

    return _twChannels[channel];
}

RGBChannel* LedModule::getRGBChannel(uint8_t channel)
{
    if (channel >= LED_RGB_ChannelCount)
    {
        logErrorP("Invalid RGB channel requested: %d", channel);
        return nullptr;
    }

    return _rgbChannels[channel];
}

LedModule openknxLedModule;
