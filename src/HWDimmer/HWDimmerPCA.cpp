#include "HWDimmerPCA.h"

/**
 * @brief Construct a new HWDimmerPCA::HWDimmerPCA object
 * 
 * @param type PCA dimmer type
 */
HWDimmerPCA::HWDimmerPCA(HWDimmerPCA::PCAType type) :HWDimmer(16)
{
    Wire1.setSDA(LEDMODULE_WIRE1_SDA);
    Wire1.setSCL(LEDMODULE_WIRE1_SCL);
    Wire1.begin(); // I2C1
    pwm.begin();
    logInfoP("pwm.setPWMFreq(1000)");
    pwm.setPWMFreq(1000);
    logInfoP("Wire1.setClock(1000000)");
    Wire1.setClock(1000000);
    pwm = Adafruit_PWMServoDriver(0x40, Wire1);

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
        pwm.setPWM(channel, 0, level*16);
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

    logInfoP("Dimm back to last known values");
}
