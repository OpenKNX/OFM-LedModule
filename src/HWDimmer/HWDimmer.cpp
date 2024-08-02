#include "HWDimmer.h"

/**
 * @brief Construct a new HWDimmer::HWDimmer object
 * 
 * @param numChannels 
 */
HWDimmer::HWDimmer(uint8_t numChannels)
{
    this->numChannels = numChannels;
    levels = new uint16_t[numChannels]{0};
}

/**
 * @brief Destroy the HWDimmer::HWDimmer object
 * 
 */
HWDimmer::~HWDimmer()
{
    delete[] levels;
}

/**
 * @brief Set level of selected channel to value
 * 
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmer::setLevel(uint16_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if(channel < numChannels)
    {
        levels[channel] = level;
        isValidChannel = true;
    }
    else
    {
        logErrorP("Invalid channel: %d", channel);
    }
    return isValidChannel;
}

/**
 * @brief Get level of selected channel to value
 * 
 * @param channel selected channel
 * @return uint8_t current level
 */
uint16_t HWDimmer::getLevel(uint8_t channel)
{
    uint16_t tmpLevel = 0;

    if(channel < numChannels)
    {
        tmpLevel = levels[channel];
    }
    return tmpLevel;
}

/**
 * @brief Prefix of this module when using OpenKNX log functions
 * 
 * @return std::string Prefix
 */
std::string HWDimmer::logPrefix()
{
    return "HWDimmer";
}
