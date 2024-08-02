#include <knx.h>
#include "LightChannel.h"

/**
 * @brief Construct a new Light Channel:: Light Channel object
 * 
 * @param channelIndex Index for this channel
 * @param pDimmer pointer to HWDimmer instance used on this platform
 * @param hwChannels array of HW channels for this light
 * @param numChannels number of HW channels this light has
 */
LightChannel::LightChannel(uint8_t channelIndex, HWDimmer* pDimmer, uint8_t hwChannels[], uint8_t numChannels) : _brightness()
{
  _channelIndex = channelIndex;
  this->_numChannels = numChannels;

  this->_pDimmer = pDimmer;
  logDebugP("debug setup");

  this->_pHWChannels = hwChannels;
}

/**
 * @brief Name definition of this channel
 * 
 * @return const std::string 
 */
const std::string LightChannel::name()
{
  return "LightChannel";
}

/**
 * @brief Loop function to call update on a regular basis
 * 
 */
void LightChannel::loop()
{
  if(delayCheckMillis(_lastTimestamp, UPDATE_DELAY))
  {
    _lastTimestamp = millis();
    update();
  }
}
