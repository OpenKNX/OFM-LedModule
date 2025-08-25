#include "LightChannel.h"
#include <knx.h>
/*test*/

/**
 * @brief Construct a new Light Channel:: Light Channel object
 *
 * @param channelIndex Index for this channel
 * @param pDimmer pointer to HWDimmer instance used on this platform
 * @param hwChannels array of HW channels for this light
 * @param numChannels number of HW channels this light has
 */
LightChannel::LightChannel(uint8_t channelIndex, HWDimmer* pDimmer, uint8_t hwChannels[], uint8_t numChannels) : _brightness(0, 0, VALUE_KNX_COUNT)
{
    _channelIndex = channelIndex;
    this->_numChannels = numChannels;

    this->_pDimmer = pDimmer;
    logDebugP("debug setup");

    this->_pHWChannels = hwChannels;
}

/**
 * @brief Name definition of this channel
 *
 * @return const std::string
 */
const std::string LightChannel::name()
{
    return "LightChannel";
}

/**
 * @brief Loop function to call update on a regular basis
 *
 */
void LightChannel::loop()
{
    update();
    processFrontOutput();
}

void LightChannel::processFrontInput(bool frontControlEnabled)
{
#ifdef LEDMODULE_FRONT_PLATE_USED
    if (!frontControlEnabled)
        return;

    bool buttonPressed = false;
    for (int i = 0; i < _numChannels; i++)
    {
        uint8_t channelIndex = _pHWChannels[i];
        buttonPressed |= openknxGPIOModule.digitalRead(OPENKNX_LED_GPIO_INPUT_OFFSET + channelIndex) == OPENKNX_LED_GPIO_INPUT_ACTIVE_ON;
    }

    if (buttonPressed &&
        delayCheck(_currentManualModeLastChange, LED_OUTPUT_DEBOUNCE))
    {
        _currentManualMode = !_currentManualMode;
        _currentManualModeLastChange = delayTimerInit();
        logDebugP("processInput: manual mode button toggle (_currentManualMode: %u)", _currentManualMode);
    }
#endif
}

void LightChannel::processFrontOutput()
{
#ifdef LEDMODULE_FRONT_PLATE_USED
    for (int i = 0; i < _numChannels; i++)
    {
        float ledOnPercent = 0;
        uint32_t ledOnTime = 0;

        if (_currentManualMode)
        {
            ledOnPercent = 1;
            ledOnTime = LED_OUTPUT_LED_PHASE;
        }
        else
        {
            ledOnPercent = _brightness.value() / (float)BRIGHTNESS_MAX;

            // minimum of 1 % and maximum of 99 % LED on to signal automatic mode
            if (ledOnPercent < 0.01)
                ledOnPercent = 0.01;
            else if (ledOnPercent > 0.99)
                ledOnPercent = 0.99;

            ledOnTime = round(LED_OUTPUT_LED_PHASE * ledOnPercent);
        }

        if (ledOnTime == LED_OUTPUT_LED_PHASE)
        {
            if (!_currentLedOn[i])
                setOutputLed(i, true);
        }
        else if (ledOnTime == 0)
        {
            if (_currentLedOn[i])
                setOutputLed(i, false);
        }
        else if (_currentLedOn[i] && delayCheck(_currentLedChangeStarted[i], ledOnTime))
            setOutputLed(i, false);
        else if (!_currentLedOn[i] && delayCheck(_currentLedChangeStarted[i], LED_OUTPUT_LED_PHASE - ledOnTime))
            setOutputLed(i, true);

        if (_currentLedOnTime[i] != ledOnTime)
        {
            _currentLedOnTime[i] = ledOnTime;
            logDebugP("processOutput (channel: %u, ledOnPercent: %.2f, ledOnTime: %u)", i, ledOnPercent, ledOnTime);
        }
    }
#endif
}

void LightChannel::setOutputLed(uint8_t hwChannelIndex, bool on)
{
#ifdef LEDMODULE_FRONT_PLATE_USED
    uint8_t channelIndex = _pHWChannels[hwChannelIndex];
    openknxGPIOModule.digitalWrite(OPENKNX_LED_GPIO_OUTPUT_OFFSET + channelIndex, on ? OPENKNX_LED_GPIO_OUTPUT_ACTIVE_ON : !OPENKNX_LED_GPIO_OUTPUT_ACTIVE_ON);
    _currentLedChangeStarted[hwChannelIndex] = delayTimerInit();
    _currentLedOn[hwChannelIndex] = on;
    //logDebugP("setOutputLed (channel: %u, on: %u)", channelIndex, on);
#endif
}

bool LightChannel::getNight()
{
    return _isNight;
}

bool LightChannel::getLock()
{
    return _isLocked;
}

void LightChannel::setLock(bool lock)
{
    _isLocked = lock;
}