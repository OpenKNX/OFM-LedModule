#include <knx.h>
#include "SingleChannel.h"


SingleChannel::SingleChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[1]) : LightChannel(index, pDimmer, hwChannels, 1)
{

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
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());

  uint8_t tmpu8=0;
  int16_t relKO = (ko.asap() - LED_SC_KoOffset);
  // check if channel is valid
  if ((int8_t)(relKO / LED_SC_KoBlockSize) == channelIndex())
      relKO = relKO % LED_SC_KoBlockSize;
  else
      relKO = -1;
    
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
          _brightness.setTargetValue(tmpu8 > 0 ? tmpu8 : 255, millis(), ParamLED_SC_LightDimmTime_);
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

      case LED_SC_KoScene_: break;

      case LED_SC_KoSceneStatus_: break;

      default:
        logDebugP("Unknown KO %d", relKO);
        break;
    }
  }

}
