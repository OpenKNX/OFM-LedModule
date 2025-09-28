#pragma once
//// test file for led stripes with ws2811 chip
#include "HWDimmer.h"
#include "OpenKNX.h"
#include "hardware.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

#include "LedModuleConfig.h"

#define DIM_RESOLUTION_BIT 8
#define DIM_RANGE ((1 << DIM_RESOLUTION_BIT) - 1)

class HWDimmerWS : public HWDimmer
{
  public:
    enum WSType
    {
        WS2811,
    };

    HWDimmerWS(HWDimmerWS::WSType type, uint8_t pin, uint16_t numLeds);

    std::string logPrefix();
    bool setLevel(uint16_t level, uint8_t channel);
    //bool setLevel(uint16_t r_level, uint16_t g_level, uint16_t b_level, uint8_t channel);
    uint16_t scale(uint16_t level, HWDimmer::DimLUTType lutType);
    uint16_t getScaleMax(HWDimmer::DimLUTType lutType);
    void outputLUT();

  private:
    Adafruit_NeoPixel _pixels;
    uint8_t _pin;
    uint16_t _numLeds;
};
