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
    analogWriteRange(DIM_RANGE);

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
bool HWDimmerRP2040::setLevel(uint16_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if(HWDimmer::setLevel(level, channel))
    {
        isValidChannel = true;
        analogWrite(pins[channel], min(level, DIM_RANGE));
    }
    return isValidChannel;
}

/**
 * @brief Scale uint8 value to range of this HWDimmer (uint16)
 * 
 * @param level level as uint8
 * @param lutType lookup table selection
 * @return uint16_t level in new scale
 */
uint16_t HWDimmerRP2040::scale(uint8_t level, HWDimmer::DimLUTType lutType)
{
    return dimLUT[lutType].Val(level);
}

/**
 * @brief Get maximum allowed value in selected scale
 * 
 * @param lutType lookup table selection
 * @return uint16_t maximum value of range
 */
uint16_t HWDimmerRP2040::getScaleMax(HWDimmer::DimLUTType lutType)
{
    return dimLUT[lutType].Max();
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
HWDimmer::LUT<VALUE_KNX_COUNT> HWDimmer::dimLUT[] = {HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.0), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.5)};
