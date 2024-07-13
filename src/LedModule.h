#pragma once
#include "OpenKNX.h"
#include "LightChannel.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "HWDimmer.h"
#include "HWDimmerPCA.h"
#include "HWDimmerRP2040.h"
#include "LedModuleConfig.h"

class LedModule : public OpenKNX::Module
{
  private:

    LightChannel *_channels[LEDMODULE_MAX_LIGHT_CHANNELS];
    uint8_t _channelIterator = 0;
    uint32_t _timer1 = 0;
    uint32_t _timer2 = 0;
    uint32_t _timerCheckConnection = 0;

    bool doResetPwm = false;


    OpenKNX::Flash::Driver * _ledStorage = nullptr;

    // lightConfig
    // ===========
    // Based on 6 channels, we can have up to 6 lights (with 1 channel each) or 1 light (with 6 channels)
    // Everything in between in possible, regardless if it makes sense
    // Examples:
    // * 2 lights: 1x RGBW + 1x TW => light[0] gets channels 0-3 and light[1] gets channels 4-5
    // * ...
    // 

    
    HWDimmer *dimmer;

    void setupCustomFlash();
    void setupChannels();

    void activateScene(uint8_t sceneId);
    //void dimmTo(uint32_t currentValues[5], uint32_t newValues[5]);
    //void dimmChannelsTo(uint8_t hardwareChannels[MAX_LIGHT_CHANNELS], uint8_t targetBrightness[MAX_LIGHT_CHANNELS]);
    void dimmLoop(); // This is the main dimm loop
    int checkConnection();

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
    void dimmLightTo(uint8_t lightId, uint32_t newValues[LEDMODULE_MAX_LIGHT_CHANNELS]);
};

extern LedModule openknxLedModule;
