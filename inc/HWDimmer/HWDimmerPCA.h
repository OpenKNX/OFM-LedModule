#pragma once

#include "OpenKNX.h"
#include "hardware.h"
#include "HWDimmer.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "ledmodulecfg.h"

class HWDimmerPCA : public HWDimmer
{
    public:
        enum PCAType
        {
            PCA9685,
        };

        HWDimmerPCA(HWDimmerPCA::PCAType type);

        std::string logPrefix();
        bool setLevel(uint8_t level, uint8_t channel);
        bool checkConnection();
        void reconnect();

    private:
        Adafruit_PWMServoDriver pwm;
};
