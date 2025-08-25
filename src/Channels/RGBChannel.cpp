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

    _channelActive = hwChannels[0] != LED_INVALID_HW_CHANNEL && hwChannels[1] != LED_INVALID_HW_CHANNEL && hwChannels[2] != LED_INVALID_HW_CHANNEL;

    Colors::HSV hsv(_hue.value(), _saturation.value(), _UFP16(_brightness.value(), 2));

    KoLED_RGB_StateOnOff_.value(false, DPT_State);
    KoLED_RGB_BrightnessStatus_.value((uint16_t)(_brightness.value() / VALUE_KNX_MULTIPLY), DPT_Scaling);
    KoLED_RGB_ColorTemperatureStatus_.valueNoSend((uint16_t)0, Dpt(7, 600));
    KoLED_RGB_HSVStatus_.valueNoSend(hsv.toUint32(), DPT_Colour_RGB);
    KoLED_RGB_RGBStatus_.valueNoSend(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);

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
    uint16_t tmpBrightness = _brightness.value();
    Colors::HSV hsv(tmpHue, tmpSat, _UFP16(tmpBrightness, 2));

    bool stateOn = tmpBrightness > 0;

    if (_lastBrightnessLevel != tmpBrightness)
    {
        _lastBrightnessLevel = tmpBrightness;

        if ((bool)KoLED_RGB_StateOnOff_.value(DPT_State) != stateOn)
            KoLED_RGB_StateOnOff_.value(tmpBrightness > 0, DPT_State);
    }

    if (_lastHueValue != tmpHue || _lastSatValue != tmpSat)
    {
        _lastHueValue = tmpHue;
        _lastSatValue = tmpSat;
    }

    if (delayCheckMillis(_lastTimestamp, UPDATE_DELAY*4))
    {
        _lastTimestamp = millis();

        if (((uint16_t)KoLED_RGB_BrightnessStatus_.value(DPT_Scaling)* VALUE_KNX_MULTIPLY) != tmpBrightness)
        {
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
            KoLED_RGB_BrightnessStatus_.value(  (uint16_t)(tmpBrightness/ VALUE_KNX_MULTIPLY), DPT_Scaling);
        }

        if ((uint32_t)KoLED_RGB_HSVStatus_.value(DPT_Colour_RGB) != hsv.toUint32())
        {
            logDebugP("update: Hue: %d -> %d Sat: %d -> %d", _lastHueValue, tmpHue, _lastSatValue, tmpSat);

            if (stateOn)
            {
                KoLED_RGB_HSVStatus_.value(hsv.toUint32(), DPT_Colour_RGB);
                KoLED_RGB_RGBStatus_.value(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
            }
            else
            {
                KoLED_RGB_HSVStatus_.valueNoSend(hsv.toUint32(), DPT_Colour_RGB);
                KoLED_RGB_RGBStatus_.valueNoSend(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
            }
        } 
        
        Colors::RGB farbe = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp)) << 2));
        logDebugP("R: %d G: %d B: %d", farbe.Red() , farbe.Green() , farbe.Blue() );
        logDebugP("Scale: R%d G%d B%d", _pDimmer->scale(farbe.Red(),(HWDimmer::DimLUTType)0) , _pDimmer->scale(farbe.Green(),(HWDimmer::DimLUTType)0) , _pDimmer->scale(farbe.Blue(),(HWDimmer::DimLUTType)0) );
        logDebugP("H: %d S: %d V: %d", _hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), _brightness.step(_lastDimTimestamp) );
        logDebugP("HSV: %d", Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), _brightness.step(_lastDimTimestamp) ) );
    
    }
}

void RGBChannel::loop()
{
    if (!_channelActive)
        return;

    LightChannel::loop();

    if (delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
    {
        _lastDimTimestamp = millis();

        //Colors::RGB rgb = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp)) << 2));
        Colors::RGB rgb = Colors::hsv2rgb(  Colors::HSV(  _hue.step(_lastDimTimestamp),  _saturation.step(_lastDimTimestamp),  ((uint16_t)_brightness.step(_lastDimTimestamp))   )    );
        
        if (_pHWChannels[0] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Red(), (HWDimmer::DimLUTType)ParamLED_RGB_DimCurve_), _pHWChannels[0]);
        }
        if (_pHWChannels[1] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Green(), (HWDimmer::DimLUTType)ParamLED_RGB_DimCurve_), _pHWChannels[1]);
        }
        if (_pHWChannels[2] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Blue(), (HWDimmer::DimLUTType)ParamLED_RGB_DimCurve_), _pHWChannels[2]);
        }
    }
    // Stairway Timeout
    if (((getStairTime() + (ParamLED_RGB_StairCaseTimer_ * 1000)) <= millis()) && getStairTrigger())
    {
        setStairTrigger(0);
        if (ParamLED_RGB_StartupBehavior_)
        {
            setLastOnValue(_brightness.value());
        }
        _brightness.setTargetValue(0, millis(), dimmingTimeOFF());
    }

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

    processFrontInput();
}

void RGBChannel::processFrontInput()
{
    LightChannel::processFrontInput(ParamLED_RGB_FrontControl_);
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
                if (!getLock())
                {
                    setSwitch(ko.value(DPT_Switch));
                }
                break;

            case LED_RGB_KoStateOnOff_:
                break;

            case LED_RGB_KoLocking_:
                setLock(ko.value(DPT_Switch));
                KoLED_RGB_StateLocking_.value(getLock(), DPT_Switch);
                break;

            case LED_RGB_KoStateLocking_:
                break;

            case LED_RGB_KoBrightness_:
                if (!getLock())
                {
                    setBrightness(   (u_int16_t)((u_int16_t)ko.value(DPT_Scaling)* VALUE_KNX_MULTIPLY)   );
                }
                break;

            case LED_RGB_KoBrightnessStatus_:
                break;

            case LED_RGB_KoDimRel_:
                if (!getLock())
                {
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
                }
                break;

            case LED_RGB_KoScene_:
                if (!getLock())
                {
                    handleScene(ko.value(DPT_SceneNumber));
                }
                break;

            case LED_RGB_KoColorTemperature_:
                if (!getLock())
                {
                    setColorTemperature(ko.value(Dpt(7, 600)));
                }
                break;

            case LED_RGB_KoColorTemperatureStatus_:
                break;

            case LED_RGB_KoRGB_:
                if (!getLock())
                {
                    setRGB(ko.value(DPT_Colour_RGB));
                }
                break;

            case LED_RGB_KoRGBStatus_:
                break;

            case LED_RGB_KoHSV_:
                if (!getLock())
                {
                    setHSV(ko.value(DPT_Colour_RGB));
                }
                break;

            case LED_RGB_KoHSVStatus_:
                break;

            // Day or Night
            case LED_RGB_KoNight_:
                if (!getLock())
                {
                    setNight(ko.value(DPT_Switch));
                }
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
        if (sceneNr == _scenes[i].sceneNr - 1)
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
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness()), millis(), dimmingTime(1));
                    }
                    if (_scenes[i].valueType == ValueType::COLOR)
                    {
                        Colors::HSV tmpHSV = _scenes[i].HSV();
                        Colors::RGB tmpRGB = Colors::hsv2rgb(tmpHSV);
                        logDebugP("IN: %d, %d, %d COLOR: %d, %d, %d RGB: %d, %d, %d", _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2], tmpHSV.Hue(), tmpHSV.Sat(), tmpHSV.Val(), tmpRGB._red, tmpRGB._green, tmpRGB._blue);
                        _hue.setTargetValue(tmpHSV._hue, millis(), dimmingTime(1) );
                        _brightness.setTargetValue(checkMinMaxBrightness(tmpHSV.Val()), millis(), dimmingTime(1) );
                        _saturation.setTargetValue(tmpHSV._sat, millis(), dimmingTime(1) );
                    }
                    if (_scenes[i].valueType == ValueType::HUE)
                    {
                        _hue.setTargetValue(_scenes[i].value[0], millis(), dimmingTime(1));
                    }
                    if (_scenes[i].valueType == ValueType::SATURATION)
                    {
                        _saturation.setTargetValue(_scenes[i].value[2], millis(), dimmingTime(1));
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
    return getNight() ? ParamLED_RGB_LightDimmTimeNightON_ : ParamLED_RGB_LightDimmTimeDayON_;
}

uint16_t RGBChannel::dimmingTimeOFF()
{
    return getNight() ? ParamLED_RGB_LightDimmTimeNightOFF_ : ParamLED_RGB_LightDimmTimeDayOFF_;
}

uint16_t RGBChannel::dimmingTime(bool _switch)
{
    return _switch ? dimmingTimeON() : dimmingTimeOFF();
}

uint16_t RGBChannel::dimmingValStartup()
{
    return ParamLED_RGB_StartupBehavior_ ? getLastOnValue() : dimmingValMax();
}

uint16_t RGBChannel::dimmingValMin()
{
    return ParamLED_RGB_BrighnessMin_ * VALUE_KNX_MULTIPLY;
}

uint16_t RGBChannel::dimmingValMax()
{
    return getNight() ? (ParamLED_RGB_BrighnessMaxNight_* VALUE_KNX_MULTIPLY) : (ParamLED_RGB_BrighnessMaxDay_* VALUE_KNX_MULTIPLY);
}

uint16_t RGBChannel::checkMinMaxBrightness(uint16_t _bright)
{
    if (_bright < (ParamLED_RGB_BrighnessMin_* VALUE_KNX_MULTIPLY))
    {
        _bright = (ParamLED_RGB_BrighnessMin_* VALUE_KNX_MULTIPLY);
    }
    if (_bright > dimmingValMax())
    {
        _bright = dimmingValMax();
    }
    return _bright;
}

uint16_t RGBChannel::dimmingValTarget(bool _switch)
{
    return _switch ? dimmingValStartup() : 0;
}

void RGBChannel::setStartupColor()
{
    if (ParamLED_RGB_StartupBehavior_)
    {
        setHue(getLastOnValueHue());
        setSaturation(getLastOnValueSat());
    }
    else
        RGBpicker(getDefaultColor());
}

uint8_t RGBChannel::getDefaultColor()
{
    return getNight() ? ParamLED_RGB_ColorNight_ : ParamLED_RGB_ColorDay_;
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
        _brightness.setTargetValue(ParamLED_RGB_BrighnessMin_* VALUE_KNX_MULTIPLY, millis(), 1);
        // in case of stairway light
        if (ParamLED_RGB_StairCaseActive_ && ParamLED_RGB_StaicCaseTrigger_ == 0)
        {
            setStairTime(millis());
            setStairTrigger(1);
        }
        setStartupColor();

        _brightness.setTargetValue(dimmingValTarget(_switch), millis(), dimmingTime(_switch));
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
            setLastOnValue(_brightness.value());
            setLastOnValueHue(_hue.value());
            setLastOnValueSat(_saturation.value());

            _brightness.setTargetValue(dimmingValTarget(_switch), millis(), dimmingTime(_switch));
        }
    }
    logDebugP("dimmingValTarget: %3X", dimmingValTarget(_switch));
    logDebugP("dimmingTime: %5X", dimmingTime(_switch));
    logDebugP("parammaxday: %5X",(ParamLED_RGB_BrighnessMaxDay_*VALUE_KNX_MULTIPLY)  );
}

void RGBChannel::setHue(uint16_t hue)
{
    logDebugP("setHue: %3X", _hue);
    // hue max 16384
    _hue.setTargetValue(hue, millis(), dimmingTimeON());
}

void RGBChannel::setSaturation(uint16_t saturation)
{
    logDebugP("setHue: %3X", saturation);
    // saturation max 1024
    _saturation.setTargetValue(saturation, millis(), dimmingTimeON());
}

void RGBChannel::setBrightness(uint16_t _bright)
{
    logDebugP("setBrightness: %3X", _bright);
    _bright = checkMinMaxBrightness(_bright);
    _brightness.setTargetValue(_bright, millis(), dimmingTimeON());
}

void RGBChannel::setNight(bool _night)
{
    _isNight = _night;
    _brightness.setRange(ParamLED_RGB_BrighnessMin_, dimmingValMax());

    if (!getNight())
    {
        logDebugP("Tag");
        RGBpicker(getDefaultColor());
        if (_brightness.value() == ParamLED_RGB_BrighnessMaxNight_* VALUE_KNX_MULTIPLY)
        {
            _brightness.setTargetValue(ParamLED_RGB_BrighnessMaxDay_* VALUE_KNX_MULTIPLY, millis(), 2 * ParamLED_RGB_LightDimmTimeDayON_);
        }
    }
    else
    {
        logDebugP("Nacht");
        RGBpicker(getDefaultColor());
        if (_brightness.value() > ParamLED_RGB_BrighnessMaxNight_* VALUE_KNX_MULTIPLY)
        {
            _brightness.setTargetValue(ParamLED_RGB_BrighnessMaxNight_* VALUE_KNX_MULTIPLY, millis(), 2 * ParamLED_RGB_LightDimmTimeNightON_);
        }
    }
}

void RGBChannel::relDimUp()
{
    _brightness.setTargetValue(dimmingValMax(), millis(), ParamLED_RGB_LightDimmTimeRel_);
}

void RGBChannel::relDimDown()
{
    _brightness.setTargetValue(dimmingValMin(), millis(), ParamLED_RGB_LightDimmTimeRel_);
}

void RGBChannel::relDimStop()
{
    _brightness.setTargetValue(_brightness.value(), millis(), 1);
}

void RGBChannel::setColorTemperature(uint16_t colorTemp)
{
    colorTemp = checkMinMaxColorTemp(colorTemp);
    logDebugP("ColorTemp: %5X", colorTemp);
    logDebugP("ColorTemp RGB: %8X", conv_Temp2RGB(colorTemp));
    setRGB(conv_Temp2RGB(colorTemp));
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
    //setBrightness(hsv.Val());

    //_hue.setTargetValue(hsv._hue, millis(), ParamLED_RGB_LightDimmTimeDayON_);
    //_saturation.setTargetValue(hsv._sat, millis(), ParamLED_RGB_LightDimmTimeDayON_);
    //_brightness.setTargetValue(hsv.Val(), millis(), ParamLED_RGB_LightDimmTimeDayON_);

    logDebugP("HUE:%05X%", _hue.value());
    logDebugP("SAT:%05X%", _saturation.value());
    logDebugP("BRE:%05X%", _brightness.value());
}

void RGBChannel::RGBpicker(uint8_t _selection)
{
    logDebugP("color selection:%3X%", _selection);
    logDebugP("Color DAY: %3X", ParamLED_RGB_ColorDay_);
    logDebugP("Color NIGHT: %3X", ParamLED_RGB_ColorNight_);
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
    //setBrightness(hsv.Val());

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
        _g = 200.0 * log10((float)_temp) - 508.0;
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