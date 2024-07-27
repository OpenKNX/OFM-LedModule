#include "HWDimmer.h"

/**
 * @brief Construct a new HWDimmer::HWDimmer object
 * 
 * @param numChannels 
 */
HWDimmer::HWDimmer(uint8_t numChannels)
{
    this->numChannels = numChannels;
    dimChannels = new DimChannel[numChannels];
}

/**
 * @brief Destroy the HWDimmer::HWDimmer object
 * 
 */
HWDimmer::~HWDimmer()
{
    delete[] dimChannels;
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
        dimChannels[channel].level = level;
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
uint8_t HWDimmer::getLevel(uint8_t channel)
{
    uint8_t tmpLevel = 0;

    if(channel < numChannels)
    {
        tmpLevel = dimChannels[channel].level;
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

/**
 * @brief Loop to control dim process of individual HW channels
 * 
 */
void HWDimmer::dimLoop()
{
    static uint32_t lastMillis = 0;

    if(millis() - lastMillis >= DIMLOOP_DELAY) // perform actual dimming every 10ms
    {
        lastMillis = millis();
        for(int ch = 0; ch < numChannels; ch++)
        {
            switch (dimChannels[ch].task)
            {
            default:
            case DimTask::DIM_IDLE:
                /* code */
                break;

            case DimTask::DIM_TO_VALUE:
                int32_t dimTime = lastMillis - dimChannels[ch].startTimestamp;
                int32_t tmpLevel = (dimTime * dimChannels[ch].deltaLevel) / dimChannels[ch].dimDurationAbs + dimChannels[ch].lastLevel;

                // Check if upper or lower target is reached
                if(dimChannels[ch].dir == 1 && tmpLevel >= dimChannels[ch].targetLevel || dimChannels[ch].dir == 0 && tmpLevel <= dimChannels[ch].targetLevel)
                {
                    tmpLevel  =  dimChannels[ch].targetLevel;
                    dimChannels[ch].task = DIM_IDLE;
                    logInfoP("CH: %i finished @ %i/%i", ch, tmpLevel, dimChannels[ch].targetLevel);
                }
                setLevel((uint8_t)tmpLevel, ch);
                //logDebugP("Set CH: %d, @%ims %i",ch, dimTime, channels[ch].level);

                if(dimTime > dimChannels[ch].dimDurationAbs * 1.1) // TODO: remove after some more tests on different platforms
                {
                    dimChannels[ch].task = DIM_IDLE;
                    logInfoP("CH: %i failed @%ims, val: %i", ch, dimTime, dimChannels[ch].level);
                }
                break;
            }
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
    dimToLevel(channel, VALUE_MIN, DIMLOOP_DELAY); // Dimm to level in one dimm cycle
}

/**
 * @brief Switch on HW channel
 * 
 * @param channel selected channel
 */
void HWDimmer::switchOn(uint8_t channel)
{    
    dimToLevel(channel, VALUE_MAX, DIMLOOP_DELAY); // Dimm to level in one dimm cycle
}

/**
 * @brief Set level of selected HW channel
 * 
 * @param channel selected channel
 * @param level new level
 */
void HWDimmer::dimToLevel(uint8_t channel, uint8_t level, uint32_t dimTime)
{
    if(channel < numChannels)
    {
        dimChannels[channel].task = DIM_TO_VALUE;
        dimChannels[channel].targetLevel = level;
        dimChannels[channel].lastLevel = dimChannels[channel].level;
        dimChannels[channel].deltaLevel = dimChannels[channel].targetLevel - dimChannels[channel].lastLevel;
        dimChannels[channel].dir =  dimChannels[channel].targetLevel > dimChannels[channel].level ? 1 : 0;
        dimChannels[channel].dimDurationAbs = dimTime >= 1 ? dimTime : 1;
        dimChannels[channel].startTimestamp = millis();
        logInfoP("Dimming CH: %d from %d to %d", channel, dimChannels[channel].lastLevel, dimChannels[channel].targetLevel);
    }
    else
    {
        logErrorP("Invalid channel: %d", channel);
    }
}
