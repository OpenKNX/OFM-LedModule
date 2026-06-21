#include "RGBTWChannel.h"
#include <knx.h>

RGBTWChannel::RGBTWChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[3])
    : LightChannel(index, pDimmer, hwChannels, 3), _hue(0, 0, H_PART - 1), _saturation(0, 0, VAL_RANGE)
{
    logInfoP("Trying to read Config from KNX...");
    logHexInfoP((uint8_t*)knx.paramData(LED_RGBTW_ParamCalcIndex(LED_RGBTW_ChSceneA_Type)), 8);
    _scenes = new SceneConfig[N_SCENES];
    memcpy(_scenes, knx.paramData(LED_RGBTW_ParamCalcIndex(LED_RGBTW_ChSceneA_Type)), N_SCENES * sizeof(SceneConfig));

    _channelActive = hwChannels[0] != LED_INVALID_HW_CHANNEL && hwChannels[1] != LED_INVALID_HW_CHANNEL && hwChannels[2] != LED_INVALID_HW_CHANNEL && hwChannels[3] != LED_INVALID_HW_CHANNEL && hwChannels[4] != LED_INVALID_HW_CHANNEL    ;

    Colors::HSV hsv(_hue.value(), _saturation.value(), _UFP16(_brightness.value(), 2));

    KoLED_RGBTW_ChStateOnOff.value(false, DPT_State);
    KoLED_RGBTW_ChBrightnessStatus.value((uint16_t)(_brightness.value() / VALUE_KNX_MULTIPLY), DPT_Scaling);
    KoLED_RGBTW_ChColorTemperatureStatus.valueNoSend((uint16_t)0, Dpt(7, 600));
    KoLED_RGBTW_ChHSVStatus.valueNoSend(hsv.toUint32(), DPT_Colour_RGB);
    KoLED_RGBTW_ChRGBStatus.valueNoSend(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);

#ifdef EXT_DEBUG_LOG
    logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
    for (int i = 0; i < N_SCENES; i++)
        logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", _channelIndex, _scenes[i].sceneNr, _scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed, _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2]);
#endif
}

const std::string RGBTWChannel::name()
{
    return "RGBTWChannel";
}

void RGBTWChannel::update()
{
    uint16_t tmpBrightness = _brightness.value();
    uint16_t tmpHue = _hue.value();
    uint16_t tmpSat = _saturation.value();
    uint16_t tmpColor = _lastColorTemp;
    Colors::HSV hsv(tmpHue, tmpSat, tmpBrightness);
    bool stateOn = tmpBrightness > 0;

    StatusOutput::sendSwitch(KoLED_RGBTW_ChStateOnOff, ParamLED_RGBTW_ChStatusOnOffSend, stateOn, ParamLED_RGBTW_ChStatusOnOffTimeMS, _statusSendOnOffTimer);

    uint8_t koBrightness = (uint8_t)(round((float)(((uint32_t)tmpBrightness / VALUE_KNX_MULTIPLY * 1000) / 100)) / 10.0);
    StatusOutput::sendValue<uint8_t>(KoLED_RGBTW_ChBrightnessStatus, DPT_Scaling, ParamLED_RGBTW_ChStatusBrightnessSend, _brightness.dimming(), koBrightness, _statusBrightness, ParamLED_RGBTW_ChStatusBrightnessTimeMS, ParamLED_RGBTW_ChStatusBrightnessMinChangePercent, ParamLED_RGBTW_ChStatusBrightnessMinChangeAbsolute, STATUS_SEND_RATE_MS);

    // colour temperature is CPU intensive to derive, so only recompute when hue/saturation changed
    if (ParamLED_RGBTW_ChStatusTempSend && (_lastHueValue != tmpHue || _lastSatValue != tmpSat))
        tmpColor = conv_RGB2Temp(Colors::hsv2rgb(hsv).toUint32());
    StatusOutput::sendValue<uint16_t>(KoLED_RGBTW_ChColorTemperatureStatus, Dpt(7, 600), ParamLED_RGBTW_ChStatusTempSend, _hue.dimming() || _saturation.dimming(), tmpColor, _statusColorTemp, ParamLED_RGBTW_ChStatusTempTimeMS, ParamLED_RGBTW_ChStatusTempMinChangePercent, ParamLED_RGBTW_ChStatusTempMinChangeAbsolute, STATUS_SEND_RATE_MS, stateOn);

    bool colorInTransition = _hue.dimming() || _saturation.dimming() || _brightness.dimming();
    StatusOutput::sendValue<uint32_t>(KoLED_RGBTW_ChRGBStatus, DPT_Colour_RGB, ParamLED_RGBTW_ChStatusRGBSend, colorInTransition, Colors::hsv2rgb(hsv).toUint32(), _statusRgb, ParamLED_RGBTW_ChStatusRGBTimeMS, 0, 0, STATUS_SEND_RATE_MS, stateOn);

    StatusOutput::sendValue<uint32_t>(KoLED_RGBTW_ChHSVStatus, DPT_Colour_RGB, ParamLED_RGBTW_ChStatusHSVSend, colorInTransition, hsv.toUint32(), _statusHsv, ParamLED_RGBTW_ChStatusHSVTimeMS, 0, 0, STATUS_SEND_RATE_MS, stateOn);

    if (delayCheckMillis(_debugTimer, DEBUG_DELAY))
    {
        if (_lastBrightnessLevel != tmpBrightness)
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);

        if (_lastColorTemp != tmpColor)
            logDebugP("update: CT: %d -> %d", _lastColorTemp, tmpColor);

        if (_lastBrightnessLevel != tmpBrightness)
        {
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
            logDebugP("Brightness KO: %d, TMP: %d", (uint16_t)KoLED_RGBTW_ChBrightnessStatus.value(DPT_Scaling), tmpBrightness);
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
    StatusOutput::sendValue<float>(KoLED_RGBTW_ChCurrent, DPT_Value_Electric_Current, ParamLED_RGBTW_ChCurrentSend, false, current, _statusCurrent, ParamLED_RGBTW_ChCurrentSendCyclicTimeMS, ParamLED_RGBTW_ChCurrentSendMinChangePercent, ParamLED_RGBTW_ChCurrentSendMinChangeAbsolute, STATUS_SEND_RATE_MS, true, 1000.0f);

    float voltage0 = _pDimmer->getVoltage(_pHWChannels[0]);
    float voltage1 = _pDimmer->getVoltage(_pHWChannels[1]);
    float voltage2 = _pDimmer->getVoltage(_pHWChannels[2]);
    float power = (voltage0 * current0 + voltage1 * current1 + voltage2 * current2) / 1000.0f;
    StatusOutput::sendValue<float>(KoLED_RGBTW_ChPower, DPT_Value_Power, ParamLED_RGBTW_ChPowerSend, false, power, _statusPower, ParamLED_RGBTW_ChPowerSendCyclicTimeMS, ParamLED_RGBTW_ChPowerSendMinChangePercent, ParamLED_RGBTW_ChPowerSendMinChangeAbsolute, STATUS_SEND_RATE_MS);

    processDeviceProtection(KoLED_RGBTW_ChDeviceProtConstCurrent, KoLED_RGBTW_ChDeviceProtOverload, ParamLED_RGBTW_ChDeviceProtActive, ParamLED_RGBTW_ChDeviceProtConstCurrent, ParamLED_RGBTW_ChDeviceProtOverloadPercent, ParamLED_RGBTW_ChDeviceProtOverloadTimeMS, _deviceProtOverloadTimer, ParamLED_RGBTW_ChDeviceProtCutOff, current);

    float voltage = (voltage0 + voltage1 + voltage2) / 3.0f; // as voltage should be the same anyway for all channels, we just take the average here
    processLampProtection(KoLED_RGBTW_ChLampProtConstCurrent, KoLED_RGBTW_ChLampProtOverload, ParamLED_RGBTW_ChLampProtActive, ParamLED_RGBTW_ChLampProtCableLength, ParamLED_RGBTW_ChLampProtCableCrossSect, ParamLED_RGBTW_ChLampProtConstPower, ParamLED_RGBTW_ChLampProtOverloadPercent, ParamLED_RGBTW_ChLampProtOverloadTimeMS, _lampProtOverloadTimer, ParamLED_RGBTW_ChLampProtCutOff, current, voltage, 3);
}

void RGBTWChannel::loop()
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
            _pDimmer->setLevel(_pDimmer->scale(rgb.Red(), (HWDimmer::DimLUTType)ParamLED_RGBTW_ChDimCurve), _pHWChannels[0]);
        if (_pHWChannels[1] < LED_ChannelCount)
            _pDimmer->setLevel(_pDimmer->scale(rgb.Green(), (HWDimmer::DimLUTType)ParamLED_RGBTW_ChDimCurve), _pHWChannels[1]);
        if (_pHWChannels[2] < LED_ChannelCount)
            _pDimmer->setLevel(_pDimmer->scale(rgb.Blue(), (HWDimmer::DimLUTType)ParamLED_RGBTW_ChDimCurve), _pHWChannels[2]);
        if (_pHWChannels[3] < LED_ChannelCount)
            _pDimmer->setLevel(_pDimmer->scale(rgb.WarmWhite(), (HWDimmer::DimLUTType)ParamLED_RGBTW_ChDimCurve), _pHWChannels[3]);
        if (_pHWChannels[4] < LED_ChannelCount)
            _pDimmer->setLevel(_pDimmer->scale(rgb.ColdWhite(), (HWDimmer::DimLUTType)ParamLED_RGBTW_ChDimCurve), _pHWChannels[4]);

    }

    // Stairway Timeout
    if (getStairTrigger() && delayCheckMillis(getStairTime(), ParamLED_RGBTW_ChStairCaseTimeMS))
    {
        setStairTrigger(0);
        if (ParamLED_RGBTW_ChStartupBehavior)
            setLastOnValue(_brightness.value());
        _brightness.setTargetValue(0, dimmingTimeOFF());
    }
}

void RGBTWChannel::processInputKo(GroupObject& ko)
{
    Colors::HSV hsv;
    Colors::RGB rgb;
    int16_t relKO = (ko.asap() - LED_RGBTW_KoOffset);

    logDebugP("processInputKo Channel");
    logHexDebugP(ko.valueRef(), ko.valueSize());

    // check if channel is valid
    if ((int8_t)(relKO / LED_RGBTW_KoBlockSize) == channelIndex())
        relKO = relKO % LED_RGBTW_KoBlockSize;
    else
        relKO = -1;

    if (relKO == LED_RGBTW_KoChLocking)
        _isLocked = ko.value(DPT_Switch);
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_RGBTW_KoChSwitch:
                if (!getLock())
                    setSwitch(ko.value(DPT_Switch));
                break;

            case LED_RGB_KoChSwitchNoDim:
                if (!getLock())
                    setSwitchNoDim(ko.value(DPT_Switch));
                break;

            case LED_RGBTW_KoChLocking:
                setLock(ko.value(DPT_Switch));
                KoLED_RGBTW_ChStateLocking.value(getLock(), DPT_Switch);
                break;

            case LED_RGBTW_KoChBrightness:
                if (!getLock())
                    setBrightness((u_int16_t)((u_int16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
                break;

            case LED_RGBTW_KoChBrightnessRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_RGBTW_ChBrightnessRel.valueRef();

                    if (tmpu16 >= 0x09)
                        relDimUp();
                    if (tmpu16 > 0x00 && tmpu16 < 0x08)
                        relDimDown();
                    if (tmpu16 == 0x00 || tmpu16 == 0x08)
                        relDimStop();
                }
                break;

            case LED_RGBTW_KoChScene:
                if (!getLock())
                {
                    handleScene(ko.value(DPT_SceneNumber));
                    _sceneNumberActive = (uint8_t)ko.value(DPT_SceneNumber) + 1;
                }
                break;

            case LED_RGBTW_KoChColorTemperature:
                if (!getLock())
                    setColorTemperature(ko.value(Dpt(7, 600)));
                break;

            case LED_RGBTW_KoChColorTemperatureRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_RGBTW_ChColorTemperatureRel.valueRef();

                    if (tmpu16 >= 0x09)
                        relDimUpColor();
                    if (tmpu16 > 0x00 && tmpu16 < 0x08)
                        relDimDownColor();
                    if (tmpu16 == 0x00 || tmpu16 == 0x08)
                        relDimStopColor();
                }
                break;

            case LED_RGBTW_KoChRGB:
                if (!getLock())
                    setRGB(ko.value(DPT_Colour_RGB));
                break;

            case LED_RGBTW_KoChHSV:
                if (!getLock())
                    setHSV(ko.value(DPT_Colour_RGB));
                break;

            // Day or Night
            case LED_RGBTW_KoChNight:
                if (!getLock())
                    setNight(ko.value(DPT_Switch));
                break;

            case LED_RGBTW_KoChStateOnOff:
            case LED_RGBTW_KoChStateLocking:
            case LED_RGBTW_KoChBrightnessStatus:
            case LED_RGBTW_KoChColorTemperatureStatus:
            case LED_RGBTW_KoChRGBStatus:
            case LED_RGBTW_KoChHSVStatus:
                // read-only
                break;

            default:
                logDebugP("Unknown KO %d", relKO);
                break;
        }
    }
}

void RGBTWChannel::handleScene(uint8_t sceneNr)
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
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness() * VALUE_KNX_MULTIPLY), dimmingTime(1));

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
                        _hue.setTargetValue(_scenes[i].value[0], dimmingTime(1));

                    if (_scenes[i].valueType == ValueType::SATURATION)
                        _saturation.setTargetValue(_scenes[i].value[2], dimmingTime(1));

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

uint16_t RGBTWChannel::dimmingTimeON()
{
    return getNight() ? ParamLED_RGBTW_ChLightDimmNightOnTime : ParamLED_RGBTW_ChLightDimmDayOnTime;
}

uint16_t RGBTWChannel::dimmingTimeOFF()
{
    return getNight() ? ParamLED_RGBTW_ChLightDimmNightOffTime : ParamLED_RGBTW_ChLightDimmDayOffTime;
}

uint16_t RGBTWChannel::dimmingTime(bool switchOn)
{
    return switchOn ? dimmingTimeON() : dimmingTimeOFF();
}

uint16_t RGBTWChannel::dimmingValStartup()
{
    return ParamLED_RGBTW_ChStartupBehavior ? getLastOnValue() : dimmingValMax();
}

uint16_t RGBTWChannel::dimmingValMin()
{
    return ParamLED_RGBTW_ChBrightnessMin * VALUE_KNX_MULTIPLY;
}

uint16_t RGBTWChannel::dimmingValMax()
{
    return getNight() ? (ParamLED_RGBTW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY) : (ParamLED_RGBTW_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY);
}

uint16_t RGBTWChannel::checkMinMaxBrightness(uint16_t bright)
{
    if (bright < (ParamLED_RGBTW_ChBrightnessMin * VALUE_KNX_MULTIPLY))
        bright = (ParamLED_RGBTW_ChBrightnessMin * VALUE_KNX_MULTIPLY);
    if (bright > dimmingValMax())
        bright = dimmingValMax();
    return bright;
}

uint16_t RGBTWChannel::dimmingValTarget(bool switchOn)
{
    return switchOn ? dimmingValStartup() : 0;
}

void RGBTWChannel::setStartupColor()
{
    if (ParamLED_RGBTW_ChStartupBehavior)
    {
        setHue(getLastOnValueHue());
        setSaturation(getLastOnValueSat());
    }
    else
        RGBpicker(getDefaultColor());
}

uint8_t RGBTWChannel::getDefaultColor()
{
    return getNight() ? ParamLED_RGBTW_ChColorNight : ParamLED_RGBTW_ChColorDay;
}

uint16_t RGBTWChannel::checkMinMaxColorTemp(uint16_t colorTemp)
{
    // Werte evtl auf Globalen Parameter setzen
    if (colorTemp > 8000)
        colorTemp = 8000;
    else if (colorTemp < 1000)
        colorTemp = 1000;
    return colorTemp;
}

void RGBTWChannel::setSwitch(bool switchOn)
{
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_RGBTW_ChBrightnessMin * VALUE_KNX_MULTIPLY, 1);
        // in case of stairway light
        if (ParamLED_RGBTW_ChStairCaseActive && ParamLED_RGBTW_ChStairCaseTrigger == 0)
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
        if (ParamLED_RGBTW_ChStairCaseActive && ParamLED_RGBTW_ChStairCaseTrigger == 1)
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
    logDebugP("parammaxday: %5X", (ParamLED_RGBTW_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY));
    markBudgetRequest(dimmingTime(switchOn));
}

void RGBTWChannel::setSwitchNoDim(bool switchOn)
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

void RGBTWChannel::setHue(uint16_t hue)
{
    _sceneNumberActive = 0;
    logDebugP("setHue: %3X", hue);
    // hue max 16384
    _hue.setTargetValue(hue, dimmingTimeON());
    markBudgetRequest(dimmingTimeON());
}

void RGBTWChannel::setSaturation(uint16_t saturation)
{
    _sceneNumberActive = 0;
    logDebugP("setSaturation: %3X", saturation);
    // saturation max 1024
    _saturation.setTargetValue(saturation, dimmingTimeON());
    markBudgetRequest(dimmingTimeON());
}

void RGBTWChannel::setBrightness(uint16_t bright)
{
    _sceneNumberActive = 0;
    logDebugP("setBrightness: %3X", bright);
    bright = checkMinMaxBrightness(bright);
    _brightness.setTargetValue(bright, dimmingTimeON());
    markBudgetRequest(dimmingTimeON());
}

void RGBTWChannel::setNight(bool night)
{

    if (ParamLED_RGBTW_ChScenesDisableNightSw || (!ParamLED_RGBTW_ChScenesDisableNightSw && _sceneNumberActive == 0))
    {
        _isNight = night;
        _brightness.setRange(ParamLED_RGBTW_ChBrightnessMin, dimmingValMax());

        if (!getNight())
        {
            logDebugP("Tag");
            RGBpicker(getDefaultColor());
            if (_brightness.value() == ParamLED_RGBTW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
                _brightness.setTargetValue(ParamLED_RGBTW_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGBTW_ChLightDimmDayOnTime);
        }
        else
        {
            logDebugP("Nacht");
            RGBpicker(getDefaultColor());
            if (_brightness.value() > ParamLED_RGBTW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
                _brightness.setTargetValue(ParamLED_RGBTW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGBTW_ChLightDimmNightOnTime);
        }
    }
    markBudgetRequest(2 * dimmingTimeON());
}

void RGBTWChannel::relDimUp()
{
    _sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMax(), ParamLED_RGBTW_ChLightDimmRelTime);
    markBudgetRequest(ParamLED_RGBTW_ChLightDimmRelTime);
}

void RGBTWChannel::relDimDown()
{
    _sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMin(), ParamLED_RGBTW_ChLightDimmRelTime);
    markBudgetRequest(ParamLED_RGBTW_ChLightDimmRelTime);
}

void RGBTWChannel::relDimStop()
{
    _sceneNumberActive = 0;
    _brightness.setTargetValue(_brightness.value(), 1);
}

void RGBTWChannel::setColorTemperature(uint16_t colorTemp)
{
    _sceneNumberActive = 0;
    colorTemp = checkMinMaxColorTemp(colorTemp);
    logDebugP("ColorTemp: %5X", colorTemp);
    logDebugP("ColorTemp RGB: %8X", conv_Temp2RGB(colorTemp));
    setRGB(conv_Temp2RGB(colorTemp));

    if (_brightness.value() > 0)
        setBrightness(_brightness.value());

    // Colour-temperature status is emitted from update() once the hue/saturation fade settles
    // (the derived value that the lamp actually shows), so it is no longer sent directly here.
}

void RGBTWChannel::relDimUpColor()
{
    //_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_UP");
    setColorTemperature(10000);
}

void RGBTWChannel::relDimDownColor()
{
    //_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_DOWN");
    setColorTemperature(1000);
}

void RGBTWChannel::relDimStopColor()
{
    //_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_STOP");
    // setColorTemperature(_colorTemperature.value(), 1);
}

void RGBTWChannel::setRGB(uint32_t RGBvalue)
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

void RGBTWChannel::RGBpicker(uint8_t selection)
{
    logDebugP("color selection:%3X%", selection);
    logDebugP("Color DAY: %3X", ParamLED_RGBTW_ChColorDay);
    logDebugP("Color NIGHT: %3X", ParamLED_RGBTW_ChColorNight);
    switch (selection)
    {
        case 1:
            /* color rot */
            logDebugP("color rot");
            _hue.setTargetValue(0, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 2:
            /* color  orange*/
            logDebugP("color orange");
            _hue.setTargetValue(1365, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 3:
            /* color  gelb*/
            logDebugP("color gelb");
            _hue.setTargetValue(2730, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 4:
            /* color  gelbgrün*/
            logDebugP("color gelbgrün");
            _hue.setTargetValue(4095, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 5:
            /* color  grün*/
            logDebugP("color grün");
            _hue.setTargetValue(5460, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 6:
            /* color  aquamarin*/
            logDebugP("color aquamarin");
            _hue.setTargetValue(6825, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 7:
            /* color  türkis*/
            logDebugP("color türkis");
            _hue.setTargetValue(8190, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 8:
            /* color  mint*/
            logDebugP("color mint");
            _hue.setTargetValue(9555, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 9:
            /* color  blau*/
            logDebugP("color blau");
            _hue.setTargetValue(10920, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 10:
            /* color  lila*/
            logDebugP("color lila");
            _hue.setTargetValue(12285, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 11:
            /* color  rosa*/
            logDebugP("color rosa");
            _hue.setTargetValue(13650, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 12:
            /* color  violett*/
            logDebugP("color violett");
            _hue.setTargetValue(15015, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 13:
            /* color  weiß*/
            logDebugP("color weiß");
            _hue.setTargetValue(0, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(0, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 14:
            /* color  random steady*/
            logDebugP("color random steady");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 15:
            /* color  random changing*/
            logDebugP("color random changing");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
            break;
        case 16:
            /* color  custom*/
            logDebugP("color custom");
            _hue.setTargetValue(0, ParamLED_RGBTW_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGBTW_ChLightDimmDayOnTime);
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

void RGBTWChannel::setHSV(uint32_t HSVvalue)
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

uint32_t RGBTWChannel::conv_Temp2RGB(int temp)
{
    uint8_t r, g, b = 0;
    if (temp > 10000)
        return 0x000000;

    // red
    // below 6500 = 255
    // above 6500 = temp^-1,02 * 1.000.000 + 125,9
    if (temp >= 6500)
        r = (uint8_t)round(pow((float)temp, -1.02) * 1000000.0 + 125.9);
    else
        r = 255;

    // green
    // below 6500 = 100 x ln(temp) - 623
    // above 6500 = temp^-1,06 * 1.000.000 +165
    if (temp <= 6500)
        g = 200.0 * log10((float)temp) - 508.0;
    if (temp >= 6500)
        g = (uint8_t)round(pow((float)temp, -1.06) * 1000000.0 + 164.16);

    // blue
    // below 2000 = 0
    // 2000 - 6500 = 200 x ln(temp)-1500
    // above 6500 = 255
    if (temp <= 1900)
        b = 0;
    if (temp >= 1900)
        b = (uint8_t)round(477.5 * log10(temp) - 1565.6);
    if (temp >= 6800)
        b = 255;

    return (uint32_t)r << 16 | g << 8 | b;
}

int RGBTWChannel::conv_RGB2Temp(uint32_t target_rgb)
{
    uint8_t r_target = (target_rgb >> 16) & 0xFF;
    uint8_t g_target = (target_rgb >> 8) & 0xFF;
    uint8_t b_target = target_rgb & 0xFF;

    int low = 1000;
    int high = 10000;
    int best_temp = low;
    uint32_t min_error = 0x300;

    // Rough binary search
    while (low <= high)
    {
        int mid = (low + high) / 2;
        uint32_t rgb_mid = conv_Temp2RGB(mid);
        uint8_t r_mid = (rgb_mid >> 16) & 0xFF;
        uint8_t g_mid = (rgb_mid >> 8) & 0xFF;
        uint8_t b_mid = rgb_mid & 0xFF;
        uint32_t error = (r_mid - r_target) * (r_mid - r_target) + (g_mid - g_target) * (g_mid - g_target) + (b_mid - b_target) * (b_mid - b_target);
        logDebugP("Temp: %d bestTemp:%d R:%d G:%d B:%d Error:%d Target R:%d G:%d B:%d RGB:%d", mid, best_temp, r_mid, g_mid, b_mid, error, r_target, g_target, b_target, target_rgb);
        if (error < min_error)
        {
            min_error = error;
            best_temp = mid;
        }

        // Simple heuristic: go towards larger RGB values

        if ((r_mid + g_mid + b_mid) < (r_target + g_target + b_target))
            low = mid + 10;
        else
            high = mid - 10;
    }

    // Fine adjustment in the range ±25K

    // int fine_low = (best_temp - 25 < 1000) ? 1000 : best_temp - 25;
    // int fine_high = (best_temp + 25 > 10000) ? 10000 : best_temp + 25;
    int fine_low = (best_temp - 1000 < 1000) ? 1000 : best_temp - 1000;
    int fine_high = (best_temp + 1000 > 10000) ? 10000 : best_temp + 1000;

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

bool RGBTWChannel::getCO1()
{
    return ParamLED_RGBTW_ChCO1;
}

bool RGBTWChannel::getCO2()
{
    return ParamLED_RGBTW_ChCO2;
}

bool RGBTWChannel::getCO3()
{
    return ParamLED_RGBTW_ChCO3;
}


// EOF