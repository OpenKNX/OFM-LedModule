#include "RGBChannel.h"
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
    KoLED_RGB_ChBrightnessStatus.value(LedBrightness::levelToKnx(_brightness.value()), DPT_Value_1_Ucount);
    KoLED_RGB_ChColorTemperatureStatus.valueNoSend((uint16_t)0, Dpt(7, 600));
    KoLED_RGB_ChHSVStatus.valueNoSend(hsv.toUint32(), DPT_Colour_RGB);
    KoLED_RGB_ChRGBStatus.valueNoSend(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);

#ifdef EXT_DEBUG_LOG
    logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
    for (int i = 0; i < N_SCENES; i++)
        logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", _channelIndex, _scenes[i].sceneNr, _scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed, _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2]);
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

    StatusOutput::sendSwitch(KoLED_RGB_ChStateOnOff, ParamLED_RGB_ChStatusOnOffSend, stateOn, ParamLED_RGB_ChStatusOnOffTimeMS, _statusSendOnOffTimer);

    uint8_t koBrightness = brightnessRaw(tmpBrightness);
    StatusOutput::sendValue<uint8_t>(KoLED_RGB_ChBrightnessStatus, DPT_Value_1_Ucount, ParamLED_RGB_ChStatusBrightnessSend, _brightness.dimming(), koBrightness, _statusBrightness, ParamLED_RGB_ChStatusBrightnessTimeMS, ParamLED_RGB_ChStatusBrightnessMinChangePercent, LedBrightness::parameterToKnx(ParamLED_RGB_ChStatusBrightnessMinChangeAbsolute), STATUS_SEND_RATE_MS);

    // colour temperature is CPU intensive to derive: only recompute after hue/saturation changed and, while
    // they are still changing, at most once per STATUS_SEND_RATE_MS (the status is rate limited to that anyway)
    bool hueSatInTransition = _hue.dimming() || _saturation.dimming() || _relColorTempActive;
    if (_lastHueValue != tmpHue || _lastSatValue != tmpSat)
        _colorTempDirty = true;
    if (ParamLED_RGB_ChStatusTempSend && _colorTempDirty && (!hueSatInTransition || delayCheckMillis(_colorTempCalcTimer, STATUS_SEND_RATE_MS)))
    {
        // derived from the colour at full brightness, as dimmed RGB values would distort the result
        tmpColor = Colors::rgb2temp(Colors::hsv2rgb(Colors::HSV(tmpHue, tmpSat, VALUE_KNX_COUNT - 1)).toUint32());
        _colorTempDirty = false;
        _colorTempCalcTimer = delayTimerInit();
    }
    StatusOutput::sendValue<uint16_t>(KoLED_RGB_ChColorTemperatureStatus, Dpt(7, 600), ParamLED_RGB_ChStatusTempSend, hueSatInTransition, tmpColor, _statusColorTemp, ParamLED_RGB_ChStatusTempTimeMS, ParamLED_RGB_ChStatusTempMinChangePercent, ParamLED_RGB_ChStatusTempMinChangeAbsolute, STATUS_SEND_RATE_MS, stateOn);

    bool colorInTransition = hueSatInTransition || _brightness.dimming();
    StatusOutput::sendValue<uint32_t>(KoLED_RGB_ChRGBStatus, DPT_Colour_RGB, ParamLED_RGB_ChStatusRGBSend, colorInTransition, Colors::hsv2rgb(hsv).toUint32(), _statusRgb, ParamLED_RGB_ChStatusRGBTimeMS, 0, 0, STATUS_SEND_RATE_MS, stateOn);

    StatusOutput::sendValue<uint32_t>(KoLED_RGB_ChHSVStatus, DPT_Colour_RGB, ParamLED_RGB_ChStatusHSVSend, colorInTransition, hsv.toUint32(), _statusHsv, ParamLED_RGB_ChStatusHSVTimeMS, 0, 0, STATUS_SEND_RATE_MS, stateOn);

    if (delayCheckMillis(_debugTimer, DEBUG_DELAY))
    {
        if (_lastBrightnessLevel != tmpBrightness)
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);

        if (_lastColorTemp != tmpColor)
            logDebugP("update: CT: %d -> %d", _lastColorTemp, tmpColor);

        if (_lastBrightnessLevel != tmpBrightness)
        {
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
            logDebugP("Brightness KO: %d, TMP: %d", (uint16_t)KoLED_RGB_ChBrightnessStatus.value(DPT_Value_1_Ucount), tmpBrightness);
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

    float current0 = _pDimmer->getCurrent(_pHWChannels[0]);
    float current1 = _pDimmer->getCurrent(_pHWChannels[1]);
    float current2 = _pDimmer->getCurrent(_pHWChannels[2]);
    float current = current0 + current1 + current2;
    StatusOutput::sendValue<float>(KoLED_RGB_ChCurrent, DPT_Value_Electric_Current, ParamLED_RGB_ChCurrentSend, false, current, _statusCurrent, ParamLED_RGB_ChCurrentSendCyclicTimeMS, ParamLED_RGB_ChCurrentSendMinChangePercent, ParamLED_RGB_ChCurrentSendMinChangeAbsolute, STATUS_SEND_RATE_MS, true, 1000.0f);

    float voltage0 = _pDimmer->getVoltage(_pHWChannels[0]);
    float voltage1 = _pDimmer->getVoltage(_pHWChannels[1]);
    float voltage2 = _pDimmer->getVoltage(_pHWChannels[2]);
    float power = (voltage0 * current0 + voltage1 * current1 + voltage2 * current2) / 1000.0f;
    StatusOutput::sendValue<float>(KoLED_RGB_ChPower, DPT_Value_Power, ParamLED_RGB_ChPowerSend, false, power, _statusPower, ParamLED_RGB_ChPowerSendCyclicTimeMS, ParamLED_RGB_ChPowerSendMinChangePercent, ParamLED_RGB_ChPowerSendMinChangeAbsolute, STATUS_SEND_RATE_MS);

    processDeviceProtection(KoLED_RGB_ChDeviceProtConstCurrent, KoLED_RGB_ChDeviceProtOverload, ParamLED_RGB_ChDeviceProtActive, ParamLED_RGB_ChDeviceProtConstCurrent, ParamLED_RGB_ChDeviceProtOverloadPercent, ParamLED_RGB_ChDeviceProtOverloadTimeMS, _deviceProtOverloadTimer, ParamLED_RGB_ChDeviceProtCutOff, current);

    float voltage = (voltage0 + voltage1 + voltage2) / 3.0f; // as voltage should be the same anyway for all channels, we just take the average here
    processLampProtection(KoLED_RGB_ChLampProtConstCurrent, KoLED_RGB_ChLampProtOverload, ParamLED_RGB_ChLampProtActive, ParamLED_RGB_ChLampProtCableLength, ParamLED_RGB_ChLampProtCableCrossSect, ParamLED_RGB_ChLampProtConstPower, ParamLED_RGB_ChLampProtOverloadPercent, ParamLED_RGB_ChLampProtOverloadTimeMS, _lampProtOverloadTimer, ParamLED_RGB_ChLampProtCutOff, current, voltage, 3);
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
        if (_relColorTempActive)
            stepRelColorTemp(_lastDimTimestamp);

        Colors::RGB rgb = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t)_brightness.step(_lastDimTimestamp))));

        if (_pHWChannels[0] < LED_ChannelCount)
            _pDimmer->setLevel(_pDimmer->scale(rgb.Red(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve), _pHWChannels[0]);
        if (_pHWChannels[1] < LED_ChannelCount)
            _pDimmer->setLevel(_pDimmer->scale(rgb.Green(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve), _pHWChannels[1]);
        if (_pHWChannels[2] < LED_ChannelCount)
            _pDimmer->setLevel(_pDimmer->scale(rgb.Blue(), (HWDimmer::DimLUTType)ParamLED_RGB_ChDimCurve), _pHWChannels[2]);

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
            setLastOnValue(_brightness.value());
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
        relKO = relKO % LED_RGB_KoBlockSize;
    else
        relKO = -1;

    if (relKO == LED_RGB_KoChLocking)
        _isLocked = ko.value(DPT_Switch);
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_RGB_KoChSwitch:
                if (!getLock())
                    setSwitch(ko.value(DPT_Switch));
                break;

            case LED_RGB_KoChSwitchNoDim:
                if (!getLock())
                    setSwitchNoDim(ko.value(DPT_Switch));
                break;

            case LED_RGB_KoChLocking:
                setLock(ko.value(DPT_Switch));
                KoLED_RGB_ChStateLocking.value(getLock(), DPT_Switch);
                break;

            case LED_RGB_KoChBrightness:
                if (!getLock())
                    setBrightness(LedBrightness::knxToLevel(*ko.valueRef()));
                break;

            case LED_RGB_KoChBrightnessRel:
                if (!getLock())
                {
                    uint8_t tmpu8 = *KoLED_RGB_ChBrightnessRel.valueRef();
                    uint8_t stepCode = tmpu8 & 0x07;

                    if (stepCode == 0)
                        relDimStop();
                    else if (tmpu8 & 0x08)
                        relDimUp(stepCode);
                    else
                        relDimDown(stepCode);
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
                    setColorTemperature(ko.value(Dpt(7, 600)));
                break;

            case LED_RGB_KoChColorTemperatureRel:
                if (!getLock())
                {
                    uint8_t tmpu8 = *KoLED_RGB_ChColorTemperatureRel.valueRef();
                    uint8_t stepCode = tmpu8 & 0x07;

                    if (stepCode == 0)
                        relDimStopColor();
                    else if (tmpu8 & 0x08)
                        relDimUpColor(stepCode);
                    else
                        relDimDownColor(stepCode);
                }
                break;

            case LED_RGB_KoChRGB:
                if (!getLock())
                    setRGB(ko.value(DPT_Colour_RGB));
                break;

            case LED_RGB_KoChHSV:
                if (!getLock())
                    setHSV(ko.value(DPT_Colour_RGB));
                break;

            // Day or Night
            case LED_RGB_KoChNight:
                if (!getLock())
                    setNight(ko.value(DPT_Switch));
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
                        _brightness.setTargetValue(checkMinMaxBrightness(LedBrightness::parameterToLevel(_scenes[i].Brightness())), dimmingTime(1));

                    if (_scenes[i].valueType == ValueType::COLOR)
                    {
                        Colors::HSV tmpHSV = _scenes[i].HSV();
                        Colors::RGB tmpRGB = Colors::hsv2rgb(tmpHSV);
                        logDebugP("IN: %d, %d, %d COLOR: %d, %d, %d RGB: %d, %d, %d", _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2], tmpHSV.Hue(), tmpHSV.Sat(), tmpHSV.Val(), tmpRGB._red, tmpRGB._green, tmpRGB._blue);
                        _hue.setTargetValue(tmpHSV._hue, dimmingTime(1));
                        _brightness.setTargetValue(checkMinMaxBrightness(tmpHSV._val), dimmingTime(1));
                        _saturation.setTargetValue(tmpHSV._sat, dimmingTime(1));
                    }

                    if (_scenes[i].valueType == ValueType::HUE)
                        _hue.setTargetValue(_scenes[i].Hue(), dimmingTime(1));

                    if (_scenes[i].valueType == ValueType::SATURATION)
                        _saturation.setTargetValue(_scenes[i].Saturation(), dimmingTime(1));

                    markBudgetRequest(dimmingTime(1));
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
    return LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMin);
}

uint16_t RGBChannel::dimmingValMax()
{
    return getNight() ? LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMaxNight) : LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMaxDay);
}

uint16_t RGBChannel::checkMinMaxBrightness(uint16_t bright)
{
    if (bright < LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMin))
        bright = LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMin);
    if (bright > dimmingValMax())
        bright = dimmingValMax();
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
        colorTemp = 8000;
    else if (colorTemp < 1000)
        colorTemp = 1000;
    return colorTemp;
}

void RGBChannel::setSwitch(bool switchOn)
{
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMin), 1);
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
    logDebugP("parammaxday: %5X", LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMaxDay));
    markBudgetRequest(dimmingTime(switchOn));
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
    markBudgetRequest(1);
}

void RGBChannel::setHue(uint16_t hue)
{
    _sceneNumberActive = 0;
    logDebugP("setHue: %3X", hue);
    // hue max 16384
    _hue.setTargetValue(hue, dimmingTimeON());
    markBudgetRequest(dimmingTimeON());
}

void RGBChannel::setSaturation(uint16_t saturation)
{
    _sceneNumberActive = 0;
    logDebugP("setSaturation: %3X", saturation);
    // saturation max 1024
    _saturation.setTargetValue(saturation, dimmingTimeON());
    markBudgetRequest(dimmingTimeON());
}

void RGBChannel::setBrightness(uint16_t bright)
{
    _sceneNumberActive = 0;
    logDebugP("setBrightness: %3X", bright);
    bright = checkMinMaxBrightness(bright);
    _brightness.setTargetValue(bright, dimmingTimeON());
    markBudgetRequest(dimmingTimeON());
}

void RGBChannel::setNight(bool night)
{

    if (ParamLED_RGB_ChScenesDisableNightSw || (!ParamLED_RGB_ChScenesDisableNightSw && _sceneNumberActive == 0))
    {
        _isNight = night;
        _brightness.setRange(dimmingValMin(), dimmingValMax());

        if (!getNight())
        {
            logDebugP("Tag");
            RGBpicker(getDefaultColor());
            if (_brightness.value() == LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMaxNight))
                _brightness.setTargetValue(LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMaxDay), 2 * ParamLED_RGB_ChLightDimmDayOnTime);
        }
        else
        {
            logDebugP("Nacht");
            RGBpicker(getDefaultColor());
            if (_brightness.value() > LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMaxNight))
                _brightness.setTargetValue(LedBrightness::parameterToLevel(ParamLED_RGB_ChBrightnessMaxNight), 2 * ParamLED_RGB_ChLightDimmNightOnTime);
        }
    }
    markBudgetRequest(2 * dimmingTimeON());
}

void RGBChannel::relDimUp(uint8_t stepCode)
{
    _sceneNumberActive = 0;
    uint16_t dimTime = _brightness.relDim(true, stepCode, dimmingValMax(), VALUE_KNX_COUNT - 1, ParamLED_RGB_ChLightDimmRelTime);
    markBudgetRequest(dimTime);
}

void RGBChannel::relDimDown(uint8_t stepCode)
{
    _sceneNumberActive = 0;
    uint16_t dimTime = _brightness.relDim(false, stepCode, dimmingValMin(), VALUE_KNX_COUNT - 1, ParamLED_RGB_ChLightDimmRelTime);
    markBudgetRequest(dimTime);
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
    logDebugP("ColorTemp RGB: %8X", Colors::temp2rgb(colorTemp));

    // only change the colour (like TW channels): brightness stays as it is, so the light is not switched
    // on while off and a running brightness fade is not interrupted
    Colors::HSV hsv = Colors::rgb2hsv(Colors::RGB(Colors::temp2rgb(colorTemp)));
    setHue(hsv._hue);
    setSaturation(hsv._sat);

    // Colour-temperature status is emitted from update() once the hue/saturation fade settles
    // (the derived value that the lamp actually shows), so it is no longer sent directly here.
}

void RGBChannel::relDimUpColor(uint8_t stepCode)
{
    _sceneNumberActive = 0;
    logDebugP("relDim_UP");
    relDimColorTemp(true, stepCode);
}

void RGBChannel::relDimDownColor(uint8_t stepCode)
{
    _sceneNumberActive = 0;
    logDebugP("relDim_DOWN");
    relDimColorTemp(false, stepCode);
}

void RGBChannel::relDimStopColor()
{
    _sceneNumberActive = 0;
    logDebugP("relDim_STOP");
    _relColorTempActive = false;
}

void RGBChannel::relDimColorTemp(bool up, uint8_t stepCode)
{
    // continue from the colour temperature being dimmed, otherwise start from the one closest to the current colour
    if (!_relColorTempActive)
    {
        _relColorTemp = DimmableValue<uint16_t>(Colors::rgb2temp(Colors::hsv2rgb(Colors::HSV(_hue.value(), _saturation.value(), VALUE_KNX_COUNT - 1)).toUint32()));
        _relColorTempHue = _hue.target();
        _relColorTempSat = _saturation.target();
        _relColorTempActive = true;
    }

    const uint16_t colorTempMin = checkMinMaxColorTemp(0);
    const uint16_t colorTempMax = checkMinMaxColorTemp(0xFFFF);
    uint16_t dimTime = _relColorTemp.relDim(up, stepCode, up ? colorTempMax : colorTempMin, colorTempMax - colorTempMin, ParamLED_RGB_ChLightDimmRelTime);
    markBudgetRequest(dimTime);
}

void RGBChannel::stepRelColorTemp(uint32_t timestamp)
{
    // another colour command took over hue/saturation: stop following the colour temperature
    if (_hue.target() != _relColorTempHue || _saturation.target() != _relColorTempSat)
    {
        _relColorTempActive = false;
        return;
    }

    Colors::HSV hsv = Colors::rgb2hsv(Colors::RGB(Colors::temp2rgb(_relColorTemp.step(timestamp))));
    _relColorTempHue = hsv._hue;
    _relColorTempSat = hsv._sat;
    _hue.setTargetValue(hsv._hue, 0);
    _saturation.setTargetValue(hsv._sat, 0);

    if (!_relColorTemp.dimming())
        _relColorTempActive = false;
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
                        hsv = Colors::rgb2hsv( Colors::temp2rgb(ParamLED_RGB_ChColorDay) );
                        _hue.setTargetValue( hsv._hue  , ParamLED_RGB_ChLightDimmDayOnTime);
                        _saturation.setTargetValue( hsv._sat  , ParamLED_RGB_ChLightDimmDayOnTime);
                        }
                  if (  RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( Colors::temp2rgb(ParamLED_RGB_ChColorNight) );
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

bool RGBChannel::getCO1()
{
    return ParamLED_RGB_ChCO1;
}

bool RGBChannel::getCO2()
{
    return ParamLED_RGB_ChCO2;
}

bool RGBChannel::getCO3()
{
    return ParamLED_RGB_ChCO3;
}

// EOF