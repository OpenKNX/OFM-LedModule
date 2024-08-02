#include <knx.h>
#include "RGBChannel.h"


RGBChannel::RGBChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[3]) : LightChannel(index, pDimmer, hwChannels, 3), _hue(0), _saturation(0)
{

}

const std::string RGBChannel::name()
{
  return "RGBChannel";
}

void RGBChannel::update()
{ 
  uint8_t tmpHue = _hue.value();
  uint8_t tmpSat = _saturation.value();
  uint8_t tmpBright = _brightness.value();
  Colors::HSV hsv(tmpHue, tmpSat, tmpBright);

  if(_lastBrightnessLevel != tmpBright)
  {
    logDebugP("update: Br: %d -> %d", _lastBrightnessLevel, tmpBright);
    _lastBrightnessLevel = tmpBright;
    KoLED_RGB_BrightnessStatus_.value(tmpBright, DPT_Percent_U8);
    KoLED_RGB_StateOnOff_.value(tmpBright > 0, DPT_State);
    KoLED_RGB_HSVStatus_.value(hsv.toUint32(), DPT_Colour_RGB);
    KoLED_RGB_RGBStatus_.value(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
  }
  else if(_lastHueValue != tmpHue || _lastSatValue != tmpSat)
  {
    logDebugP("update: Hue: %d -> %d Sat: %d -> %d", _lastHueValue, tmpHue, _lastSatValue, tmpSat);
    _lastHueValue = tmpHue;
    _lastSatValue = tmpSat;
    KoLED_RGB_HSVStatus_.value(hsv.toUint32(), DPT_Colour_RGB);
    KoLED_RGB_RGBStatus_.value(Colors::hsv2rgb(hsv).toUint32(), DPT_Colour_RGB);
  }
}

void RGBChannel::loop()
{
  LightChannel::loop();

  if(delayCheckMillis(_lastDimTimestamp, DIMLOOP_DELAY))
  {
    _lastDimTimestamp = millis();
    
    Colors::RGB rgb = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), _brightness.step(_lastDimTimestamp)));
    // logDebugP("R: %d G: %d B: %d", rgb.red, rgb.green, rgb.blue);

    if(_pHWChannels[0] < LED_ChannelCount)
    {
        _pDimmer->setLevel(_pDimmer->scale(rgb.red, HWDimmer::DimLUTType::Log1_5), _pHWChannels[0]);
    }
    if(_pHWChannels[1] < LED_ChannelCount)
    {
        _pDimmer->setLevel(_pDimmer->scale(rgb.green, HWDimmer::DimLUTType::Log1_5), _pHWChannels[1]);
    }
    if(_pHWChannels[2] < LED_ChannelCount)
    {
        _pDimmer->setLevel(_pDimmer->scale(rgb.blue, HWDimmer::DimLUTType::Log1_5), _pHWChannels[2]);
    }
  }
}

void RGBChannel::processInputKo(GroupObject& ko)
{
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());

  uint32_t LED_RGB = 0;
  uint32_t LED_HSV = 0;
  Colors::HSV hsv;
  Colors::RGB rgb;
  uint8_t tmpu8=0;
  int16_t relKO = (ko.asap() - LED_RGB_KoOffset);
  // check if channel is valid
  if ((int8_t)(relKO / LED_RGB_KoBlockSize) == channelIndex())
      relKO = relKO % LED_RGB_KoBlockSize;
  else
      relKO = -1;

  if(relKO == LED_RGB_KoLocking_)
  {
    _isLocked = ko.value(DPT_Switch);
  }
  else if(!_isLocked)
  {
    switch (relKO)
    {
      case LED_RGB_KoSwitch_: 
      if(ko.value(DPT_Switch))
        {
          tmpu8 = KoLED_RGB_Brightness_.value(DPT_DecimalFactor);
          _brightness.setTargetValue(tmpu8 > 0 ? tmpu8 : 255, millis(), ParamLED_RGB_LightDimmTime_);
        }
        else
        {
          _brightness.setTargetValue(0, millis(), ParamLED_RGB_LightDimmTime_);
        }
      break;

      case LED_RGB_KoStateOnOff_: break;
      case LED_RGB_KoLocking_: break;

      case LED_RGB_KoBrightness_: 
        _brightness.setTargetValue(ko.value(DPT_DecimalFactor), millis(), ParamLED_RGB_LightDimmTime_);
      break;

      case LED_RGB_KoBrightnessStatus_: break;
      case LED_RGB_KoDimRel_: break;
      case LED_RGB_KoScene_: break;
      case LED_RGB_KoSceneStatus_: break;

      case LED_RGB_KoRGB_:
        LED_RGB = ko.value(DPT_Colour_RGB);
        hsv = Colors::rgb2hsv(Colors::RGB(LED_RGB));
        logDebugP("H: %d, S: %d V: %d", hsv.hue>>2, hsv.sat, hsv.val);
        rgb = Colors::hsv2rgb(hsv);
        logDebugP("R: %02X, G: %02X B: %02X", rgb.red, rgb.green, rgb.blue);

        _hue.setTargetValue(hsv.hue >> 2, millis(), ParamLED_RGB_LightDimmTime_);
        _saturation.setTargetValue(hsv.sat, millis(), ParamLED_RGB_LightDimmTime_);
        _brightness.setTargetValue(hsv.val, millis(), ParamLED_RGB_LightDimmTime_);
        break;

      case LED_RGB_KoHSV_: 
      
        LED_HSV = ko.value(DPT_Colour_RGB);
        hsv = Colors::HSV(LED_HSV);
        logDebugP("H: %d, S: %d V: %d", hsv.hue>>2, hsv.sat, hsv.val);
        rgb = Colors::hsv2rgb(hsv);
        logDebugP("R: %02X, G: %02X B: %02X", rgb.red, rgb.green, rgb.blue);

        _hue.setTargetValue(hsv.hue >> 2, millis(), ParamLED_RGB_LightDimmTime_);
        _saturation.setTargetValue(hsv.sat, millis(), ParamLED_RGB_LightDimmTime_);
        _brightness.setTargetValue(hsv.val, millis(), ParamLED_RGB_LightDimmTime_);
      break;

      default:
        logDebugP("Unknown KO %d", relKO);
        break;
    }
  }
}
