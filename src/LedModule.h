#pragma once
#include "OpenKNX.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include "HWDimmer.h"
#if defined(LEDMODULE_DIMMER_PCA9685)
  #include "HWDimmerPCA.h"
#else
  #if defined(LEDMODULE_DIMMMER_RP2040)
    #include "HWDimmerRP2040.h"
  #endif
#endif
#include "LedModuleConfig.h"
#include "SingleChannel.h"
#include "TWChannel.h"
#include "RGBChannel.h"

class LedModule : public OpenKNX::Module
{
  private:
    uint32_t _timer1 = 0;
    uint32_t _timer2 = 0;
    uint32_t _timerCheckConnection = 0;
    bool doResetPwm = false;

    OpenKNX::Flash::Driver *_ledStorage = nullptr;
    HWDimmer *_pDimmer;
    uint8_t _SC_HWChannels[LED_SC_ChannelCount][1];
    uint8_t _TW_HWChannels[LED_TW_ChannelCount][2];
    uint8_t _RGB_HWChannels[LED_RGB_ChannelCount][3];
    SingleChannel *_singleChannels[LED_SC_ChannelCount];
    TWChannel *_twChannels[LED_TW_ChannelCount];
    RGBChannel *_rgbChannels[LED_RGB_ChannelCount];

    void setupCustomFlash();
    void setupChannels();

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
    void init();

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
