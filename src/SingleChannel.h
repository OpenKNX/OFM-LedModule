#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"
#include "LightChannel.h"

class SingleChannel : public LightChannel
{
  public:
    SingleChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[1]);
    void processInputKo(GroupObject& ko);
    void update();


  private:
    const std::string name() override;

};