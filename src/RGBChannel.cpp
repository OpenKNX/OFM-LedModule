#include <knx.h>
#include "RGBChannel.h"


RGBChannel::RGBChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[3]) : LightChannel(index, pDimmer, hwChannels, 3)
{

}

const std::string RGBChannel::name()
{
  return "RGBChannel";
}

void RGBChannel::update()
{ 
  uint8_t tmpLevel1 = pDimmer->getLevel(hwChannels[0]);
  uint8_t tmpLevel2 = pDimmer->getLevel(hwChannels[1]);
  uint8_t tmpLevel3 = pDimmer->getLevel(hwChannels[1]);
  if(lastLevel != tmpLevel1 + tmpLevel2 + tmpLevel3)
  {
    logDebugP("update KO: %d -> %d", lastLevel, tmpLevel1 + tmpLevel2 + tmpLevel3);
    lastLevel = tmpLevel1 + tmpLevel2 + tmpLevel3;
    KoLED_RGB_BrightnessStatus_.value((uint8_t)((tmpLevel1 + tmpLevel2 + tmpLevel3 +0U)/3), DPT_Percent_U8);
    KoLED_RGB_StateOnOff_.value(tmpLevel1 > 0 || tmpLevel2 > 0 || tmpLevel3 > 0, DPT_State);
  }
}

void RGBChannel::processInputKo(GroupObject& ko)
{
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());

  int16_t relKO = (ko.asap() - LED_RGB_KoOffset);
  // check if channel is valid
  if ((int8_t)(relKO / LED_RGB_KoBlockSize) == channelIndex())
      relKO = relKO % LED_RGB_KoBlockSize;
  else
      relKO = -1;
  uint32_t LED_RGB = 0;

  switch (relKO)
  {
    case LED_RGB_KoSwitch_: 
    if(ko.value(DPT_Switch))
      {
        pDimmer->dimToLevel(hwChannels[0], 255, 1000);
        pDimmer->dimToLevel(hwChannels[1], 255, 1000);
        pDimmer->dimToLevel(hwChannels[2], 255, 1000);
      }
      else
      {
        pDimmer->dimToLevel(hwChannels[0], 0, 1000);
        pDimmer->dimToLevel(hwChannels[1], 0, 1000);
        pDimmer->dimToLevel(hwChannels[2], 0, 1000);
      }
    break;
    case LED_RGB_KoStateOnOff_: break;
    case LED_RGB_KoLocking_: break;
    case LED_RGB_KoBrightness_: 
        pDimmer->dimToLevel(hwChannels[0], ko.value(DPT_DecimalFactor), 1000);
        pDimmer->dimToLevel(hwChannels[1], ko.value(DPT_DecimalFactor), 1000);
        pDimmer->dimToLevel(hwChannels[2], ko.value(DPT_DecimalFactor), 1000);
    break;
    case LED_RGB_KoBrightnessStatus_: break;
    case LED_RGB_KoDimRel_: break;
    case LED_RGB_KoScene_: break;
    case LED_RGB_KoSceneStatus_: break;

    case LED_RGB_KoRGB_:
      LED_RGB = ko.value(DPT_Colour_RGB);
      pDimmer->dimToLevel(hwChannels[0], (LED_RGB >> 16) & 0xFF, 1000);
      pDimmer->dimToLevel(hwChannels[1], (LED_RGB >> 8) & 0xFF, 1000);
      pDimmer->dimToLevel(hwChannels[2], LED_RGB & 0xFF, 1000);
      break;

    case LED_RGB_KoHSV_: break;

    default:
      logDebugP("Unknown KO %d", relKO);
      break;
  }
}
