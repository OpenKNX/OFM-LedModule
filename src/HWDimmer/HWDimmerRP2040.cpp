#include "HWDimmerRP2040.h"
#include "OpenKNX.h"

/**
 * @brief Construct a new HWDimmerRP2040::HWDimmerRP2040 object
 * 
 * @param pins pointer to array holding pin numbers to be used as PWM
 * @param numChannels number of channels
 */
HWDimmerRP2040::HWDimmerRP2040(uint8_t pins[], uint8_t numChannels) :HWDimmer(numChannels)
{
    this->pins = pins;
    for(uint8_t ch = 0; ch < numChannels; ch++)
    {
        pinMode(this->pins[ch], OUTPUT);
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
bool HWDimmerRP2040::setLevel(uint8_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if(HWDimmer::setLevel(level, channel))
    {
        isValidChannel = true;
        analogWrite(pins[channel],level);
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
