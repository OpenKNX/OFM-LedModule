#include "HWDimmerWS.h"

//// test file for led stripes with ws2811 chip
/**
 * @brief Construct a new HWDimmerPCA::HWDimmerPCA object
 *
 * @param type WS dimmer type
 */
HWDimmerWS::HWDimmerWS(HWDimmerWS::WSType type, uint8_t pin, uint16_t numLeds) : HWDimmer(LEDMODULE_MAX_LIGHT_CHANNELS), _pin(pin), _numLeds(numLeds)
{
    _pixels = Adafruit_NeoPixel(_numLeds, _pin, NEO_KHZ400);
    for (uint8_t ch = 0; ch < numChannels; ch++)
    {
        setLevel(0, ch);
    }
}

/**
 * @brief Set level of selected channel to value
 *
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmerWS::setLevel(uint16_t level, uint8_t channel)
{
    // logInfoP("setLevel_1");
    bool isValidChannel = false;
    if (setLevelInternal(level, channel))
    {
        isValidChannel = true;
        _pixels.clear();
        for (int i = 0; i < 100; i++)
        {
            uint16_t tmpLevel = getLevel(channel);
            _pixels.setPixelColor(i, _pixels.Color(tmpLevel, tmpLevel, tmpLevel));
            _pixels.show();
        }
        // logInfoP("setLevel_3");
    }
    return isValidChannel;
}
// bool HWDimmerWS::setLevel(uint16_t r_level, uint16_t g_level, uint16_t b_level, uint8_t channel)
// {
//     // logInfoP("setLevel_1");
//     bool isValidChannel = false;
//     if (HWDimmer::setLevel(r_level, channel) && HWDimmer::setLevel(g_level, channel) && HWDimmer::setLevel(b_level, channel))
//     {
//         isValidChannel = true;
//         _pixels.clear();
//         for (int i = 0; i < _numLeds; i++)
//         {
//             _pixels.setPixelColor(i, _pixels.Color(r_level, g_level, b_level));
//             _pixels.show();
//         }
//         // logInfoP("setLevel_3");
//     }
//     return isValidChannel;
// }

/**
 * @brief Scale uint16 value to range of this HWDimmer (uint16)
 *
 * @param level level as uint16
 * @param lutType lookup table selection
 * @return uint16_t level in new scale
 */
uint16_t HWDimmerWS::scale(uint16_t level, HWDimmer::DimLUTType lutType)
{
    return dimLUT[lutType].Val(level);
}

/**
 * @brief Get maximum allowed value in selected scale
 *
 * @param lutType lookup table selection
 * @return uint16_t maximum value of range
 */
uint16_t HWDimmerWS::getScaleMax(HWDimmer::DimLUTType lutType)
{
    return dimLUT[lutType].Max();
}

/**
 * @brief Prefix of this module when using OpenKNX log functions
 *
 * @return std::string Prefix
 */
std::string HWDimmerWS::logPrefix()
{
    return "WSHWDimmer";
}

void HWDimmerWS::outputLUT()
{
    for (int i = 0; i < VALUE_KNX_COUNT; i++)
    {
        logDebugP("Count%d: %d", i, dimLUT[0].Val(i));
    }
}

/**
 * @brief Linear lookup tables to map 255% level to PCA driver levels
 *  0: Linear, 1: logarithmic x^1.5
 */
HWDimmer::LUT<VALUE_KNX_COUNT> HWDimmer::dimLUT[] = {HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.0), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.5), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 2)};
