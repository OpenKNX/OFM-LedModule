#include "RGBChannel.h"
#include <Math.h>
#include <knx.h>

RGBChannel::RGBChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[3])
    : LightChannel(index, pDimmer, hwChannels, 3), _hue(0, 0, H_PART - 1), _saturation(0, 0, VAL_RANGE)
{
    logInfoP("Trying to read Config from KNX...");
    logHexInfoP((uint8_t*)knx.paramData(LED_RGB_ParamCalcIndex(LED_RGB_SceneA_Type_)), 8);
    _scenes = new SceneConfig[N_SCENES];
    memcpy(_scenes, knx.paramData(LED_RGB_ParamCalcIndex(LED_RGB_SceneA_Type_)), N_SCENES * sizeof(SceneConfig));

#ifdef EXT_DEBUG_LOG
    logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
    for (int i = 0; i < N_SCENES; i++)
    {
        logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", _channelIndex, _scenes[i].sceneNr, _scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed, _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2]);
    }
#endif
}

const std::string RGBChannel::name()
{
    return "RGBChannel";
}

void RGBChannel::update()
{
    uint16_t tmpHue = _hue.value();
    uint16_t tmpSat = _saturation.value();
    uint8_t tmpBright = _brightness.value();
    Colors::HSV hsv(tmpHue, tmpSat, _UFP16(tmpBright, 2));

    if (_lastBrightnessLevel != tmpBright)
    {
        logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBright);
        _lastBrightnessLevel = tmpBright;
        KoLED_RGB_BrightnessStatus_.value(tmpBright, DPT_Percent_U8);
        KoLED_RGB_StateOnOff_.value(tmpBright > 0, DPT_State);
        KoLED_RGB_HSVStatus_.value(hsv.toUint32(), DPT_Colour_RGB);
        KoLED_RGB_RGBStatus_.value(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
    }
    else if (_lastHueValue != tmpHue || _lastSatValue != tmpSat)
    {
        logDebugP("update: Hue: %d -> %d Sat: %d -> %d", _lastHueValue, tmpHue, _lastSatValue, tmpSat);
        _lastHueValue = tmpHue;
        _lastSatValue = tmpSat;
        KoLED_RGB_HSVStatus_.value(hsv.toUint32(), DPT_Colour_RGB);
        KoLED_RGB_RGBStatus_.value(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
    }
}

void RGBChannel::loop()
{
    LightChannel::loop();

    if (delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
    {
        _lastDimTimestamp = millis();

        Colors::RGB rgb = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp)) << 2));

        if (_pHWChannels[0] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Red(), HWDimmer::DimLUTType::Log1_5), _pHWChannels[0]);
        }
        if (_pHWChannels[1] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Green(), HWDimmer::DimLUTType::Log1_5), _pHWChannels[1]);
        }
        if (_pHWChannels[2] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Blue(), HWDimmer::DimLUTType::Log1_5), _pHWChannels[2]);
        }
    }
    // Stairway Timeout
    /*
    if (((getStairTime() + (ParamLED_RGB_StairCaseTimer_ * 1000)) <= millis()) && getStairTrigger())
    {
        setStairTrigger(0);
        if (!RGB_night())
        {
            _brightness.setTargetValue(0, millis(), ParamLED_RGB_LightDimmTimeDayOFF_);
        }
        else if (RGB_night())
        {
            _brightness.setTargetValue(0, millis(), ParamLED_RGB_LightDimmTimeNightOFF_);
        }
    }*/
    // Trigger RGB Change
    /*    if (_brightness.getRGBChangingTrigger() && !RGB_night() && (_brightness.getRGBChangingTime() + ParamLED_RGB_ColorTimeDay_) <= millis())
        {
            logInfoP("hu:%5X%", _hue.value());
            _brightness.setRGBChangingTime(millis());
            _hue.setTargetValue(random(0x3FFF), millis(), ParamLED_RGB_ColorTimeDay_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_ColorTimeDay_);
        }
        if (_brightness.getRGBChangingTrigger() && RGB_night() && (_brightness.getRGBChangingTime() + ParamLED_RGB_ColorTimeNight_) <= millis())
        {
            _brightness.setRGBChangingTime(millis());
            _hue.setTargetValue(random(0x3FFF), millis(), ParamLED_RGB_ColorTimeNight_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_ColorTimeNight_);
            logInfoP("hue_val:%5X%", _hue.value());
        }*/
}

bool RGBChannel::RGB_night()
{
    return _rgb_night;
}

void RGBChannel::processInputKo(GroupObject& ko)
{
    Colors::HSV hsv;
    Colors::RGB rgb;
    int16_t relKO = (ko.asap() - LED_RGB_KoOffset);

    logDebugP("processInputKo Channel");
    logHexDebugP(ko.valueRef(), ko.valueSize());
    // check if channel is valid
    if ((int8_t)(relKO / LED_RGB_KoBlockSize) == channelIndex())
    {
        relKO = relKO % LED_RGB_KoBlockSize;
    }
    else
    {
        relKO = -1;
    }

    if (relKO == LED_RGB_KoLocking_)
    {
        _isLocked = ko.value(DPT_Switch);
    }
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_RGB_KoSwitch_:
                setSwitch(ko.value(DPT_Switch));
                break;

            case LED_RGB_KoStateOnOff_:
                break;
                
            case LED_RGB_KoLocking_:
                break;

            case LED_RGB_KoBrightness_:
                setBrightness(ko.value(DPT_Percent_U8));
                break;

            case LED_RGB_KoBrightnessStatus_:
                int16_t tmpu16;
                tmpu16 = *KoLED_RGB_DimRel_.valueRef();

                if (tmpu16 >= 0x09)
                {
                    relDimUp();
                }
                if (tmpu16 > 0x00 && tmpu16 < 0x08)
                {
                    relDimDown();
                }
                if (tmpu16 == 0x00 || tmpu16 == 0x08)
                {
                    relDimStop();
                }
                break;

            case LED_RGB_KoScene_:
                handleScene(ko.value(DPT_SceneNumber));
                break;
            case LED_RGB_KoSceneStatus_:
                break;

            case LED_RGB_KoColorTemperature_:
                setColorTemperature(ko.value(Dpt(7, 600)));

                break;

            case LED_RGB_KoColorTemperatureStatus_:
                break;

            case LED_RGB_KoRGB_:
                setRGB(ko.value(DPT_Colour_RGB));
                break;

            case LED_RGB_KoRGBStatus_:
                break;

            case LED_RGB_KoHSV_:
                setHSV(ko.value(DPT_Colour_RGB));
                break;

            case LED_RGB_KoHSVStatus_:
                break;

            // Day or Night
            case LED_RGB_KoNight_:
                setNight(ko.value(DPT_Switch));
                break;

            default:
                logDebugP("Unknown KO %d", relKO);
                break;
        }
    }
}

void RGBChannel::handleScene(uint8_t sceneNr)
{
    logDebugP("Scene: %d", sceneNr);
    for (int i = 0; i < N_SCENES; i++)
    {
        if (sceneNr + 1 == _scenes[i].sceneNr)
        {
            logDebugP("Typ: %d,%d", _scenes[i].funcType, _scenes[i].valueType);
            switch (_scenes[i].funcType)
            {
                default:
                case SceneConfig::FuncType::INACTIVE:
                    break;

                case SceneConfig::FuncType::VALUE:
                    if (_scenes[i].valueType == ValueType::BRIGHTNESS)
                    {
                        _brightness.setTargetValue(_scenes[i].Brightness(), millis(), ParamLED_RGB_LightDimmTimeDayON_);
                    }
                    if (_scenes[i].valueType == ValueType::COLOR)
                    {
                        Colors::HSV tmpHSV = _scenes[i].HSV();
                        Colors::RGB tmpRGB = Colors::hsv2rgb(tmpHSV);
                        logDebugP("IN: %d, %d, %d COLOR: %d, %d, %d RGB: %d, %d, %d", _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2], tmpHSV.Hue(), tmpHSV.Sat(), tmpHSV.Val(), tmpRGB._red, tmpRGB._green, tmpRGB._blue);
                        _hue.setTargetValue(tmpHSV._hue, millis(), ParamLED_RGB_LightDimmTimeDayON_);
                        _brightness.setTargetValue(tmpHSV.Val(), millis(), ParamLED_RGB_LightDimmTimeDayON_);
                        _saturation.setTargetValue(tmpHSV._sat, millis(), ParamLED_RGB_LightDimmTimeDayON_);
                    }
                    if (_scenes[i].valueType == ValueType::HUE)
                    {
                        _hue.setTargetValue(_scenes[i].value[0], millis(), ParamLED_RGB_LightDimmTimeDayON_);
                    }
                    if (_scenes[i].valueType == ValueType::SATURATION)
                    {
                        _saturation.setTargetValue(_scenes[i].value[2], millis(), ParamLED_RGB_LightDimmTimeDayON_);
                    }
                    break;

                case SceneConfig::FuncType::FUNCTION:
                    break;
                case SceneConfig::FuncType::SEQUENCE:
                    break;
                case SceneConfig::FuncType::LOCKING:
                    break;
            }
        }
    }
}

uint16_t RGBChannel::dimmingTimeON()
{
    return RGB_night() ? ParamLED_RGB_LightDimmTimeNightON_ : ParamLED_RGB_LightDimmTimeDayON_;
}

uint16_t RGBChannel::dimmingTimeOFF()
{
    return RGB_night() ? ParamLED_RGB_LightDimmTimeNightOFF_ : ParamLED_RGB_LightDimmTimeDayOFF_;
}

uint16_t RGBChannel::dimmingTime(bool _switch)
{
    return _switch ? dimmingTimeON() : dimmingTimeOFF();
}

uint8_t RGBChannel::dimmingValMaxBehavior()
{
    return ParamLED_RGB_StartupBehavior_ ? getLastOnValue() : maxDimVal();
}

uint8_t RGBChannel::maxDimVal()
{
    return RGB_night() ? ParamLED_RGB_BrighnessMaxNight_ : ParamLED_RGB_BrighnessMaxDay_;
}

uint8_t RGBChannel::upperTargetValue()
{
    return ParamLED_RGB_StartupBehavior_ ? getLastOnValue() : maxDimVal();
}

uint8_t RGBChannel::dimmingTarget(bool _switch)
{
    return _switch ? dimmingValMaxBehavior() : 0;
}

uint16_t RGBChannel::checkMinMaxColorTemp(uint16_t colorTemp)
{
    // Werte evtl auf Globalen Parameter setzen
    if (colorTemp > 8000)
    {
        colorTemp = 8000;
    }
    else if (colorTemp < 1000)
    {
        colorTemp = 1000;
    }
    return colorTemp;
}

void RGBChannel::setSwitch(bool _switch)
{
    if (_switch)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_RGB_BrighnessMin_, millis(), 1);
        // in case of stairway light
        if (ParamLED_RGB_StairCaseActive_ && ParamLED_RGB_StaicCaseTrigger_ == 0)
        {
            setStairTime(millis());
            setStairTrigger(1);
        }
        _brightness.setTargetValue(dimmingTarget(_switch), millis(), dimmingTime(_switch));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_RGB_StairCaseActive_ && ParamLED_RGB_StaicCaseTrigger_ == 1)
        {
            setStairTime(millis());
            setStairTrigger(1);
        }
        else
        {
            _brightness.setTargetValue(dimmingTarget(_switch), millis(), dimmingTime(_switch));
        }
    }
    logDebugP("dimmingTarget: %3X", dimmingTarget(_switch));
    logDebugP("dimmingTime: %5X", dimmingTime(_switch));
}

void RGBChannel::setHue(uint16_t hue)
{
    logDebugP("setHue: %3X", _hue);
    // hue max 16384
    _hue.setTargetValue(hue, millis(), dimmingTime(RGB_night()));
}

void RGBChannel::setSaturation(uint16_t saturation)
{
    logDebugP("setHue: %3X", saturation);
    // saturation max 1024
    _saturation.setTargetValue(saturation, millis(), dimmingTime(RGB_night()));
}

void RGBChannel::setBrightness(uint8_t _bright)
{
    logDebugP("setBrightness: %3X", _bright);
    // brightness max 255
    _brightness.setTargetValue(_bright, millis(), dimmingTime(RGB_night()));
}

void RGBChannel::setNight(bool _night)
{
    _rgb_night = _night;
    _brightness.setRange(ParamLED_RGB_BrighnessMin_, maxDimVal());

    if (_night)
    {
        logDebugP("Tag");

        if (_brightness.value() == ParamLED_RGB_BrighnessMaxNight_)
        {
            _brightness.setTargetValue(ParamLED_RGB_BrighnessMaxDay_, millis(), 2 * ParamLED_RGB_LightDimmTimeDayON_);
        }
    }
    else
    {
        logDebugP("Nacht");

        if (_brightness.value() > ParamLED_RGB_BrighnessMaxNight_)
        {
            _brightness.setTargetValue(ParamLED_RGB_BrighnessMaxNight_, millis(), 2 * ParamLED_RGB_LightDimmTimeNightON_);
        }
    }
}

void RGBChannel::relDimUp()
{
    _brightness.setTargetValue(maxDimVal(), millis(), ParamLED_RGB_LightDimmTimeRel_);
}

void RGBChannel::relDimDown()
{
    _brightness.setTargetValue(ParamLED_RGB_BrighnessMin_, millis(), ParamLED_RGB_LightDimmTimeRel_);
}

void RGBChannel::relDimStop()
{
    _brightness.setTargetValue(_brightness.value(), millis(), 1);
}

void RGBChannel::setColorTemperature(uint16_t colorTemp)
{
    colorTemp = checkMinMaxColorTemp(colorTemp);
    //_colorTemperature.setTargetValue(colorTemp, millis(), dimmingTimeON());
}

void RGBChannel::setRGB(uint32_t RGBvalue)
{
    Colors::HSV hsv;
    Colors::RGB rgb;
    // LED_RGB = ko.value(DPT_Colour_RGB);
    logDebugP("raw: %06X", RGBvalue);
    rgb = Colors::RGB(RGBvalue);
    logDebugP("R: %05X, G: %05X B: %05X", rgb._red, rgb._green, rgb._blue);
    hsv = Colors::rgb2hsv(rgb);
    logDebugP("H: %d, S: %d V: %d", hsv.Hue(), hsv.Sat(), hsv.Val());

    setHue(hsv._hue);
    setSaturation(hsv._sat);
    setBrightness(hsv.Val());

    //_hue.setTargetValue(hsv._hue, millis(), ParamLED_RGB_LightDimmTimeDayON_);
    //_saturation.setTargetValue(hsv._sat, millis(), ParamLED_RGB_LightDimmTimeDayON_);
    //_brightness.setTargetValue(hsv.Val(), millis(), ParamLED_RGB_LightDimmTimeDayON_);

    logDebugP("HUE:%05X%", _hue.value());
    logDebugP("SAT:%05X%", _saturation.value());
    logDebugP("BRE:%05X%", _brightness.value());
    ;
}

void RGBChannel::RGBpicker(uint8_t _selection)
{
    logDebugP("color selection:%3X%", _selection);
    switch (_selection)
    {
        case 1:
            /* color rot */
            logDebugP("color rot");
            _hue.setTargetValue(0, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 2:
            /* color  orange*/
            logDebugP("color orange");
            _hue.setTargetValue(1365, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 3:
            /* color  gelb*/
            logDebugP("color gelb");
            _hue.setTargetValue(2730, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 4:
            /* color  gelbgrün*/
            logDebugP("color gelbgrün");
            _hue.setTargetValue(4095, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 5:
            /* color  grün*/
            logDebugP("color grün");
            _hue.setTargetValue(5460, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 6:
            /* color  aquamarin*/
            logDebugP("color aquamarin");
            _hue.setTargetValue(6825, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 7:
            /* color  türkis*/
            logDebugP("color türkis");
            _hue.setTargetValue(8190, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 8:
            /* color  mint*/
            logDebugP("color mint");
            _hue.setTargetValue(9555, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 9:
            /* color  blau*/
            logDebugP("color blau");
            _hue.setTargetValue(10920, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 10:
            /* color  lila*/
            logDebugP("color lila");
            _hue.setTargetValue(12285, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 11:
            /* color  rosa*/
            logDebugP("color rosa");
            _hue.setTargetValue(13650, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 12:
            /* color  violett*/
            logDebugP("color violett");
            _hue.setTargetValue(15015, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 13:
            /* color  weiß*/
            logDebugP("color weiß");
            _hue.setTargetValue(0, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(0, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 14:
            /* color  random steady*/
            logDebugP("color random steady");
            _hue.setTargetValue(random(0x3FFF), millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 15:
            /* color  random changing*/
            logDebugP("color random changing");
            _hue.setTargetValue(random(0x3FFF), millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 16:
            /* color  custom*/
            logDebugP("color custom");
            _hue.setTargetValue(0, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            _saturation.setTargetValue(1024, millis(), ParamLED_RGB_LightDimmTimeDayON_);
            break;
        case 17:
            /* color temperature */
            logDebugP("color temperature");
            /*
                  Colors::HSV hsv;
                  if ( !RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( conv_Temp2RGB(ParamLED_RGB_ColorDay_) );
                        _hue.setTargetValue( hsv._hue , millis() , ParamLED_RGB_LightDimmTimeDayON_);
                        _saturation.setTargetValue( hsv._sat , millis() , ParamLED_RGB_LightDimmTimeDayON_);
                        }
                  if (  RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( conv_Temp2RGB(ParamLED_RGB_ColorNight_) );
                        _hue.setTargetValue( hsv._hue , millis() , ParamLED_RGB_LightDimmTimeNightON_);
                        _saturation.setTargetValue( hsv._sat , millis() , ParamLED_RGB_LightDimmTimeNightON_);
                        }
            */
            break;

        default:
            break;
    }
}

void RGBChannel::setHSV(uint32_t HSVvalue)
{
    Colors::HSV hsv;
    Colors::RGB rgb;
    hsv = Colors::HSV(HSVvalue);
    logDebugP("H: %d, S: %d V: %d", hsv._hue >> 2, hsv._sat, hsv._val);
    rgb = Colors::hsv2rgb(hsv);
    logDebugP("R: %02X, G: %02X B: %02X", rgb._red, rgb._green, rgb._blue);


    setHue(hsv._hue);
    setSaturation(hsv._sat);
    setBrightness(hsv.Val());

    //_hue.setTargetValue(hsv._hue, millis(), ParamLED_RGB_LightDimmTimeDayON_);
    //_saturation.setTargetValue(hsv._sat, millis(), ParamLED_RGB_LightDimmTimeDayON_);
    //_brightness.setTargetValue(hsv.Val(), millis(), ParamLED_RGB_LightDimmTimeDayON_);
}

uint32_t RGBChannel::conv_Temp2RGB(int _temp)
{
    uint8_t _r, _g, _b = 0;
    if (_temp > 10000)
    {
        return 0x000000;
    }

    // red
    // unter 6500 = 255
    // über 6500 = _temp^-1,02 * 1.000.000 + 125,9
    if (_temp >= 6500)
    {
        _r = (uint8_t)round(pow((float)_temp, -1.02) * 1000000.0 + 125.9);
    }
    else
    {
        _r = 255;
    }

    // green
    // unter 6500 = 100 x ln(_temp) - 623
    // über 6500 = _temp^-1,06 * 1.000.000 +165
    if (_temp <= 6500)
    {
        _g = (uint8_t)round(100.0 * (float)log10(_temp) - 623.0);
    }
    if (_temp >= 6500)
    {
        _g = (uint8_t)round(pow((float)_temp, -1.06) * 1000000.0 + 164.16);
    }

    // blue
    // unter 2000 = 0
    // 2000 - 6500 = 200 x ln(_temp)-1500
    // über 6500 = 255
    if (_temp <= 1900)
    {
        _b = 0;
    }
    if (_temp >= 1900)
    {
        _b = (uint8_t)round(477.5 * log10(_temp) - 1565.6);
    }
    if (_temp >= 6800)
    {
        _b = 255;
    }

    return (uint32_t)_r << 16 | _g << 8 | _b;
}

// EOF