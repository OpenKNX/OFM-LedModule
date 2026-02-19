#include "LightChannel.h"
#include "CentralObject.h"
#include <knx.h>

COChannel::COChannel(uint8_t index, HWDimmer* pDimmer, uint8_t hwChannels[3])
    : LightChannel(index, pDimmer, hwChannels, 3), _hue(0, 0, H_PART - 1), _saturation(0, 0, VAL_RANGE)
{
}

const std::string COChannel::name()
{
    return "COChannel";
}

void COChannel::update()
{
  
}

void COChannel::loop()
{
    if (!_channelActive)
        return;

    
}

void COChannel::processInputKo(GroupObject& ko)
{
    Colors::HSV hsv;
    Colors::RGB rgb;
    int16_t relKO = (ko.asap() - LED_CO_KoOffset);

    logDebugP("processInputKo Channel");
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
                //KoLED_CO_ChStateLocking.value(getLock(), DPT_Switch);
                break;

            case LED_CO_KoChBrightness:
                if (!getLock())
                    setBrightness((u_int16_t)((u_int16_t)ko.value(DPT_Scaling) * VALUE_KNX_MULTIPLY));
                break;

            case LED_CO_KoChBrightnessRel:
                if (!getLock())
                {
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

            //case LED_CO_KoChStateOnOff:
            //case LED_CO_KoChStateLocking:
            //case LED_CO_KoChBrightnessStatus:
            //case LED_CO_KoChColorTemperatureStatus:
            //case LED_CO_KoChRGBStatus:
            //case LED_CO_KoChHSVStatus:
              

            default:
                logDebugP("Unknown KO %d", relKO);
                break;
        }
    }
}

void COChannel::handleScene(uint8_t sceneNr)
{
    logDebugP("Scene: %d", sceneNr);
    for (int i = 0; i < N_SCENES; i++)
    {
        if (sceneNr == _scenes[i].sceneNr - 1)
        {
            logDebugP("Typ: %d,%d", _scenes[i].funcType, _scenes[i].valueType);
            switch (_scenes[i].funcType)
            {
                default:
                case SceneConfig::FuncType::INACTIVE:
                    break;

                case SceneConfig::FuncType::VALUE:
                    if (_scenes[i].valueType == ValueType::BRIGHTNESS)
                        _brightness.setTargetValue(checkMinMaxBrightness(_scenes[i].Brightness() * VALUE_KNX_MULTIPLY), dimmingTime(1));

                    if (_scenes[i].valueType == ValueType::COLOR)
                    {
                        Colors::HSV tmpHSV = _scenes[i].HSV();
                        Colors::RGB tmpRGB = Colors::hsv2rgb(tmpHSV);
                        logDebugP("IN: %d, %d, %d COLOR: %d, %d, %d RGB: %d, %d, %d", _scenes[i].value[0], _scenes[i].value[1], _scenes[i].value[2], tmpHSV.Hue(), tmpHSV.Sat(), tmpHSV.Val(), tmpRGB._red, tmpRGB._green, tmpRGB._blue);
                        _hue.setTargetValue(tmpHSV._hue, dimmingTime(1));
                        _brightness.setTargetValue(checkMinMaxBrightness(tmpHSV.Val()), dimmingTime(1));
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
}



void COChannel::setSwitch(bool switchOn)
{

//_singleChannels[0]->setSwitch(switchOn);


  for (uint8_t ch = 0; ch < LED_ChannelCount; ch++) {
    if (ch < LED_SC_ChannelCount)
      _singleChannels[ch]->setSwitch(switchOn);
    if (ch < LED_TW_ChannelCount)
      _twChannels[ch]->setSwitch(switchOn);
    if (ch < LED_RGB_ChannelCount)
      _rgbChannels[ch]->setSwitch(switchOn);
    /*if (ch < LED_RGBW_ChannelCount)
      _rgbwChannels[ch]->setSwitch(switchOn);
    if (ch < LED_RGBTW_ChannelCount)
      _rgbtwChannels[ch]->setSwitch(switchOn);*/
  }


/*
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switch_ON");
        _brightness.setTargetValue(ParamLED_RGB_ChBrightnessMin * VALUE_KNX_MULTIPLY, 1);
        // in case of stairway light
        if (ParamLED_RGB_ChStairCaseActive && ParamLED_RGB_ChStairCaseTrigger == 0)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        setStartupColor();

        _brightness.setTargetValue(dimmingValTarget(switchOn), dimmingTime(switchOn));
    }
    else
    {
        logDebugP("switch_OFF");
        // in case of stairway light
        if (ParamLED_RGB_ChStairCaseActive && ParamLED_RGB_ChStairCaseTrigger == 1)
        {
            setStairTime(delayTimerInit());
            setStairTrigger(1);
        }
        else
        {
            setLastOnValue(_brightness.value());
            setLastOnValueHue(_hue.value());
            setLastOnValueSat(_saturation.value());

            _brightness.setTargetValue(dimmingValTarget(switchOn), dimmingTime(switchOn));
        }
    }
    logDebugP("dimmingValTarget: %3X", dimmingValTarget(switchOn));
    logDebugP("dimmingTime: %5X", dimmingTime(switchOn));
    logDebugP("parammaxday: %5X", (ParamLED_RGB_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY));

    */
}

void COChannel::setSwitchNoDim(bool switchOn)
{/*
    _sceneNumberActive = 0;
    if (switchOn)
    {
        logDebugP("switchNoDim_ON");
        setStartupColor();
        _brightness.setTargetValue(dimmingValTarget(switchOn), 1);
    }
    else
    {
        logDebugP("switchNoDim_OFF");
        setLastOnValue(_brightness.value());
        setLastOnValueHue(_hue.value());
        setLastOnValueSat(_saturation.value());
        _brightness.setTargetValue(dimmingValTarget(switchOn), 1);
    }*/
}

void COChannel::setHue(uint16_t hue)
{
    /*_sceneNumberActive = 0;
    logDebugP("setHue: %3X", hue);
    // hue max 16384
    _hue.setTargetValue(hue, dimmingTimeON());*/
}

void COChannel::setSaturation(uint16_t saturation)
{
    /*_sceneNumberActive = 0;
    logDebugP("setSaturation: %3X", saturation);
    // saturation max 1024
    _saturation.setTargetValue(saturation, dimmingTimeON());*/
}

void COChannel::setBrightness(uint16_t bright)
{
    /*_sceneNumberActive = 0;
    logDebugP("setBrightness: %3X", bright);
    bright = checkMinMaxBrightness(bright);
    _brightness.setTargetValue(bright, dimmingTimeON());*/
}

void COChannel::setNight(bool night)
{
/*
    if (ParamLED_RGB_ChScenesDisableNightSw || (!ParamLED_RGB_ChScenesDisableNightSw && _sceneNumberActive == 0))
    {
        _isNight = night;
        _brightness.setRange(ParamLED_RGB_ChBrightnessMin, dimmingValMax());

        if (!getNight())
        {
            logDebugP("Tag");
            RGBpicker(getDefaultColor());
            if (_brightness.value() == ParamLED_RGB_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
                _brightness.setTargetValue(ParamLED_RGB_ChBrightnessMaxDay * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGB_ChLightDimmDayOnTime);
        }
        else
        {
            logDebugP("Nacht");
            RGBpicker(getDefaultColor());
            if (_brightness.value() > ParamLED_RGB_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY)
                _brightness.setTargetValue(ParamLED_RGB_ChBrightnessMaxNight * VALUE_KNX_MULTIPLY, 2 * ParamLED_RGB_ChLightDimmNightOnTime);
        }
    }
*/
}

void COChannel::relDimUp()
{
    /*_sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMax(), ParamLED_RGB_ChLightDimmRelTime);*/
}

void COChannel::relDimDown()
{
    /*_sceneNumberActive = 0;
    _brightness.setTargetValue(dimmingValMin(), ParamLED_RGB_ChLightDimmRelTime);*/
}

void COChannel::relDimStop()
{
    /*_sceneNumberActive = 0;
    _brightness.setTargetValue(_brightness.value(), 1);*/
}

void COChannel::setColorTemperature(uint16_t colorTemp)
{
    /*_sceneNumberActive = 0;
    colorTemp = checkMinMaxColorTemp(colorTemp);
    logDebugP("ColorTemp: %5X", colorTemp);
    logDebugP("ColorTemp RGB: %8X", conv_Temp2RGB(colorTemp));
    setRGB(conv_Temp2RGB(colorTemp));

    if (_brightness.value() > 0)
        setBrightness(_brightness.value());

    KoLED_RGB_ChColorTemperatureStatus.value(colorTemp, Dpt(7, 600));*/
}

void COChannel::relDimUpColor()
{
    /*//_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_UP");
    setColorTemperature(10000);*/
}

void COChannel::relDimDownColor()
{
    /*//_boost = false;
    _sceneNumberActive = 0;
    logDebugP("relDim_DOWN");
    setColorTemperature(1000);  */
}

void COChannel::relDimStopColor()
{
    /*//_boost = false;   
    _sceneNumberActive = 0;
    logDebugP("relDim_STOP");
    // setColorTemperature(_colorTemperature.value(), 1);*/
}

void COChannel::setRGB(uint32_t RGBvalue)
{
    /*_sceneNumberActive = 0;
    Colors::HSV hsv;
    Colors::RGB rgb;
    // LED_RGB = ko.value(DPT_Colour_RGB);
    logDebugP("raw: %06X", RGBvalue);
    rgb = Colors::RGB(RGBvalue);
    logDebugP("R: %05X, G: %05X B: %05X", rgb._red, rgb._green, rgb._blue);
    hsv = Colors::rgb2hsv(rgb);
    logDebugP("h: %d, s: %d v: %d", hsv._hue, hsv._sat, hsv._val);
    logDebugP("H: %d, S: %d V: %d", hsv.Hue(), hsv.Sat(), hsv.Val());

    setHue(hsv._hue);
    setSaturation(hsv._sat);
    // setBrightness(hsv.Val());
    setBrightness(hsv._val);

    //_hue.setTargetValue(hsv._hue, ParamLED_RGB_ChLightDimmDayOnTime);
    //_saturation.setTargetValue(hsv._sat, ParamLED_RGB_ChLightDimmDayOnTime);
    //_brightness.setTargetValue(hsv.Val(), ParamLED_RGB_ChLightDimmDayOnTime);

    logDebugP("HUE:%05X%", _hue.value());
    logDebugP("SAT:%05X%", _saturation.value());
    logDebugP("BRE:%05X%", _brightness.value());*/
}

void COChannel::RGBpicker(uint8_t selection)
{/*
    /*logDebugP("color selection:%3X%", selection);
    logDebugP("Color DAY: %3X", ParamLED_RGB_ChColorDay);
    logDebugP("Color NIGHT: %3X", ParamLED_RGB_ChColorNight);
    switch (selection)
    {
        case 1:
            /* color rot *
            logDebugP("color rot");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 2:
            /* color  orange*
            logDebugP("color orange");
            _hue.setTargetValue(1365, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 3:
            /* color  gelb*
            logDebugP("color gelb");
            _hue.setTargetValue(2730, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 4:
            /* color  gelbgrün*
            logDebugP("color gelbgrün");
            _hue.setTargetValue(4095, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 5:
            /* color  grün*
            logDebugP("color grün");
            _hue.setTargetValue(5460, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 6:
            /* color  aquamarin*
            logDebugP("color aquamarin");
            _hue.setTargetValue(6825, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 7:
            /* color  türkis*
            logDebugP("color türkis");
            _hue.setTargetValue(8190, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 8:
            /* color  mint*
            logDebugP("color mint");
            _hue.setTargetValue(9555, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 9:
            /* color  blau*
            logDebugP("color blau");
            _hue.setTargetValue(10920, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 10:
            /* color  lila*
            logDebugP("color lila");
            _hue.setTargetValue(12285, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 11:
            /* color  rosa*
            logDebugP("color rosa");
            _hue.setTargetValue(13650, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 12:
            /* color  violett*
            logDebugP("color violett");
            _hue.setTargetValue(15015, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 13:
            /* color  weiß*
            logDebugP("color weiß");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 14:
            /* color  random steady*
            logDebugP("color random steady");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 15:
            /* color  random changing*
            logDebugP("color random changing");
            _hue.setTargetValue(random(0x3FFF), ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 16:
            /* color  custom*
            logDebugP("color custom");
            _hue.setTargetValue(0, ParamLED_RGB_ChLightDimmDayOnTime);
            _saturation.setTargetValue(1024, ParamLED_RGB_ChLightDimmDayOnTime);
            break;
        case 17:
            /* color temperature *
            logDebugP("color temperature");
            /*
                  Colors::HSV hsv;
                  if ( !RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( conv_Temp2RGB(ParamLED_RGB_ChColorDay) );
                        _hue.setTargetValue( hsv._hue  , ParamLED_RGB_ChLightDimmDayOnTime);
                        _saturation.setTargetValue( hsv._sat  , ParamLED_RGB_ChLightDimmDayOnTime);
                        }
                  if (  RGB_night() )
                        {
                        hsv = Colors::rgb2hsv( conv_Temp2RGB(ParamLED_RGB_ChColorNight) );
                        _hue.setTargetValue( hsv._hue  , ParamLED_RGB_ChLightDimmTimeNightON);
                        _saturation.setTargetValue( hsv._sat  , ParamLED_RGB_ChLightDimmTimeNightON);
                        }
            *
            break;

        default:
            break;
    }*/
}

void COChannel::setHSV(uint32_t HSVvalue)
{
    /*_sceneNumberActive = 0;
    Colors::HSV hsv;
    Colors::RGB rgb;
    hsv = Colors::HSV(HSVvalue);
    logDebugP("H: %d, S: %d V: %d", hsv._hue >> 2, hsv._sat, hsv._val);
    rgb = Colors::hsv2rgb(hsv);
    logDebugP("R: %02X, G: %02X B: %02X", rgb._red, rgb._green, rgb._blue);

    setHue(hsv._hue);
    setSaturation(hsv._sat);
    // setBrightness(hsv.Val());
    setBrightness(hsv._val);

    //_hue.setTargetValue(hsv._hue, ParamLED_RGB_ChLightDimmDayOnTime);
    //_saturation.setTargetValue(hsv._sat, ParamLED_RGB_ChLightDimmDayOnTime);
    //_brightness.setTargetValue(hsv.Val(), ParamLED_RGB_ChLightDimmDayOnTime);*/
}

uint32_t COChannel::conv_Temp2RGB(int temp)
{
    /*uint8_t r, g, b = 0;
    if (temp > 10000)
        return 0x000000;

    // red
    // below 6500 = 255
    // above 6500 = temp^-1,02 * 1.000.000 + 125,9
    if (temp >= 6500)
        r = (uint8_t)round(pow((float)temp, -1.02) * 1000000.0 + 125.9);
    else
        r = 255;

    // green
    // below 6500 = 100 x ln(temp) - 623
    // above 6500 = temp^-1,06 * 1.000.000 +165
    if (temp <= 6500)
        g = 200.0 * log10((float)temp) - 508.0;
    if (temp >= 6500)
        g = (uint8_t)round(pow((float)temp, -1.06) * 1000000.0 + 164.16);

    // blue
    // below 2000 = 0
    // 2000 - 6500 = 200 x ln(temp)-1500
    // above 6500 = 255
    if (temp <= 1900)
        b = 0;
    if (temp >= 1900)
        b = (uint8_t)round(477.5 * log10(temp) - 1565.6);
    if (temp >= 6800)
        b = 255;

    return (uint32_t)r << 16 | g << 8 | b;
}

int COChannel::conv_RGB2Temp(uint32_t target_rgb)
{
    uint8_t r_target = (target_rgb >> 16) & 0xFF;
    uint8_t g_target = (target_rgb >> 8) & 0xFF;
    uint8_t b_target = target_rgb & 0xFF;

    int low = 1000;
    int high = 10000;
    int best_temp = low;
    uint32_t min_error = 0x300;

    // Rough binary search
    while (low <= high)
    {
        int mid = (low + high) / 2;
        uint32_t rgb_mid = conv_Temp2RGB(mid);
        uint8_t r_mid = (rgb_mid >> 16) & 0xFF;
        uint8_t g_mid = (rgb_mid >> 8) & 0xFF;
        uint8_t b_mid = rgb_mid & 0xFF;
        uint32_t error = (r_mid - r_target) * (r_mid - r_target) + (g_mid - g_target) * (g_mid - g_target) + (b_mid - b_target) * (b_mid - b_target);
        logDebugP("Temp: %d bestTemp:%d R:%d G:%d B:%d Error:%d Target R:%d G:%d B:%d RGB:%d", mid, best_temp, r_mid, g_mid, b_mid, error, r_target, g_target, b_target, target_rgb);
        if (error < min_error)
        {
            min_error = error;
            best_temp = mid;
        }

        // Simple heuristic: go towards larger RGB values

        if ((r_mid + g_mid + b_mid) < (r_target + g_target + b_target))
            low = mid + 10;
        else
            high = mid - 10;
    }

    // Fine adjustment in the range ±25K

    // int fine_low = (best_temp - 25 < 1000) ? 1000 : best_temp - 25;
    // int fine_high = (best_temp + 25 > 10000) ? 10000 : best_temp + 25;
    int fine_low = (best_temp - 1000 < 1000) ? 1000 : best_temp - 1000;
    int fine_high = (best_temp + 1000 > 10000) ? 10000 : best_temp + 1000;

    for (int temp = fine_low; temp <= fine_high; temp += 1)
    {
        uint32_t rgb = conv_Temp2RGB(temp);
        uint8_t r = (rgb >> 16) & 0xFF;
        uint8_t g = (rgb >> 8) & 0xFF;
        uint8_t b = rgb & 0xFF;
        uint32_t error = (r - r_target) * (r - r_target) + (g - g_target) * (g - g_target) + (b - b_target) * (b - b_target);

        if (error < min_error)
        {
            min_error = error;
            best_temp = temp;
        }
    }

    return best_temp;*/
}

// EOF