#pragma once

#include "Channels/LightChannel.h"
#include "HWDimmer/HWDimmer.h"
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
    uint16_t dimmingTime(bool switchOn);
    uint16_t dimmingValStartup();
    uint16_t dimmingValMin();
    uint16_t dimmingValMax();
    uint16_t dimmingValTarget(bool switchOn);
    uint16_t checkMinMaxBrightness(uint16_t bright);

    void setSwitch(bool switchOn) override;
    void setSwitchNoDim(bool switchOn) override;
    void setBrightness(uint16_t bright);
    void setNight(bool night) override;
    void relDimUp();
    void relDimDown();
    void relDimStop();

    bool getCO1();
    bool getCO2();
    bool getCO3();

#ifdef LEDMODULE_MAX_TOTAL_BRIGHTNESS
    float budgetFootprint() override
    {
        return _channelActive ? (float)_brightness.target() / (float)VALUE_KNX_COUNT : 0.0f;
    }
#endif

  private:
    const std::string name() override;

    void handleScene(uint8_t sceneNr);
    enum ValueType
    {
        BRIGHTNESS = 0
    };
};
