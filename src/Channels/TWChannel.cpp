#include "TWChannel.h"
#include <knx.h>

TWChannel::TWChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[2])
    : LightChannel(index, pDimmer, hwChannels, 2), _colorTemperature(4000, ParamLED_TW_ColorTempWW_, ParamLED_TW_ColorTempCW_)
{
    logInfoP("Trying to read Config from KNX...");
    logHexInfoP((uint8_t*)knx.paramData(LED_TW_ParamCalcIndex(LED_TW_SceneA_Type_)), 8);
    _scenes = new SceneConfig[N_SCENES];
    memcpy(_scenes, knx.paramData(LED_TW_ParamCalcIndex(LED_TW_SceneA_Type_)), N_SCENES * sizeof(SceneConfig));

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
    uint8_t tmpbright = _brightness.value();
    uint16_t tmpColor = _colorTemperature.value();
    if (_lastBrightnessLevel != tmpbright)
    {
        logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpbright);
        _lastBrightnessLevel = tmpbright;
        _lastColorTemp = tmpColor;
        KoLED_TW_BrightnessStatus_.value(tmpbright, DPT_Percent_U8);
        KoLED_TW_StateOnOff_.value(tmpbright > 0, DPT_State);
    }
    if (_lastColorTemp != tmpColor)
    {
        logDebugP("update: CT: %d -> %d", _lastColorTemp, tmpColor);
        _lastColorTemp = tmpColor;
        KoLED_TW_ColorTemperatureStatus_.value(tmpColor, Dpt(7, 600));
    }
}

void TWChannel::loop()
{
    LightChannel::loop();

    if (delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
    {
        _lastDimTimestamp = millis();
        uint16_t brightValue = _pDimmer->scale(_brightness.step(_lastDimTimestamp), HWDimmer::DimLUTType::Log1_5);
        uint16_t colorTempValue = _colorTemperature.step(_lastDimTimestamp);

        uint16_t ww = ((uint32_t)brightValue * (colorTempValue - ParamLED_TW_ColorTempWW_)) / (ParamLED_TW_ColorTempCW_ - ParamLED_TW_ColorTempWW_);
        uint16_t cw = ((uint32_t)brightValue * (ParamLED_TW_ColorTempCW_ - colorTempValue)) / (ParamLED_TW_ColorTempCW_ - ParamLED_TW_ColorTempWW_);

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
    if (((getStairTime() + (ParamLED_SC_StairCaseTimer_ * 1000)) <= millis()) && getStairTrigger())
    {
        setStairTrigger(0);
        if (ParamLED_TW_StartupBehavior_)
        {
            setLastOnValue(_brightness.value());
        }
        _brightness.setTargetValue(0, millis(), dimmingTimeOFF());
    }
}

bool TWChannel::TW_night()
{
    return _tw_night;
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

    if (relKO == LED_TW_KoLocking_)
    {
        _isLocked = ko.value(DPT_Switch);
    }
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_TW_KoSwitch_:
                setSwitch(ko.value(DPT_Switch));
                break;

            case LED_TW_KoStateOnOff_:
                break;
            case LED_TW_KoLocking_:
                break;

            case LED_TW_KoBrightness_:
                setBrightness(ko.value(DPT_Percent_U8));
                break;

            case LED_TW_KoBrightnessStatus_:
                break;
            case LED_TW_KoDimRel_:
                int16_t tmpu16;
                tmpu16 = *KoLED_TW_DimRel_.valueRef();

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

            case LED_TW_KoScene_:
                handleScene(ko.value(DPT_SceneNumber));
                break;

            case LED_TW_KoSceneStatus_:
                break;

            case LED_TW_KoColorTemperature_:
                setColorTemperature(ko.value(Dpt(7, 600)));

                break;

            case LED_TW_KoColorTemperatureStatus_:
                break;

            // Day or Night
            case LED_TW_KoNight_:
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
        if (sceneNr + 1 == _scenes[i].sceneNr)
        {
            switch (_scenes[i].funcType)
            {
                default:
                case SceneConfig::FuncType::INACTIVE:
                    break;

                case SceneConfig::FuncType::VALUE:
                    if (_scenes[i].valueType == ValueType::BRIGHTNESS || _scenes[i].valueType == ValueType::COMBINED)
                    {
                        _brightness.setTargetValue(_scenes[i].Brightness(), millis(), ParamLED_TW_LightDimmTimeDayON_);
                    }
                    if (_scenes[i].valueType == ValueType::TEMTPERATURE || _scenes[i].valueType == ValueType::COMBINED)
                    {
                        _colorTemperature.setTargetValue(_scenes[i].ColorTemperature(), millis(), ParamLED_TW_LightDimmTimeDayON_);
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
    return TW_night() ? ParamLED_TW_LightDimmTimeNightON_ : ParamLED_SC_LightDimmTimeDayON_;
}

uint16_t TWChannel::dimmingTimeOFF()
{
    return TW_night() ? ParamLED_TW_LightDimmTimeNightOFF_ : ParamLED_SC_LightDimmTimeDayOFF_;
}

uint16_t TWChannel::dimmingTime(bool _switch)
{
    return _switch ? dimmingTimeON() : dimmingTimeOFF();
}

uint8_t TWChannel::dimmingValMaxBehavior()
{
    return ParamLED_TW_StartupBehavior_ ? getLastOnValue() : maxDimVal();
}

uint8_t TWChannel::maxDimVal()
{
    return TW_night() ? ParamLED_TW_BrighnessMaxNight_ : ParamLED_TW_BrighnessMaxDay_;
}

uint8_t TWChannel::upperTargetValue()
{
    return ParamLED_TW_StartupBehavior_ ? getLastOnValue() : maxDimVal();
}

uint8_t TWChannel::dimmingTarget(bool _switch)
{
    return _switch ? dimmingValMaxBehavior() : 0;
}

uint16_t TWChannel::checkMinMaxColorTemp(uint16_t colorTemp)
{
    if (colorTemp > ParamLED_TW_ColorTempCW_)
    {
        colorTemp = ParamLED_TW_ColorTempCW_;
    }
    else if (colorTemp < ParamLED_TW_ColorTempWW_)
    {
        colorTemp = ParamLED_TW_ColorTempWW_;
    }
    return colorTemp;
}

void TWChannel::setSwitch(bool _switch)
{
    if (_switch)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_TW_BrighnessMin_, millis(), 1);
        // in case of stairway light
        if (ParamLED_TW_StairCaseActive_ && ParamLED_TW_StaicCaseTrigger_ == 0)
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
        if (ParamLED_TW_StairCaseActive_ && ParamLED_TW_StaicCaseTrigger_ == 1)
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

void TWChannel::setBrightness(uint8_t _bright)
{
    logDebugP("setBrightness: %3X", _bright);
    _brightness.setTargetValue(_bright, millis(), dimmingTimeON());
}

void TWChannel::setNight(bool _night)
{
    _tw_night = _night;
    _brightness.setRange(ParamLED_TW_BrighnessMin_, maxDimVal());

    if (_night)
    {
        logDebugP("Tag");

        if (_brightness.value() == ParamLED_TW_BrighnessMaxNight_)
        {
            _brightness.setTargetValue(ParamLED_TW_BrighnessMaxDay_, millis(), 2 * ParamLED_TW_LightDimmTimeDayON_);
        }
    }
    else
    {
        logDebugP("Nacht");

        if (_brightness.value() > ParamLED_TW_BrighnessMaxNight_)
        {
            _brightness.setTargetValue(ParamLED_TW_BrighnessMaxNight_, millis(), 2 * ParamLED_TW_LightDimmTimeNightON_);
        }
    }
}

void TWChannel::relDimUp()
{
    logDebugP("relDim_UP");
    _brightness.setTargetValue(maxDimVal(), millis(), ParamLED_TW_LightDimmTimeRel_);
}

void TWChannel::relDimDown()
{
    logDebugP("relDim_DOWN");
    _brightness.setTargetValue(ParamLED_TW_BrighnessMin_, millis(), ParamLED_TW_LightDimmTimeRel_);
}

void TWChannel::relDimStop()
{
    logDebugP("relDim_STOP");
    _brightness.setTargetValue(_brightness.value(), millis(), 1);
}

void TWChannel::setColorTemperature(uint16_t colorTemp)
{
    colorTemp = checkMinMaxColorTemp(colorTemp);
    _colorTemperature.setTargetValue(colorTemp, millis(), dimmingTimeON());
}