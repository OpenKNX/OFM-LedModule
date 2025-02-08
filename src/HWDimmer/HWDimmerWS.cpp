#include "HWDimmerWS.h"

//// test file for led stripes with ws2811 chip
/**
 * @brief Construct a new HWDimmerPCA::HWDimmerPCA object
 *
 * @param type WS dimmer type
 */
HWDimmerWS::HWDimmerWS(HWDimmerWS::WSType type) : HWDimmer(LEDMODULE_MAX_LIGHT_CHANNELS)
{
    /*#ifdef LEDMODULE_WIRE_SDA
        LEDMODULE_WIRE.setSDA(LEDMODULE_WIRE_SDA);
    #endif
    #ifdef LEDMODULE_WIRE_SCL
        LEDMODULE_WIRE.setSCL(LEDMODULE_WIRE_SCL);
    #endif
    LEDMODULE_WIRE.begin();
    #ifdef LEDMODULE_WIRE_CLOCK_FREQ
        LEDMODULE_WIRE.setClock(LEDMODULE_WIRE_CLOCK_FREQ);
    #endif
    */
    pwm = Adafruit_NeoPixel(100, 20, NEO_KHZ400);
    // pwm.begin();
    // pwm.setPWMFreq(LEDMODULE_PWM_FREQ);
    for (uint8_t ch = 0; ch < numChannels; ch++)
    {
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
bool HWDimmerWS::setLevel(uint16_t level, uint8_t channel)
{
    // logInfoP("setLevel_1");
    bool isValidChannel = false;
    if (HWDimmer::setLevel(level, channel))
    {
        // logInfoP("setLevel_2");
        isValidChannel = true;
        // pwm.setPWM(channel,0, min(level, DIM_RANGE));
        pwm.clear();
        for (int i = 0; i < 100; i++)
        {
            pwm.setPixelColor(i, pwm.Color(level, level, level));
            pwm.show();
        }
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
uint16_t HWDimmerWS::scale(uint8_t level, HWDimmer::DimLUTType lutType)
{
    return dimLUT[lutType].Val(level);
}

/**
 * @brief Get maximum allowed value in selected scale
 *
 * @param lutType lookup table selection
 * @return uint16_t maximum value of range
 */
uint16_t HWDimmerWS::getScaleMax(HWDimmer::DimLUTType lutType)
{
    return dimLUT[lutType].Max();
}

/**
 * @brief Prefix of this module when using OpenKNX log functions
 *
 * @return std::string Prefix
 */
std::string HWDimmerWS::logPrefix()
{
    return "WSHWDimmer";
}

/**
 * @brief Check I2C connection with PCA driver
 *
 * @return true connection is ok
 * @return false connection has error
 */
bool HWDimmerWS::checkConnection()
{
    byte error;
    bool isOK = false;
// -----------------------------------------------------------------------------
    LEDMODULE_WIRE.beginTransmission(LEDMODULE_I2C);
    error = LEDMODULE_WIRE.endTransmission();

    if (error == 0)
    {
        isOK = true;
    }
    else
    {
        logErrorP("PCA9685 PWM not available via I2C %s", error);
    }

// -----------------------------------------------------------------------------
    LEDMODULE_WIRE.requestFrom(LEDMODULE_I2C, 1);
    if (LEDMODULE_WIRE.read() < 0)
    {
        logErrorP("PCA9685 0x40 fehler");
    }
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
void HWDimmerWS::reconnect()
{
    logInfoP("Reset PWM as connection is back");
    logInfoP("pwm.begin()");
    // pwm.begin();
    logInfoP("pwm.setPWMFreq(1000)");
    // pwm.setPWMFreq(1000);

    logInfoP("Dim back to last known values");
}

/**
 * @brief Linear lookup tables to map 255% level to PCA driver levels
 *  0: Linear, 1: logarithmic x^1.5
 */
HWDimmer::LUT<VALUE_KNX_COUNT> HWDimmer::dimLUT[] = {HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.0), HWDimmer::LUT<VALUE_KNX_COUNT>(DIM_RANGE, 1.5)};
