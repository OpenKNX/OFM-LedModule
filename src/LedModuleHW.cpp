#include "OpenKNX.h"
#include "LedModuleHW.h"



LedModuleHW::LedModuleHW(Adafruit_PWMServoDriver pwm) : pwm(pwm) {
    // Set all channels of all lights to -1 (=undefined) that we later 
    // can distinguish which channels are mapped
    for (uint8_t lightId=0;lightId<LEDMODULE_MAX_LIGHT_CHANNELS;lightId++) {
        for (uint8_t lightChannel=0;lightChannel<LEDMODULE_MAX_LIGHT_CHANNELS;lightChannel++) {
        lights[lightId].channelMapping[lightChannel] = -1;
        lights[lightId].currentBrightness[lightChannel] = 0;
        }
    }
}


std::string LedModuleHW::logPrefix() {
  return "LedModuleHW";
};

void LedModuleHW::addLight(uint8_t lightId, uint8_t lightType, uint8_t hwChannels[LEDMODULE_MAX_LIGHT_CHANNELS], uint32_t dimmDuration) {

    if (lightType == 1) { //RGBW
        lights[lightId].channelMapping[0] = hwChannels[0];
        lights[lightId].channelMapping[1] = hwChannels[1];
        lights[lightId].channelMapping[2] = hwChannels[2];
        lights[lightId].channelMapping[3] = hwChannels[3];
    } else if (lightType == 2) { //RGB
        lights[lightId].channelMapping[0] = hwChannels[0];
        lights[lightId].channelMapping[1] = hwChannels[1];
        lights[lightId].channelMapping[2] = hwChannels[2];
    } else if (lightType == 3) { //TW
        lights[lightId].channelMapping[0] = hwChannels[0];
        lights[lightId].channelMapping[1] = hwChannels[1];
    } else if (lightType == 4) { //Single channel
        lights[lightId].channelMapping[0] = hwChannels[0];
    }
    lights[lightId].dimmDuration = dimmDuration;
    lights[lightId].isConfigured = true;
    lights[lightId].processing = false;
}

void LedModuleHW::dimmLightTo(uint8_t lightId, uint32_t targetBrightness[LEDMODULE_MAX_LIGHT_CHANNELS]) {

    /*
    !!!! DO NOT CALCULATE ALL CHANNELS, BUT ONLY CHANNELS DEFINED FOR THE CORESPONDING LIGHT !!!!
    */


    int diffSteps[LEDMODULE_MAX_LIGHT_CHANNELS];

    for (uint8_t channelId = 0; channelId < LEDMODULE_MAX_LIGHT_CHANNELS; channelId++) {
        
        //Ignore not mapped hw channels
        if (lights[lightId].channelMapping[channelId] == -1) continue;

        if ((targetBrightness[channelId] < 0) || (targetBrightness[channelId] > 255)) {
            logErrorP("newValue %i is not between 0 and 255: %i", channelId, targetBrightness[channelId]);
            return;
        }

        //logDebugP("CH %i new target brightnesses %d - current: %d",channelId,targetBrightness[channelId],lights[lightId].currentBrightness[channelId]);

        lights[lightId].targetBrightness[channelId] = targetBrightness[channelId];

        //calculate the difference for each channel
        diffSteps[channelId] = targetBrightness[channelId] - lights[lightId].currentBrightness[channelId];
    }




    /* As we want a consistent dimming over all LED channels, we have to calculate the LED channel with
    * highest brightness difference. All LED channels with lesser brightness differences have to wait 
    * one or a few loop cycles.
    * 
    * First we identify the channel with the max brightness steps to do. Based on this value we can
    * calculate waitSteps for each LED channel.
    */
    int maxSteps = 0;
    for (uint8_t channelId = 0; channelId < LEDMODULE_MAX_LIGHT_CHANNELS; channelId++) {

        //Ignore not mapped hw channels
        if (lights[lightId].channelMapping[channelId] == -1) {
            //logDebugP("light %i ignore channel %i as its mapping is %i", lightId, channelId, lights[lightId].channelMapping[channelId]);
            continue;
        }

        //If diff is negativ, make it positiv
        int d = diffSteps[channelId];
        if (d < 0) {
            d = d * -1;
        }

        if (d > maxSteps) {
            maxSteps = d;
        }
        diffSteps[channelId] = d;
    }

    if (maxSteps == 0) maxSteps = 1;
    //logDebugP("max steps: %i", maxSteps);

    lights[lightId].maxSteps = maxSteps;

    // As we now know how many steps we have to go max, we can calculate how 
    // many cycles every channel has to wait until it changes it's value
    // For the channel with maxSteps it is maxSteps / diffSteps = 0 (e.g. (255/255) = 0) => change every cycle
    // For a channel which only has to change half brightness, it is floor(255/128) = 1 => change every second cycle
    // I know, this method is not perfect, especially in edge cases, but it is good enough for now

    for (uint8_t channelId = 0; channelId < LEDMODULE_MAX_LIGHT_CHANNELS; channelId++) {

        //Ignore not mapped hw channels
        if (lights[lightId].channelMapping[channelId] == -1) continue;

        lights[lightId].cyclesToWait[channelId] = (maxSteps / diffSteps[channelId]) - 1; // floor

        //logDebugP("LEDCH[%i] - cyclesToWait: %i", i, cyclesToWait[i]);
        if (lights[lightId].cyclesToWait[channelId] < 0 ) lights[lightId].cyclesToWait[channelId] = lights[lightId].cyclesToWait[channelId] * -1;

        logDebugP("LEDCH[%i] - HWCHANNEL %d - Target: %d - Steps: %i | cyclesToWait: %i ", channelId, lights[lightId].channelMapping[channelId], lights[lightId].targetBrightness[channelId], diffSteps[channelId], lights[lightId].cyclesToWait[channelId]);

    }

    lights[lightId].processing = true;
    lights[lightId].processingStarted = millis();
    lights[lightId].dimmStepEveryMillis = lights[lightId].dimmDuration / lights[lightId].maxSteps;

    logDebugP("lightID %d is processing",lightId);

}



// dimmLoop is the main dimmLoop
void LedModuleHW::dimmLoop() {

    uint32_t dimmLoopStartMillis = millis();

    for (uint8_t lightId=0;lightId<LEDMODULE_MAX_LIGHT_CHANNELS;lightId++) {

        // Ignore not configured lights
        if (!lights[lightId].isConfigured) continue;


        // Go to next light, if this light is not in processing state
        if (!lights[lightId].processing) continue;

        uint32_t processingTime = millis() - lights[lightId].processingStarted;

        if (processingTime >= 5000) {
            if (processingTime % 5000 == 0) {
                logDebugP("L %d processingTime to long: %i", lightId, processingTime);
            }
        }

        // If we could not dimm all lights in the last loop cycle, we resume, were we left off
        if (lightId < latestLightIdInMainLoop) continue;

        if (processingTime % 3000 == 0) {
            logDebugP("DEBUG XXX");
        }

        // If next processingStep time not reached, continue with next light
        uint32_t nextProcessingStep = lights[lightId].lastProcessingStep + lights[lightId].dimmStepEveryMillis;
        if (nextProcessingStep > millis()) continue;

        if (processingTime % 3000 == 0) {
            logDebugP("DEBUG YYY");
        }

        //logDebugP("working on light: %i", lightId);

        uint8_t processing = 0;

        for (uint8_t lightChannel=0;lightChannel<LEDMODULE_MAX_LIGHT_CHANNELS;lightChannel++) {



           // logDebugP("light %i - set hardware channelmapping: %i -> %i",lightId, lightChannel,lights[lightId].channelMapping[lightChannel]);

            // Ignore not configured channels
            if (lights[lightId].channelMapping[lightChannel] == -1) continue;

            logDebugP("dimmstep; lightID: %d, channel: %d, hwchannel %d", lightId, lightChannel, lights[lightId].channelMapping[lightChannel]);

            //If we have to wait for consistent dimming go to next channel
            if (lights[lightId].cyclesWaited[lightChannel] < lights[lightId].cyclesToWait[lightChannel]) {
                lights[lightId].cyclesWaited[lightChannel]++;
                continue;
            } else {
                lights[lightId].cyclesWaited[lightChannel] = 0;
            }



            if (lights[lightId].currentBrightness[lightChannel] < lights[lightId].targetBrightness[lightChannel]) {
                lights[lightId].currentBrightness[lightChannel]++;
                processing++;
            } else if (lights[lightId].currentBrightness[lightChannel] > lights[lightId].targetBrightness[lightChannel]) {
                lights[lightId].currentBrightness[lightChannel]--;
                processing++;
            }

            uint8_t hardwareChannel = lights[lightId].channelMapping[lightChannel];
            //Dimm linear for now (255 * 16 = 4095)

             //logDebugP("light %i - set hardware channel: %i -> %i (%i)", lightId, hardwareChannel, lights[lightId].currentBrightness[lightChannel], lights[lightId].targetBrightness[lightChannel]);


            pwm.setPWM(hardwareChannel, 0, lights[lightId].currentBrightness[lightChannel] * 16);

        }

        lights[lightId].lastProcessingStep = millis();

        //logDebugP("light %i %i", lightId,processing);

        //If no channel is processing anymore, set light processing to false
        if (processing == 0) {
            lights[lightId].processing = false;
            uint32_t duration = millis() - lights[lightId].processingStarted;
            logDebugP("light %i dimming took ms %i", lightId,duration);
        }

        if (processingTime % 3000 == 0) {
            logDebugP("DEBUG ZZZ");
        }

        // If we are to long in the dimming loop, return to main loop
        if ((millis() - dimmLoopStartMillis) >= 6) {
            latestLightIdInMainLoop = lightId;
            return;
        }

    }

}

void LedModuleHW::dimmToLastAfterI2cIsBack() {
    for (uint8_t lightId=0;lightId<LEDMODULE_MAX_LIGHT_CHANNELS;lightId++) {
        lights[lightId].processing = false;
        for (uint8_t lightChannel=0;lightChannel<LEDMODULE_MAX_LIGHT_CHANNELS;lightChannel++) {
            
            lights[lightId].targetBrightness[lightChannel] = lights[lightId].currentBrightness[lightChannel];
            lights[lightId].currentBrightness[lightChannel] = 0;
        }
        dimmLightTo(lightId, lights[lightId].targetBrightness);
    }        
}