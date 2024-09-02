#include <knx.h>
#include "SingleChannel.h"


SingleChannel::SingleChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[1]) : LightChannel(index, pDimmer, hwChannels, 1)
{
  logInfoP("Trying to read Config from KNX...");
  logHexInfoP((uint8_t*) knx.paramData(LED_SC_ParamCalcIndex(LED_SC_SceneA_Type_)), 8);
  _scenes = new SceneConfig[N_SCENES];
  memcpy(_scenes, knx.paramData(LED_SC_ParamCalcIndex(LED_SC_SceneA_Type_)), N_SCENES * sizeof(SceneConfig));
  logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
  for(int i = 0; i < N_SCENES; i++)
  {
    logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",_channelIndex, _scenes[i].sceneNr,_scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed,   _scenes[i].value[0],  _scenes[i].value[1], _scenes[i].value[2]);
  }
}

const std::string SingleChannel::name()
{
  return "SingleChannel";
}

void SingleChannel::update()
{ 
  uint8_t tmpBrightness = _brightness.value();
  if(_lastBrightnessLevel != tmpBrightness)
  {
    logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBrightness);
    _lastBrightnessLevel = tmpBrightness;
    KoLED_SC_BrightnessStatus_.value(tmpBrightness, DPT_Percent_U8);
    KoLED_SC_StateOnOff_.value(tmpBrightness > 0, DPT_State);
  }
}

void SingleChannel::loop()
{
  LightChannel::loop();

  if(_pHWChannels[0] < LED_ChannelCount)
  {
    if(delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
    {
      _lastDimTimestamp = millis();
      _pDimmer->setLevel(_pDimmer->scale(_brightness.step(_lastDimTimestamp), HWDimmer::DimLUTType::Log1_5), _pHWChannels[0]);
    }
  }
}

void SingleChannel::processInputKo(GroupObject& ko)
{
  uint8_t tmpu8 = 0;
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
    
  if(relKO == LED_SC_KoLocking_)
  {
    _isLocked = ko.value(DPT_Switch);
  }
  else if(!_isLocked)
  {
    switch (relKO)
    {
      case LED_SC_KoSwitch_:
        if(ko.value(DPT_Switch))
        {
          tmpu8 = KoLED_SC_Brightness_.value(DPT_DecimalFactor);
          _brightness.setTargetValue(tmpu8 > 0 ? tmpu8 : BRIGHTNESS_MAX, millis(), ParamLED_SC_LightDimmTime_);
        }
        else
        {
          _brightness.setTargetValue(0, millis(), ParamLED_SC_LightDimmTime_);
        }
      break;

      case LED_SC_KoStateOnOff_: break;

      case LED_SC_KoBrightness_: 
          _brightness.setTargetValue(ko.value(DPT_Percent_U8), millis(), ParamLED_SC_LightDimmTime_);
      break;

      case LED_SC_KoBrightnessStatus_: break;

      case LED_SC_KoDimRel_: break;

      case LED_SC_KoScene_: 
        handleScene(ko.value(DPT_SceneNumber));
        break;

      case LED_SC_KoSceneStatus_: break;

      default:
        logDebugP("Unknown KO %d", relKO);
        break;
    }
  }

}

void SingleChannel::handleScene(uint8_t sceneNr)
{
  for(int i = 0; i < N_SCENES; i++)
  {
    if(sceneNr == _scenes[i].sceneNr)
    {
      switch(_scenes[i].funcType)
      {
        default:
        case SceneConfig::FuncType::INACTIVE: break;

        case SceneConfig::FuncType::VALUE: 
          if(_scenes[i].valueType == ValueType::BRIGHTNESS)
          {
            _brightness.setTargetValue(_scenes[i].Brightness(), millis(), ParamLED_SC_LightDimmTime_);
          }
        break;

        case SceneConfig::FuncType::FUNCTION: break;
        case SceneConfig::FuncType::SEQUENCE: break;
        case SceneConfig::FuncType::LOCKING: break;
      }
    }
  }
}