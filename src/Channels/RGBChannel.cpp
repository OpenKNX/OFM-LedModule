#include "RGBChannel.h"
#include <Math.h>
#include <knx.h>

RGBChannel::RGBChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[3])
    : LightChannel(index, pDimmer, hwChannels, 3), _hue(0, 0, H_PART - 1), _saturation(0, 0, VAL_RANGE)
{
    logInfoP("Trying to read Config from KNX...");
    logHexInfoP((uint8_t*)knx.paramData(LED_RGB_ParamCalcIndex(LED_RGB_ChSceneA_Type)), 8);
    _scenes = new SceneConfig[N_SCENES];
    memcpy(_scenes, knx.paramData(LED_RGB_ParamCalcIndex(LED_RGB_ChSceneA_Type)), N_SCENES * sizeof(SceneConfig));

    _channelActive = hwChannels[0] != LED_INVALID_HW_CHANNEL && hwChannels[1] != LED_INVALID_HW_CHANNEL && hwChannels[2] != LED_INVALID_HW_CHANNEL;

    Colors::HSV hsv(_hue.value(), _saturation.value(), _UFP16(_brightness.value(), 2));

    KoLED_RGB_ChStateOnOff.value(false, DPT_State);
    KoLED_RGB_ChBrightnessStatus.value((uint16_t)(_brightness.value() / VALUE_KNX_MULTIPLY), DPT_Scaling);
    KoLED_RGB_ChColorTemperatureStatus.valueNoSend((uint16_t)0, Dpt(7, 600));
    KoLED_RGB_ChHSVStatus.valueNoSend(hsv.toUint32(), DPT_Colour_RGB);
    KoLED_RGB_ChRGBStatus.valueNoSend(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);

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
    // Colors::HSV hsv(tmpHue, tmpSat, _UFP16(tmpBrightness, 2));
    Colors::HSV hsv(tmpHue, tmpSat, tmpBrightness);

    bool stateOn = tmpBrightness > 0;

    if (_lastBrightnessLevel != tmpBrightness)
    {
        _lastBrightnessLevel = tmpBrightness;

        if ((bool)KoLED_RGB_ChStateOnOff.value(DPT_State) != stateOn)
            KoLED_RGB_ChStateOnOff.value(tmpBrightness > 0, DPT_State);
    }

    if (_lastHueValue != tmpHue || _lastSatValue != tmpSat)
    {
        _lastHueValue = tmpHue;
        _lastSatValue = tmpSat;
    }

    if (delayCheckMillis(_lastTimestamp, UPDATE_DELAY * 5))
    {
        _lastTimestamp = delayTimerInit();

        if (((uint16_t)KoLED_RGB_ChBrightnessStatus.value(DPT_Scaling) * VALUE_KNX_MULTIPLY) != tmpBrightness)
        {
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
            logDebugP("Brightness KO: %d, TMP: %d", (uint16_t)KoLED_RGB_ChBrightnessStatus.value(DPT_Scaling), tmpBrightness);
            // KoLED_RGB_BrightnessStatus.value((uint16_t)(tmpBrightness/ VALUE_KNX_MULTIPLY), DPT_Scaling);
            KoLED_RGB_ChBrightnessStatus.value(map(tmpBrightness, -1, VALUE_KNX_COUNT - 2, 0, 100), DPT_Scaling);
        }

        if ((uint32_t)KoLED_RGB_ChHSVStatus.value(DPT_Colour_RGB) != hsv.toUint32())
        {
            logDebugP("update: Hue: %d -> %d Sat: %d -> %d", _lastHueValue, tmpHue, _lastSatValue, tmpSat);

            if (stateOn)
            {
                KoLED_RGB_ChHSVStatus.value(hsv.toUint32(), DPT_Colour_RGB);
                KoLED_RGB_ChRGBStatus.value(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
            }
            else
            {
                KoLED_RGB_ChHSVStatus.valueNoSend(hsv.toUint32(), DPT_Colour_RGB);
                KoLED_RGB_ChRGBStatus.valueNoSend(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
            }
        }

        Colors::RGB farbe = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp)) << 2));
        logDebugP("R: %d G: %d B: %d", farbe.Red(), farbe.Green(), farbe.Blue());
        logDebugP("Scale: R%d G%d B%d", _pDimmer->scale(farbe.Red(), (HWDimmer::DimLUTType)0), _pDimmer->scale(farbe.Green(), (HWDimmer::DimLUTType)0), _pDimmer->scale(farbe.Blue(), (HWDimmer::DimLUTType)0));
        logDebugP("H: %d S: %d V: %d", _hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), _brightness.step(_lastDimTimestamp));
        logDebugP("HSV: %d", Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), _brightness.step(_lastDimTimestamp)));
        logDebugP("Bright: %d > LUTval: %d", _brightness.value(), _pDimmer->scale(_brightness.value(), (HWDimmer::DimLUTType)0));
        logDebugP("HUE: %d > LUTval: %d", _hue.value(), _pDimmer->scale(_hue.value(), (HWDimmer::DimLUTType)0));
    }
}

void RGBChannel::loop()
{
    if (!_channelActive)
        return;

    LightChannel::loop();

    bool needsPowerUp =
        _hue.value() == 0 && _hue.target() > 0 ||
        _saturation.value() == 0 && _saturation.target() > 0 ||
        _brightness.value() == 0 && _brightness.target() > 0;
    bool canDim = !needsPowerUp || _pDimmer->powerSupplyAvailableOrRequest();
    if (canDim && delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
    {
        _lastDimTimestamp = delayTimerInit();

        // Colors::RGB rgb = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp)) << 2));
        Colors::RGB rgb = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp))));

        if (_pHWChannels[0] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Red(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve), _pHWChannels[0]);
        }
        if (_pHWChannels[1] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Green(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve), _pHWChannels[1]);
        }
        if (_pHWChannels[2] < LED_ChannelCount)
        {
            _pDimmer->setLevel(_pDimmer->scale(rgb.Blue(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve), _pHWChannels[2]);
        }
    }

    // Stairway Timeout
    if (getStairTrigger() && delayCheckMillis(getStairTime(), ParamLED_RGB_ChStairCaseTimer * 1000))
    {
        setStairTrigger(0);
        if (ParamLED_RGB_ChStartupBehavior)
        {
            setLastOnValue(_brightness.value());
        }
        _brightness.setTargetValue(0, dimmingTimeOFF());
    }

    // Trigger RGB Change
    /*    if (_brightness.getRGBChangingTrigger() && !RGB_night() &&
              delayCheckMillis(_brightness.getRGBChangingTime(), ParamLED_RGB_ChColorTimeDay))
        {
            logInfoP("hu:%5X%", _hue.value());
            _brightness.setRGBChangingTime(delayTimerInit());
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChColorTimeDay);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChColorTimeDay);
        }
        if (_brightness.getRGBChangingTrigger() && RGB_night() &&
            delayCheckMillis(_brightness.getRGBChangingTime(), ParamLED_RGB_ChColorTimeNight))
        {
            _brightness.setRGBChangingTime(delayTimerInit());
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChColorTimeNight);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChColorTimeNight);
            logInfoP("hue_val:%5X%", _hue.value());
        }*/
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

    if (relKO == LED_RGB_KoChLocking)
    {
        _isLocked = ko.value(DPT_Switch);
    }
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_RGB_KoChSwitch:
                if (!getLock())
                {
                    setSwitch(ko.value(DPT_Switch));
                }
                break;

            case LED_RGB_KoChStateOnOff:
                break;

            case LED_RGB_KoChLocking:
                setLock(ko.value(DPT_Switch));
                KoLED_RGB_ChStateLocking.value(getLock(), DPT_Switch);
                break;

            case LED_RGB_KoChStateLocking:
                break;

            case LED_RGB_KoChBrightness:
                if (!getLock())
                {
                    setBrightness((u_int16_t)((u_int16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
                }
                break;

            case LED_RGB_KoChBrightnessStatus:
                break;

            case LED_RGB_KoChDimRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_RGB_ChDimRel.valueRef();

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

            case LED_RGB_KoChScene:
                if (!getLock())
                {
                    handleScene(ko.value(DPT_SceneNumber));
                }
                break;

            case LED_RGB_KoChColorTemperature:
                if (!getLock())
                {
                    setColorTemperature(ko.value(Dpt(7, 600)));
                }
                break;

            case LED_RGB_KoChColorTemperatureStatus:
                break;

            case LED_RGB_KoChRGB:
                if (!getLock())
                {
                    setRGB(ko.value(DPT_Colour_RGB));
                }
                break;

            case LED_RGB_KoChRGBStatus:
                break;

            case LED_RGB_KoChHSV:
                if (!getLock())
                {
                    setHSV(ko.value(DPT_Colour_RGB));
                }
                break;

            case LED_RGB_KoChHSVStatus:
                break;

            // Day or Night
            case LED_RGB_KoChNight:
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
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness()), dimmingTime(1));
                    }
                    if (_scenes[i].valueType == ValueType::COLOR)
                    {
                        Colors::HSV tmpHSV = _scenes[i].HSV();
                        Colors::RGB tmpRGB = Colors::hsv2rgb(tmpHSV);
                        logDebugP("IN: %d, %d, %d COLOR: %d, %d, %d RGB: %d, %d, %d", _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2], tmpHSV.Hue(), tmpHSV.Sat(), tmpHSV.Val(), tmpRGB._red, tmpRGB._green, tmpRGB._blue);
                        _hue.setTargetValue(tmpHSV._hue, dimmingTime(1));
                        _brightness.setTargetValue(checkMinMaxBrightness(tmpHSV.Val()), dimmingTime(1));
                        _saturation.setTargetValue(tmpHSV._sat, dimmingTime(1));
                    }
                    if (_scenes[i].valueType == ValueType::HUE)
                    {
                        _hue.setTargetValue(_scenes[i].value[0], dimmingTime(1));
                    }
                    if (_scenes[i].valueType == ValueType::SATURATION)
                    {
                        _saturation.setTargetValue(_scenes[i].value[2], dimmingTime(1));
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
    return getNight() ? ParamLED_RGB_ChLightDimmTimeNightON : ParamLED_RGB_ChLightDimmTimeDayON;
}

uint16_t RGBChannel::dimmingTimeOFF()
{
    return getNight() ? ParamLED_RGB_ChLightDimmTimeNightOFF : ParamLED_RGB_ChLightDimmTimeDayOFF;
}

uint16_t RGBChannel::dimmingTime(bool _switch)
{
    return _switch ? dimmingTimeON() : dimmingTimeOFF();
}

uint16_t RGBChannel::dimmingValStartup()
{
    return ParamLED_RGB_ChStartupBehavior ? getLastOnValue() : dimmingValMax();
}

uint16_t RGBChannel::dimmingValMin()
{
    return ParamLED_RGB_ChBrighnessMin * VALUE_KNX_MULTIPLY;
}

uint16_t RGBChannel::dimmingValMax()
{
    return getNight() ? (ParamLED_RGB_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY) : (ParamLED_RGB_ChBrighnessMaxDay * VALUE_KNX_MULTIPLY);
}

uint16_t RGBChannel::checkMinMaxBrightness(uint16_t _bright)
{
    if (_bright < (ParamLED_RGB_ChBrighnessMin * VALUE_KNX_MULTIPLY))
    {
        _bright = (ParamLED_RGB_ChBrighnessMin * VALUE_KNX_MULTIPLY);
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
    if (ParamLED_RGB_ChStartupBehavior)
    {
        setHue(getLastOnValueHue());
        setSaturation(getLastOnValueSat());
    }
    else
        RGBpicker(getDefaultColor());
}

uint8_t RGBChannel::getDefaultColor()
{
    return getNight() ? ParamLED_RGB_ChColorNight : ParamLED_RGB_ChColorDay;
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
        _brightness.setTargetValue(ParamLED_RGB_ChBrighnessMin * VALUE_KNX_MULTIPLY, 1);
        // in case of stairway light
        if (ParamLED_RGB_ChStairCaseActive && ParamLED_RGB_ChStaicCaseTrigger == 0)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        setStartupColor();

        _brightness.setTargetValue(dimmingValTarget(_switch), dimmingTime(_switch));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_RGB_ChStairCaseActive && ParamLED_RGB_ChStaicCaseTrigger == 1)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        else
        {
            setLastOnValue(_brightness.value());
            setLastOnValueHue(_hue.value());
            setLastOnValueSat(_saturation.value());

            _brightness.setTargetValue(dimmingValTarget(_switch), dimmingTime(_switch));
        }
    }
    logDebugP("dimmingValTarget: %3X", dimmingValTarget(_switch));
    logDebugP("dimmingTime: %5X", dimmingTime(_switch));
    logDebugP("parammaxday: %5X", (ParamLED_RGB_ChBrighnessMaxDay * VALUE_KNX_MULTIPLY));
}

void RGBChannel::setHue(uint16_t hue)
{
    logDebugP("setHue: %3X", _hue);
    // hue max 16384
    _hue.setTargetValue(hue, dimmingTimeON());
}

void RGBChannel::setSaturation(uint16_t saturation)
{
    logDebugP("setSaturation: %3X", saturation);
    // saturation max 1024
    _saturation.setTargetValue(saturation, dimmingTimeON());
}

void RGBChannel::setBrightness(uint16_t _bright)
{
    logDebugP("setBrightness: %3X", _bright);
    _bright = checkMinMaxBrightness(_bright);
    _brightness.setTargetValue(_bright, dimmingTimeON());
}

void RGBChannel::setNight(bool _night)
{
    _isNight = _night;
    _brightness.setRange(ParamLED_RGB_ChBrighnessMin, dimmingValMax());

    if (!getNight())
    {
        logDebugP("Tag");
        RGBpicker(getDefaultColor());
        if (_brightness.value() == ParamLED_RGB_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY)
        {
            _brightness.setTargetValue(ParamLED_RGB_ChBrighnessMaxDay * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGB_ChLightDimmTimeDayON);
        }
    }
    else
    {
        logDebugP("Nacht");
        RGBpicker(getDefaultColor());
        if (_brightness.value() > ParamLED_RGB_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY)
        {
            _brightness.setTargetValue(ParamLED_RGB_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGB_ChLightDimmTimeNightON);
        }
    }
}

void RGBChannel::relDimUp()
{
    _brightness.setTargetValue(dimmingValMax(), ParamLED_RGB_ChLightDimmTimeRel);
}

void RGBChannel::relDimDown()
{
    _brightness.setTargetValue(dimmingValMin(), ParamLED_RGB_ChLightDimmTimeRel);
}

void RGBChannel::relDimStop()
{
    _brightness.setTargetValue(_brightness.value(), 1);
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
    logDebugP("h: %d, s: %d v: %d", hsv._hue, hsv._sat, hsv._val);
    logDebugP("H: %d, S: %d V: %d", hsv.Hue(), hsv.Sat(), hsv.Val());

    setHue(hsv._hue);
    setSaturation(hsv._sat);
    // setBrightness(hsv.Val());
    setBrightness(hsv._val);

    //_hue.setTargetValue(hsv._hue, ParamLED_RGB_ChLightDimmTimeDayON);
    //_saturation.setTargetValue(hsv._sat, ParamLED_RGB_ChLightDimmTimeDayON);
    //_brightness.setTargetValue(hsv.Val(), ParamLED_RGB_ChLightDimmTimeDayON);

    logDebugP("HUE:%05X%", _hue.value());
    logDebugP("SAT:%05X%", _saturation.value());
    logDebugP("BRE:%05X%", _brightness.value());
}

void RGBChannel::RGBpicker(uint8_t _selection)
{
    logDebugP("color selection:%3X%", _selection);
    logDebugP("Color DAY: %3X", ParamLED_RGB_ChColorDay);
    logDebugP("Color NIGHT: %3X", ParamLED_RGB_ChColorNight);
    switch (_selection)
    {
        case 1:
            /* color rot */
            logDebugP("color rot");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 2:
            /* color  orange*/
            logDebugP("color orange");
            _hue.setTargetValue(1365, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 3:
            /* color  gelb*/
            logDebugP("color gelb");
            _hue.setTargetValue(2730, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 4:
            /* color  gelbgrün*/
            logDebugP("color gelbgrün");
            _hue.setTargetValue(4095, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 5:
            /* color  grün*/
            logDebugP("color grün");
            _hue.setTargetValue(5460, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 6:
            /* color  aquamarin*/
            logDebugP("color aquamarin");
            _hue.setTargetValue(6825, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 7:
            /* color  türkis*/
            logDebugP("color türkis");
            _hue.setTargetValue(8190, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 8:
            /* color  mint*/
            logDebugP("color mint");
            _hue.setTargetValue(9555, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 9:
            /* color  blau*/
            logDebugP("color blau");
            _hue.setTargetValue(10920, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 10:
            /* color  lila*/
            logDebugP("color lila");
            _hue.setTargetValue(12285, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 11:
            /* color  rosa*/
            logDebugP("color rosa");
            _hue.setTargetValue(13650, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 12:
            /* color  violett*/
            logDebugP("color violett");
            _hue.setTargetValue(15015, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 13:
            /* color  weiß*/
            logDebugP("color weiß");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(0, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 14:
            /* color  random steady*/
            logDebugP("color random steady");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 15:
            /* color  random changing*/
            logDebugP("color random changing");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 16:
            /* color  custom*/
            logDebugP("color custom");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmTimeDayON);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmTimeDayON);
            break;
        case 17:
            /* color temperature */
            logDebugP("color temperature");
            /*
                  Colors::HSV hsv;
                  if ( !RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( conv_Temp2RGB(ParamLED_RGB_ChColorDay) );
                        _hue.setTargetValue( hsv._hue  , ParamLED_RGB_ChLightDimmTimeDayON);
                        _saturation.setTargetValue( hsv._sat  , ParamLED_RGB_ChLightDimmTimeDayON);
                        }
                  if (  RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( conv_Temp2RGB(ParamLED_RGB_ChColorNight) );
                        _hue.setTargetValue( hsv._hue  , ParamLED_RGB_ChLightDimmTimeNightON);
                        _saturation.setTargetValue( hsv._sat  , ParamLED_RGB_ChLightDimmTimeNightON);
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
    // setBrightness(hsv.Val());
    setBrightness(hsv._val);

    //_hue.setTargetValue(hsv._hue, ParamLED_RGB_ChLightDimmTimeDayON);
    //_saturation.setTargetValue(hsv._sat, ParamLED_RGB_ChLightDimmTimeDayON);
    //_brightness.setTargetValue(hsv.Val(), ParamLED_RGB_ChLightDimmTimeDayON);
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