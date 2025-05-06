#pragma once

#include "HWDimmer\HWDimmer.h"
#include "Channels\LightChannel.h"
#include "OpenKNX.h"
#include <Arduino.h>

class SingleChannel : public LightChannel
{
  public:
    SingleChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[1]);
    void processInputKo(GroupObject& ko);
    void update();
    void loop();
    bool _sc_night = 0;
    bool SC_night();

    uint16_t dimmingTimeON();
    uint16_t dimmingTimeOFF();
    uint16_t dimmingTime(bool _switch);
    uint8_t dimmingValMaxBehavior();
    uint8_t maxDimVal();
    uint8_t upperTargetValue();
    uint8_t dimmingTarget(bool _switch);
    

    void setSwitch(bool _switch);
    void setBrightness(uint8_t _bright);
    void setNight(bool _night);
    void relDimUp();
    void relDimDown();
    void relDimStop();

  private:
    const std::string name() override;
    void processFrontInput();

    void handleScene(uint8_t sceneNr);
    enum ValueType
    {
        BRIGHTNESS = 0
    };
};
