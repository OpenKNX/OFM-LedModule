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
 * @brief Loop function to update status output on a regular basis
 *
 */
void HWDimmer::loop()
{
    processFrontInput();
    processFrontOutput();
}

void HWDimmer::processFrontInput()
{
#ifdef LEDMODULE_FRONT_PLATE_USED
    // if (!ParamHTA_ChManualMode)
    //     return;

    // bool buttonPressed = openknxGPIOModule.digitalRead(OPENKNX_HTA_GPIO_INPUT_OFFSET + _channelIndex) == GPIO_INPUT_ON;
    // if (buttonPressed)
    // {
    //     if (_currentButtonPressed)
    //     {
    //         if (_currentManualMode &&
    //             delayCheck(_currentButtonPressedStarted, HTA_MANUAL_MODE_CHANGE_TO_AUTO_TIME_DELAY) &&
    //             (ParamHTA_ChManualModeChangeToAuto == HTA_MANUAL_MODE_CHANGE_TO_AUTO_BUTTON || ParamHTA_ChManualModeChangeToAuto == HTA_MANUAL_MODE_CHANGE_TO_AUTO_BUTTON_TIME))
    //         {
    //             _currentManualMode = false;
    //             logDebugP("processInput: manual mode button off (_currentManualMode: %u, buttonPressed: %u, _currentButtonPressed: %u, _currentButtonPressedStarted: %u)", _currentManualMode, buttonPressed, _currentButtonPressed, _currentButtonPressedStarted);
    //         }
    //     }
    //     else
    //     {
    //         if (_currentManualMode)
    //         {
    //             _currentManualModeOn = !_currentManualModeOn;
    //             logDebugP("processInput: manual mode button toggle (_currentManualMode: %u, buttonPressed: %u, _currentButtonPressed: %u)", _currentManualMode, buttonPressed, _currentButtonPressed);
    //         }
    //         else
    //         {
    //             _currentManualMode = true;
    //             _currentManualModeOn = true;
    //             _currentManualModeStarted = delayTimerInit();
    //             logDebugP("processInput: manual mode button on (_currentManualMode: %u, buttonPressed: %u, _currentButtonPressed: %u)", _currentManualMode, buttonPressed, _currentButtonPressed);
    //         }

    //         _currentButtonPressed = true;
    //         _currentButtonPressedStarted = delayTimerInit();
    //     }
    // }
    // else
    //     _currentButtonPressed = false;

    // if (_currentManualMode &&
    //     delayCheck(_currentManualModeStarted, ParamHTA_ChManualModeChangeToAutoTimeMS) &&
    //     (ParamHTA_ChManualModeChangeToAuto == HTA_MANUAL_MODE_CHANGE_TO_AUTO_TIME || ParamHTA_ChManualModeChangeToAuto == HTA_MANUAL_MODE_CHANGE_TO_AUTO_BUTTON_TIME))
    // {
    //     _currentManualMode = false;
    //     logDebugP("processInput: manual mode time off (_currentManualMode: %u, _currentManualModeStarted: %u)", _currentManualMode, _currentManualModeStarted);
    // }
#endif
}

void HWDimmer::processFrontOutput()
{
#ifdef LEDMODULE_FRONT_PLATE_USED

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
bool HWDimmer::setLevel(uint16_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if (channel < numChannels)
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

    if (channel < numChannels)
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

/**
 * @brief Output LUT (implementation in sub classes)
 */
void HWDimmer::outputLUT()
{
}