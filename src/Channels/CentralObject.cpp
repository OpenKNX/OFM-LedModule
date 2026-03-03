#include "CentralObject.h"
#include "../LedModule.h"
#include "LightChannel.h"
#include <knx.h>

COChannel::COChannel(uint8_t index, HWDimmer *pDimmer, uint8_t hwChannels[3])
    : LightChannel(index, pDimmer, hwChannels, 3), _hue(0, 0, H_PART - 1),
      _saturation(0, 0, VAL_RANGE) {}

const std::string COChannel::name() { return "COChannel"; }

void COChannel::update() {}

void COChannel::loop() {
  if (!_channelActive)
    return;
}

void COChannel::processInputKo(GroupObject &ko) {
  logDebugP("processInputKo Channel CO %d", ((ko.asap()- LED_CO_KoBlockOffset)/ LED_CO_KoBlockSize)   );
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
  else if (!_isLocked) {
    switch (relKO) {
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
        setBrightness(
            (u_int16_t)((u_int16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
      break;

    case LED_CO_KoChBrightnessRel:
      if (!getLock()) {
        int16_t tmpu16;
        tmpu16 = *KoLED_CO_ChBrightnessRel.valueRef();

        if (tmpu16 >= 0x09)
          relDimUp();
        if (tmpu16 > 0x00 && tmpu16 < 0x08)
          relDimDown();
        if (tmpu16 == 0x00 || tmpu16 == 0x08)
          relDimStop();
      }
      break;

    case LED_CO_KoChScene:
      if (!getLock()) {
        handleScene(ko.value(DPT_SceneNumber));
        _sceneNumberActive = (uint8_t)ko.value(DPT_SceneNumber) + 1;
      }
      break;

    case LED_CO_KoChColorTemperature:
      if (!getLock())
        setColorTemperature(ko.value(Dpt(7, 600)));
      break;

    case LED_CO_KoChColorTemperatureRel:
      if (!getLock()) {
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

void COChannel::handleScene(uint8_t sceneNr) {
  /*
  logDebugP("Scene: %d", sceneNr);
  for (int i = 0; i < N_SCENES; i++) {
    if (sceneNr == _scenes[i].sceneNr - 1) {
      logDebugP("Typ: %d,%d", _scenes[i].funcType, _scenes[i].valueType);
      switch (_scenes[i].funcType) {
      default:
      case SceneConfig::FuncType::INACTIVE:
        break;

      case SceneConfig::FuncType::VALUE:
        if (_scenes[i].valueType == ValueType::BRIGHTNESS)
          _brightness.setTargetValue(
              checkMinMaxBrightness(_scenes[i].Brightness() *
                                    VALUE_KNX_MULTIPLY),
              dimmingTime(1));

        if (_scenes[i].valueType == ValueType::COLOR) {
          Colors::HSV tmpHSV = _scenes[i].HSV();
          Colors::RGB tmpRGB = Colors::hsv2rgb(tmpHSV);
          logDebugP("IN: %d, %d, %d COLOR: %d, %d, %d RGB: %d, %d, %d",
                    _scenes[i].value[0], _scenes[i].value[1],
                    _scenes[i].value[2], tmpHSV.Hue(), tmpHSV.Sat(),
                    tmpHSV.Val(), tmpRGB._red, tmpRGB._green, tmpRGB._blue);
          _hue.setTargetValue(tmpHSV._hue, dimmingTime(1));
          _brightness.setTargetValue(checkMinMaxBrightness(tmpHSV.Val()),
                                     dimmingTime(1));
          _saturation.setTargetValue(tmpHSV._sat, dimmingTime(1));
        }

        if (_scenes[i].valueType == ValueType::HUE)
          _hue.setTargetValue(_scenes[i].value[0], dimmingTime(1));

        if (_scenes[i].valueType == ValueType::SATURATION)
          _saturation.setTargetValue(_scenes[i].value[2], dimmingTime(1));

        break;

      case SceneConfig::FuncType::FUNCTION:
        break;
      case SceneConfig::FuncType::SEQUENCE:
        break;
      case SceneConfig::FuncType::LOCKING:
        break;
      }
    }
  }
  */
}

void COChannel::setSwitch(bool switchOn) {

  for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
    if (ch < LED_SC_ChannelCount ) {
      //openknxLedModule._singleChannels[ch]->setSwitch(switchOn);
      logDebugP("CO setSwitch SC: %d > %d _____ %d : %d : %d", ch, openknxLedModule._singleChannels[ch]->isActive(), ParamLED_CH_CO1, ParamLED_CH_CO2, ParamLED_CH_CO3);
    }
    if (ch < LED_TW_ChannelCount ) {
      //openknxLedModule._twChannels[ch]->setSwitch(switchOn);
      logDebugP("CO setSwitch TW: %d > %d _____ %d : %d : %d", ch, openknxLedModule._twChannels[ch]->isActive(), ParamLED_CH_CO1, ParamLED_CH_CO2, ParamLED_CH_CO3);
    }
    if (ch < LED_RGB_ChannelCount ) {
      //openknxLedModule._rgbChannels[ch]->setSwitch(switchOn);
      logDebugP("CO setSwitch RGB: %d > %d _____ %d : %d : %d", ch, openknxLedModule._rgbChannels[ch]->isActive(), ParamLED_CH_CO1, ParamLED_CH_CO2, ParamLED_CH_CO3);
    }
    if (ch < LED_RGBW_ChannelCount ) {
      //openknxLedModule._rgbwChannels[ch]->setSwitch(switchOn);
      logDebugP("CO setSwitch RGBW: %d > %d _____ %d : %d : %d", ch, openknxLedModule._rgbwChannels[ch]->isActive(), ParamLED_CH_CO1, ParamLED_CH_CO2, ParamLED_CH_CO3);
    }
    if (ch < LED_RGBTW_ChannelCount && openknxLedModule._rgbtwChannels[ch]->isActive()) {
      //openknxLedModule._rgbtwChannels[ch]->setSwitch(switchOn);
      logDebugP("CO setSwitch RGBTW: %d > %d", ch, openknxLedModule._rgbtwChannels[ch]->isActive());
    }
  }



}

void COChannel::setSwitchNoDim(bool switchOn) {
  for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
    if (ch < LED_SC_ChannelCount) {
      openknxLedModule._singleChannels[ch]->setSwitchNoDim(switchOn);
    }
    if (ch < LED_TW_ChannelCount) {
      openknxLedModule._twChannels[ch]->setSwitch(switchOn);
    }
    if (ch < LED_RGB_ChannelCount) {
      openknxLedModule._rgbChannels[ch]->setSwitchNoDim(switchOn);
    }
    if (ch < LED_RGBW_ChannelCount) {
      openknxLedModule._rgbwChannels[ch]->setSwitchNoDim(switchOn);
    }
    if (ch < LED_RGBTW_ChannelCount) {
      openknxLedModule._rgbtwChannels[ch]->setSwitchNoDim(switchOn);
    }
  }
}

  void COChannel::setHue(uint16_t hue) {
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
      if (ch < LED_SC_ChannelCount) {
        //openknxLedModule._singleChannels[ch]->setHue(hue);
      }
      if (ch < LED_TW_ChannelCount) {
        //openknxLedModule._twChannels[ch]->setHue(hue);
      }
      if (ch < LED_RGB_ChannelCount) {
        openknxLedModule._rgbChannels[ch]->setHue(hue);
      }
      if (ch < LED_RGBW_ChannelCount) {
        openknxLedModule._rgbwChannels[ch]->setHue(hue);
      }
      if (ch < LED_RGBTW_ChannelCount) {
        openknxLedModule._rgbtwChannels[ch]->setHue(hue);
      }
    }
  }

  void COChannel::setSaturation(uint16_t saturation) {
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
      if (ch < LED_SC_ChannelCount) {
        //openknxLedModule._singleChannels[ch]->setSaturation(saturation);
      }
      if (ch < LED_TW_ChannelCount) {
        //openknxLedModule._twChannels[ch]->setSaturation(saturation);
      }
      if (ch < LED_RGB_ChannelCount) {
        openknxLedModule._rgbChannels[ch]->setSaturation(saturation);
      }
      if (ch < LED_RGBW_ChannelCount) {
        openknxLedModule._rgbwChannels[ch]->setSaturation(saturation);
      }
      if (ch < LED_RGBTW_ChannelCount) {
        openknxLedModule._rgbtwChannels[ch]->setSaturation(saturation);
      }
    }
  }

  void COChannel::setBrightness(uint16_t bright) {
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
      if (ch < LED_SC_ChannelCount) {
        openknxLedModule._singleChannels[ch]->setBrightness(bright);
      }
      if (ch < LED_TW_ChannelCount) {
        openknxLedModule._twChannels[ch]->setBrightness(bright);
      }
      if (ch < LED_RGB_ChannelCount) {
        openknxLedModule._rgbChannels[ch]->setBrightness(bright);
      }
      if (ch < LED_RGBW_ChannelCount) {
        openknxLedModule._rgbwChannels[ch]->setBrightness(bright);
      }
      if (ch < LED_RGBTW_ChannelCount) {
        openknxLedModule._rgbtwChannels[ch]->setBrightness(bright);
      }
    }
  }

  void COChannel::setNight(bool night) {
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
      if (ch < LED_SC_ChannelCount) {
        openknxLedModule._singleChannels[ch]->setNight(night);
      }
      if (ch < LED_TW_ChannelCount) {
        openknxLedModule._twChannels[ch]->setNight(night);
      }
      if (ch < LED_RGB_ChannelCount) {
        openknxLedModule._rgbChannels[ch]->setNight(night);
      }
      if (ch < LED_RGBW_ChannelCount) {
        openknxLedModule._rgbwChannels[ch]->setNight(night);
      }
      if (ch < LED_RGBTW_ChannelCount) {
        openknxLedModule._rgbtwChannels[ch]->setNight(night);
      }
    }
  }

  void COChannel::relDimUp() {
    /*_sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMax(),
    ParamLED_RGB_ChLightDimmRelTime);*/
  }

  void COChannel::relDimDown() {
    /*_sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMin(),
    ParamLED_RGB_ChLightDimmRelTime);*/
  }

  void COChannel::relDimStop() {
    /*_sceneNumberActive = 0;
    _brightness.setTargetValue(_brightness.value(), 1);*/
  }

  void COChannel::setColorTemperature(uint16_t colorTemp) {
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
      if (ch < LED_SC_ChannelCount) {
        //openknxLedModule._singleChannels[ch]->setColorTemperature(colorTemp);
      }
      if (ch < LED_TW_ChannelCount) {
        openknxLedModule._twChannels[ch]->setColorTemperature(colorTemp);
      }
      if (ch < LED_RGB_ChannelCount) {
        openknxLedModule._rgbChannels[ch]->setColorTemperature(colorTemp);
      }
      if (ch < LED_RGBW_ChannelCount) {
        openknxLedModule._rgbwChannels[ch]->setColorTemperature(colorTemp);
      }
      if (ch < LED_RGBTW_ChannelCount) {
        openknxLedModule._rgbtwChannels[ch]->setColorTemperature(colorTemp);
      }
    }
  }

  void COChannel::relDimUpColor() {
    /*//_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_UP");
    setColorTemperature(10000);*/
  }

  void COChannel::relDimDownColor() {
    /*//_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_DOWN");
    setColorTemperature(1000);  */
  }

  void COChannel::relDimStopColor() {
    /*//_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_STOP");
    // setColorTemperature(_colorTemperature.value(), 1);*/
  }

  void COChannel::setRGB(uint32_t RGBvalue) {
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
      if (ch < LED_SC_ChannelCount) {
        //openknxLedModule._singleChannels[ch]->setRGB(RGBvalue);
      }
      if (ch < LED_TW_ChannelCount) {
        //openknxLedModule._twChannels[ch]->setRGB(RGBvalue);
      }
      if (ch < LED_RGB_ChannelCount) {
        openknxLedModule._rgbChannels[ch]->setRGB(RGBvalue);
      }
      if (ch < LED_RGBW_ChannelCount) {
        openknxLedModule._rgbwChannels[ch]->setRGB(RGBvalue);
      }
      if (ch < LED_RGBTW_ChannelCount) {
        openknxLedModule._rgbtwChannels[ch]->setRGB(RGBvalue);
      }
    }
  }

  void COChannel::RGBpicker(uint8_t selection) {}

  void COChannel::setHSV(uint32_t HSVvalue) {
    for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
      if (ch < LED_SC_ChannelCount) {
        //openknxLedModule._singleChannels[ch]->setHSV(HSVvalue);
      }
      if (ch < LED_TW_ChannelCount) {
        //openknxLedModule._twChannels[ch]->setHSV(HSVvalue);
      }
      if (ch < LED_RGB_ChannelCount) {
        openknxLedModule._rgbChannels[ch]->setHSV(HSVvalue);
      }
      if (ch < LED_RGBW_ChannelCount) {
        openknxLedModule._rgbwChannels[ch]->setHSV(HSVvalue);
      }
      if (ch < LED_RGBTW_ChannelCount) {
        openknxLedModule._rgbtwChannels[ch]->setHSV(HSVvalue);
      }
    }
  }
  uint32_t COChannel::conv_Temp2RGB(int temp) { return 0; }

  // EOF