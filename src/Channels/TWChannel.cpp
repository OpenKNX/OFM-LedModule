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
    uint16_t tmpColor = _colorTemperature.value();
    bool stateOn = tmpBrightness > 0;

    if (ParamLED_TW_ChStatusOnOffSend)
    {
        if ((bool)KoLED_TW_ChStateOnOff.value(DPT_State) != stateOn ||
            ParamLED_TW_ChStatusOnOffTimeMS > 0 && delayCheckMillis(_statusSendOnOffTimer, ParamLED_TW_ChStatusOnOffTimeMS))
        {
            KoLED_TW_ChStateOnOff.value(stateOn, DPT_State);
            _statusSendOnOffTimer = delayTimerInit();
        }
    }

    if (ParamLED_TW_ChStatusBrightnessSend)
    {
        float brightnessDifference = abs(_lastBrightnessLevel - tmpBrightness);
        if ((brightnessDifference > EPSILON &&
            (_lastBrightnessLevel > 0 && brightnessDifference >= _lastBrightnessLevel * ParamLED_TW_ChStatusBrightnessMinChangePercent / 100.0f ||
             brightnessDifference >= ParamLED_TW_ChStatusBrightnessMinChangeAbsolute)) ||
            ParamLED_TW_ChStatusBrightnessTimeMS > 0 && delayCheckMillis(_statusSendBrightnessTimer, ParamLED_TW_ChStatusBrightnessTimeMS))
        {
            u8_t KO_Val = (u8_t)(round((float)(((u32_t)tmpBrightness / VALUE_KNX_MULTIPLY*1000)/100))/10.0);
            KoLED_TW_ChBrightnessStatus.value(KO_Val, DPT_Scaling);
            //KoLED_TW_ChBrightnessStatus.value((uint16_t)(tmpBrightness / VALUE_KNX_MULTIPLY), DPT_Scaling);
            _statusSendBrightnessTimer = delayTimerInit();
        }
    }

    if (ParamLED_TW_ChStatusTempSend)
    {
        float colorDifference = abs(_lastColorTemp - tmpColor);
        if ((colorDifference > EPSILON &&
             (_lastColorTemp > 0 && colorDifference >= _lastColorTemp * ParamLED_TW_ChStatusTempMinChangePercent / 100.0f ||
              colorDifference >= ParamLED_TW_ChStatusTempMinChangeAbsolute)) ||
            ParamLED_TW_ChStatusTempTimeMS > 0 && delayCheckMillis(_statusSendTemperaturTimer, ParamLED_TW_ChStatusTempTimeMS))
        {
            if (stateOn)
                KoLED_TW_ChColorTemperatureStatus.value(tmpColor, Dpt(7, 600));
            else
                KoLED_TW_ChColorTemperatureStatus.valueNoSend(tmpColor, Dpt(7, 600));

            _statusSendTemperaturTimer = delayTimerInit();
        }
    }

    if (delayCheckMillis(_debugTimer, DEBUG_DELAY))
    {
        if (_lastBrightnessLevel != tmpBrightness)
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);

        if (_lastColorTemp != tmpColor)
            logDebugP("update: CT: %d -> %d", _lastColorTemp, tmpColor);

        _debugTimer = delayTimerInit();
    }

    _lastBrightnessLevel = tmpBrightness;
    _lastColorTemp = tmpColor;
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
        if (!_boost)
        {
            if (_pHWChannels[0] < LED_ChannelCount)
            {
                _pDimmer->setLevel(ww, _pHWChannels[0]);
            }

            if (_pHWChannels[1] < LED_ChannelCount)
            {
                _pDimmer->setLevel(cw, _pHWChannels[1]);
            }
        }
    }

    // Stairway Timeout
    if (getStairTrigger() && delayCheckMillis(getStairTime(), ParamLED_TW_ChStairCaseTimeMS))
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
                if (!getLock())
                {
                    setSwitch(ko.value(DPT_Switch));
                }
                break;

            case LED_TW_KoChSwitchNoDim:
                if (!getLock())
                {
                    setSwitchNoDim(ko.value(DPT_Switch));
                }
                break;

            case LED_TW_KoChSwitchBoost:
                if (!getLock())
                {
                    setBoost(ko.value(DPT_Switch));
                }
                break;

            case LED_TW_KoChLocking:
                setLock(ko.value(DPT_Switch));
                KoLED_RGB_ChStateLocking.value(getLock(), DPT_Switch);
                break;

            case LED_TW_KoChBrightness:
                if (!getLock())
                {
                    setBrightness((uint16_t)((uint16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
                }
                break;

            case LED_TW_KoChBrightnessRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_TW_ChBrightnessRel.valueRef();

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

            case LED_TW_KoChScene:
                if (!getLock())
                {
                    handleScene(ko.value(DPT_SceneNumber));
                    _sceneNumberActive = (uint8_t)ko.value(DPT_SceneNumber) + 1;
                }
                break;

            case LED_TW_KoChColorTemperature:
                if (!getLock())
                {
                    setColorTemperature(ko.value(Dpt(7, 600)));
                }
                break;

            /*case LED_TW_KoChColorRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_TW_ChColorRel.valueRef();

                    if (tmpu16 >= 0x09)
                    {
                        relDimUpColor();
                    }
                    if (tmpu16 > 0x00 && tmpu16 < 0x08)
                    {
                        relDimDownColor();
                    }
                    if (tmpu16 == 0x00 || tmpu16 == 0x08)
                    {
                        relDimStopColor();
                    }
                }
                break;*/

            // Day or Night
            case LED_TW_KoChNight:
                setNight(ko.value(DPT_Switch));
                break;

            case LED_TW_KoChStateOnOff:
            case LED_TW_KoChStateLocking:
            case LED_TW_KoChBrightnessStatus:
            case LED_TW_KoChColorTemperatureStatus:
                // read-only
                break;

            default:
                logDebugP("Unknown KO %d", relKO);
                break;
        }
    }
}

void TWChannel::handleScene(uint8_t sceneNr)
{
    _boost = false;
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
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness()*VALUE_KNX_MULTIPLY), dimmingTime(1));
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
    return getNight() ? ParamLED_TW_ChLightDimmNightOnTime : ParamLED_TW_ChLightDimmDayOnTime;
}

uint16_t TWChannel::dimmingTimeOFF()
{
    return getNight() ? ParamLED_TW_ChLightDimmNightOffTime : ParamLED_TW_ChLightDimmDayOffTime;
}

uint16_t TWChannel::dimmingTime(bool switchOn)
{
    return switchOn ? dimmingTimeON() : dimmingTimeOFF();
}

uint16_t TWChannel::dimmingValStartup()
{
    return ParamLED_TW_ChStartupBehavior ? getLastOnValue() : dimmingValMax();
}

uint16_t TWChannel::dimmingValMin()
{
    return ParamLED_TW_ChBrightnessMin * VALUE_KNX_MULTIPLY;
}

uint16_t TWChannel::dimmingValMax()
{
    return getNight() ? (ParamLED_TW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY) : (ParamLED_TW_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY);
}

uint16_t TWChannel::dimmingValTarget(bool switchOn)
{
    return switchOn ? dimmingValStartup() : 0;
}

uint16_t TWChannel::checkMinMaxBrightness(uint16_t bright)
{
    if (bright < (ParamLED_TW_ChBrightnessMin * VALUE_KNX_MULTIPLY))
    {
        bright = (ParamLED_TW_ChBrightnessMin * VALUE_KNX_MULTIPLY);
    }
    if (bright > dimmingValMax())
    {
        bright = dimmingValMax();
    }
    return bright;
}

int32_t TWChannel::dimmingTempStartup()
{
    return ParamLED_TW_ChStartupBehavior ? getLastOnValueTemp() : dimmingTempMax();
}

int32_t TWChannel::dimmingTempMax()
{
    return getNight() ? ParamLED_TW_ChColorTempNight : ParamLED_TW_ChColorTempDay;
}

int32_t TWChannel::dimmingTempTarget(bool switchOn)
{
    return switchOn ? dimmingTempStartup() : 0;
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

void TWChannel::setSwitch(bool switchOn)
{
    _boost = false;
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_TW_ChBrightnessMin * VALUE_KNX_MULTIPLY, 1);
        // in case of stairway light
        if (ParamLED_TW_ChStairCaseActive && ParamLED_TW_ChStairCaseTrigger == 0)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        _brightness.setTargetValue(dimmingValTarget(switchOn), dimmingTime(switchOn));
        _colorTemperature.setTargetValue(dimmingTempTarget(switchOn), dimmingTime(switchOn));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_TW_ChStairCaseActive && ParamLED_TW_ChStairCaseTrigger == 1)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        else
        {
            setLastOnValue(_brightness.value());
            setLastOnValueTemp(_colorTemperature.value());

            _brightness.setTargetValue(dimmingValTarget(switchOn), dimmingTime(switchOn));
        }
    }
    logDebugP("dimmingValTarget: %3X", dimmingValTarget(switchOn));
    logDebugP("dimmingTempTarget: %3X", dimmingTempTarget(switchOn));
    logDebugP("dimmingTime: %5X", dimmingTime(switchOn));
}

void TWChannel::setSwitchNoDim(bool switchOn)
{
    _boost = false;
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(dimmingValTarget(switchOn), 1);
        _colorTemperature.setTargetValue(dimmingTempTarget(switchOn), 1);
    }
    else
    {
        logDebugP("switch_OFF");
        setLastOnValue(_brightness.value());
        setLastOnValueTemp(_colorTemperature.value());
        _brightness.setTargetValue(dimmingValTarget(switchOn), 1);
    }
}

void TWChannel::setBoost(bool switchOn)
{
    if (switchOn)
    {
        logDebugP("Boost_ON");
        //_brightness.setTargetValue(dimmingValTarget(switchOn), 1);
        //_pDimmer->setLevel(0xFFF, _pHWChannels[0]);
        //_pDimmer->setLevel(0xFFF, _pHWChannels[1]);
        _boost = true;
        uint16_t ww = _pDimmer->getScaleMax((HWDimmer::DimLUTType)ParamLED_TW_ChDimCurve);
        uint16_t cw = _pDimmer->getScaleMax((HWDimmer::DimLUTType)ParamLED_TW_ChDimCurve);

        if (_pHWChannels[0] < LED_ChannelCount)
        {
            _pDimmer->setLevel(ww, _pHWChannels[0]);
        }

        if (_pHWChannels[1] < LED_ChannelCount)
        {
            _pDimmer->setLevel(cw, _pHWChannels[1]);
        }
    }
    else
    {
        logDebugP("Boost_OFF");
        //_brightness.setTargetValue(dimmingValTarget(switchOn), 1);
        _boost = false;
        _pDimmer->setLevel(0x0, _pHWChannels[0]);
        _pDimmer->setLevel(0x0, _pHWChannels[1]);
    }
}

void TWChannel::setBrightness(uint16_t bright)
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("setBrightness: %3X", bright);
    bright = checkMinMaxBrightness(bright);
    _brightness.setTargetValue(bright, dimmingTimeON());
}

void TWChannel::setNight(bool night)
{
    if (ParamLED_TW_ChScenesDisableNightSw || (!ParamLED_TW_ChScenesDisableNightSw && _sceneNumberActive == 0))
    {
        _boost = false;
        _isNight = night;
        _brightness.setRange(ParamLED_TW_ChBrightnessMin * VALUE_KNX_MULTIPLY, dimmingValMax());

        if (!night)
        {
            logDebugP("Tag");

            if (_brightness.value() == ParamLED_TW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
            {
                _brightness.setTargetValue(ParamLED_TW_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY, 2 * ParamLED_TW_ChLightDimmDayOnTime);
            }
        }
        else
        {
            logDebugP("Nacht");

            if (_brightness.value() > ParamLED_TW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
            {
                _brightness.setTargetValue(ParamLED_TW_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY, 2 * ParamLED_TW_ChLightDimmNightOnTime);
            }
        }
    }
}

void TWChannel::relDimUp()
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_UP");
    _brightness.setTargetValue(dimmingValMax(), ParamLED_TW_ChLightDimmRelTime);
}

void TWChannel::relDimDown()
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_DOWN");
    _brightness.setTargetValue(dimmingValMin(), ParamLED_TW_ChLightDimmRelTime);
}

void TWChannel::relDimStop()
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_STOP");
    _brightness.setTargetValue(_brightness.value(), 1);
}

void TWChannel::setColorTemperature(uint16_t colorTemp)
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("setColorTemperature (colorTemp=%u)", colorTemp);
    colorTemp = checkMinMaxColorTemp(colorTemp);
    _colorTemperature.setTargetValue(colorTemp, dimmingTimeON());
}

void TWChannel::relDimUpColor()
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_UP");
    _colorTemperature.setTargetValue(ParamLED_TW_ChColorTempCW, ParamLED_TW_ChLightDimmRelTime);
}

void TWChannel::relDimDownColor()
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_DOWN");
    _colorTemperature.setTargetValue(ParamLED_TW_ChColorTempWW, ParamLED_TW_ChLightDimmRelTime);
}

void TWChannel::relDimStopColor()
{
    _boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_STOP");
    _colorTemperature.setTargetValue(_colorTemperature.value(), 1);
}


