#include "SingleChannel.h"
#include <knx.h>

SingleChannel::SingleChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[1])
    : LightChannel(index, pDimmer, hwChannels, 1)
{
    logInfoP("Trying to read Config from KNX...");
    logHexInfoP((uint8_t*)knx.paramData(LED_SC_ParamCalcIndex(LED_SC_SceneA_Type_)), 8);
    _scenes = new SceneConfig[N_SCENES];
    memcpy(_scenes, knx.paramData(LED_SC_ParamCalcIndex(LED_SC_SceneA_Type_)), N_SCENES * sizeof(SceneConfig));

    _channelActive = hwChannels[0] != LED_INVALID_HW_CHANNEL;

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
    uint8_t tmpBrightness = _brightness.value();
    if (_lastBrightnessLevel != tmpBrightness)
    {
        _lastBrightnessLevel = tmpBrightness;

        bool stateOn = tmpBrightness > 0;
        if ((bool)KoLED_SC_StateOnOff_.value(DPT_State) != stateOn)
            KoLED_SC_StateOnOff_.value(tmpBrightness > 0, DPT_State);
    }

    if (delayCheckMillis(_lastTimestamp, UPDATE_DELAY))
    {
        _lastTimestamp = millis();

        if ((uint8_t)KoLED_SC_BrightnessStatus_.value(DPT_Scaling) != tmpBrightness)
        {
            logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
            KoLED_SC_BrightnessStatus_.value(tmpBrightness, DPT_Scaling);
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
        if (delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
        {
            _lastDimTimestamp = millis();
            _pDimmer->setLevel(_pDimmer->scale(_brightness.step(_lastDimTimestamp), (HWDimmer::DimLUTType)ParamLED_SC_DimCurve_), _pHWChannels[0]);
        }
        // Stairway Timeout
        if (((getStairTime() + (ParamLED_SC_StairCaseTimer_ * 1000)) <= millis()) && getStairTrigger())
        {
            setStairTrigger(0);
            if (ParamLED_SC_StartupBehavior_)
            {
                setLastOnValue(_brightness.value());
            }
            _brightness.setTargetValue(0, millis(), dimmingTimeOFF());
            /*
                        if (!SC_night())
                        {
                            _brightness.setTargetValue(0, millis(), ParamLED_SC_LightDimmTimeDayOFF_);
                        }
                        else if (SC_night())
                        {
                            _brightness.setTargetValue(0, millis(), ParamLED_SC_LightDimmTimeNightOFF_);
                        }*/
        }
    }

    processFrontInput();
}

void SingleChannel::processFrontInput()
{
    LightChannel::processFrontInput(ParamLED_SC_FrontControl_);
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

    if (relKO == LED_SC_KoLocking_)
    {
        _isLocked = ko.value(DPT_Switch);
    }
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_SC_KoSwitch_:
                if (!getLock())
                {
                    setSwitch(ko.value(DPT_Switch));
                }
                break;

            case LED_SC_KoStateOnOff_:
                break;

            case LED_SC_KoLocking_:
                setLock(ko.value(DPT_Switch));
                break;

            case LED_SC_KoStateLocking_:
                break;

            case LED_SC_KoBrightness_:
                if (!getLock())
                {
                    setBrightness(ko.value(DPT_Scaling));
                }
                break;

            case LED_SC_KoBrightnessStatus_:
                break;

            case LED_SC_KoDimRel_:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_SC_DimRel_.valueRef();

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

            case LED_SC_KoScene_:
                if (!getLock())
                {
                    handleScene(ko.value(DPT_SceneNumber));
                }
                break;

            // Day or Night
            case LED_SC_KoNight_:
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
                        _brightness.setTargetValue(_scenes[i].Brightness(), millis(), ParamLED_SC_LightDimmTimeDayON_);
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
    logDebugP("dimmingTimeDON: %5X", ParamLED_SC_LightDimmTimeDayON_);
    logDebugP("dimmingTimeNON: %5X", ParamLED_SC_LightDimmTimeNightON_);
    return getNight() ? ParamLED_SC_LightDimmTimeNightON_ : ParamLED_SC_LightDimmTimeDayON_;
}

uint16_t SingleChannel::dimmingTimeOFF()
{
    logDebugP("dimmingTimeDOFF: %5X", ParamLED_SC_LightDimmTimeDayOFF_);
    logDebugP("dimmingTimeNOFF: %5X", ParamLED_SC_LightDimmTimeNightOFF_);
    return getNight() ? ParamLED_SC_LightDimmTimeNightOFF_ : ParamLED_SC_LightDimmTimeDayOFF_;
}

uint16_t SingleChannel::dimmingTime(bool _switch)
{
    return _switch ? dimmingTimeON() : dimmingTimeOFF();
}

uint8_t SingleChannel::dimmingValStartup()
{
    return ParamLED_SC_StartupBehavior_ ? getLastOnValue() : dimmingValMax();
}

uint8_t SingleChannel::dimmingValMax()
{
    return getNight() ? ParamLED_SC_BrighnessMaxNight_ : ParamLED_SC_BrighnessMaxDay_;
}

uint8_t SingleChannel::dimmingValTarget(bool _switch)
{
    return _switch ? dimmingValStartup() : 0;
}

uint8_t SingleChannel::checkMinMaxBrightness(uint8_t _bright)
{
    if (_bright < ParamLED_SC_BrighnessMin_)
    {
        _bright = ParamLED_SC_BrighnessMin_;
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
        _brightness.setTargetValue(ParamLED_SC_BrighnessMin_, millis(), 1);
        // in case of stairway light
        if (ParamLED_SC_StairCaseActive_ && ParamLED_SC_StaicCaseTrigger_ == 0)
        {
            setStairTime(millis());
            setStairTrigger(1);
        }
        _brightness.setTargetValue(dimmingValTarget(_switch), millis(), dimmingTime(_switch));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_SC_StairCaseActive_ && ParamLED_SC_StaicCaseTrigger_ == 1)
        {
            setStairTime(millis());
            setStairTrigger(1);
        }
        else
        {
            setLastOnValue(_brightness.value());
            _brightness.setTargetValue(dimmingValTarget(_switch), millis(), dimmingTime(_switch));
        }
    }
    logDebugP("dimmingValTarget: %3X", dimmingValTarget(_switch));
    logDebugP("dimmingTime: %5X", dimmingTime(_switch));
}

void SingleChannel::setBrightness(uint8_t _bright)
{
    logDebugP("setBrightness: %3X", _bright);
    _bright = checkMinMaxBrightness(_bright);
    _brightness.setTargetValue(_bright, millis(), dimmingTimeON());
}

void SingleChannel::setNight(bool _night)
{
    _isNight = _night;
    _brightness.setRange(ParamLED_SC_BrighnessMin_, dimmingValMax());

    if (!_night)
    {
        logDebugP("Tag");

        if (_brightness.value() == ParamLED_SC_BrighnessMaxNight_)
        {
            _brightness.setTargetValue(ParamLED_SC_BrighnessMaxDay_, millis(), 2 * ParamLED_SC_LightDimmTimeDayON_);
        }
    }
    else
    {
        logDebugP("Nacht");

        if (_brightness.value() > ParamLED_SC_BrighnessMaxNight_)
        {
            _brightness.setTargetValue(ParamLED_SC_BrighnessMaxNight_, millis(), 2 * ParamLED_SC_LightDimmTimeNightON_);
        }
    }
}

void SingleChannel::relDimUp()
{
    logDebugP("relDim_UP");
    _brightness.setTargetValue(dimmingValMax(), millis(), ParamLED_SC_LightDimmTimeRel_);
}

void SingleChannel::relDimDown()
{
    logDebugP("relDim_DOWN");
    _brightness.setTargetValue(ParamLED_SC_BrighnessMin_, millis(), ParamLED_SC_LightDimmTimeRel_);
}

void SingleChannel::relDimStop()
{
    logDebugP("relDim_STOP");
    _brightness.setTargetValue(_brightness.value(), millis(), 1);
}
