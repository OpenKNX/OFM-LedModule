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
    uint16_t dimmingTime(bool _switch);
    uint16_t dimmingValStartup();
    uint16_t dimmingValMin();
    uint16_t dimmingValMax();
    uint16_t dimmingValTarget(bool _switch);
    uint16_t checkMinMaxBrightness(uint16_t _bright);
    int32_t dimmingTempStartup();
    int32_t dimmingTempMax();
    int32_t dimmingTempTarget(bool _switch);
    uint16_t checkMinMaxColorTemp(uint16_t colorTemp);
    int32_t getLastOnValueTemp() { return _lastOnValueTemp; }
    void setLastOnValueTemp(int32_t lastOnValueTemp) { _lastOnValueTemp = lastOnValueTemp; }

    void setSwitch(bool _switch);
    void setSwitchNoDim(bool _switch);
    void setBoost(bool _switch);
    void setBrightness(uint16_t _bright);
    void setNight(bool _night) override;
    void relDimUp();
    void relDimDown();
    void relDimStop();
    void setColorTemperature(uint16_t colorTemp);

  private:
    const std::string name() override;

    uint16_t _lastColorTemp = 0;
    DimmableValue<uint16_t> _colorTemperature;

    int32_t _lastOnValueTemp = 4000;

    void handleScene(uint8_t sceneNr);
    enum ValueType
    {
        BRIGHTNESS = 0,
        TEMTPERATURE = 1,
        COMBINED = 2,
    };
};
