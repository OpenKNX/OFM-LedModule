#pragma once
//// test file for led stripes with ws2811 chip
#include "OpenKNX.h"
#include "hardware.h"
#include "HWDimmer.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#include "LedModuleConfig.h"

#define DIM_RESOLUTION_BIT  8
#define DIM_RANGE           ((1 << DIM_RESOLUTION_BIT) - 1)

class HWDimmerWS : public HWDimmer
{
    public:

        enum WSType
        {
            WS2811,
        };

        HWDimmerWS(HWDimmerWS::WSType type);

        std::string logPrefix();
        bool setLevel(uint16_t level, uint8_t channel);
        uint16_t scale(uint8_t level, HWDimmer::DimLUTType lutType);
        uint16_t getScaleMax(HWDimmer::DimLUTType lutType);
        bool checkConnection();
        void reconnect();

    private:
    
        Adafruit_NeoPixel pwm;
};
