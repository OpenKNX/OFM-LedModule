#pragma once

#include "HWDimmer.h"
#include "OpenKNX.h"
#include "hardware.h"

#define DIM_RESOLUTION_BIT 16
#define DIM_RANGE ((1 << DIM_RESOLUTION_BIT) - 1)

class HWDimmerRP2040 : public HWDimmer
{
  public:
    HWDimmerRP2040(uint8_t pins[], uint8_t numChannels, uint16_t pwmFreq);

    bool setLevel(uint16_t level, uint8_t channel);
    uint16_t scale(uint16_t level, HWDimmer::DimLUTType lutType);
    uint16_t getScaleMax(HWDimmer::DimLUTType lutType);
    void outputLUT();
    bool checkConnection();
    void runTestMode();
    std::string logPrefix();

  private:
    uint8_t *pins;
};
