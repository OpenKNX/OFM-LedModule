#include <knx.h>
#include "TWChannel.h"


TWChannel::TWChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[2]) : LightChannel(index, pDimmer, hwChannels, 2)
{

}

const std::string TWChannel::name()
{
  return "TWChannel";
}

void TWChannel::update()
{ 
  uint8_t tmpLevel1 = pDimmer->getLevel(hwChannels[0]);
  uint8_t tmpLevel2 = pDimmer->getLevel(hwChannels[1]);
  if(lastLevel !=tmpLevel1 + tmpLevel2)
  {
    logDebugP("update KO: %d -> %d", lastLevel, uint8_t(tmpLevel1 + tmpLevel2));
    lastLevel = tmpLevel1 + tmpLevel2;
    logDebugP("new: %d", lastLevel);
    KoLED_TW_BrightnessStatus_.value((uint8_t)((tmpLevel1 + tmpLevel2 +0U)/2), DPT_Percent_U8);
    KoLED_TW_StateOnOff_.value(tmpLevel1 > 0 ||tmpLevel2 > 0, DPT_State);
  }
}

void TWChannel::processInputKo(GroupObject& ko)
{
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());

  int16_t relKO = (ko.asap() - LED_TW_KoOffset);
  // check if channel is valid
  if ((int8_t)(relKO / LED_TW_KoBlockSize) == channelIndex())
      relKO = relKO % LED_TW_KoBlockSize;
  else
      relKO = -1;

  switch (relKO)
  {
    case LED_TW_KoSwitch_:
      if(ko.value(DPT_Switch))
      {
        pDimmer->dimToLevel(hwChannels[0], 255, 1000);
        pDimmer->dimToLevel(hwChannels[1], 255, 1000);
      }
      else
      {
        pDimmer->dimToLevel(hwChannels[0], 0, 1000);
        pDimmer->dimToLevel(hwChannels[1], 0, 1000);
      }
    break;
    case LED_TW_KoStateOnOff_: break;
    case LED_TW_KoLocking_: break;
    case LED_TW_KoBrightness_: 
        pDimmer->dimToLevel(hwChannels[0], ko.value(DPT_DecimalFactor), 1000);
        pDimmer->dimToLevel(hwChannels[1], ko.value(DPT_DecimalFactor), 1000);
    break;
    case LED_TW_KoBrightnessStatus_: break;
    case LED_TW_KoDimRel_: break;
    case LED_TW_KoScene_: break;
    case LED_TW_KoSceneStatus_: break;
    case LED_TW_KoColorTemperature_: break;
    case LED_TW_KoColorTemperatureStatus_: break;
    default:
      logDebugP("Unknown KO %d", relKO);
      break;
  }
}


void TWChannel::dimLoop()
{


}
