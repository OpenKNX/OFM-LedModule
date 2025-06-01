#include "HWDimmerRP2040.h"
#include "OpenKNX.h"

/**
 * @brief Construct a new HWDimmerRP2040::HWDimmerRP2040 object
 *
 * @param pins pointer to array holding pin numbers to be used as PWM
 * @param numChannels number of channels
 */
HWDimmerRP2040::HWDimmerRP2040(uint8_t pins[], uint8_t numChannels, uint16_t pwmFreq) : HWDimmer(numChannels)
{
    this->pins = pins;
    this->pwmFreq = pwmFreq;

    for (uint8_t ch = 0; ch < numChannels; ch++)
    {
        pinMode(this->pins[ch], OUTPUT);
        setLevel(0, ch);
    }
    analogWriteFreq(pwmFreq);
    analogWriteRange(DIM_RANGE);

    logDebugP("HWDimmerRP2040: pwmFreq=%d, dimRange=%d", pwmFreq, DIM_RANGE);

    //###ToDo: only for testing - remove again
    pinMode(23, OUTPUT);

    #if 0
        logDebugP("Lookup table:");
        for(int i=0; i<256; i+=16)
        {
            int idx = i;
            logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++),lut.Val(idx++));
        }
    #endif
}

void HWDimmerRP2040::loop()
{
    HWDimmer::loop();

#ifdef LED_MPX_OUT_PIN
    if (delayCheck(sampleTimer, 10))
    {
        selectMultiplexerChannel(0);
        attachInterrupt(digitalPinToInterrupt(LED_PWM_PIN_A), HWDimmerRP2040::interruptSample, RISING);

        sampleTimer = delayTimerInit();
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               

    if (delayCheck(testTimer, 1000))
    {
        float volt = (float)sampleValue / 4095 * (float)3.3;
        float ampere = volt / 50 / 0.01; // gain 50, 0.01 Ohm shunt resistor
        logDebugP("ADC: %.4f A, %.4f V (%u)", ampere, volt, sampleValue);

        testTimer = delayTimerInit();
    }
#endif
}

void HWDimmerRP2040::interruptSample()
{
    detachInterrupt(digitalPinToInterrupt(LED_PWM_PIN_A));
    
    // Cancel any existing alarm first
    if (timerAlarmId >= 0) {
        cancel_alarm(timerAlarmId);
    }
    
    // Schedule a timer to trigger towards the end of the PWM cycle
    timerAlarmId = add_alarm_in_us(sampleTriggerDelays[0], timerCallback, nullptr, true);
    
    if (timerAlarmId < 0)
        openknx.logger.logWithPrefix("OFM-LedModule", "Failed to set timer alarm");
}

/**
 * @brief Timer callback function that gets called after the specified delay
 * 
 * @param alarm_id The alarm identifier
 * @param user_data User data passed to the alarm (nullptr in our case)
 * @return int64_t Return 0 to not reschedule the alarm
 */
int64_t HWDimmerRP2040::timerCallback(alarm_id_t alarm_id, void* user_data) {
    //###ToDo: only for testing - remove again
    digitalWrite(23, HIGH);
    digitalWrite(23, LOW);

#ifdef LED_MPX_OUT_PIN
    sampleValue = analogRead(LED_MPX_OUT_PIN);
#endif
    
    // Return 0 to not repeat the alarm
    return 0;
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
    if (HWDimmer::setLevel(level, channel))
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
HWDimmer::LUT<VALUE_KNX_COUNT> HWDimmer::dimLUT[] = {HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.0), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.5), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 2)};
