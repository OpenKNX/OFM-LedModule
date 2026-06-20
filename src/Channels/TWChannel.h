#pragma once

#include "Channels/LightChannel.h"
#include "HWDimmer/HWDimmer.h"
#include "OpenKNX.h"
#include <Arduino.h>

class TWChannel : public LightChannel
{
  public:
    TWChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[2]);
    void processInputKo(GroupObject& ko);
    void dimLoop();
    void update();
    void loop();

    uint16_t dimmingTimeON();
    uint16_t dimmingTimeOFF();
    uint16_t dimmingTime(bool switchOn);
    uint16_t dimmingValStartup();
    uint16_t dimmingValMin();
    uint16_t dimmingValMax();
    uint16_t dimmingValTarget(bool switchOn);
    uint16_t checkMinMaxBrightness(uint16_t bright);
    int32_t dimmingTempStartup();
    int32_t dimmingTempMax();
    int32_t dimmingTempTarget(bool switchOn);
    uint16_t checkMinMaxColorTemp(uint16_t colorTemp);
    int32_t getLastOnValueTemp() { return _lastOnValueTemp; }
    void setLastOnValueTemp(int32_t lastOnValueTemp) { _lastOnValueTemp = lastOnValueTemp; }
    int32_t getCurrentTempValue() { return _colorTemperature.value(); }

    void setSwitch(bool switchOn) override;
    void setSwitchNoDim(bool switchOn) override;
    void setBoost(bool switchOn);
    void setBrightness(uint16_t bright);
    void setBrightnessNoDim(uint16_t bright);
    void setNight(bool night) override;
    void relDimUp();
    void relDimDown();
    void relDimStop();
    void setColorTemperature(uint16_t colorTemp);
    void setColorTemperatureNoDim(uint16_t colorTemp);
    void relDimUpColor();
    void relDimDownColor();
    void relDimStopColor();

    bool getCO1();
    bool getCO2();
    bool getCO3();

  private:
    const std::string name() override;

    uint16_t _lastColorTemp = 0;
    DimmableValue<uint16_t> _colorTemperature;

    int32_t _lastOnValueTemp = (ParamLED_TW_ChColorTempCW + ParamLED_TW_ChColorTempWW) / 2;
    bool _boost = false;
    StatusValueState _statusColorTemp;

    void handleScene(uint8_t sceneNr);
    enum ValueType
    {
        BRIGHTNESS = 0,
        TEMTPERATURE = 1,
        COMBINED = 2,
    };
};
