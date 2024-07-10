#include "HWDimmer.h"

#define VALUE_MIN 0
#define VALUE_MAX 255

/**
 * @brief Construct a new HWDimmer::HWDimmer object
 * 
 * @param numChannels 
 */
HWDimmer::HWDimmer(uint8_t numChannels)
{
    levels = new uint8_t[numChannels];
    channels = new DimmChannel[numChannels];
}

/**
 * @brief Destroy the HWDimmer::HWDimmer object
 * 
 */
HWDimmer::~HWDimmer()
{
    delete[] levels;
    delete[] channels;
}

/**
 * @brief Set level of selected channel to value
 * 
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmer::setLevel(uint8_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if(channel < numChannels)
    {
        levels[channel] = level;
        isValidChannel = true;
    }
    return isValidChannel;
}

/**
 * @brief Get level of selected channel to value
 * 
 * @param channel selected channel
 * @return uint8_t current level
 */
uint8_t HWDimmer::getLevel(uint8_t channel)
{
    uint8_t tmpLevel = 0;

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
    return "IHWDimmer";
}

/**
 * @brief Loop to control dim process of individual HW channels
 * 
 */
void HWDimmer::dimLoop()
{
    for(int ch = 0; ch < numChannels; ch++)
    {
        switch (channels[ch].task)
        {
        default:
        case DimTask::DIM_IDLE:
            /* code */
            break;
            
        case DimTask::DIM_STOP:
            /* code */
            break;
            
        case DimTask::DIM_ON:
                channels[ch].level = VALUE_MAX;
                setLevel(channels[ch].level, ch);
                channels[ch].task = DIM_IDLE;
            break;
            
        case DimTask::DIM_OFF:
                channels[ch].level = VALUE_MIN;
                setLevel(channels[ch].level, ch);
                channels[ch].task = DIM_IDLE;
            break;
            
        case DimTask::DIM_SOFTON:
            /* code */
            break;
            
        case DimTask::DIM_SOFTOFF:
            /* code */
            break;
            
        case DimTask::DIM_UP:

            break;
            
        case DimTask::DIM_DOWN:
            /* code */
            break;

        case DimTask::DIM_SET:
                channels[ch].level = channels[ch].targetLevel;
                setLevel(channels[ch].level, ch);
                channels[ch].task = DIM_IDLE;
            break;
        }

    }
}

/**
 * @brief Switch off HW channel
 * 
 * @param channel selected channel
 */
void HWDimmer::switchOff(uint8_t channel)
{
    if(channel < numChannels)
    {
        channels[channel].task = DIM_ON;
        channels[channel].targetLevel = VALUE_MAX;
    }
}

/**
 * @brief Switch on HW channel
 * 
 * @param channel selected channel
 */
void HWDimmer::switchOn(uint8_t channel)
{    
    if(channel < numChannels)
    {
        channels[channel].task = DIM_ON;
        channels[channel].targetLevel = VALUE_MIN;
    }
}

/**
 * @brief Set level of selected HW channel
 * 
 * @param channel selected channel
 * @param level new level
 */
void HWDimmer::dimToLevel(uint8_t channel, uint8_t level)
{
    if(channel < numChannels)
    {
        channels[channel].task = DIM_SET;
        channels[channel].targetLevel = level;
    }
}
