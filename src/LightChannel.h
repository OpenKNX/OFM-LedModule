#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"

class LightChannel : public OpenKNX::Channel
{
  public:
    LightChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[], uint8_t numChannels);
    void loop();
    void processInputKo(GroupObject& ko);
    void Save();
    void Restore();

  protected:
    virtual void update();
    uint8_t numChannels;
    HWDimmer* pDimmer;
    uint8_t *hwChannels;

    uint32_t lastTimestamp = 0;
    
    uint16_t lastLevel = 0;

  private:
    const std::string name() override;
};