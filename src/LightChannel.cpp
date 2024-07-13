#include <knx.h>
#include "LightChannel.h"


LightChannel::LightChannel(uint8_t index, HWDimmer* pDimmer)
{
  _channelIndex = index;

  this->pDimmer = pDimmer;
  logInfoP("setup");
  logIndentUp();
  logDebugP("debug setup");
  logTraceP("trace setup");
  logIndentDown();

  lightType = ParamLED_Lighttype_;
  dimmDuration = ParamLED_LightDimmTime_;

  hwChannels[0] = ParamLED_LightChannelA_;  // TODO: replace this quick hach with propper implementation
  hwChannels[1] = lightType==3? ParamLED_LightChannelB_ : -1; // TW
  hwChannels[2] = lightType==2? ParamLED_LightChannelC_ : -1; // RGB
  hwChannels[3] = lightType==1? ParamLED_LightChannelD_ : -1; // RGBW
}

const std::string LightChannel::name()
{
  return "LightChannel";
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
  uint8_t sceneId = 0;

  switch(RelKO(ko.asap()))
  {
    case LED_KoScene_:
      logDebugP("lightScene received");
      sceneId = KoLED_Scene_.value(DPT_SceneNumber);
      activateScene(sceneId);
      
    break;

    case LED_KoOnOff_:
      logInfoP("OnOff: %d",(uint16_t)KoLED_OnOff_.value(DPT_Switch));
      setOnOff((bool)KoLED_OnOff_.value(DPT_Switch));
    break;

    case LED_KoBrightness_:
      logInfoP("Brightness: %d",(uint16_t)KoLED_Brightness_.value(DPT_DecimalFactor));
      setBrightness((uint8_t)KoLED_Brightness_.value(DPT_DecimalFactor));
    break;
    
    default: 
    break;
  }
}

void LightChannel::setOnOff(bool OnOff)
{
  uint32_t values[4] = {0};
  if (ParamLED_Lighttype_ == 3)
  {
      if(OnOff)
      {
        values[0] = 255;
        values[1] = 255;
      }
    dimLightTo(values, ParamLED_LightDimmTime_);
    KoLED_StateOnOff_.value(OnOff, DPT_Switch);
    KoLED_BrightnessStatus_.value(values[0], DPT_DecimalFactor);
  }
}

void LightChannel::setBrightness(uint8_t level)
{
  uint32_t values[4] = {0};
  if (ParamLED_Lighttype_ == 3)
  {
      for(int i=0; i<4; i++) values[i]=level;
    dimLightTo(values, ParamLED_LightDimmTime_);
    KoLED_BrightnessStatus_.value(level, DPT_DecimalFactor);
    KoLED_StateOnOff_.value(level > 0, DPT_Switch);
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

    dimLightTo(values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);
    KoLED_BrightnessStatus_.value((values[0]+values[1]+values[2]+values[3])/4, DPT_DecimalFactor);

  }
  else if (lightType == 2) { // RGB
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

    dimLightTo(values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);
    KoLED_BrightnessStatus_.value((values[0]+values[1]+values[2])/3, DPT_DecimalFactor);

  }
  else if (lightType == 3) { // TW
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

    dimLightTo(values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);
    KoLED_BrightnessStatus_.value((values[0]+values[1])/2, DPT_DecimalFactor);
  }
  else if (lightType == 4) { // single channel
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

    dimLightTo(values, ParamLED_LightDimmTime_);

    KoLED_StateOnOff_.value(stateOnOff, DPT_Switch);
    KoLED_SceneStatus_.value(sceneId, DPT_SceneNumber);
    KoLED_BrightnessStatus_.value(values[0], DPT_DecimalFactor);
  }
}

void LightChannel::dimLightTo(uint32_t targetBrightness[LEDMODULE_MAX_LIGHT_CHANNELS], uint16_t dimTime)
{
  for (uint8_t channelId = 0; channelId < LEDMODULE_MAX_LIGHT_CHANNELS; channelId++)
  {
      // Ignore not mapped hw channels
      if (hwChannels[channelId] == -1)
          continue;

      if ((targetBrightness[channelId] < 0) || (targetBrightness[channelId] > 255))
      {
          logErrorP("newValue %i is not between 0 and 255: %i", channelId, targetBrightness[channelId]);
          return;
      }
      pDimmer->dimToLevel(hwChannels[channelId], targetBrightness[channelId], dimTime);
  }
}
