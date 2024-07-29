#include <knx.h>
#include "LightChannel.h"

#define DIMLOOP_DELAY       10 // ms
#define VALUE_MIN           0
#define VALUE_MAX           255
#define VALUE_KNX_COUNT     256

LightChannel::LightChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[], uint8_t numChannels)
{
  _channelIndex = index;
    this->numChannels = numChannels;

  this->pDimmer = pDimmer;
  logDebugP("debug setup");

  this->hwChannels = hwChannels;
}

const std::string LightChannel::name()
{
  return "LightChannel";
}

void LightChannel::update()
{
    logDebugP("UPDATE");
}

void LightChannel::loop()
{
  if(millis() - lastTimestamp > 500)
  {
    lastTimestamp = millis();
    update();
  }
}

void LightChannel::processInputKo(GroupObject& ko)
{
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());
}
