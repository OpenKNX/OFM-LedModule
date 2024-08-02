#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"
#include "LightChannel.h"

class TWChannel : public LightChannel
{
  public:

    TWChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[2]);
    void processInputKo(GroupObject& ko);
    void dimLoop();
    void update();
    void loop();

  private:
  
    const std::string name() override;
  
    uint16_t _lastColorTemp = 0;
    DimmableValue<uint16_t> _colorTemperature;
};
