#pragma once

#include "HWDimmer.h"
#include "LightChannel.h"
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
    bool _tw_night = 0;
    bool TW_night();

    uint16_t dimmingTimeON();
    uint16_t dimmingTimeOFF();
    uint16_t dimmingTime(bool _switch);
    uint8_t dimmingValMaxBehavior();
    uint8_t maxDimVal();
    uint8_t upperTargetValue();
    uint8_t dimmingTarget(bool _switch);
    uint16_t checkMinMaxColorTemp(uint16_t colorTemp);

    void setSwitch(bool _switch);
    void setBrightness(uint8_t _bright);
    void setNight(bool _night);
    void relDimUp();
    void relDimDown();
    void relDimStop();
    void setColorTemperature(uint16_t colorTemp);

  private:
    const std::string name() override;
    void processFrontInput();

    uint16_t _lastColorTemp = 0;
    DimmableValue<uint16_t> _colorTemperature;

    void handleScene(uint8_t sceneNr);
    enum ValueType
    {
        BRIGHTNESS = 0,
        TEMTPERATURE = 1,
        COMBINED = 2,
    };
};
