#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"

// converts a relative (to the channel start) KO number to an absolute KO number of the device
#define AbsKO(asap)    (LED_KoOffset + LED_KoBlockSize * _channelIndex + asap)

// converts a absolute KO number to a relative KO number (offset to the starting KO number of the channel)
#define RelKO(asap)    (asap - LED_KoOffset - LED_KoBlockSize * _channelIndex)

class LightChannel : public OpenKNX::Channel
{
  private:
    const std::string name() override;
    HWDimmer* pDimmer;
    void activateScene(uint8_t sceneId);

    void setBrightness(uint8_t level);
    void setOnOff(bool onOff);
    uint8_t hwChannels[LEDMODULE_MAX_LIGHT_CHANNELS];
    uint8_t lightType;
    uint32_t dimmDuration;

  public:
    LightChannel(uint8_t channel_number, HWDimmer* pDimmer);
    void Loop();
    void ProcessInputKo(GroupObject& ko);
    void Save();
    void Restore();
    void dimLightTo(uint32_t targetBrightness[LEDMODULE_MAX_LIGHT_CHANNELS], uint16_t dimTime);
};