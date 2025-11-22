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
    _pDimmer->loop();
    update();
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
    KoLED_SC_ChStateLocking.value(_isLocked, DPT_State);
}

void LightChannel::processSendValue(GroupObject& ko, Dpt dpt, bool send, uint8_t sendMinChangePercent, uint16_t sendMinChangeAbsolute, uint32_t sendCyclicTimeMS, uint32_t& cyclicSendTimer, float& lastSentValue, float currentValue, uint16_t checkMultiply)
{
    if (!send)
        return;

    uint16_t currentDifference = round(abs(lastSentValue - currentValue * checkMultiply));
    if (currentDifference > 0)
    {
        if (lastSentValue > 0 && currentDifference >= lastSentValue * sendMinChangePercent / checkMultiply &&
            currentDifference >= sendMinChangeAbsolute)
        {
            ko.value(currentValue, dpt);
            lastSentValue = currentValue * checkMultiply;
        }
        else
            ko.valueNoSend(currentValue, dpt);
    }

    if (sendCyclicTimeMS > 0 && delayCheckMillis(cyclicSendTimer, sendCyclicTimeMS))
    {
        ko.value(currentValue, dpt);
        lastSentValue = currentValue * checkMultiply;
        cyclicSendTimer = delayTimerInit();
    }
}

void LightChannel::processDeviceProtection(GroupObject& koConstCurrent, GroupObject& koOverload, bool active, uint8_t constCurrent, uint8_t overloadPercent, uint32_t overloadTimeMS, uint32_t& overloadTimer, bool cutOff, float current)
{
    if (!active)
        return;

    if (current <= constCurrent)
    {
        overloadTimer = 0;

        if (koConstCurrent.value(DPT_Switch))
            koConstCurrent.value(false, DPT_Switch);
        if (koOverload.value(DPT_Switch))
            koOverload.value(false, DPT_Switch);
    }
    else
    {
        if (!koConstCurrent.value(DPT_Switch))
            koConstCurrent.value(true, DPT_Switch);

        bool shouldCutOff = false;
        if (current > constCurrent * (100 + overloadPercent) / 100)
        {
            shouldCutOff = true;

            if (!koOverload.value(DPT_Switch))
                koOverload.value(true, DPT_Switch);
        }
        else
        {
            if (overloadTimer == 0)
                overloadTimer = delayTimerInit();
            else if (delayCheckMillis(overloadTimer, overloadTimeMS))
                shouldCutOff = true;
        }

        if (cutOff && shouldCutOff)
            setSwitchNoDim(false);
    }
}

void LightChannel::processLampProtection(GroupObject& koConstCurrent, GroupObject& koOverload, bool active, uint16_t cableLength, uint8_t cableCrossSect, uint8_t constPower, uint8_t overloadPercent, uint32_t overloadTimeMS, uint32_t& overloadTimer, bool cutOff, float current, float voltage, uint8_t channelCount)
{
    if (!active)
        return;

    float cableCrossSectMM2 = 1.5f;
    switch (cableCrossSect)
    {
        case 0:
            cableCrossSectMM2 = 0.75f;
            break;
        case 1:
            cableCrossSectMM2 = 1.5f;
            break;
        case 2:
            cableCrossSectMM2 = 2.5f;
            break;
        case 3:
            cableCrossSectMM2 = 4.0f;
            break;
        default:
            logErrorP("processLampProtection: Invalid cable cross-section: %d", cableCrossSectMM2);
            break;
    }

    // we assume for multi channel lamps all current is delivered in one conductor and separately returned
    float voltageDropToLamp = (CABLE_RESISTANCE_PER_METER * cableLength / cableCrossSectMM2) * current * channelCount;
    float voltageDropFromLamp = (CABLE_RESISTANCE_PER_METER * cableLength / cableCrossSectMM2) * current;
    float effectiveVoltage = voltage - voltageDropToLamp - voltageDropFromLamp;

    float power = effectiveVoltage * current;
    if (power <= constPower)
    {
        overloadTimer = 0;

        if (koConstCurrent.value(DPT_Switch))
            koConstCurrent.value(false, DPT_Switch);
        if (koOverload.value(DPT_Switch))
            koOverload.value(false, DPT_Switch);
    }
    else
    {
        if (!koConstCurrent.value(DPT_Switch))
            koConstCurrent.value(true, DPT_Switch);

        bool shouldCutOff = false;
        if (power > constPower * (100 + overloadPercent) / 100)
        {
            shouldCutOff = true;

            if (!koOverload.value(DPT_Switch))
                koOverload.value(true, DPT_Switch);
        }
        else
        {
            if (overloadTimer == 0)
                overloadTimer = delayTimerInit();
            else if (delayCheckMillis(overloadTimer, overloadTimeMS))
                shouldCutOff = true;
        }

        if (cutOff && shouldCutOff)
            setSwitchNoDim(false);
    }
}