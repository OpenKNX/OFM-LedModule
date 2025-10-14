#include "RGBChannel.h"
#include <Math.h>
#include <knx.h>
#include <cmath>
#include <algorithm>

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
    uint16_t tmpBrightness = _brightness.value();
    uint16_t tmpHue = _hue.value();
    uint16_t tmpSat = _saturation.value();
    uint16_t tmpColor = _lastColorTemp;
    Colors::HSV hsv(tmpHue, tmpSat, tmpBrightness);
    bool stateOn = tmpBrightness > 0;

    if (ParamLED_RGB_ChStatusOnOffSend)
    {
        if ((bool)KoLED_RGB_ChStateOnOff.value(DPT_State) != stateOn ||
            ParamLED_RGB_ChStatusOnOffTimeMS > 0 && delayCheckMillis(_statusSendOnOffTimer, ParamLED_RGB_ChStatusOnOffTimeMS))
        {
            KoLED_RGB_ChStateOnOff.value(stateOn, DPT_State);
            _statusSendOnOffTimer = delayTimerInit();
        }
    }

    if (ParamLED_RGB_ChStatusBrightnessSend)
    {
        float brightnessDifference = abs(_lastBrightnessLevel - tmpBrightness);
        if (brightnessDifference > _lastBrightnessLevel * ParamLED_RGB_ChStatusBrightnessMinChangePercent / 100.0f ||
            brightnessDifference > ParamLED_RGB_ChStatusBrightnessMinChangeAbsolute ||
            ParamLED_RGB_ChStatusBrightnessTimeMS > 0 && delayCheckMillis(_statusSendBrightnessTimer, ParamLED_RGB_ChStatusBrightnessTimeMS))
        {
            // KoLED_RGB_BrightnessStatus.value((uint16_t)(tmpBrightness / VALUE_KNX_MULTIPLY), DPT_Scaling);
            u8_t KO_Val = (u8_t)(round((float)(((u32_t)tmpBrightness / VALUE_KNX_MULTIPLY*1000)/100))/10.0);
            KoLED_RGB_ChBrightnessStatus.value(KO_Val, DPT_Scaling);
            //KoLED_RGB_ChBrightnessStatus.value(map(tmpBrightness, -1, VALUE_KNX_COUNT - 2, 0, 100), DPT_Scaling);
            _statusSendBrightnessTimer = delayTimerInit();
        }
    }

    if (ParamLED_RGB_ChStatusTempSend)
    {
        // as color temperature calculation is quite CPU intensive,
        // we only do this when status sending is enabled
        // and color temperature can only change if hue or saturation changes
        if (_lastHueValue != tmpHue || _lastSatValue != tmpSat)
            tmpColor = conv_RGB2Temp(Colors::hsv2rgb(hsv).toUint32());

        float colorDifference = abs(_lastColorTemp - tmpColor);
        if (colorDifference > _lastColorTemp * ParamLED_RGB_ChStatusTempMinChangePercent / 100.0f ||
            colorDifference > ParamLED_RGB_ChStatusTempMinChangeAbsolute ||
            ParamLED_RGB_ChStatusTempTimeMS > 0 && delayCheckMillis(_statusSendTemperaturTimer, ParamLED_RGB_ChStatusTempTimeMS))
        {
            if (stateOn)
                KoLED_RGB_ChColorTemperatureStatus.value(tmpColor, Dpt(7, 600));
            else
                KoLED_RGB_ChColorTemperatureStatus.valueNoSend(tmpColor, Dpt(7, 600));

            _statusSendTemperaturTimer = delayTimerInit();
        }
    }

    if (ParamLED_RGB_ChStatusRGBSend)
    {
        if (_lastHueValue != tmpHue || _lastSatValue != tmpSat ||
            ParamLED_RGB_ChStatusRGBTimeMS > 0 && delayCheckMillis(_statusSendRgbTimer, ParamLED_RGB_ChStatusRGBTimeMS))
        {
            if (stateOn)
                KoLED_RGB_ChRGBStatus.value(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
            else
                KoLED_RGB_ChRGBStatus.valueNoSend(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);

            _statusSendRgbTimer = delayTimerInit();
        }
    }

    if (ParamLED_RGB_ChStatusHSVSend)
    {
        if (_lastHueValue != tmpHue || _lastSatValue != tmpSat ||
            ParamLED_RGB_ChStatusHSVTimeMS > 0 && delayCheckMillis(_statusSendHsvTimer, ParamLED_RGB_ChStatusHSVTimeMS))
        {
            if (stateOn)
                KoLED_RGB_ChHSVStatus.value(hsv.toUint32(), DPT_Colour_RGB);
            else
                KoLED_RGB_ChHSVStatus.valueNoSend(hsv.toUint32(), DPT_Colour_RGB);

            _statusSendHsvTimer = delayTimerInit();
        }
    }

    if (delayCheckMillis(_debugTimer, DEBUG_DELAY))
    {
        if (_lastBrightnessLevel != tmpBrightness)
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);

        if (_lastColorTemp != tmpColor)
            logDebugP("update: CT: %d -> %d", _lastColorTemp, tmpColor);

        if (_lastBrightnessLevel != tmpBrightness)
        {
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
            logDebugP("Brightness KO: %d, TMP: %d", (uint16_t)KoLED_RGB_ChBrightnessStatus.value(DPT_Scaling), tmpBrightness);
        }

        if (_lastHueValue != tmpHue || _lastSatValue != tmpSat)
            logDebugP("update: Hue: %d -> %d Sat: %d -> %d", _lastHueValue, tmpHue, _lastSatValue, tmpSat);

        // Colors::RGB farbe = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp)) << 2));
        // logDebugP("R: %d G: %d B: %d", farbe.Red(), farbe.Green(), farbe.Blue());
        // logDebugP("Scale: R%d G%d B%d", _pDimmer->scale(farbe.Red(), (HWDimmer::DimLUTType)0), _pDimmer->scale(farbe.Green(), (HWDimmer::DimLUTType)0), _pDimmer->scale(farbe.Blue(), (HWDimmer::DimLUTType)0));
        // logDebugP("H: %d S: %d V: %d", _hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), _brightness.step(_lastDimTimestamp));
        // logDebugP("HSV: %d", Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), _brightness.step(_lastDimTimestamp)));
        // logDebugP("Bright: %d > LUTval: %d", _brightness.value(), _pDimmer->scale(_brightness.value(), (HWDimmer::DimLUTType)0));
        // logDebugP("HUE: %d > LUTval: %d", _hue.value(), _pDimmer->scale(_hue.value(), (HWDimmer::DimLUTType)0));

        _debugTimer = delayTimerInit();
    }

    _lastBrightnessLevel = tmpBrightness;
    _lastColorTemp = tmpColor;
    _lastHueValue = tmpHue;
    _lastSatValue = tmpSat;
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

        // RGB for WS 2812 HWDimmer
        // if (_pHWChannels[0] < LED_ChannelCount && _pHWChannels[1] < LED_ChannelCount && _pHWChannels[2] < LED_ChannelCount)
        // {
        //     _pDimmer->setLevel(
        //         _pDimmer->scale(rgb.Red(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve),
        //         _pDimmer->scale(rgb.Green(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve),
        //         _pDimmer->scale(rgb.Blue(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve),
        //         _pHWChannels[0]);
        // }
    }

    // Stairway Timeout
    if (getStairTrigger() && delayCheckMillis(getStairTime(), ParamLED_RGB_ChStairCaseTimeMS))
    {
        setStairTrigger(0);
        if (ParamLED_RGB_ChStartupBehavior)
        {
            setLastOnValue(_brightness.value());
        }
        _brightness.setTargetValue(0, dimmingTimeOFF());
    }
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

            case LED_RGB_KoChSwitchNoDim:
                if (!getLock())
                {
                    setSwitchNoDim(ko.value(DPT_Switch));
                }
                break;

            case LED_RGB_KoChLocking:
                setLock(ko.value(DPT_Switch));
                KoLED_RGB_ChStateLocking.value(getLock(), DPT_Switch);
                break;

            case LED_RGB_KoChBrightness:
                if (!getLock())
                {
                    setBrightness((u_int16_t)((u_int16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
                }
                break;

            case LED_RGB_KoChBrightnessRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_RGB_ChBrightnessRel.valueRef();

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
                    _sceneNumberActive = (uint8_t)ko.value(DPT_SceneNumber) + 1;
                }
                break;

            case LED_RGB_KoChColorTemperature:
                if (!getLock())
                {
                    setColorTemperature(ko.value(Dpt(7, 600)));
                }
                break;

            case LED_RGB_KoChRGB:
                if (!getLock())
                {
                    setRGB(ko.value(DPT_Colour_RGB));
                }
                break;

            case LED_RGB_KoChHSV:
                if (!getLock())
                {
                    setHSV(ko.value(DPT_Colour_RGB));
                }
                break;

            // Day or Night
            case LED_RGB_KoChNight:
                if (!getLock())
                {
                    setNight(ko.value(DPT_Switch));
                }
                break;

            case LED_RGB_KoChStateOnOff:
            case LED_RGB_KoChStateLocking:
            case LED_RGB_KoChBrightnessStatus:
            case LED_RGB_KoChColorTemperatureStatus:
            case LED_RGB_KoChRGBStatus:
            case LED_RGB_KoChHSVStatus:
                // read-only
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
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness()*VALUE_KNX_MULTIPLY), dimmingTime(1));
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
    return getNight() ? ParamLED_RGB_ChLightDimmNightOnTime : ParamLED_RGB_ChLightDimmDayOnTime;
}

uint16_t RGBChannel::dimmingTimeOFF()
{
    return getNight() ? ParamLED_RGB_ChLightDimmNightOffTime : ParamLED_RGB_ChLightDimmDayOffTime;
}

uint16_t RGBChannel::dimmingTime(bool switchOn)
{
    return switchOn ? dimmingTimeON() : dimmingTimeOFF();
}

uint16_t RGBChannel::dimmingValStartup()
{
    return ParamLED_RGB_ChStartupBehavior ? getLastOnValue() : dimmingValMax();
}

uint16_t RGBChannel::dimmingValMin()
{
    return ParamLED_RGB_ChBrightnessMin * VALUE_KNX_MULTIPLY;
}

uint16_t RGBChannel::dimmingValMax()
{
    return getNight() ? (ParamLED_RGB_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY) : (ParamLED_RGB_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY);
}

uint16_t RGBChannel::checkMinMaxBrightness(uint16_t bright)
{
    if (bright < (ParamLED_RGB_ChBrightnessMin * VALUE_KNX_MULTIPLY))
    {
        bright = (ParamLED_RGB_ChBrightnessMin * VALUE_KNX_MULTIPLY);
    }
    if (bright > dimmingValMax())
    {
        bright = dimmingValMax();
    }
    return bright;
}

uint16_t RGBChannel::dimmingValTarget(bool switchOn)
{
    return switchOn ? dimmingValStartup() : 0;
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

void RGBChannel::setSwitch(bool switchOn)
{
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_RGB_ChBrightnessMin * VALUE_KNX_MULTIPLY, 1);
        // in case of stairway light
        if (ParamLED_RGB_ChStairCaseActive && ParamLED_RGB_ChStairCaseTrigger == 0)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        setStartupColor();

        _brightness.setTargetValue(dimmingValTarget(switchOn), dimmingTime(switchOn));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_RGB_ChStairCaseActive && ParamLED_RGB_ChStairCaseTrigger == 1)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        else
        {
            setLastOnValue(_brightness.value());
            setLastOnValueHue(_hue.value());
            setLastOnValueSat(_saturation.value());

            _brightness.setTargetValue(dimmingValTarget(switchOn), dimmingTime(switchOn));
        }
    }
    logDebugP("dimmingValTarget: %3X", dimmingValTarget(switchOn));
    logDebugP("dimmingTime: %5X", dimmingTime(switchOn));
    logDebugP("parammaxday: %5X", (ParamLED_RGB_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY));
}

void RGBChannel::setSwitchNoDim(bool switchOn)
{
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switchNoDim_ON");
        setStartupColor();
        _brightness.setTargetValue(dimmingValTarget(switchOn), 1);
    }
    else
    {
        logDebugP("switchNoDim_OFF");
        setLastOnValue(_brightness.value());
        setLastOnValueHue(_hue.value());
        setLastOnValueSat(_saturation.value());
        _brightness.setTargetValue(dimmingValTarget(switchOn), 1);
    }
}

void RGBChannel::setHue(uint16_t hue)
{
    _sceneNumberActive = 0;
    logDebugP("setHue: %3X", hue);
    // hue max 16384
    _hue.setTargetValue(hue, dimmingTimeON());
}

void RGBChannel::setSaturation(uint16_t saturation)
{
    _sceneNumberActive = 0;
    logDebugP("setSaturation: %3X", saturation);
    // saturation max 1024
    _saturation.setTargetValue(saturation, dimmingTimeON());
}

void RGBChannel::setBrightness(uint16_t bright)
{
    _sceneNumberActive = 0;
    logDebugP("setBrightness: %3X", bright);
    bright = checkMinMaxBrightness(bright);
    _brightness.setTargetValue(bright, dimmingTimeON());
}

void RGBChannel::setNight(bool night)
{

    if (ParamLED_RGB_ChScenesDisableNightSw || (!ParamLED_RGB_ChScenesDisableNightSw && _sceneNumberActive == 0))
    {
        _isNight = night;
        _brightness.setRange(ParamLED_RGB_ChBrightnessMin, dimmingValMax());

        if (!getNight())
        {
            logDebugP("Tag");
            RGBpicker(getDefaultColor());
            if (_brightness.value() == ParamLED_RGB_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
            {
                _brightness.setTargetValue(ParamLED_RGB_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGB_ChLightDimmDayOnTime);
            }
        }
        else
        {
            logDebugP("Nacht");
            RGBpicker(getDefaultColor());
            if (_brightness.value() > ParamLED_RGB_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
            {
                _brightness.setTargetValue(ParamLED_RGB_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGB_ChLightDimmNightOnTime);
            }
        }
    }
}

void RGBChannel::relDimUp()
{
    _sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMax(), ParamLED_RGB_ChLightDimmRelTime);
}

void RGBChannel::relDimDown()
{
    _sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMin(), ParamLED_RGB_ChLightDimmRelTime);
}

void RGBChannel::relDimStop()
{
    _sceneNumberActive = 0;
    _brightness.setTargetValue(_brightness.value(), 1);
}

void RGBChannel::setColorTemperature(uint16_t colorTemp)
{
    _sceneNumberActive = 0;
    colorTemp = checkMinMaxColorTemp(colorTemp);
    logDebugP("ColorTemp: %5X", colorTemp);
    logDebugP("ColorTemp RGB: %8X", conv_Temp2RGB(colorTemp));
    setRGB(conv_Temp2RGB(colorTemp));

    if (_brightness.value() > 0)
    {
        setBrightness(_brightness.value());
    }

    KoLED_RGB_ChColorTemperatureStatus.value(colorTemp, Dpt(7, 600));
}

void RGBChannel::setRGB(uint32_t RGBvalue)
{
    _sceneNumberActive = 0;
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

    //_hue.setTargetValue(hsv._hue, ParamLED_RGB_ChLightDimmDayOnTime);
    //_saturation.setTargetValue(hsv._sat, ParamLED_RGB_ChLightDimmDayOnTime);
    //_brightness.setTargetValue(hsv.Val(), ParamLED_RGB_ChLightDimmDayOnTime);

    logDebugP("HUE:%05X%", _hue.value());
    logDebugP("SAT:%05X%", _saturation.value());
    logDebugP("BRE:%05X%", _brightness.value());
}

void RGBChannel::RGBpicker(uint8_t selection)
{
    logDebugP("color selection:%3X%", selection);
    logDebugP("Color DAY: %3X", ParamLED_RGB_ChColorDay);
    logDebugP("Color NIGHT: %3X", ParamLED_RGB_ChColorNight);
    switch (selection)
    {
        case 1:
            /* color rot */
            logDebugP("color rot");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 2:
            /* color  orange*/
            logDebugP("color orange");
            _hue.setTargetValue(1365, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 3:
            /* color  gelb*/
            logDebugP("color gelb");
            _hue.setTargetValue(2730, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 4:
            /* color  gelbgrün*/
            logDebugP("color gelbgrün");
            _hue.setTargetValue(4095, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 5:
            /* color  grün*/
            logDebugP("color grün");
            _hue.setTargetValue(5460, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 6:
            /* color  aquamarin*/
            logDebugP("color aquamarin");
            _hue.setTargetValue(6825, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 7:
            /* color  türkis*/
            logDebugP("color türkis");
            _hue.setTargetValue(8190, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 8:
            /* color  mint*/
            logDebugP("color mint");
            _hue.setTargetValue(9555, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 9:
            /* color  blau*/
            logDebugP("color blau");
            _hue.setTargetValue(10920, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 10:
            /* color  lila*/
            logDebugP("color lila");
            _hue.setTargetValue(12285, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 11:
            /* color  rosa*/
            logDebugP("color rosa");
            _hue.setTargetValue(13650, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 12:
            /* color  violett*/
            logDebugP("color violett");
            _hue.setTargetValue(15015, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 13:
            /* color  weiß*/
            logDebugP("color weiß");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 14:
            /* color  random steady*/
            logDebugP("color random steady");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 15:
            /* color  random changing*/
            logDebugP("color random changing");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 16:
            /* color  custom*/
            logDebugP("color custom");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 17:
            /* color temperature */
            logDebugP("color temperature");
            /*
                  Colors::HSV hsv;
                  if ( !RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( conv_Temp2RGB(ParamLED_RGB_ChColorDay) );
                        _hue.setTargetValue( hsv._hue  , ParamLED_RGB_ChLightDimmDayOnTime);
                        _saturation.setTargetValue( hsv._sat  , ParamLED_RGB_ChLightDimmDayOnTime);
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
    _sceneNumberActive = 0;
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

    //_hue.setTargetValue(hsv._hue, ParamLED_RGB_ChLightDimmDayOnTime);
    //_saturation.setTargetValue(hsv._sat, ParamLED_RGB_ChLightDimmDayOnTime);
    //_brightness.setTargetValue(hsv.Val(), ParamLED_RGB_ChLightDimmDayOnTime);
}

uint32_t RGBChannel::conv_Temp2RGB(int temp)
{
    uint8_t r, g, b = 0;
    if (temp > 10000)
    {
        return 0x000000;
    }

    // red
    // below 6500 = 255
    // above 6500 = temp^-1,02 * 1.000.000 + 125,9
    if (temp >= 6500)
    {
        r = (uint8_t)round(pow((float)temp, -1.02) * 1000000.0 + 125.9);
    }
    else
    {
        r = 255;
    }

    // green
    // below 6500 = 100 x ln(temp) - 623
    // above 6500 = temp^-1,06 * 1.000.000 +165
    if (temp <= 6500)
    {
        g = 200.0 * log10((float)temp) - 508.0;
    }
    if (temp >= 6500)
    {
        g = (uint8_t)round(pow((float)temp, -1.06) * 1000000.0 + 164.16);
    }

    // blue
    // below 2000 = 0
    // 2000 - 6500 = 200 x ln(temp)-1500
    // above 6500 = 255
    if (temp <= 1900)
    {
        b = 0;
    }
    if (temp >= 1900)
    {
        b = (uint8_t)round(477.5 * log10(temp) - 1565.6);
    }
    if (temp >= 6800)
    {
        b = 255;
    }

    return (uint32_t)r << 16 | g << 8 | b;
}

// uint32_t RGBChannel::conv_Temp2RGB(int temp)
// {
//     uint8_t r, g, b = 0;
    
//     // Clamp to practical range
//     double temperature = CLAMP(temp, 1000.0, 40000.0);
//     temperature = temperature / 100.0;

//     double R, G, B;

//     // Red
//     if (temperature <= 66.0)
//         R = 255.0;
//     else
//         R = 329.698727446 * pow(temperature - 60.0, -0.1332047592);

//     // Green
//     if (temperature <= 66.0)
//         G = 99.4708025861 * logl(temperature) - 161.1195681661;
//     else
//         G = 288.1221695283 * pow(temperature - 60.0, -0.0755148492);

//     // Blue
//     if (temperature >= 66.0)
//         B = 255.0;
//     else if (temperature <= 19.0)
//         B = 0.0;
//     else
//         B = 138.5177312231 * logl(temperature - 10.0) - 305.0447927307;

//     // Clamp and round
//     r = (int)CLAMP(R, 0.0, 255.0);
//     g = (int)CLAMP(G, 0.0, 255.0);
//     b = (int)CLAMP(B, 0.0, 255.0);

//     return (uint32_t)r << 16 | g << 8 | b;
// }

int RGBChannel::conv_RGB2Temp(Colors::RGB target_rgb)
{
    uint8_t r_target = target_rgb._red;
    uint8_t g_target = target_rgb._green;
    uint8_t b_target = target_rgb._blue;

    int low = 1000;
    int high = 10000;
    int best_temp = low;
    uint32_t min_error = 0xFFFFFFFF;

    // Rough binary search
    while (low <= high)
    {
        int mid = (low + high) / 2;
        uint32_t rgb_mid = conv_Temp2RGB(mid);
        uint8_t r_mid = (rgb_mid >> 16) & 0xFF;
        uint8_t g_mid = (rgb_mid >> 8) & 0xFF;
        uint8_t b_mid = rgb_mid & 0xFF;
        uint32_t error = (r_mid - r_target) * (r_mid - r_target) + (g_mid - g_target) * (g_mid - g_target) + (b_mid - b_target) * (b_mid - b_target);

        if (error < min_error)
        {
            min_error = error;
            best_temp = mid;
        }

        // Simple heuristic: go towards larger RGB values

        if ((r_mid + g_mid + b_mid) < (r_target + g_target + b_target))
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    // Fine adjustment in the range ±25K

    int fine_low = (best_temp - 25 < 1000) ? 1000 : best_temp - 25;
    int fine_high = (best_temp + 25 > 10000) ? 10000 : best_temp + 25;

    for (int temp = fine_low; temp <= fine_high; temp += 1)
    {
        uint32_t rgb = conv_Temp2RGB(temp);
        uint8_t r = (rgb >> 16) & 0xFF;
        uint8_t g = (rgb >> 8) & 0xFF;
        uint8_t b = rgb & 0xFF;
        uint32_t error = (r - r_target) * (r - r_target) + (g - g_target) * (g - g_target) + (b - b_target) * (b - b_target);

        if (error < min_error)
        {
            min_error = error;
            best_temp = temp;
        }
    }

    return best_temp;
}

// int RGBChannel::conv_RGB2Temp(Colors::RGB target_rgb)
// {
//     uint8_t r = target_rgb._red;
//     uint8_t g = target_rgb._green;
//     uint8_t b = target_rgb._blue;

//     // 1. Normalize RGB (0–255 → 0–1)
//     double R = r / 255.0;
//     double G = g / 255.0;
//     double B = b / 255.0;

//     // 2. Apply gamma correction (sRGB)
//     auto linearize = [](double c) {
//         return pow(c, 2.2);
//     };
//     R = linearize(R);
//     G = linearize(G);
//     B = linearize(B);

//     // 3. Convert linear RGB to XYZ
//     double X = R * 0.4124 + G * 0.3576 + B * 0.1805;
//     double Y = R * 0.2126 + G * 0.7152 + B * 0.0722;
//     double Z = R * 0.0193 + G * 0.1192 + B * 0.9505;

//     double sum = X + Y + Z;
//     if (sum == 0) return 0.0;  // Prevent division by zero

//     // 4. Convert to chromaticity coordinates (x, y)
//     double x = X / sum;
//     double y = Y / sum;

//     // 5. Apply McCamy's formula
//     double n = (x - 0.3320) / (y - 0.1858);
//     double cct = 449.0 * pow(n, 3) + 3525.0 * pow(n, 2) + 6823.3 * n + 5520.33;

//     return cct;  // Color temperature in Kelvin
// }

// EOF