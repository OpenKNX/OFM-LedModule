#include <knx.h>
#include "RGBChannel.h"


RGBChannel::RGBChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[3]) : LightChannel(index, pDimmer, hwChannels, 3), _hue(0, 0, H_PART - 1), _saturation(0, 0, VAL_RANGE)
{
  logInfoP("Trying to read Config from KNX...");
  logHexInfoP((uint8_t*) knx.paramData(LED_RGB_ParamCalcIndex(LED_RGB_SceneA_Type_)), 8);
  _scenes = new SceneConfig[N_SCENES];
  memcpy(_scenes, knx.paramData(LED_RGB_ParamCalcIndex(LED_RGB_SceneA_Type_)), N_SCENES * sizeof(SceneConfig));
  
#ifdef EXT_DEBUG_LOG
  logDebugP("Idx\tScNr\tFUNC\tVAL\tLkObj\tLkFnc\tFix\tval0\tval1\tval2");
  for(int i = 0; i < N_SCENES; i++)
  {
    logDebugP("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",_channelIndex, _scenes[i].sceneNr,_scenes[i].funcType, _scenes[i].valueType, _scenes[i].lockObj, _scenes[i].lockFunc, _scenes[i].isFixed,   _scenes[i].value[0],  _scenes[i].value[1], _scenes[i].value[2]);
  }
#endif
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
    
    Colors::RGB rgb = Colors::hsv2rgb(Colors::HSV(_hue.step(_lastDimTimestamp), _saturation.step(_lastDimTimestamp), ((uint16_t) _brightness.step(_lastDimTimestamp)) <<2));

    if(_pHWChannels[0] < LED_ChannelCount)
    {
        _pDimmer->setLevel(_pDimmer->scale(rgb.Red(), HWDimmer::DimLUTType::Log1_5), _pHWChannels[0]);
    }
    if(_pHWChannels[1] < LED_ChannelCount)
    {
        _pDimmer->setLevel(_pDimmer->scale(rgb.Green(), HWDimmer::DimLUTType::Log1_5), _pHWChannels[1]);
    }
    if(_pHWChannels[2] < LED_ChannelCount)
    {
        _pDimmer->setLevel(_pDimmer->scale(rgb.Blue(), HWDimmer::DimLUTType::Log1_5), _pHWChannels[2]);
    }
  }
}

void RGBChannel::processInputKo(GroupObject& ko)
{
  uint32_t LED_RGB = 0;
  uint32_t LED_HSV = 0;
  Colors::HSV hsv;
  Colors::RGB rgb;
  uint8_t tmpu8 = 0;
  int16_t relKO = (ko.asap() - LED_RGB_KoOffset);
  
  logDebugP("processInputKo Channel");
  logHexDebugP(ko.valueRef(), ko.valueSize());

  // check if channel is valid
  if ((int8_t)(relKO / LED_RGB_KoBlockSize) == channelIndex())
  {
    relKO = relKO % LED_RGB_KoBlockSize;
  }
  else
  {
    relKO = -1;
  }

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
          _brightness.setTargetValue(tmpu8 > 0 ? tmpu8 : BRIGHTNESS_MAX, millis(), ParamLED_RGB_LightDimmTime_);
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

      case LED_RGB_KoScene_: 
        handleScene(ko.value(DPT_SceneNumber));
      break;
      case LED_RGB_KoSceneStatus_: break;

      case LED_RGB_KoRGB_:
        LED_RGB = ko.value(DPT_Colour_RGB);
        rgb = Colors::RGB(LED_RGB);
        logDebugP("R: %03X, G: %03X B: %03X", rgb._red, rgb._green, rgb._blue);
        hsv = Colors::rgb2hsv(rgb);
        logDebugP("H: %d, S: %d V: %d", hsv.Hue(), hsv.Sat(), hsv.Val());

        _hue.setTargetValue(hsv._hue, millis(), ParamLED_RGB_LightDimmTime_);
        _saturation.setTargetValue(hsv._sat, millis(), ParamLED_RGB_LightDimmTime_);
        _brightness.setTargetValue(hsv.Val(), millis(), ParamLED_RGB_LightDimmTime_);
        break;

      case LED_RGB_KoRGBStatus_:
      break;

      case LED_RGB_KoHSV_: 
        LED_HSV = ko.value(DPT_Colour_RGB);
        hsv = Colors::HSV(LED_HSV);
        logDebugP("H: %d, S: %d V: %d", hsv._hue>>2, hsv._sat, hsv._val);
        rgb = Colors::hsv2rgb(hsv);
        logDebugP("R: %02X, G: %02X B: %02X", rgb._red, rgb._green, rgb._blue);

        _hue.setTargetValue(hsv._hue, millis(), ParamLED_RGB_LightDimmTime_);
        _saturation.setTargetValue(hsv._sat, millis(), ParamLED_RGB_LightDimmTime_);
        _brightness.setTargetValue(hsv.Val(), millis(), ParamLED_RGB_LightDimmTime_);
      break;
      
      case LED_RGB_KoHSVStatus_:
      break; 

      default:
        logDebugP("Unknown KO %d", relKO);
        break;
    }
  }
}

void RGBChannel::handleScene(uint8_t sceneNr)
{
  logDebugP("Scene: %d", sceneNr);
  for(int i = 0; i < N_SCENES; i++)
  {
    if(sceneNr+1 == _scenes[i].sceneNr)
    {
      logDebugP("Typ: %d,%d", _scenes[i].funcType,_scenes[i].valueType);
      switch(_scenes[i].funcType)
      {
        default:
        case SceneConfig::FuncType::INACTIVE: break;

        case SceneConfig::FuncType::VALUE: 
          if(_scenes[i].valueType == ValueType::BRIGHTNESS)
          {
            _brightness.setTargetValue(_scenes[i].Brightness(), millis(), ParamLED_RGB_LightDimmTime_);
          }
          if(_scenes[i].valueType == ValueType::COLOR)
          {
            Colors::HSV tmpHSV = _scenes[i].HSV();
            Colors::RGB tmpRGB = Colors::hsv2rgb(tmpHSV);
            logDebugP("IN: %d, %d, %d COLOR: %d, %d, %d RGB: %d, %d, %d", _scenes[i].value[0],_scenes[i].value[1],_scenes[i].value[2], tmpHSV.Hue(), tmpHSV.Sat(), tmpHSV.Val(), tmpRGB._red, tmpRGB._green, tmpRGB._blue);
            _hue.setTargetValue(tmpHSV._hue, millis(), ParamLED_RGB_LightDimmTime_);
            _brightness.setTargetValue(tmpHSV.Val(), millis(), ParamLED_RGB_LightDimmTime_);
            _saturation.setTargetValue(tmpHSV._sat, millis(), ParamLED_RGB_LightDimmTime_);
          }
          if(_scenes[i].valueType == ValueType::HUE)
          {
            _hue.setTargetValue(_scenes[i].value[0], millis(), ParamLED_RGB_LightDimmTime_);
          }
          if(_scenes[i].valueType == ValueType::SATURATION)
          {
            _saturation.setTargetValue(_scenes[i].value[2], millis(), ParamLED_RGB_LightDimmTime_);
          }
        break;

        case SceneConfig::FuncType::FUNCTION: break;
        case SceneConfig::FuncType::SEQUENCE: break;
        case SceneConfig::FuncType::LOCKING: break;
      }
    }
  }
}