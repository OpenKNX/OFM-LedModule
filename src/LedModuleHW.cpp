#include "LedModuleHW.h"
#include "OpenKNX.h"

LedModuleHW::LedModuleHW(HWDimmer *pDimmer) : pDimmer(pDimmer)
{
    // Set all channels of all lights to -1 (=undefined) that we later
    // can distinguish which channels are mapped
    for (uint8_t lightId = 0; lightId < LEDMODULE_MAX_LIGHT_CHANNELS; lightId++)
    {
        for (uint8_t lightChannel = 0; lightChannel < LEDMODULE_MAX_LIGHT_CHANNELS; lightChannel++)
        {
            lights[lightId].channelMapping[lightChannel] = -1;
            lights[lightId].currentBrightness[lightChannel] = 0;
        }
    }
}

std::string LedModuleHW::logPrefix()
{
    return "LedModuleHW";
};

void LedModuleHW::addLight(uint8_t lightId, uint8_t lightType, uint8_t hwChannels[LEDMODULE_MAX_LIGHT_CHANNELS], uint32_t dimmDuration)
{
    if (lightType == 1)
    { // RGBW
        lights[lightId].channelMapping[0] = hwChannels[0];
        lights[lightId].channelMapping[1] = hwChannels[1];
        lights[lightId].channelMapping[2] = hwChannels[2];
        lights[lightId].channelMapping[3] = hwChannels[3];
    }
    else if (lightType == 2)
    { // RGB
        lights[lightId].channelMapping[0] = hwChannels[0];
        lights[lightId].channelMapping[1] = hwChannels[1];
        lights[lightId].channelMapping[2] = hwChannels[2];
    }
    else if (lightType == 3)
    { // TW
        lights[lightId].channelMapping[0] = hwChannels[0];
        lights[lightId].channelMapping[1] = hwChannels[1];
    }
    else if (lightType == 4)
    { // Single channel
        lights[lightId].channelMapping[0] = hwChannels[0];
    }
    lights[lightId].dimmDuration = dimmDuration;
    lights[lightId].isConfigured = true;
    lights[lightId].processing = false;
}

void LedModuleHW::dimmLightTo(uint8_t lightId, uint32_t targetBrightness[LEDMODULE_MAX_LIGHT_CHANNELS], uint16_t dimTime)
{
    for (uint8_t channelId = 0; channelId < LEDMODULE_MAX_LIGHT_CHANNELS; channelId++)
    {
        // Ignore not mapped hw channels
        if (lights[lightId].channelMapping[channelId] == -1)
            continue;

        if ((targetBrightness[channelId] < 0) || (targetBrightness[channelId] > 255))
        {
            logErrorP("newValue %i is not between 0 and 255: %i", channelId, targetBrightness[channelId]);
            return;
        }

        lights[lightId].targetBrightness[channelId] = targetBrightness[channelId];
        pDimmer->dimToLevel(channelId, lights[lightId].targetBrightness[channelId], dimTime);
    }
}

// dimmLoop is the main dimmLoop
void LedModuleHW::dimmLoop()
{
    pDimmer->dimLoop();
}

void LedModuleHW::dimmToLastAfterI2cIsBack()
{
    for (uint8_t lightId = 0; lightId < LEDMODULE_MAX_LIGHT_CHANNELS; lightId++)
    {
        lights[lightId].processing = false;
        for (uint8_t lightChannel = 0; lightChannel < LEDMODULE_MAX_LIGHT_CHANNELS; lightChannel++)
        {

            lights[lightId].targetBrightness[lightChannel] = lights[lightId].currentBrightness[lightChannel];
            lights[lightId].currentBrightness[lightChannel] = 0;
        }
        dimmLightTo(lightId, lights[lightId].targetBrightness, 100);
    }
}