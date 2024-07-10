#pragma once

#include "OpenKNX.h"
#include "hardware.h"
#include "HWDimmer.h"

class HWDimmerRP2040 : public HWDimmer
{
    public:
        HWDimmerRP2040(uint8_t pins[], uint8_t numChannels);

        bool setLevel(uint8_t level, uint8_t channel);
        std::string logPrefix();

    private:
        uint8_t *pins;
};
