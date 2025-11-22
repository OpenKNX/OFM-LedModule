#include "HWDimmerPCA.h"

/**
 * @brief Construct a new HWDimmerPCA::HWDimmerPCA object
 *
 * @param type PCA dimmer type
 */
HWDimmerPCA::HWDimmerPCA(HWDimmerPCA::PCAType type, uint8_t addr, uint16_t pwmFreq) : HWDimmer(LEDMODULE_MAX_LIGHT_CHANNELS), _addr(addr)
{
#ifdef LEDMODULE_WIRE_SDA
    LEDMODULE_WIRE.setSDA(LEDMODULE_WIRE_SDA);
#endif
#ifdef LEDMODULE_WIRE_SCL
    LEDMODULE_WIRE.setSCL(LEDMODULE_WIRE_SCL);
#endif
    LEDMODULE_WIRE.begin();
#ifdef LEDMODULE_WIRE_CLOCK_FREQ
    LEDMODULE_WIRE.setClock(LEDMODULE_WIRE_CLOCK_FREQ);
#endif
    _pwm = Adafruit_PWMServoDriver(_addr, LEDMODULE_WIRE);
    _pwm.begin();
    _pwm.setPWMFreq(pwmFreq);
    for (uint8_t ch = 0; ch < numChannels; ch++)
        setLevel(0, ch);
}

/**
 * @brief Set level of selected channel to value
 *
 * @param level new value
 * @param channel selected channel
 * @return true when channel is available
 * @return false when channel is invalid
 */
bool HWDimmerPCA::setLevel(uint16_t level, uint8_t channel)
{
    // logInfoP("setLevel_1");
    bool isValidChannel = false;
    if (setLevelInternal(level, channel))
    {
        // logInfoP("setLevel: %3X",level);
        isValidChannel = true;
        _pwm.setPWM(channel, 0, min(getLevel(channel), DIM_RANGE));
        // logInfoP("setLevel_3");
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
uint16_t HWDimmerPCA::scale(uint16_t level, HWDimmer::DimLUTType lutType)
{
    // logDebugP("#: %3X, Wert: %3X", level, dimLUT[lutType].Val(level));
    return dimLUT[lutType].Val(level);
}

/**
 * @brief Get maximum allowed value in selected scale
 *
 * @param lutType lookup table selection
 * @return uint16_t maximum value of range
 */
uint16_t HWDimmerPCA::getScaleMax(HWDimmer::DimLUTType lutType)
{
    return dimLUT[lutType].Max();
}

/**
 * @brief Prefix of this module when using OpenKNX log functions
 *
 * @return std::string Prefix
 */
std::string HWDimmerPCA::logPrefix()
{
    return "PCAHWDimmer";
}

/**
 * @brief Check I2C connection with PCA driver
 *
 * @return true connection is ok
 * @return false connection has error
 */
bool HWDimmerPCA::checkConnection()
{
    byte error;
    bool isOK = false;
    // -----------------------------------------------------------------------------
    LEDMODULE_WIRE.beginTransmission(_addr);
    error = LEDMODULE_WIRE.endTransmission();

    if (error == 0)
        isOK = true;
    else
        logErrorP("PCA9685 PWM not available via I2C %s", error);

    // -----------------------------------------------------------------------------
    LEDMODULE_WIRE.requestFrom(_addr, 1);
    if (LEDMODULE_WIRE.read() < 0)
        logErrorP("PCA9685 0x40 fehler");
    else
    {
        // logErrorP("PCA9685 0x40 i.O.");
        isOK = true;
    }

    // -----------------------------------------------------------------------------
    return isOK;
}

/**
 * @brief Reconnecto to PCA driver and reinit
 *
 */
void HWDimmerPCA::reconnect()
{
    logInfoP("Reset PWM as connection is back");
    logInfoP("_pwm.begin()");
    _pwm.begin();
    logInfoP("_pwm.setPWMFreq(1000)");
    _pwm.setPWMFreq(1000);

    logInfoP("Dim back to last known values");
}

void HWDimmerPCA::outputLUT()
{
    for (int i = 0; i < VALUE_KNX_COUNT; i++)
        logDebugP("Count%d: %d", i, dimLUT[0].Val(i));
}

void HWDimmerPCA::runTestMode()
{
    // ToDo
}

/**
 * @brief Linear lookup tables to map 255% level to PCA driver levels
 *  0: Linear, 1: logarithmic x^1.5, 2: logarithmic x^2
 */
HWDimmer::LUT<VALUE_KNX_COUNT> HWDimmer::dimLUT[] = {HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.0), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.5), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 2)};
