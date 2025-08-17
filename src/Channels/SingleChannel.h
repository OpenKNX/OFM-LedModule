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

    uint16_t dimmingTimeON();
    uint16_t dimmingTimeOFF();
    uint16_t dimmingTime(bool _switch);
    uint16_t dimmingValStartup();
    uint16_t dimmingValMax();
    uint16_t dimmingValTarget(bool _switch);
    uint16_t checkMinMaxBrightness(uint16_t _bright);

    void setSwitch(bool _switch);
    void setBrightness(uint16_t _bright);
    void setNight(bool _night) override;
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
