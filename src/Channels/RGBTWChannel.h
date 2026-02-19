#pragma once

#include "Channels/LightChannel.h"
#include "Colors.h"
#include "HWDimmer/HWDimmer.h"
#include "OpenKNX.h"
#include <Arduino.h>

class RGBTWChannel : public LightChannel
{
  public:
    RGBTWChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[3]);
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
    void setStartupColor();
    uint8_t getDefaultColor();
    uint16_t checkMinMaxColorTemp(uint16_t colorTemp);
    uint16_t getLastOnValueHue() { return _lastOnValueHue; }
    void setLastOnValueHue(uint16_t lastOnValueHue) { _lastOnValueHue = lastOnValueHue; }
    uint16_t getLastOnValueSat() { return _lastOnValueSat; }
    void setLastOnValueSat(uint16_t lastOnValueSat) { _lastOnValueSat = lastOnValueSat; }

    void setSwitch(bool switchOn) override;
    void setSwitchNoDim(bool switchOn) override;
    void setHue(uint16_t hue);
    void setSaturation(uint16_t saturation);
    void setBrightness(uint16_t bright);
    void setNight(bool night) override;
    void relDimUp();
    void relDimDown();
    void relDimStop();
    void setColorTemperature(uint16_t colorTemp);
    void relDimUpColor();
    void relDimDownColor();
    void relDimStopColor();
    void RGBpicker(uint8_t selection);
    void setRGB(uint32_t RGBvalue);
    void setHSV(uint32_t HSVvalue);
    uint32_t conv_Temp2RGB(int temp);
    int conv_RGB2Temp(uint32_t target_rgb);

  private:
    const std::string name() override;

    uint16_t _lastHueValue = 0;
    uint16_t _lastSatValue = 0;
    uint16_t _lastColorTemp = 4000;

    uint16_t _lastOnValueHue = 0;
    uint16_t _lastOnValueSat = 0;

    DimmableValue<uint16_t> _hue;
    DimmableValue<uint16_t> _saturation;

    uint32_t _statusSendTemperaturTimer = 0;
    uint32_t _statusSendRgbTimer = 0;
    uint32_t _statusSendHsvTimer = 0;

    void handleScene(uint8_t sceneNr);
    enum ValueType
    {
        BRIGHTNESS = 0,
        COLOR = 1,
        HUE = 2,
        SATURATION = 3
    };
};
