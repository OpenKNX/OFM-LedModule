#pragma once
#include "OpenKNX.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#ifdef OPENKNX_LED_TEMPSENS_ADDR
  #include <Temperature_LM75_Derived.h>
#endif
#include "HWDimmer/HWDimmer.h"
#ifdef LEDMODULE_DIMMER_PCA9685
  #include "HWDimmer/HWDimmerPCA.h"
#endif
#ifdef LEDMODULE_DIMMMER_RP2040
  #include "HWDimmer/HWDimmerRP2040.h"
#endif
#ifdef LEDMODULE_DIMMMER_WS
  #include "HWDimmer/HWDimmerWS.h"
#endif
#include "LedModuleConfig.h"
#include "Channels/SingleChannel.h"
#include "Channels/TWChannel.h"
#include "Channels/RGBChannel.h"

#define PWM_FREQUENCY_FACTOR 200 // based on ETS drop down

#define TEMPERATURE_MIN_DIFFERENCE 0.5

class LedModule : public OpenKNX::Module
{
  private:
    uint32_t _timer1 = 0;
    uint32_t _timer2 = 0;
    uint32_t _timerCheckConnection = 0;
    float _lastTemperatureSent = 0;
    uint32_t _temperaturSentTimer = 0;
    bool _doResetPwm = false;

    OpenKNX::Flash::Driver *_ledStorage = nullptr;
    HWDimmer *_pDimmer;
    uint8_t _SC_HWChannels[LED_SC_ChannelCount][1];
    uint8_t _TW_HWChannels[LED_TW_ChannelCount][2];
    uint8_t _RGB_HWChannels[LED_RGB_ChannelCount][3];
    SingleChannel *_singleChannels[LED_SC_ChannelCount];
    TWChannel *_twChannels[LED_TW_ChannelCount];
    RGBChannel *_rgbChannels[LED_RGB_ChannelCount];

#ifdef OPENKNX_LED_TEMPSENS_ADDR
    Generic_LM75_9_to_12Bit_OneShot _temperature = Generic_LM75_9_to_12Bit_OneShot(&OPENKNX_GPIO_WIRE, OPENKNX_LED_TEMPSENS_ADDR);
#endif

    void setupCustomFlash();
    void setupChannels();
    void setupFrontPlate();
    void setupVoltageMeasurement();
    void setupConstantCurrentMode();

  public:
    void loop(bool configured) override;
    void setup(bool configured) override;
#ifdef OPENKNX_DUALCORE
    void loop1(bool configured) override;
    void setup1(bool configured) override;
#endif
    const std::string name() override;
    const std::string version() override;
    void processInputKo(GroupObject &ko) override;
    bool processCommand(const std::string cmd, bool diagnoseKo);
    void savePower() override;
    void showHelp() override;

    enum LightType
    {
        Single = 1,
        TunableWhite = 2,
        RGB = 3,
        RGBW = 4,
        RGBTW = 5
    };
};

extern LedModule openknxLedModule;
