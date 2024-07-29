#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"
#include "LightChannel.h"

class RGBChannel : public LightChannel
{
  public:
    RGBChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[3]);
    void processInputKo(GroupObject& ko);
    void update();

  private:
    const std::string name() override;

};