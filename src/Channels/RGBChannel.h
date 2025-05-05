#pragma once

#include "Colors.h"
#include "HWDimmer.h"
#include "LightChannel.h"
#include "OpenKNX.h"
#include <Arduino.h>

class RGBChannel : public LightChannel
{
  public:
    RGBChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[3]);
    void processInputKo(GroupObject& ko);
    void update();
    void loop();
    bool _rgb_night = 0;
    bool RGB_night();

    uint16_t dimmingTimeON();
    uint16_t dimmingTimeOFF();
    uint16_t dimmingTime(bool _switch);
    uint8_t dimmingValMaxBehavior();
    uint8_t maxDimVal();
    uint8_t upperTargetValue();
    uint8_t dimmingTarget(bool _switch);
    uint8_t colorPicker();
    uint16_t checkMinMaxColorTemp(uint16_t colorTemp);

    void setSwitch(bool _switch);
    void setHue(uint16_t _hue);
    void setSaturation(uint16_t saturation);
    void setBrightness(uint8_t _bright);
    void switchNight(bool _night);
    void relDimUp();
    void relDimDown();
    void relDimStop();
    void setColorTemperature(uint16_t colorTemp);
    void RGBpicker(uint8_t _selection);
    void setRGB(uint32_t RGBvalue);
    void setHSV(uint32_t HSVvalue);
    uint32_t conv_Temp2RGB(int _temp);
  
  private:
    const std::string name() override;
    void processFrontInput();

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
