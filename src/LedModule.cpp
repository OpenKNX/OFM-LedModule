#include "LedModule.h"
#include "LedModuleConfig.h"
#include "OpenKNX.h"

#include "Colors.h"

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
    logDebugP("INIT");
#if defined(LEDMODULE_DIMMER_PCA9685)
    dimmer = new HWDimmerPCA(HWDimmerPCA::PCAType::PCA9685);
#else
#if defined(LEDMODULE_DIMMMER_RP2040)
    _pDimmer = new HWDimmerRP2040(dimPins, LEDMODULE_MAX_LIGHT_CHANNELS, LEDMODULE_PWM_FREQ);
#else // create dummy driver to have dimmer initialized
    dimmer = new HWDimmer(1);
    logErrorP("Unknown PWM driver %s ('RP2040' and 'PCA9685PW' are supported)", LEDMODULE_PWMDRIVER);
#endif
#endif

    delay(2500);

#if 1
    Colors::HSV tmpHSV;
    Colors::RGB tmpRGB;
    Colors::HSV tmpHSV2;
    Colors::RGB tmpRGB2;

    uint32_t tic, toc;
    int n = 1000000;
    tic = millis();
    for (int i = 0; i < n; i++)
    {
        tmpRGB = Colors::hsv2rgb(tmpHSV);
    }
    toc = millis();
    logDebugP("hsv2rgb: count: %d, time [ms]: %d, per calc [us]: %f", n, toc - tic, ((toc - tic) * 1000.0f) / n);

    tic = millis();
    for (int i = 0; i < n; i++)
    {
        tmpHSV2 = Colors::rgb2hsv(tmpRGB);
    }
    toc = millis();
    logDebugP("rgb2hsv: count: %d, time [ms]: %d, per calc [us]: %f", n, toc - tic, ((toc - tic) * 1000.0f) / n);

    delay(5000);
#endif
#if 0  
    int16_t errCount =0;

    for(uint16_t h=00; h<2048*4; h+=32)
    {
        logDebugP("h: %d", h);
        for(uint16_t s=0; s<512; s+=4)
        {
            for(uint16_t v=1; v<512; v+=4)
            {
                tmpHSV = Colors::HSV(h,s,v);
                tmpRGB = Colors::hsv2rgb(tmpHSV);
                tmpHSV2 = Colors::rgb2hsv(tmpRGB);
                tmpRGB2 = Colors::hsv2rgb(tmpHSV2);

                if(abs(tmpRGB.Red() - tmpRGB2.Red())>0 || abs(tmpRGB.Green() - tmpRGB2.Green())>0 || abs(tmpRGB.Blue() - tmpRGB2.Blue())>0)
                {
                    logDebugP("  ERR: R: %d, G: %d, B: %d vs R: %d, G: %d, B: %d", tmpRGB._red, tmpRGB._green, tmpRGB._blue, tmpRGB2._red, tmpRGB2._green,  tmpRGB2._blue);
                    logDebugP("  ERR: H: %d, S: %d, V: %d vs H: %d, S: %d, V: %d", tmpHSV._hue, tmpHSV._sat,tmpHSV._val,tmpHSV2._hue, tmpHSV2._sat, tmpHSV2._val  );
                    
                    errCount++;
                }
            }
        }
    }
    logDebugP("Finished HSV with %d errors!", errCount);

errCount =0;
    
    for(uint16_t r=00; r<257; r++)
    {
        logDebugP("r: %d", r);
        for(uint16_t g=00; g<257; g++)
        {
            for(uint16_t b=00; b<257; b++)
            {
                tmpRGB = Colors::RGB(r<<2,g<<2,b<<2);
                tmpHSV = Colors::rgb2hsv(tmpRGB);
                tmpRGB2 = Colors::hsv2rgb(tmpHSV);
                tmpHSV2 = Colors::rgb2hsv(tmpRGB2);

                if(tmpRGB.Red() != tmpRGB2.Red() || tmpRGB.Green() != tmpRGB2.Green() || tmpRGB.Blue() != tmpRGB2.Blue())
                {
                    logDebugP("* ERR: R: %d, G: %d, B: %d vs R: %d, G: %d, B: %d", tmpRGB._red, tmpRGB._green, tmpRGB._blue, tmpRGB2._red, tmpRGB2._green,  tmpRGB2._blue);
                    logDebugP("* ERR: H: %d, S: %d, V: %d vs H: %d, S: %d, V: %d", tmpHSV._hue, tmpHSV._sat,tmpHSV._val,tmpHSV2._hue, tmpHSV2._sat, tmpHSV2._val  );
                    
                    errCount++;
                }
              /*  if(tmpHSV.Hue()!=tmpHSV2.Hue() || tmpHSV.Sat() != tmpHSV2.Sat() || tmpHSV.Val() != tmpHSV2.Val())
                {
                    logDebugP("^ ERR: R: %d, G: %d, B: %d vs R: %d, G: %d, B: %d", tmpRGB._red, tmpRGB._green, tmpRGB._blue, tmpRGB2._red, tmpRGB2._green,  tmpRGB2._blue);
                    logDebugP("^ ERR: H: %d, S: %d, V: %d vs H: %d, S: %d, V: %d", tmpHSV._hue, tmpHSV._sat,tmpHSV._val,tmpHSV2._hue, tmpHSV2._sat, tmpHSV2._val  );
                    errCount++;
                }*/
            }
        }
    }
    logDebugP("Finished RGBwith %d errors!", errCount);

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
}

void LedModule::setupChannels()
{
    memset(_SC_HWChannels, 0xFF, LED_SC_ChannelCount);
    memset(_TW_HWChannels, 0xFF, LED_TW_ChannelCount * 2);
    memset(_RGB_HWChannels, 0xFF, LED_RGB_ChannelCount * 3);

    for (uint8_t _channelIndex = 0; _channelIndex < LED_ChannelCount; _channelIndex++)
    {
        switch (ParamLED_CH_Lighttype)
        {
            case LightType::Single:
                _SC_HWChannels[ParamLED_CH_SC_Light - 1][0] = _channelIndex;
                break;

            case LightType::TunableWhite:
                if (ParamLED_CH_TW_Function == 1)
                {
                    _TW_HWChannels[ParamLED_CH_TW_Light - 1][0] = _channelIndex;
                }
                else if (ParamLED_CH_TW_Function == 2)
                {
                    _TW_HWChannels[ParamLED_CH_TW_Light - 1][1] = _channelIndex;
                }
                break;

            case LightType::RGB:
                if (ParamLED_CH_RGB_Function == 1)
                {
                    _RGB_HWChannels[ParamLED_CH_RGB_Light - 1][0] = _channelIndex;
                }
                else if (ParamLED_CH_RGB_Function == 2)
                {
                    _RGB_HWChannels[ParamLED_CH_RGB_Light - 1][1] = _channelIndex;
                }
                else if (ParamLED_CH_RGB_Function == 3)
                {
                    _RGB_HWChannels[ParamLED_CH_RGB_Light - 1][2] = _channelIndex;
                }
                break;

            // TODO: Add other light type implementations here
            case LightType::RGBW:
            case LightType::RGBTW:
            default:
                break;
        }
    }
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount)
        {
            _singleChannels[ch] = new SingleChannel(ch, _pDimmer, _SC_HWChannels[ch]);
        }
        if (ch < LED_TW_ChannelCount)
        {
            _twChannels[ch] = new TWChannel(ch, _pDimmer, _TW_HWChannels[ch]);
        }
        if (ch < LED_RGB_ChannelCount)
        {
            _rgbChannels[ch] = new RGBChannel(ch, _pDimmer, _RGB_HWChannels[ch]);
        }
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
        if (!_pDimmer->checkConnection())
        {
            if (doResetPwm)
            {
                _pDimmer->reconnect();
            }
        }
        else
        {
            doResetPwm = true;
        }
        _timerCheckConnection = millis();
    }

    if (knx.configured())
    {
        for (int i = 0; i < LED_SC_ChannelCount; i++)
        {
            _singleChannels[i]->loop();
        }
        for (int i = 0; i < LED_TW_ChannelCount; i++)
        {
            _twChannels[i]->loop();
        }
        for (int i = 0; i < LED_RGB_ChannelCount; i++)
        {
            _rgbChannels[i]->loop();
        }
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

void LedModule::processInputKo(GroupObject &ko)
{
    logDebugP("proc.Ko GA%04X", ko.asap());
    logHexDebugP(ko.valueRef(), ko.valueSize());

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
    else if (asap >= LED_RGB_KoBlockOffset && asap < (LED_RGB_KoBlockOffset + LED_RGB_ChannelCount * LED_RGB_KoBlockSize))
    {
        channelnumber = (asap - LED_RGB_KoBlockOffset) / LED_RGB_KoBlockSize;
        logDebugP("RGB %d", channelnumber);
        _rgbChannels[channelnumber]->processInputKo(ko);
    }
}

void LedModule::showHelp()
{
    openknx.console.printHelpLine("ledModule", "Print ledModule configuration");
}

bool LedModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 9) == "ledModule")
    {
        logInfoP("======================== Information ===========================================");
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
        logInfoP("CH\tTYPE\tSC\tTW\tFunc\tRGB\tFunc");
        for (int _channelIndex = 0; _channelIndex < LED_ChannelCount; _channelIndex++)
        {
            logInfoP("%d\t%d\t%d\t%d\t%d\t%d\t%d", _channelIndex, ParamLED_CH_Lighttype, ParamLED_CH_SC_Light, ParamLED_CH_TW_Light, ParamLED_CH_TW_Function, ParamLED_CH_RGB_Light, ParamLED_CH_RGB_Function);
        }
        for (int i = 0; i < LED_SC_ChannelCount; i++)
        {
            logInfoP("SC %d: CH: %d", i, _SC_HWChannels[i][0]);
        }
        for (int i = 0; i < LED_TW_ChannelCount; i++)
        {
            logInfoP("TW %d: Cold: %d, Warm: %d", i, _TW_HWChannels[i][0], _TW_HWChannels[i][1]);
        }
        for (int i = 0; i < LED_RGB_ChannelCount; i++)
        {
            logInfoP("RGB %d: Red: %d, Green: %d, Blue: %d", i, _RGB_HWChannels[i][0], _RGB_HWChannels[i][1], _RGB_HWChannels[i][2]);
        }
        logIndentDown();
        logInfoP("--------------------------------------------------------------------------------");
        return true;
    }
    
    if (cmd.substr(0, 8) == "ledState")
    {
        for(int i=0; i<LED_ChannelCount; i++)
        {
           logDebugP("CH%d: %d",i, _pDimmer->getLevel(i));
        }
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