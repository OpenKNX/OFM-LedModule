#pragma once
#include "OpenKNX.h"
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#ifdef OPENKNX_LED_TEMPSENS_ADDR
    #include <PCT2075.h>
#endif
#include "HWDimmer/HWDimmer.h"
#ifdef LEDMODULE_DIMMER_PCA9685
    #include "HWDimmer/HWDimmerPCA.h"
    #include <Adafruit_PWMServoDriver.h>
#endif
#ifdef LEDMODULE_DIMMMER_RP2040
    #include "HWDimmer/HWDimmerRP2040.h"
#endif
#ifdef LEDMODULE_DIMMMER_WS
    #include "HWDimmer/HWDimmerWS.h"
#endif
#include "Channels/CentralObject.h"
#include "Channels/LightChannel.h"
#include "Channels/RGBChannel.h"
#include "Channels/RGBTWChannel.h"
#include "Channels/RGBWChannel.h"
#include "Channels/SingleChannel.h"
#include "Channels/TWChannel.h"
#include "LedModuleConfig.h"

#define PWM_FREQUENCY_FACTOR 200 // based on ETS drop down

#define TEMPERATURE_INVALID -1000.0f
#define TEMPERATURE_MIN_DIFFERENCE 0.5

class LedModule : public OpenKNX::Module
{
  private:
    uint32_t _timer1 = 0;
    uint32_t _timer2 = 0;
    uint32_t _timerCheckConnection = 0;
    float _lastTemperatureSent = 0;
    uint32_t _temperaturSendTimer = 0;
    float _lastTotalCurrentSent = 0.0f;
    uint32_t _totalCurrentSendTimer = 0;
    float _lastTotalPowerSent = 0.0f;
    uint32_t _totalPowerSendTimer = 0;
    bool _doResetPwm = false;

    OpenKNX::Flash::Driver *_ledStorage = nullptr;
    HWDimmer *_pDimmer;
    uint8_t _SC_HWChannels[LED_SC_ChannelCount][1];
    uint8_t _TW_HWChannels[LED_TW_ChannelCount][2];
    uint8_t _RGB_HWChannels[LED_RGB_ChannelCount][3];
    uint8_t _RGBW_HWChannels[LED_RGBW_ChannelCount][4];
    uint8_t _RGBTW_HWChannels[LED_RGBTW_ChannelCount][5];
    uint8_t _CO_HWChannels[LED_CO_ChannelCount][1];

#ifdef OPENKNX_LED_TEMPSENS_ADDR
    PCT2075 _temperature = PCT2075(OPENKNX_LED_TEMPSENS_ADDR, &OPENKNX_GPIO_WIRE);
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

    const uint8_t (*getSCHWChannels())[1] { return _SC_HWChannels; }
    const uint8_t (*getTWHWChannels())[2] { return _TW_HWChannels; }
    const uint8_t (*getRGBHWChannels())[3] { return _RGB_HWChannels; }
    const uint8_t (*getRGBWHWChannels())[4] { return _RGBW_HWChannels; }
    const uint8_t (*getRGBTWHWChannels())[5] { return _RGBTW_HWChannels; }
    // const uint8_t (*getCOHWChannels())[3] { return _CO_HWChannels; }

    SingleChannel *_singleChannels[LED_SC_ChannelCount];
    TWChannel *_twChannels[LED_TW_ChannelCount];
    RGBChannel *_rgbChannels[LED_RGB_ChannelCount];
    RGBWChannel *_rgbwChannels[LED_RGBW_ChannelCount];
    RGBTWChannel *_rgbtwChannels[LED_RGBTW_ChannelCount];
    COChannel *_coChannels[LED_CO_ChannelCount];

    SingleChannel *getSingleChannel(uint8_t channel);
    TWChannel *getTWChannel(uint8_t channel);
    RGBChannel *getRGBChannel(uint8_t channel);

    bool _CO_Active_SC[LED_SC_ChannelCount][LED_CO_ChannelCount];
    bool _CO_Active_TW[LED_TW_ChannelCount][LED_CO_ChannelCount];
    bool _CO_Active_RGB[LED_RGB_ChannelCount][LED_CO_ChannelCount];
    bool _CO_Active_RGBW[LED_RGBW_ChannelCount][LED_CO_ChannelCount];
    bool _CO_Active_RGBTW[LED_RGBTW_ChannelCount][LED_CO_ChannelCount];

    enum LightType
    {
        Single = 1,
        TunableWhite = 2,
        RGB = 3,
        RGBW = 4,
        RGBTW = 5,
        CentralObject = 6
    };
};

extern LedModule openknxLedModule;
