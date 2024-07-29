#include "LedModule.h"

#include "OpenKNX.h"
#include "LedModuleConfig.h"

const std::string LedModule::name()
{
    return LEDMODULE_HARDWARE_NAME;
}

const std::string LedModule::version()
{
    // hides the module in the version output on the console, because the firmware version is sufficient.
    return "";
}

void LedModule::init()
{
#if defined(LEDMODULE_DIMMER_PCA9685)
    dimmer = new HWDimmerPCA(HWDimmerPCA::PCAType::PCA9685);
#else
#if defined(LEDMODULE_DIMMMER_RP2040)
    dimmer = new HWDimmerRP2040(dimPins, LEDMODULE_MAX_LIGHT_CHANNELS, LEDMODULE_PWM_FREQ);
#else // create dummy driver to have dimmer initialized
    dimmer = new HWDimmer(1);
    logErrorP("Unknown PWM driver %s ('RP2040' and 'PCA9685PW' are supported)", LEDMODULE_PWMDRIVER);
#endif
#endif
}

void LedModule::setup(bool configured)
{
    delay(1000);
    logInfoP("Setup0");
    logIndentUp();

    setupCustomFlash();

    setupChannels();

    logIndentDown();

    logInfoP("Switch off all light on boot");
    for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
    {
        dimmer->setLevel(0, i);
    }
    delay(1000);
}

void LedModule::setupChannels()
{
    memset(SC_HWChannels, 255, LED_SC_ChannelCount);
    memset(TW_HWChannels, 255, LED_TW_ChannelCount * 2);
    memset(RGB_HWChannels, 255, LED_RGB_ChannelCount * 3);

    delay(5000);
    for (uint8_t _channelIndex = 0; _channelIndex < LED_ChannelCount; _channelIndex++)
    {
        switch (ParamLED_CH_Lighttype)
        {
        case 1:
            SC_HWChannels[ParamLED_CH_SC_Light-1][0] = _channelIndex;
            break;

        case 2:
            if(ParamLED_CH_TW_Function == 1)
            {
                TW_HWChannels[ParamLED_CH_TW_Light-1][0] = _channelIndex;
            }
            else if (ParamLED_CH_TW_Function == 2)
            {
                TW_HWChannels[ParamLED_CH_TW_Light-1][1] = _channelIndex;
            }
            break;
            
        case 3:
            if(ParamLED_CH_RGB_Function == 1)
            {
                RGB_HWChannels[ParamLED_CH_RGB_Light-1][0] = _channelIndex;
            }
            else if (ParamLED_CH_RGB_Function == 2)
            {
                RGB_HWChannels[ParamLED_CH_RGB_Light-1][1] = _channelIndex;
            }
            else if (ParamLED_CH_RGB_Function == 3)
            {
                RGB_HWChannels[ParamLED_CH_RGB_Light-1][2] = _channelIndex;
            }
            break;
        
        default:
            break;
        }
    }
    
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if(ch < LED_SC_ChannelCount)
        {
            _singleChannels[ch] = new SingleChannel(ch, dimmer, SC_HWChannels[ch]);
        }
        if(ch < LED_TW_ChannelCount)
        {
            _twChannels[ch] = new TWChannel(ch, dimmer, TW_HWChannels[ch]);
        }
        if(ch < LED_RGB_ChannelCount)
        {
            _rgbChannels[ch] = new RGBChannel(ch, dimmer, RGB_HWChannels[ch]);
        }
    }

   logDebugP("CH\tTYPE\tSC\tTW\tFNC\tRGB\tFNC");
    for(int _channelIndex = 0; _channelIndex < LED_ChannelCount; _channelIndex++)
    {
        logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d",_channelIndex, ParamLED_CH_Lighttype, ParamLED_CH_SC_Light, ParamLED_CH_TW_Light, ParamLED_CH_TW_Function, ParamLED_CH_RGB_Light, ParamLED_CH_RGB_Function);
    }
    for(int i=0; i<LED_SC_ChannelCount; i++)
    {
        logDebugP("SC %d: %d", i, SC_HWChannels[i][0]);
    }
    for(int i=0; i<LED_TW_ChannelCount; i++)
    {
        logDebugP("TW %d: %d, %d", i, TW_HWChannels[i][0],TW_HWChannels[i][1]);
    }
    for(int i=0; i<LED_RGB_ChannelCount; i++)
    {
        logDebugP("RGB %d: %d, %d, %d", i, RGB_HWChannels[i][0], RGB_HWChannels[i][1], RGB_HWChannels[i][2]);
    }
}

void LedModule::setupCustomFlash()
{
    logDebugP("initialize ledModule flash");
    OpenKNX::Flash::Driver _ledStorage;
#ifdef ARDUINO_ARCH_ESP32
    _ledStorage.init("ledModule");
#else
    // TODO: Reactivate flash storage when values are know and functionality is needed
    // _ledStorage.init("ledModule", LEDMODULE_FLASH_OFFSET, LEDMODULE_FLASH_SIZE);
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
        logInfoP("Loop0");
        _timer1 = millis();
    }

    if (delayCheck(_timerCheckConnection, 500))
    {
        // If PWM side of the Adum1251 has no power and power returns, the PWM lib is not initialized
        if (!dimmer->checkConnection())
        {
            if (doResetPwm)
            {
                dimmer->reconnect();
                //lights->dimmToLastAfterI2cIsBack(); // TODO: Implement in PCA dimmer
            }
        }
        else
        {
            doResetPwm = true;
        }
        _timerCheckConnection = millis();
    }
    
    for(int i = 0; i < LED_SC_ChannelCount; i++)
    {
        _singleChannels[i]->loop();
    }
    for(int i = 0; i < LED_TW_ChannelCount; i++)
    {
        _twChannels[i]->loop();
    }
    for(int i = 0; i < LED_RGB_ChannelCount; i++)
    {
        _rgbChannels[i]->loop();
    }
    dimmer->dimLoop();
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

void LedModule::processInputKo(GroupObject &ko)
{
     logDebugP("processInputKo GA%04X", ko.asap());
     logHexDebugP(ko.valueRef(), ko.valueSize());

    uint16_t asap = ko.asap();
    uint16_t channelnumber = 0;
    
    if(asap >= LED_SC_KoBlockOffset && asap < LED_TW_KoBlockOffset)
    {
        channelnumber = (asap - LED_SC_KoBlockOffset) / LED_SC_KoBlockSize;
     logDebugP("SC %d", channelnumber);
        _singleChannels[channelnumber]->processInputKo(ko);
    }
    else if(asap >= LED_TW_KoBlockOffset && asap < LED_RGB_KoBlockOffset)
    {
        channelnumber = (asap - LED_TW_KoBlockOffset) / LED_TW_KoBlockSize;
     logDebugP("TW %d", channelnumber);
        _twChannels[channelnumber]->processInputKo(ko);
    }
    else if(asap >= LED_RGB_KoBlockOffset && asap < (LED_RGB_KoBlockOffset + LED_RGB_ChannelCount * LED_RGB_KoBlockSize))
    {
        channelnumber = (asap - LED_RGB_KoBlockOffset) / LED_RGB_KoBlockSize;
     logDebugP("RGB %d", channelnumber);
        _rgbChannels[channelnumber]->processInputKo(ko);
    }
}

void LedModule::showHelp()
{
    openknx.console.printHelpLine("ledModule", "Print a ledModule text");
}

bool LedModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 9) == "ledModule")
    {
        logInfoP("======================== Information ===========================================");
        logInfoP("LED MODULE INFORMATION");
        logInfoP("PWM driver:              %s", LEDMODULE_PWMDRIVER);
#ifdef LEDMODULE_DIMMER_PCA9685
        logInfoP("1Wire SDA:              %s", LEDMODULE_WIRE_SDA);
        logInfoP("1Wire SCL:              %s", LEDMODULE_WIRE_SCL);
#endif
#ifdef LEDMODULE_DIMMMER_RP2040
        std::string tmp = "Dimmer Pins: ";
        for (int i = 0; i < LEDMODULE_MAX_LIGHT_CHANNELS; i++)
        {
            tmp.append(std::to_string(dimPins[i]));
            tmp.append(", ");
        }
        logInfoP("%s", tmp.c_str());
#endif
        logIndentUp();
        logInfoP("Info 1");
        logInfoP("Info 2");
        logIndentUp();
        logInfoP("Info 2a");
        logInfoP("Info 2b");
        logIndentDown();
        logInfoP("Info 3");
        logIndentDown();
        logInfoP("--------------------------------------------------------------------------------");
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

LedModule openknxLedModule;