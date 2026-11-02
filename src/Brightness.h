#pragma once
#include <stdint.h>

namespace LedBrightness
{
constexpr uint16_t LevelMax = 4095;

constexpr uint16_t knxToLevel(uint8_t raw)
{
    return (uint32_t(raw) * LevelMax + 127) / 255;
}

// ETS stores 0..250 steps, displayed as 0..100 % in 0.4 % increments.
constexpr uint8_t parameterToKnx(uint8_t steps)
{
    return (uint32_t(steps) * 255 + 125) / 250;
}

constexpr uint16_t parameterToLevel(uint8_t steps)
{
    return knxToLevel(parameterToKnx(steps));
}

constexpr uint8_t levelToKnx(uint16_t level)
{
    return (uint32_t(level) * 255 + LevelMax / 2) / LevelMax;
}

}
