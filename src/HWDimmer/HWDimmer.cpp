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
    sampleTriggerDelays = new uint64_t[numChannels]{0};

#ifdef LED_MPX_OUT_PIN
    pinMode(LED_MPX_OUT_PIN, INPUT);
    analogReadResolution(12);
#endif
    
    // Initialize multiplexer control pins as outputs
#ifdef LED_MPX_PIN_0
    pinMode(LED_MPX_PIN_0, OUTPUT);
    digitalWrite(LED_MPX_PIN_0, LOW);
#endif

#ifdef LED_MPX_PIN_1
    pinMode(LED_MPX_PIN_1, OUTPUT);
    digitalWrite(LED_MPX_PIN_1, LOW);
#endif

#ifdef LED_MPX_PIN_2
    pinMode(LED_MPX_PIN_2, OUTPUT);
    digitalWrite(LED_MPX_PIN_2, LOW);
#endif

    // Start with multiplexer channel 0 selected
    selectMultiplexerChannel(0);
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
        if (levels[channel] != level)
        {
            levels[channel] = level;
            sampleTriggerDelays[channel] = (uint64_t)((double)1 / (double)pwmFreq * (double)1000000 * ((double)level / (double)8191) * 0.3) - 10; // - 50

            logDebugP("setLevel, channel=%u, pwmFreq=%u, level=%u, trigger delay: %ju", channel, pwmFreq, level, sampleTriggerDelays[channel]);
        }
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
 * @brief Set the multiplexer channel based on the multiplexer input pins
 *
 * @param channel The channel number (0-7) to select on the multiplexer
 */
void HWDimmer::selectMultiplexerChannel(uint8_t channel)
{
#if defined(LED_MPX_PIN_0) && defined(LED_MPX_PIN_1) && defined(LED_MPX_PIN_2)

    // Ensure channel is within valid range (0-7 for a 3-bit multiplexer)
    if (channel > 7)
    {
        logErrorP("Invalid multiplexer channel: %d (valid range 0-7)", channel);
        return;
    }

    switch (channel)
    {
        case 0:
            digitalWrite(LED_MPX_PIN_0, HIGH);
            digitalWrite(LED_MPX_PIN_1, HIGH);
            digitalWrite(LED_MPX_PIN_2, HIGH);
            break;
        case 1:
            digitalWrite(LED_MPX_PIN_0, LOW);
            digitalWrite(LED_MPX_PIN_1, HIGH);
            digitalWrite(LED_MPX_PIN_2, HIGH);
            break;
        case 2:
            digitalWrite(LED_MPX_PIN_0, HIGH);
            digitalWrite(LED_MPX_PIN_1, LOW);
            digitalWrite(LED_MPX_PIN_2, HIGH);
            break;
        case 3:
            digitalWrite(LED_MPX_PIN_0, LOW);
            digitalWrite(LED_MPX_PIN_1, LOW);
            digitalWrite(LED_MPX_PIN_2, HIGH);
            break;
        case 4:
            digitalWrite(LED_MPX_PIN_0, HIGH);
            digitalWrite(LED_MPX_PIN_1, HIGH);
            digitalWrite(LED_MPX_PIN_2, LOW);
            break;
        case 5:
            digitalWrite(LED_MPX_PIN_0, LOW);
            digitalWrite(LED_MPX_PIN_1, HIGH);
            digitalWrite(LED_MPX_PIN_2, LOW);
            break;
        default:
            digitalWrite(LED_MPX_PIN_0, LOW);
            digitalWrite(LED_MPX_PIN_1, LOW);
            digitalWrite(LED_MPX_PIN_2, LOW);
            break;
    }
    
    //logDebugP("Selected multiplexer channel: %d", channel);

#endif
}

void HWDimmer::readMultiplexerChannel(uint8_t channel)
{
    //selectMultiplexerChannel(channel);

#ifdef LED_MPX_OUT_PIN
    int analgogValue = 0;
    for (size_t i = 0; i < 10; i++)
    {
        analgogValue += analogRead(LED_MPX_OUT_PIN);
    }
      logInfoP("ADC: %.4f V (%u)", ((float)analgogValue / 10) / 4095 * (float)3.3, analgogValue);
#endif
}