#pragma once

#include "OpenKNX.h"
#include "hardware.h"
#include "HWDimmer.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "LedModuleConfig.h"

#define DIM_RESOLUTION_BIT  12
#define DIM_RANGE           ((1 << DIM_RESOLUTION_BIT) - 1)

class HWDimmerPCA : public HWDimmer
{
    public:

        enum PCAType
        {
            PCA9685,
        };

        HWDimmerPCA(HWDimmerPCA::PCAType type, uint8_t addr, uint16_t pwmFreq);

        std::string logPrefix();
        bool setLevel(uint16_t level, uint8_t channel);
        uint16_t scale(uint16_t level, HWDimmer::DimLUTType lutType);
        uint16_t getScaleMax(HWDimmer::DimLUTType lutType);
        void outputLUT();
        bool checkConnection();
        void reconnect();

    private:
    
        Adafruit_PWMServoDriver _pwm;
        uint8_t _addr;
};
