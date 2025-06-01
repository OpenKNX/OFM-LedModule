#pragma once

#include "OpenKNX.h"
#include "hardware.h"
#include "HWDimmer.h"
#include "hardware/timer.h"  // For RP2040 hardware timer support

#define DIM_RESOLUTION_BIT  13
#define DIM_RANGE           ((1 << DIM_RESOLUTION_BIT) - 1)

class HWDimmerRP2040 : public HWDimmer
{
    public:
        HWDimmerRP2040(uint8_t pins[], uint8_t numChannels, uint16_t pwmFreq);

        void loop();
        static void interruptSample();
        static int64_t timerCallback(alarm_id_t alarm_id, void* user_data);
        bool setLevel(uint16_t level, uint8_t channel);
        uint16_t scale(uint8_t level, HWDimmer::DimLUTType lutType);
        uint16_t getScaleMax(HWDimmer::DimLUTType lutType);
        std::string logPrefix();
        
    private:
        uint8_t *pins;
        uint32_t sampleTimer = 0;
        inline static int timerAlarmId = -1;
        inline static int sampleValue = 0;
};
