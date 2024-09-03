#include <knx.h>
#include "TWChannel.h"


TWChannel::TWChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[2]) : LightChannel(index, pDimmer, hwChannels, 2), _colorTemperature(4000)
{
  logInfoP("Trying to read Config from KNX...");
  logHexInfoP((uint8_t*) knx.paramData(LED_TW_ParamCalcIndex(LED_TW_SceneA_Type_)), 8);
  _scenes = new SceneConfig[N_SCENES];
  memcpy(_scenes, knx.paramData(LED_TW_ParamCalcIndex(LED_TW_SceneA_Type_)), N_SCENES * sizeof(SceneConfig));
  logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
  for(int i = 0; i < N_SCENES; i++)
  {
    logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",_channelIndex, _scenes[i].sceneNr,_scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed,   _scenes[i].value[0],  _scenes[i].value[1], _scenes[i].value[2]);
  }
}

const std::string TWChannel::name()
{
  return "TWChannel";
}

void TWChannel::update()
{ 
  uint8_t tmpbright = _brightness.value();
  uint16_t tmpColor = _colorTemperature.value();
  if(_lastBrightnessLevel != tmpbright)
  {
    logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpbright);
    _lastBrightnessLevel = tmpbright;
    _lastColorTemp = tmpColor;
    KoLED_TW_BrightnessStatus_.value(tmpbright, DPT_Percent_U8);
    KoLED_TW_StateOnOff_.value(tmpbright > 0, DPT_State);
  }
  if(_lastColorTemp != tmpColor)
  {
    logDebugP("update: CT: %d -> %d", _lastColorTemp, tmpColor);
    _lastColorTemp = tmpColor;
    KoLED_TW_ColorTemperatureStatus_.value(tmpColor, Dpt(7, 600));
  }
}

void TWChannel::loop()
{
  LightChannel::loop();

  if(delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
  {
    _lastDimTimestamp = millis();
    uint16_t brightValue = _pDimmer->scale(_brightness.step(_lastDimTimestamp), HWDimmer::DimLUTType::Log1_5);
    uint16_t colorTempValue = _colorTemperature.step(_lastDimTimestamp);

    uint16_t ww = ((uint32_t)brightValue * (colorTempValue - ParamLED_TW_ColorTempWW_)) / (ParamLED_TW_ColorTempCW_ - ParamLED_TW_ColorTempWW_);
    uint16_t cw = ((uint32_t)brightValue * (ParamLED_TW_ColorTempCW_ - colorTempValue)) / (ParamLED_TW_ColorTempCW_ - ParamLED_TW_ColorTempWW_);

    if(_pHWChannels[0] < LED_ChannelCount)
    {
      _pDimmer->setLevel(ww, _pHWChannels[0]);
    }
    
    if(_pHWChannels[1] < LED_ChannelCount)
    {
      _pDimmer->setLevel(cw, _pHWChannels[1]);
    }
  }
}

void TWChannel::processInputKo(GroupObject& ko)
{
  uint16_t colorTemp = 0;
  uint8_t tmpu8 = 0;
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

  if(relKO == LED_TW_KoLocking_)
  {
    _isLocked = ko.value(DPT_Switch);
  }
  else if(!_isLocked)
  {
    switch (relKO)
    {
      case LED_TW_KoSwitch_:
        if(ko.value(DPT_Switch))
        {
          tmpu8 = KoLED_SC_Brightness_.value(DPT_DecimalFactor);
          _brightness.setTargetValue(tmpu8 > 0 ? tmpu8 : BRIGHTNESS_MAX, millis(), ParamLED_TW_LightDimmTime_);
        }
        else
        {
          _brightness.setTargetValue(0, millis(), ParamLED_TW_LightDimmTime_);
        }
      break;

      case LED_TW_KoStateOnOff_: break;
      case LED_TW_KoLocking_: break;

      case LED_TW_KoBrightness_: 
        _brightness.setTargetValue(ko.value(DPT_DecimalFactor), millis(), ParamLED_TW_LightDimmTime_);
      break;

      case LED_TW_KoBrightnessStatus_: break;
      case LED_TW_KoDimRel_: break;
      
      case LED_TW_KoScene_: 
        handleScene(ko.value(DPT_SceneNumber));
        break;
      
      case LED_TW_KoSceneStatus_: break;

      case LED_TW_KoColorTemperature_:
        colorTemp = ko.value(Dpt(7,600));
        if(colorTemp > ParamLED_TW_ColorTempCW_)
        {
          colorTemp = ParamLED_TW_ColorTempCW_;
        }
        else if(colorTemp < ParamLED_TW_ColorTempWW_)
        {
          colorTemp = ParamLED_TW_ColorTempWW_;
        }
        _colorTemperature.setTargetValue(colorTemp, millis(), ParamLED_TW_LightDimmTime_);
        break;

      case LED_TW_KoColorTemperatureStatus_: break;
      default:
        logDebugP("Unknown KO %d", relKO);
        break;
    }
  }
}


void TWChannel::dimLoop()
{


}

void TWChannel::handleScene(uint8_t sceneNr)
{
  for(int i = 0; i < N_SCENES; i++)
  {
    if(sceneNr+1 == _scenes[i].sceneNr)
    {
      switch(_scenes[i].funcType)
      {
        default:
        case SceneConfig::FuncType::INACTIVE: break;

        case SceneConfig::FuncType::VALUE: 
          if(_scenes[i].valueType == ValueType::BRIGHTNESS || _scenes[i].valueType == ValueType::COMBINED)
          {
            _brightness.setTargetValue(_scenes[i].Brightness(), millis(), ParamLED_TW_LightDimmTime_);
          }
          if(_scenes[i].valueType == ValueType::TEMTPERATURE || _scenes[i].valueType == ValueType::COMBINED)
          {
            _colorTemperature.setTargetValue(_scenes[i].ColorTemperature(), millis(), ParamLED_TW_LightDimmTime_);
          }
          logDebugP("Scene: %d, BR: %d, CT: %d", sceneNr,_scenes[i].Brightness(),_scenes[i].ColorTemperature() );
        break;

        case SceneConfig::FuncType::FUNCTION: break;
        case SceneConfig::FuncType::SEQUENCE: break;
        case SceneConfig::FuncType::LOCKING: break;
      }
    }
  }
}