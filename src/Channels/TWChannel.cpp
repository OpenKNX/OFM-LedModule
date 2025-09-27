#include "TWChannel.h"
#include <knx.h>

TWChannel::TWChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[2])
    : LightChannel(index, pDimmer, hwChannels, 2), _colorTemperature(4000, ParamLED_TW_ChColorTempWW, ParamLED_TW_ChColorTempCW)
{
    logInfoP("Trying to read Config from KNX...");
    logHexInfoP((uint8_t*)knx.paramData(LED_TW_ParamCalcIndex(LED_TW_ChSceneA_Type)), 8);
    _scenes = new SceneConfig[N_SCENES];
    memcpy(_scenes, knx.paramData(LED_TW_ParamCalcIndex(LED_TW_ChSceneA_Type)), N_SCENES * sizeof(SceneConfig));

    _channelActive = hwChannels[0] != LED_INVALID_HW_CHANNEL && hwChannels[1] != LED_INVALID_HW_CHANNEL;

    KoLED_TW_ChStateOnOff.value(false, DPT_State);
    KoLED_TW_ChBrightnessStatus.value((uint16_t)(_brightness.value() / VALUE_KNX_MULTIPLY), DPT_Scaling);
    KoLED_TW_ChColorTemperatureStatus.valueNoSend(_colorTemperature.value(), Dpt(7, 600));

#ifdef EXT_DEBUG_LOG
    logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
    for (int i = 0; i < N_SCENES; i++)
    {
        logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", _channelIndex, _scenes[i].sceneNr, _scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed, _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2]);
    }
#endif
}

const std::string TWChannel::name()
{
    return "TWChannel";
}

void TWChannel::update()
{
    uint16_t tmpBrightness = _brightness.value();
    bool stateOn = tmpBrightness > 0;

    if (_lastBrightnessLevel != tmpBrightness)
    {
        _lastBrightnessLevel = tmpBrightness;

        if ((bool)KoLED_TW_ChStateOnOff.value(DPT_State) != stateOn)
            KoLED_TW_ChStateOnOff.value(tmpBrightness > 0, DPT_State);
    }

    uint16_t tmpColor = _colorTemperature.value();
    if (_lastColorTemp != tmpColor)
        _lastColorTemp = tmpColor;

    if (delayCheckMillis(_lastTimestamp, UPDATE_DELAY))
    {
        _lastTimestamp = delayTimerInit();

        if (((uint16_t)KoLED_TW_ChBrightnessStatus.value(DPT_Scaling) * VALUE_KNX_MULTIPLY) != tmpBrightness)
        {
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
            KoLED_TW_ChBrightnessStatus.value((u_int16_t)(tmpBrightness / VALUE_KNX_MULTIPLY), DPT_Scaling);
        }

        if ((uint16_t)KoLED_TW_ChColorTemperatureStatus.value(Dpt(7, 600)) != tmpColor)
        {
            logDebugP("update: CT: %d -> %d", _lastColorTemp, tmpColor);

            if (stateOn)
                KoLED_TW_ChColorTemperatureStatus.value(tmpColor, Dpt(7, 600));
            else
                KoLED_TW_ChColorTemperatureStatus.valueNoSend(tmpColor, Dpt(7, 600));
        }
    }
}

void TWChannel::loop()
{
    if (!_channelActive)
        return;

    LightChannel::loop();

    bool needsPowerUp = _brightness.value() == 0 && _brightness.target() > 0;
    bool canDim = !needsPowerUp || _pDimmer->powerSupplyAvailableOrRequest();
    if (canDim && delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
    {
        _lastDimTimestamp = delayTimerInit();
        uint16_t brightValue = _pDimmer->scale(_brightness.step(_lastDimTimestamp), (HWDimmer::DimLUTType)ParamLED_TW_ChDimCurve);
        uint16_t colorTempValue = _colorTemperature.step(_lastDimTimestamp);

        uint16_t ww = ((uint32_t)brightValue * (colorTempValue - ParamLED_TW_ChColorTempWW)) / (ParamLED_TW_ChColorTempCW - ParamLED_TW_ChColorTempWW);
        uint16_t cw = ((uint32_t)brightValue * (ParamLED_TW_ChColorTempCW - colorTempValue)) / (ParamLED_TW_ChColorTempCW - ParamLED_TW_ChColorTempWW);

        if (_pHWChannels[0] < LED_ChannelCount)
        {
            _pDimmer->setLevel(ww, _pHWChannels[0]);
        }

        if (_pHWChannels[1] < LED_ChannelCount)
        {
            _pDimmer->setLevel(cw, _pHWChannels[1]);
        }
    }

    // Stairway Timeout
    if (getStairTrigger() && delayCheckMillis(getStairTime(), ParamLED_TW_ChStairCaseTimer * 1000))
    {
        setStairTrigger(0);
        if (ParamLED_TW_ChStartupBehavior)
        {
            setLastOnValue(_brightness.value());
        }
        _brightness.setTargetValue(0, dimmingTimeOFF());
    }
}

void TWChannel::processInputKo(GroupObject& ko)
{
    int16_t relKO = (ko.asap() - LED_TW_KoOffset);

    logDebugP("processInputKo Channel");
    logHexDebugP(ko.valueRef(), ko.valueSize());

    // check if channel is valid
    if ((int8_t)(relKO / LED_TW_KoBlockSize) == channelIndex())
    {
        relKO = relKO % LED_TW_KoBlockSize;
    }
    else
    {
        relKO = -1;
    }

    if (relKO == LED_TW_KoChLocking)
    {
        _isLocked = ko.value(DPT_Switch);
    }
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_TW_KoChSwitch:
                setSwitch(ko.value(DPT_Switch));
                break;

            case LED_TW_KoChStateOnOff:
                break;
            case LED_TW_KoChLocking:
                break;

            case LED_TW_KoChBrightness:
                setBrightness((uint16_t)((uint16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
                break;

            case LED_TW_KoChBrightnessStatus:
                break;
            case LED_TW_KoChDimRel:
                int16_t tmpu16;
                tmpu16 = *KoLED_TW_ChDimRel.valueRef();

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

            case LED_TW_KoChScene:
                handleScene(ko.value(DPT_SceneNumber));
                break;

            case LED_TW_KoChColorTemperature:
                setColorTemperature(ko.value(Dpt(7, 600)));

                break;

            case LED_TW_KoChColorTemperatureStatus:
                break;

            // Day or Night
            case LED_TW_KoChNight:
                setNight(ko.value(DPT_Switch));
                break;

            default:
                logDebugP("Unknown KO %d", relKO);
                break;
        }
    }
}

void TWChannel::handleScene(uint8_t sceneNr)
{
    for (int i = 0; i < N_SCENES; i++)
    {
        if (sceneNr == _scenes[i].sceneNr - 1)
        {
            switch (_scenes[i].funcType)
            {
                default:
                case SceneConfig::FuncType::INACTIVE:
                    break;

                case SceneConfig::FuncType::VALUE:
                    if (_scenes[i].valueType == ValueType::BRIGHTNESS || _scenes[i].valueType == ValueType::COMBINED)
                    {
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness()), dimmingTime(1));
                    }
                    if (_scenes[i].valueType == ValueType::TEMTPERATURE || _scenes[i].valueType == ValueType::COMBINED)
                    {
                        _colorTemperature.setTargetValue(_scenes[i].ColorTemperature(), dimmingTime(1));
                    }
                    logDebugP("Scene: %d, BR: %d, CT: %d", sceneNr, _scenes[i].Brightness(), _scenes[i].ColorTemperature());
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

uint16_t TWChannel::dimmingTimeON()
{
    return getNight() ? ParamLED_TW_ChLightDimmTimeNightON : ParamLED_TW_ChLightDimmTimeDayON;
}

uint16_t TWChannel::dimmingTimeOFF()
{
    return getNight() ? ParamLED_TW_ChLightDimmTimeNightOFF : ParamLED_TW_ChLightDimmTimeDayOFF;
}

uint16_t TWChannel::dimmingTime(bool _switch)
{
    return _switch ? dimmingTimeON() : dimmingTimeOFF();
}

uint16_t TWChannel::dimmingValStartup()
{
    return ParamLED_TW_ChStartupBehavior ? getLastOnValue() : dimmingValMax();
}

uint16_t TWChannel::dimmingValMin()
{
    return ParamLED_TW_ChBrighnessMin * VALUE_KNX_MULTIPLY;
}

uint16_t TWChannel::dimmingValMax()
{
    return getNight() ? (ParamLED_TW_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY) : (ParamLED_TW_ChBrighnessMaxDay * VALUE_KNX_MULTIPLY);
}

uint16_t TWChannel::dimmingValTarget(bool _switch)
{
    return _switch ? dimmingValStartup() : 0;
}

uint16_t TWChannel::checkMinMaxBrightness(uint16_t _bright)
{
    if (_bright < (ParamLED_TW_ChBrighnessMin * VALUE_KNX_MULTIPLY))
    {
        _bright = (ParamLED_TW_ChBrighnessMin * VALUE_KNX_MULTIPLY);
    }
    if (_bright > dimmingValMax())
    {
        _bright = dimmingValMax();
    }
    return _bright;
}

int32_t TWChannel::dimmingTempStartup()
{
    return ParamLED_TW_ChStartupBehavior ? getLastOnValueTemp() : dimmingTempMax();
}

int32_t TWChannel::dimmingTempMax()
{
    return getNight() ? ParamLED_TW_ChColorTempNight : ParamLED_TW_ChColorTempDay;
}

int32_t TWChannel::dimmingTempTarget(bool _switch)
{
    return _switch ? dimmingTempStartup() : 0;
}

uint16_t TWChannel::checkMinMaxColorTemp(uint16_t colorTemp)
{
    if (colorTemp > ParamLED_TW_ChColorTempCW)
    {
        colorTemp = ParamLED_TW_ChColorTempCW;
    }
    else if (colorTemp < ParamLED_TW_ChColorTempWW)
    {
        colorTemp = ParamLED_TW_ChColorTempWW;
    }
    return colorTemp;
}

void TWChannel::setSwitch(bool _switch)
{
    if (_switch)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_TW_ChBrighnessMin * VALUE_KNX_MULTIPLY, 1);
        // in case of stairway light
        if (ParamLED_TW_ChStairCaseActive && ParamLED_TW_ChStaicCaseTrigger == 0)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        _brightness.setTargetValue(dimmingValTarget(_switch), dimmingTime(_switch));
        _colorTemperature.setTargetValue(dimmingTempTarget(_switch), dimmingTime(_switch));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_TW_ChStairCaseActive && ParamLED_TW_ChStaicCaseTrigger == 1)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        else
        {
            setLastOnValue(_brightness.value());
            setLastOnValueTemp(_colorTemperature.value());

            _brightness.setTargetValue(dimmingValTarget(_switch), dimmingTime(_switch));
        }
    }
    logDebugP("dimmingValTarget: %3X", dimmingValTarget(_switch));
    logDebugP("dimmingTempTarget: %3X", dimmingTempTarget(_switch));
    logDebugP("dimmingTime: %5X", dimmingTime(_switch));
}

void TWChannel::setBrightness(uint16_t _bright)
{
    logDebugP("setBrightness: %3X", _bright);
    _bright = checkMinMaxBrightness(_bright);
    _brightness.setTargetValue(_bright, dimmingTimeON());
}

void TWChannel::setNight(bool _night)
{
    _isNight = _night;
    _brightness.setRange(ParamLED_TW_ChBrighnessMin * VALUE_KNX_MULTIPLY, dimmingValMax());

    if (!_night)
    {
        logDebugP("Tag");

        if (_brightness.value() == ParamLED_TW_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY)
        {
            _brightness.setTargetValue(ParamLED_TW_ChBrighnessMaxDay * VALUE_KNX_MULTIPLY, 2 * ParamLED_TW_ChLightDimmTimeDayON);
        }
    }
    else
    {
        logDebugP("Nacht");

        if (_brightness.value() > ParamLED_TW_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY)
        {
            _brightness.setTargetValue(ParamLED_TW_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY, 2 * ParamLED_TW_ChLightDimmTimeNightON);
        }
    }
}

void TWChannel::relDimUp()
{
    logDebugP("relDim_UP");
    _brightness.setTargetValue(dimmingValMax(), ParamLED_TW_ChLightDimmTimeRel);
}

void TWChannel::relDimDown()
{
    logDebugP("relDim_DOWN");
    _brightness.setTargetValue(dimmingValMin(), ParamLED_TW_ChLightDimmTimeRel);
}

void TWChannel::relDimStop()
{
    logDebugP("relDim_STOP");
    _brightness.setTargetValue(_brightness.value(), 1);
}

void TWChannel::setColorTemperature(uint16_t colorTemp)
{
    logDebugP("setColorTemperature (colorTemp=%u)", colorTemp);
    colorTemp = checkMinMaxColorTemp(colorTemp);
    _colorTemperature.setTargetValue(colorTemp, dimmingTimeON());
}