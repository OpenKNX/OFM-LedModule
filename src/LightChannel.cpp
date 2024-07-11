#include <knx.h>
#include "LedModuleHW.h"
#include "LightChannel.h"


LightChannel::LightChannel(uint8_t index)
{
  _channelIndex = index;
}

const std::string LightChannel::name()
{
  return "LightChannel";
}

void LightChannel::Setup(LedModuleHW* light_x)
{

  light = light_x;
  logInfoP("setup");
  logIndentUp();
  logDebugP("debug setup");
  logTraceP("trace setup");
  logIndentDown();


  uint8_t lightType = ParamLED_Lighttype_;
  uint8_t hwChannels[LEDMODULE_MAX_LIGHT_CHANNELS];
  uint32_t dimmDuration = ParamLED_LightDimmTime_;


  hwChannels[0] = ParamLED_LightChannelA_;
  hwChannels[1] = ParamLED_LightChannelB_;
  hwChannels[2] = ParamLED_LightChannelC_;
  hwChannels[3] = ParamLED_LightChannelD_;

  light->addLight(_channelIndex,lightType,hwChannels,dimmDuration);


}

void LightChannel::Loop()
{
  // Dummy Action
  delayMicroseconds(100);
}

void LightChannel::ProcessInputKo(GroupObject& ko)
{
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());

    switch(RelKO(ko.asap()))
    {
      case LED_KoScene_:
        logDebugP("lightScene received");
        uint8_t sceneId = KoLED_Scene_.value(DPT_SceneNumber);
        activateScene(sceneId);
        
      break;
    }

}

void LightChannel::activateScene(uint8_t sceneId) {
  logDebugP("channelIndex %d",_channelIndex);

  uint8_t lightId = _channelIndex;

  logDebugP("activate scene %d", sceneId);
  uint8_t lightType = ParamLED_Lighttype_;
  logDebugP("Light type %d",lightType);

  uint8_t defSceneLength = 5; //This is always 5 (RGB + W + CW) until we add something to offsets in XMLS


  if (lightType == 1) { // RGBW


    uint8_t sceneLength = 4; //3 bytes for rgb + 1 byte for w
    uint32_t values[sceneLength];


    uint32_t rgbIndex = (LED_LightColorRGB_Scene1_ + (sceneId * defSceneLength) + LED_ParamBlockOffset + _channelIndex * LED_ParamBlockSize);
    uint32_t wIndex = (LED_LightColorW_Scene1_ + (sceneId * defSceneLength) + LED_ParamBlockOffset + _channelIndex * LED_ParamBlockSize);

    uint32_t rgb = ((knx.paramInt(rgbIndex) & LED_LightColorRGB_Scene1_Mask) >> LED_LightColorRGB_Scene1_Shift); //Mask & Shift are indentical for all scenes. Therefore we can use always use Mask & Shift from Scene 1

    values[0] = (rgb >> 16) & 0xFF; //red
    values[1] = (rgb >> 8) & 0xFF; //green
    values[2] = (rgb >> 0) & 0xFF; //blue
    values[3] = knx.paramByte(wIndex); //white

    bool stateOnOff = false;


    for (int i = 0; i < sceneLength; i++) {
        uint8_t value = values[i];
        //If at least one LED channel is on, OnOff status KO should be "on"
        if (value > 0) {
            stateOnOff = true;
        }
    }
    logDebugP("lightID %d - RGBW: %d,%d,%d,%d ",lightId, values[0],values[1],values[2],values[3]);

    light->dimmLightTo(lightId, values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);

  } else if (lightType == 2) { // RGB


    uint8_t sceneLength = 3; //3 bytes for rgb
    uint32_t values[sceneLength];

    uint32_t rgbIndex = (LED_LightColorRGB_Scene1_ + (sceneId * defSceneLength) + LED_ParamBlockOffset + _channelIndex * LED_ParamBlockSize);

    uint32_t rgb = ((knx.paramInt(rgbIndex) & LED_LightColorRGB_Scene1_Mask) >> LED_LightColorRGB_Scene1_Shift); //Mask & Shift are indentical for all scenes. Therefore we can use always use Mask & Shift from Scene 1

    values[0] = (rgb >> 16) & 0xFF; //red
    values[1] = (rgb >> 8) & 0xFF; //green
    values[2] = (rgb >> 0) & 0xFF; //blue

    bool stateOnOff = false;


    for (int i = 0; i < sceneLength; i++) {
        uint8_t value = values[i];
        //If at least one LED channel is on, OnOff status KO should be "on"
        if (value > 0) {
            stateOnOff = true;
        }
    }
    logDebugP("lightID %d - RGB: %d,%d,%d ",lightId, values[0],values[1],values[2]);

    light->dimmLightTo(lightId, values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);

  } else if (lightType == 3) { // TW

    uint8_t sceneLength = 2; //1 byte for ww + 1 byte for cw
    uint32_t values[sceneLength];

    uint32_t wIndex = (LED_LightColorW_Scene1_ + (sceneId * defSceneLength) + LED_ParamBlockOffset + _channelIndex * LED_ParamBlockSize);
    values[0] = knx.paramByte(wIndex); //white
    uint32_t cwIndex = (LED_LightColorCW_Scene1_ + (sceneId * defSceneLength) + LED_ParamBlockOffset + _channelIndex * LED_ParamBlockSize);
    values[1] = knx.paramByte(cwIndex); //cold white

    bool stateOnOff = false;

    for (int i = 0; i < sceneLength; i++) {
        uint8_t value = values[i];
        //If at least one LED channel is on, OnOff status KO should be "on"
        if (value > 0) {
            stateOnOff = true;
        }
    }
    logDebugP("lightID %d - TW: %d,%d",lightId, values[0],values[1]);

    light->dimmLightTo(lightId, values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);
  } else if (lightType == 4) { // single channel

    uint8_t sceneLength = 1; //1 byte for ww
    uint32_t values[sceneLength];

    uint32_t wIndex = (LED_LightColorW_Scene1_ + (sceneId * defSceneLength) + LED_ParamBlockOffset + _channelIndex * LED_ParamBlockSize);
    values[0] = knx.paramByte(wIndex); //white

    bool stateOnOff = false;

    for (int i = 0; i < sceneLength; i++) {
        uint8_t value = values[i];
        //If at least one LED channel is on, OnOff status KO should be "on"
        if (value > 0) {
            stateOnOff = true;
        }
    }
    logDebugP("lightID %d - single channel: %d",lightId, values[0]);

    light->dimmLightTo(lightId,values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);

  }
}
