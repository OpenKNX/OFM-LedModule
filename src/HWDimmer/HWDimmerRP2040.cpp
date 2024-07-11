#include "HWDimmerRP2040.h"
#include "OpenKNX.h"

/**
 * @brief Construct a new HWDimmerRP2040::HWDimmerRP2040 object
 * 
 * @param pins pointer to array holding pin numbers to be used as PWM
 * @param numChannels number of channels
 */
HWDimmerRP2040::HWDimmerRP2040(uint8_t pins[], uint8_t numChannels, uint16_t pwmFreq) :HWDimmer(numChannels)
{
    this->pins = pins;
    for(uint8_t ch = 0; ch < numChannels; ch++)
    {
        pinMode(this->pins[ch], OUTPUT);
        setLevel(0, ch);
    }
    analogWriteFreq(pwmFreq); 
    analogWriteRange(UINT16_MAX);

    #if 0
        logDebugP("Lookup table:");
        for(int i=0; i<256; i+=16)
        {
            int idx = i;
            logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++));
        }
    #endif
}

/**
 * @brief Set level of selected channel to value
 * 
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmerRP2040::setLevel(uint8_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if(HWDimmer::setLevel(level, channel))
    {
        isValidChannel = true;
        analogWrite(pins[channel], dimLUT[DimLUTType::Log1_5].Val(level));
    }
    else
    {
        logErrorP("Invalif channel %d", channel);
    }
    return isValidChannel;
}

/**
 * @brief Prefix of this module when using OpenKNX log functions
 * 
 * @return std::string Prefix
 */
std::string HWDimmerRP2040::logPrefix()
{
    return "PicoHWDimmer";
}

/**
 * @brief Linear lookup tables to map 255% level to RP2040 PWM range
 *  0: Linear, 1: logarithmic x^1.5
 */
HWDimmer::LUT<VALUE_KNX_COUNT> HWDimmer::dimLUT[] = {HWDimmer::LUT<VALUE_KNX_COUNT>(UINT16_MAX, 1.0), HWDimmer::LUT<VALUE_KNX_COUNT>(UINT16_MAX, 1.5)};
