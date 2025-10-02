#include "SingleChannel.h"
#include <knx.h>

SingleChannel::SingleChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[1])
    : LightChannel(index, pDimmer, hwChannels, 1)
{
    logInfoP("Trying to read Config from KNX...");
    logHexInfoP((uint8_t*)knx.paramData(LED_SC_ParamCalcIndex(LED_SC_ChSceneA_Type)), 8);
    _scenes = new SceneConfig[N_SCENES];
    memcpy(_scenes, knx.paramData(LED_SC_ParamCalcIndex(LED_SC_ChSceneA_Type)), N_SCENES * sizeof(SceneConfig));

    _channelActive = hwChannels[0] != LED_INVALID_HW_CHANNEL;

    KoLED_SC_ChStateOnOff.value(false, DPT_State);
    KoLED_SC_ChBrightnessStatus.value((uint16_t)(_brightness.value() / VALUE_KNX_MULTIPLY), DPT_Scaling);

#ifdef EXT_DEBUG_LOG
    logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
    for (int i = 0; i < N_SCENES; i++)
    {
        logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", _channelIndex, _scenes[i].sceneNr, _scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed, _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2]);
    }
#endif
}

const std::string SingleChannel::name()
{
    return "SingleChannel";
}

void SingleChannel::update()
{
    uint16_t tmpBrightness = _brightness.value();
    if (_lastBrightnessLevel != tmpBrightness)
    {
        _lastBrightnessLevel = tmpBrightness;

        bool stateOn = tmpBrightness > 0;
        if ((bool)KoLED_SC_ChStateOnOff.value(DPT_State) != stateOn)
            KoLED_SC_ChStateOnOff.value(tmpBrightness > 0, DPT_State);
    }

    if (delayCheckMillis(_lastTimestamp, UPDATE_DELAY))
    {
        _lastTimestamp = delayTimerInit();

        if (((uint16_t)KoLED_SC_ChBrightnessStatus.value(DPT_Scaling) * VALUE_KNX_MULTIPLY) != tmpBrightness)
        {
            logDebugP("update: lastBrLevel: %d -> tmpBrLevel %d -> BR.value %d -> BR.step %d", _lastBrightnessLevel, tmpBrightness, _brightness.value(), 0);
            KoLED_SC_ChBrightnessStatus.value((uint16_t)(tmpBrightness / VALUE_KNX_MULTIPLY), DPT_Scaling);
        }
    }
}

void SingleChannel::loop()
{
    if (!_channelActive)
        return;

    LightChannel::loop();

    if (_pHWChannels[0] < LED_ChannelCount)
    {
        bool needsPowerUp = _brightness.value() == 0 && _brightness.target() > 0;
        bool canDim = !needsPowerUp || _pDimmer->powerSupplyAvailableOrRequest();
        if (canDim && delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
        {
            _lastDimTimestamp = delayTimerInit();
            _pDimmer->setLevel(_pDimmer->scale(_brightness.step(_lastDimTimestamp), (HWDimmer::DimLUTType)ParamLED_SC_ChDimCurve), _pHWChannels[0]);
        }

        // Stairway Timeout
        if (getStairTrigger() && delayCheckMillis(getStairTime(), ParamLED_SC_ChStairCaseTimer * 1000))
        {
            setStairTrigger(0);
            if (ParamLED_SC_ChStartupBehavior)
            {
                setLastOnValue(_brightness.value());
            }
            _brightness.setTargetValue(0, dimmingTimeOFF());
        }
    }
}

void SingleChannel::processInputKo(GroupObject& ko)
{
    // uint8_t tmpu8 = 0;
    int16_t relKO = (ko.asap() - LED_SC_KoOffset);

    logDebugP("processInputKo Channel");
    logHexDebugP(ko.valueRef(), ko.valueSize());

    // check if channel is valid
    if ((int8_t)(relKO / LED_SC_KoBlockSize) == channelIndex())
    {
        relKO = relKO % LED_SC_KoBlockSize;
    }
    else
    {
        relKO = -1;
    }

    if (relKO == LED_SC_KoChLocking)
    {
        _isLocked = ko.value(DPT_Switch);
    }
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_SC_KoChSwitch:
                if (!getLock())
                {
                    setSwitch(ko.value(DPT_Switch));
                }
                break;

            case LED_SC_KoChSwitchNoDim:
                if (!getLock())
                {
                    setSwitchNoDim(ko.value(DPT_Switch));
                }
                break;

            case LED_SC_KoChStateOnOff:
                break;

            case LED_SC_KoChLocking:
                setLock(ko.value(DPT_Switch));
                break;

            case LED_SC_KoChStateLocking:
                break;

            case LED_SC_KoChBrightness:
                if (!getLock())
                {
                    setBrightness((uint16_t)((uint16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
                }
                break;

            case LED_SC_KoChBrightnessStatus:
                break;

            case LED_SC_KoChDimRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_SC_ChDimRel.valueRef();

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

            case LED_SC_KoChScene:
                if (!getLock())
                {
                    handleScene(ko.value(DPT_SceneNumber));
                    _sceneNumberActive = (uint8_t)ko.value(DPT_SceneNumber) + 1;
                }
                break;

            // Day or Night
            case LED_SC_KoChNight:
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

void SingleChannel::handleScene(uint8_t sceneNr)
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
                    if (_scenes[i].valueType == ValueType::BRIGHTNESS)
                    {
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness() * VALUE_KNX_MULTIPLY), dimmingTime(1));
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

uint16_t SingleChannel::dimmingTimeON()
{
    return getNight() ? ParamLED_SC_ChLightDimmTimeNightON : ParamLED_SC_ChLightDimmTimeDayON;
}

uint16_t SingleChannel::dimmingTimeOFF()
{
    return getNight() ? ParamLED_SC_ChLightDimmTimeNightOFF : ParamLED_SC_ChLightDimmTimeDayOFF;
}

uint16_t SingleChannel::dimmingTime(bool _switch)
{
    return _switch ? dimmingTimeON() : dimmingTimeOFF();
}

uint16_t SingleChannel::dimmingValStartup()
{
    return ParamLED_SC_ChStartupBehavior ? getLastOnValue() : dimmingValMax();
}

uint16_t SingleChannel::dimmingValMin()
{
    return ParamLED_SC_ChBrighnessMin * VALUE_KNX_MULTIPLY;
}

uint16_t SingleChannel::dimmingValMax()
{
    return getNight() ? (ParamLED_SC_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY) : (ParamLED_SC_ChBrighnessMaxDay * VALUE_KNX_MULTIPLY);
}

uint16_t SingleChannel::dimmingValTarget(bool _switch)
{
    return _switch ? dimmingValStartup() : 0;
}

uint16_t SingleChannel::checkMinMaxBrightness(uint16_t _bright)
{
    if (_bright < (ParamLED_SC_ChBrighnessMin * VALUE_KNX_MULTIPLY))
    {
        _bright = (ParamLED_SC_ChBrighnessMin * VALUE_KNX_MULTIPLY);
    }
    if (_bright > dimmingValMax())
    {
        _bright = dimmingValMax();
    }
    return _bright;
}

void SingleChannel::setSwitch(bool _switch)
{
    if (_switch)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_SC_ChBrighnessMin * VALUE_KNX_MULTIPLY, 1);
        // in case of stairway light
        if (ParamLED_SC_ChStairCaseActive && ParamLED_SC_ChStaicCaseTrigger == 0)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        _brightness.setTargetValue(dimmingValTarget(_switch), dimmingTime(_switch));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_SC_ChStairCaseActive && ParamLED_SC_ChStaicCaseTrigger == 1)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        else
        {
            setLastOnValue(_brightness.value());
            _brightness.setTargetValue(dimmingValTarget(_switch), dimmingTime(_switch));
        }
    }
    logDebugP("dimmingValTarget: %6X", dimmingValTarget(_switch));
    logDebugP("dimmingTime: %5X", dimmingTime(_switch));
    _sceneNumberActive = 0;
}

void SingleChannel::setSwitchNoDim(bool _switch)
{
    if (_switch)
    {
        logDebugP("NoDimSwitch_ON");
        _brightness.setTargetValue(dimmingValTarget(_switch), 1);
    }
    else
    {
        logDebugP("NoDimSwitch_OFF");
        setLastOnValue(_brightness.value());
        _brightness.setTargetValue(dimmingValTarget(_switch), 1);
    }
    _sceneNumberActive = 0;
}

void SingleChannel::setBrightness(uint16_t _bright)
{
    logDebugP("setBrightness(): %9X", _bright);
    _bright = checkMinMaxBrightness(_bright);
    _brightness.setTargetValue(_bright, dimmingTimeON());
    _sceneNumberActive = 0;
}

void SingleChannel::setNight(bool _night)
{

    logDebugP("setNight() %d -> %d", ParamLED_SC_ChNightSwitchScene, _sceneNumberActive);
    logDebugP("treppenlicht %d ", ParamLED_SC_ChStairCaseActive);
    if (ParamLED_SC_ChNightSwitchScene || (!ParamLED_SC_ChNightSwitchScene && _sceneNumberActive == 0))
    {
        logDebugP("Nachtmodus:");
        _isNight = _night;
        _brightness.setRange(ParamLED_SC_ChBrighnessMin * VALUE_KNX_MULTIPLY, dimmingValMax());
        if (!_night)
        {
            logDebugP("Tag");

            if (_brightness.value() == ParamLED_SC_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY)
            {
                _brightness.setTargetValue(ParamLED_SC_ChBrighnessMaxDay * VALUE_KNX_MULTIPLY, 2 * ParamLED_SC_ChLightDimmTimeDayON);
            }
        }
        else
        {
            logDebugP("Nacht");

            if (_brightness.value() > ParamLED_SC_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY)
            {
                _brightness.setTargetValue(ParamLED_SC_ChBrighnessMaxNight * VALUE_KNX_MULTIPLY, 2 * ParamLED_SC_ChLightDimmTimeNightON);
            }
        }
    }
}

void SingleChannel::relDimUp()
{
    logDebugP("relDim_UP");
    _brightness.setTargetValue(dimmingValMax(), ParamLED_SC_ChLightDimmTimeRel);
    _sceneNumberActive = 0;
}

void SingleChannel::relDimDown()
{
    logDebugP("relDim_DOWN");
    _brightness.setTargetValue(dimmingValMin(), ParamLED_SC_ChLightDimmTimeRel);
    _sceneNumberActive = 0;
}

void SingleChannel::relDimStop()
{
    logDebugP("relDim_STOP");
    _brightness.setTargetValue(_brightness.value(), 1);
    _sceneNumberActive = 0;
}
