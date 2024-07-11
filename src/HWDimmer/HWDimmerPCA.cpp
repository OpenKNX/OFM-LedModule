#include "HWDimmerPCA.h"

/**
 * @brief Construct a new HWDimmerPCA::HWDimmerPCA object
 * 
 * @param type PCA dimmer type
 */
HWDimmerPCA::HWDimmerPCA(HWDimmerPCA::PCAType type) :HWDimmer(LEDMODULE_MAX_LIGHT_CHANNELS)
{
    #ifdef LEDMODULE_WIRE_SDA
        LEDMODULE_WIRE.setSDA(LEDMODULE_WIRE_SDA);
    #endif
    #ifdef LEDMODULE_WIRE_SCL
        LEDMODULE_WIRE.setSCL(LEDMODULE_WIRE_SCL);
    #endif
    LEDMODULE_WIRE.begin(); // I2C1
    pwm.begin();
    pwm.setPWMFreq(LEDMODULE_PWM_FREQ);
    #ifdef LEDMODULE_WIRE_CLOCK_FREQ
        LEDMODULE_WIRE.setClock(LEDMODULE_WIRE_CLOCK_FREQ);
    #endif
    pwm = Adafruit_PWMServoDriver(0x40, LEDMODULE_WIRE);

    for(uint8_t ch = 0; ch < numChannels; ch++)
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
bool HWDimmerPCA::setLevel(uint8_t level, uint8_t channel)
{
    bool isValidChannel = false;
    if(HWDimmer::setLevel(level, channel))
    {
        isValidChannel = true;
        pwm.setPWM(channel, 0, dimLUT[DimLUTType::Log1_5].Val(level));
    }
    return isValidChannel;
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

    Wire1.beginTransmission(0x40);
    error = Wire1.endTransmission();

    if (error == 0) {
        isOK = true;
    } else {
        logErrorP("PCA9685 PWM not available via I2C %s", error);
    }
    return isOK;
}

/**
 * @brief Reconnecto to PCA driver and reinit
 * 
 */
void HWDimmerPCA::reconnect()
{
    logInfoP("Reset PWM as connection is back");
    logInfoP("pwm.begin()");
    pwm.begin();
    logInfoP("pwm.setPWMFreq(1000)");
    pwm.setPWMFreq(1000);

    logInfoP("Dim back to last known values");
}

/**
 * @brief Linear lookup tables to map 255% level to PCA driver levels
 *  0: Linear, 1: logarithmic x^1.5
 */
HWDimmer::LUT<VALUE_KNX_COUNT> HWDimmer::dimLUT[] = {HWDimmer::LUT<VALUE_KNX_COUNT>(4095, 1.0), HWDimmer::LUT<VALUE_KNX_COUNT>(4095, 1.5)};
