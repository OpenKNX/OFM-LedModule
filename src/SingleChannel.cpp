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
  uint8_t tmpLevel = pDimmer->getLevel(hwChannels[0]);
  if(lastLevel != tmpLevel)
  {
    logDebugP("update KO: %d -> %d", lastLevel, tmpLevel);
    lastLevel = tmpLevel;
    KoLED_SC_BrightnessStatus_.value(tmpLevel, DPT_Percent_U8);
    KoLED_SC_StateOnOff_.value(tmpLevel>0, DPT_State);
  }
}

void SingleChannel::processInputKo(GroupObject& ko)
{
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());

  int16_t relKO = (ko.asap() - LED_SC_KoOffset);
  // check if channel is valid
  if ((int8_t)(relKO / LED_SC_KoBlockSize) == channelIndex())
      relKO = relKO % LED_SC_KoBlockSize;
  else
      relKO = -1;

  switch (relKO)
  {
    case LED_SC_KoSwitch_:
      if(ko.value(DPT_Switch))
      {
        pDimmer->dimToLevel(hwChannels[0], 255, 1000);
      }
      else
      {
        pDimmer->dimToLevel(hwChannels[0], 0, 1000);
      }
    break;

    case LED_SC_KoStateOnOff_: break;

    case LED_SC_KoLocking_: break;

    case LED_SC_KoBrightness_: 
        pDimmer->dimToLevel(hwChannels[0], ko.value(DPT_Percent_U8), 1000);
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
