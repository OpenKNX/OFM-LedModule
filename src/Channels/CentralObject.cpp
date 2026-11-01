#include "CentralObject.h"
#include "../LedModule.h"
#include "LightChannel.h"
#include <knx.h>

COChannel::COChannel(uint8_t index, HWDimmer *pDimmer, uint8_t hwChannels[3])
    : LightChannel(index, pDimmer, hwChannels, 3), _hue(0, 0, H_PART - 1),
      _saturation(0, 0, VAL_RANGE) {}

const std::string COChannel::name() { return "COChannel"; }

void COChannel::update() {}

void COChannel::loop()
{
    if (!_channelActive)
        return;
}

void COChannel::processInputKo(GroupObject &ko)
{
    CO_Number = (ko.asap() - LED_CO_KoOffset) / LED_CO_KoBlockSize;
    logDebugP("processInputKo Channel CO %d", CO_Number);
    Colors::HSV hsv;
    Colors::RGB rgb;
    int16_t relKO = (ko.asap() - LED_CO_KoOffset);

    logHexDebugP(ko.valueRef(), ko.valueSize());

    // check if channel is valid
    if ((int8_t)(relKO / LED_CO_KoBlockSize) == channelIndex())
        relKO = relKO % LED_CO_KoBlockSize;
    else
        relKO = -1;

    if (relKO == LED_CO_KoChLocking)
        _isLocked = ko.value(DPT_Switch);
    else if (!_isLocked)
    {
        switch (relKO)
        {
            case LED_CO_KoChSwitch:
                if (!getLock())
                    setSwitch(ko.value(DPT_Switch));
                break;

            case LED_CO_KoChSwitchNoDim:
                if (!getLock())
                    setSwitchNoDim(ko.value(DPT_Switch));
                break;

            case LED_CO_KoChLocking:
                setLock(ko.value(DPT_Switch));
                // KoLED_CO_ChStateLocking.value(getLock(), DPT_Switch);
                break;

            case LED_CO_KoChBrightness:
                if (!getLock())
                    setBrightness(LedBrightness::knxToLevel(*ko.valueRef()));
                break;

            case LED_CO_KoChBrightnessRel:
                if (!getLock())
                {
                    uint8_t tmpu8 = *KoLED_CO_ChBrightnessRel.valueRef();
                    uint8_t stepCode = tmpu8 & 0x07;

                    if (stepCode == 0)
                        relDimStop();
                    else if (tmpu8 & 0x08)
                        relDimUp(stepCode);
                    else
                        relDimDown(stepCode);
                }
                break;

            case LED_CO_KoChScene:
                if (!getLock())
                {
                    handleScene(ko.value(DPT_SceneNumber));
                    _sceneNumberActive = (uint8_t)ko.value(DPT_SceneNumber) + 1;
                }
                break;

            case LED_CO_KoChColorTemperature:
                if (!getLock())
                    setColorTemperature(ko.value(Dpt(7, 600)));
                break;

            case LED_CO_KoChColorTemperatureRel:
                if (!getLock())
                {
                    int16_t tmpu16;
                    tmpu16 = *KoLED_CO_ChColorTemperatureRel.valueRef();

                    if (tmpu16 >= 0x09)
                        relDimUpColor();
                    if (tmpu16 > 0x00 && tmpu16 < 0x08)
                        relDimDownColor();
                    if (tmpu16 == 0x00 || tmpu16 == 0x08)
                        relDimStopColor();
                }
                break;

            case LED_CO_KoChRGB:
                if (!getLock())
                    setRGB(ko.value(DPT_Colour_RGB));
                break;

            case LED_CO_KoChHSV:
                if (!getLock())
                    setHSV(ko.value(DPT_Colour_RGB));
                break;

            // Day or Night
            case LED_CO_KoChNight:
                if (!getLock())
                    setNight(ko.value(DPT_Switch));
                break;

                // case LED_CO_KoChStateOnOff:
                // case LED_CO_KoChStateLocking:
                // case LED_CO_KoChBrightnessStatus:
                // case LED_CO_KoChColorTemperatureStatus:
                // case LED_CO_KoChRGBStatus:
                // case LED_CO_KoChHSVStatus:

            default:
                logDebugP("Unknown KO %d", relKO);
                break;
        }
    }
}

void COChannel::handleScene(uint8_t sceneNr)
{
    // a central object has no scene table of its own: every assigned channel recalls its own scene
    logDebugP("Scene: %d", sceneNr);
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->callScene(sceneNr);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->callScene(sceneNr);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->callScene(sceneNr);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->callScene(sceneNr);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->callScene(sceneNr);
        }
    }
}

void COChannel::setSwitch(bool switchOn)
{

    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->setSwitch(switchOn);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->setSwitch(switchOn);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setSwitch(switchOn);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setSwitch(switchOn);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setSwitch(switchOn);
        }
    }
}

void COChannel::setSwitchNoDim(bool switchOn)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->setSwitchNoDim(switchOn);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->setSwitch(switchOn);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setSwitchNoDim(switchOn);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setSwitchNoDim(switchOn);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setSwitchNoDim(switchOn);
        }
    }
}

void COChannel::setHue(uint16_t hue)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->setHue(hue);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            // openknxLedModule._twChannels[ch]->setHue(hue);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setHue(hue);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setHue(hue);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setHue(hue);
        }
    }
}

void COChannel::setSaturation(uint16_t saturation)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->setSaturation(saturation);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            // openknxLedModule._twChannels[ch]->setSaturation(saturation);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setSaturation(saturation);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setSaturation(saturation);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setSaturation(saturation);
        }
    }
}

void COChannel::setBrightness(uint16_t bright)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->setBrightness(bright);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->setBrightness(bright);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setBrightness(bright);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setBrightness(bright);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setBrightness(bright);
        }
    }
}

void COChannel::setNight(bool night)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->setNight(night);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->setNight(night);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setNight(night);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setNight(night);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setNight(night);
        }
    }
}

void COChannel::relDimUp(uint8_t stepCode)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->relDimUp(stepCode);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->relDimUp(stepCode);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->relDimUp(stepCode);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->relDimUp(stepCode);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->relDimUp(stepCode);
        }
    }
}

void COChannel::relDimDown(uint8_t stepCode)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->relDimDown(stepCode);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->relDimDown(stepCode);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->relDimDown(stepCode);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->relDimDown(stepCode);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->relDimDown(stepCode);
        }
    }
}

void COChannel::relDimStop()
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            openknxLedModule._singleChannels[ch]->relDimStop();
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->relDimStop();
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->relDimStop();
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->relDimStop();
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->relDimStop();
        }
    }
}

void COChannel::setColorTemperature(uint16_t colorTemp)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->setColorTemperature(colorTemp);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->setColorTemperature(colorTemp);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setColorTemperature(colorTemp);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setColorTemperature(colorTemp);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setColorTemperature(colorTemp);
        }
    }
}

void COChannel::relDimUpColor()
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->relDimUpColor();
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->relDimUpColor();
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->relDimUpColor();
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->relDimUpColor();
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->relDimUpColor();
        }
    }
}

void COChannel::relDimDownColor()
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->relDimDownColor();
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->relDimDownColor();
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->relDimDownColor();
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->relDimDownColor();
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->relDimDownColor();
        }
    }
}

void COChannel::relDimStopColor()
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->relDimStopColor();
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            openknxLedModule._twChannels[ch]->relDimStopColor();
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->relDimStopColor();
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->relDimStopColor();
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->relDimStopColor();
        }
    }
}

void COChannel::setRGB(uint32_t RGBvalue)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->setRGB(RGBvalue);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            // openknxLedModule._twChannels[ch]->setRGB(RGBvalue);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setRGB(RGBvalue);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setRGB(RGBvalue);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setRGB(RGBvalue);
        }
    }
}

void COChannel::RGBpicker(uint8_t selection) {}

void COChannel::setHSV(uint32_t HSVvalue)
{
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++)
    {
        if (ch < LED_SC_ChannelCount && openknxLedModule._CO_Active_SC[ch][CO_Number])
        {
            // openknxLedModule._singleChannels[ch]->setHSV(HSVvalue);
        }
        if (ch < LED_TW_ChannelCount && openknxLedModule._CO_Active_TW[ch][CO_Number])
        {
            // openknxLedModule._twChannels[ch]->setHSV(HSVvalue);
        }
        if (ch < LED_RGB_ChannelCount && openknxLedModule._CO_Active_RGB[ch][CO_Number])
        {
            openknxLedModule._rgbChannels[ch]->setHSV(HSVvalue);
        }
        if (ch < LED_RGBW_ChannelCount && openknxLedModule._CO_Active_RGBW[ch][CO_Number])
        {
            openknxLedModule._rgbwChannels[ch]->setHSV(HSVvalue);
        }
        if (ch < LED_RGBTW_ChannelCount && openknxLedModule._CO_Active_RGBTW[ch][CO_Number])
        {
            openknxLedModule._rgbtwChannels[ch]->setHSV(HSVvalue);
        }
    }
}
bool COChannel::getCO1()
{
    return 0;
}
bool COChannel::getCO2()
{
    return 0;
}
bool COChannel::getCO3()
{
    return 0;
}
// EOF