#pragma once
#include "OpenKNX.h"
#include "LedModuleHW.h"
#include <Adafruit_PWMServoDriver.h>
#include "ledmodulecfg.h"


class LedModuleHW {

  public:
    LedModuleHW(Adafruit_PWMServoDriver pwm);
    void addLight(uint8_t lightId, uint8_t lightType, uint8_t hwChannels[LEDMODULE_MAX_LIGHT_CHANNELS], uint32_t dimmDuration);
    void dimmLoop();
    void dimmLightTo(uint8_t lightId, uint32_t targetBrightness[LEDMODULE_MAX_LIGHT_CHANNELS]);
    void dimmToLastAfterI2cIsBack();

    std::string logPrefix();

  private:

    uint8_t latestLightIdInMainLoop = 0;
    Adafruit_PWMServoDriver pwm;

    //uint32_t currentLedChannelValues[MAX_LIGHT_CHANNELS];

    struct lightConfig {
      bool isConfigured = false;
      bool lightIsOn = false;
      bool processing = false;
      uint32_t dimmDuration = 0;
      uint32_t dimmStepEveryMillis = 0;
      uint32_t processingStarted = 0;
      uint32_t lastProcessingStep = 0;
       //Need to be set true, if at least one channel brightness is > 0
      int8_t channelMapping[LEDMODULE_MAX_LIGHT_CHANNELS];
      uint32_t currentBrightness[LEDMODULE_MAX_LIGHT_CHANNELS];
      uint32_t targetBrightness[LEDMODULE_MAX_LIGHT_CHANNELS];
      uint8_t cyclesToWait[LEDMODULE_MAX_LIGHT_CHANNELS];
      uint8_t cyclesWaited[LEDMODULE_MAX_LIGHT_CHANNELS];
      bool channelIsProcessing[LEDMODULE_MAX_LIGHT_CHANNELS];
      uint8_t maxSteps = 0;
    };

    lightConfig lights[LEDMODULE_MAX_LIGHT_CHANNELS];

};