#pragma once

#include <Arduino.h>
#include "OpenKNX.h"


class Colors
{
    public:
    static const std::string logPrefix() { return "Color";}
    struct RGB
    {
        RGB()
        {
            red = green = blue = 0;
        }
        
        RGB(uint8_t r, uint8_t g, uint8_t b)
        {
            red = r;
            green = g;
            blue = b;
        }
        
        RGB(uint32_t rgbval)
        {
            red = (rgbval >> 16) & 0xFF;
            green = (rgbval >> 8) & 0xFF;
            blue = rgbval & 0xFF;
        }
        
        uint32_t toUint32()
        {
            return (uint32_t) red << 16 | green << 8 | blue;
        }

        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };
    
    struct HSV
    {
        HSV()
        {
            hue = sat = val = 0;
        }
        
        HSV(uint16_t h, uint8_t s, uint8_t v)
        {
            hue = h;
            sat = s;
            val = v;
        }

        HSV(uint32_t hsv_u32)
        {
            hue = ((hsv_u32 >> 16) & 0xFF) << 2;
            sat = ((hsv_u32 >> 8) & 0xFF);
            val = (hsv_u32 & 0xFF);
        }

        uint32_t toUint32()
        {
            return (uint32_t) hue << 14 | sat << 8 | val;
        }

        uint16_t hue;
        uint8_t sat;
        uint8_t val;
    };

    static RGB hsv2rgb(HSV hsv);
    static HSV rgb2hsv(RGB rgb);

};