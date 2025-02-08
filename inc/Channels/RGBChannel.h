#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"
#include "LightChannel.h"
#include "Colors.h"

class RGBChannel : public LightChannel
{
  public:
    RGBChannel(uint8_t channel_number, HWDimmer *pDimmer, uint8_t hwChannels[3]);
    void processInputKo(GroupObject &ko);
    void update();
    void loop();
    bool _rgb_night = 0;
    bool RGB_night();
    void set_RGB(uint8_t _selection);
    uint32_t conv_Temp2RGB(int _temp);

  private:
    const std::string name() override;

    uint16_t _lastHueValue = 0;
    uint16_t _lastSatValue = 0;

    DimmableValue<uint16_t> _hue;
    DimmableValue<uint16_t> _saturation;

    void handleScene(uint8_t sceneNr);
    enum ValueType
    {
        BRIGHTNESS = 0,
        COLOR = 1,
        HUE = 2,
        SATURATION = 3
    };
};
