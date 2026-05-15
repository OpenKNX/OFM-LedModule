#pragma once

#include "OpenKNX.h"
#include <Arduino.h>

#define _UFP16(val, n) (((uint16_t)val) << n)
#define _FP16(val, n) (((int16_t)val) << n)

#define _UFP16_TO_U8(val, n) ((val + ((1 << n) >> 1)) >> n)

#define H_PART ((int32_t)(4096 * 4))
// #define VAL_RANGE ((uint32_t) (255 << 2))
#define VAL_RANGE ((uint32_t)0x3FF)

class Colors
{
  public:
    static const std::string logPrefix() { return "Color"; }
    struct RGB
    {
        RGB()
        {
            _red = _green = _blue = 0;
        }

        RGB(uint16_t r, uint16_t g, uint16_t b)
        {
            _red = r;
            _green = g;
            _blue = b;
        }

        RGB(uint32_t rgbval)
        {
            //_red   = _UFP16((rgbval >> 16) & 0xFF, 4);
            _red = map(((rgbval >> 16) & 0xFF), 0, 255, 0, 4095);
            //_green = _UFP16((rgbval >> 8)  & 0xFF, 4);
            _green = map(((rgbval >> 8) & 0xFF), 0, 255, 0, 4095);
            //_blue  = _UFP16( rgbval        & 0xFF, 4);
            _blue = map((rgbval & 0xFF), 0, 255, 0, 4095);
        }

        uint32_t toUint32()
        {
            return (uint32_t)map(Red(), 0, 4095, 0, 255) << 16 | map(Green(), 0, 4095, 0, 255) << 8 | map(Blue(), 0, 4095, 0, 255);
        }

        // uint16_t Red() { return _UFP16_TO_U8(_red, 2); }
        uint16_t Red() { return _red; }

        // uint16_t Green() { return _UFP16_TO_U8(_red, 2); }
        uint16_t Green() { return _green; }

        // uint16_t Blue() { return _UFP16_TO_U8(_blue, 2); }
        uint16_t Blue() { return _blue; }

        uint16_t WarmWhite() { return _red; }
        uint16_t ColdWhite() { return _green; }
        uint16_t White() { return _red; }

        uint16_t _red;
        uint16_t _green;
        uint16_t _blue;
    };

    struct HSV
    {
        HSV()
        {
            _hue = _sat = _val = 0;
        }

        HSV(uint16_t h, uint16_t s, uint16_t v)
        {
            _hue = h;
            _sat = s;
            _val = v;
        }

        HSV(uint32_t hsv_u32)
        {
            _hue = _UFP16((hsv_u32 >> 16) & 0xFF, 6);
            //_sat = _UFP16((hsv_u32 >> 8) & 0xFF, 2);
            _sat = map(((hsv_u32 >> 8) & 0xFF), 0, 255, 0, 1023);
            //_val = _UFP16(hsv_u32 & 0xFF, 2);
            //_val = _UFP16(hsv_u32 & 0xFF, 4);
            _val = map((hsv_u32 & 0xFF), 0, 255, 0, 4095);
        }

        uint32_t toUint32()
        {
            return (uint32_t)Hue() << 16 | Sat() << 8 | Val();
        }

        uint16_t Hue() { return _UFP16_TO_U8(_hue, 6); }

        // uint16_t Sat() { return _UFP16_TO_U8(_sat, 2); }
        uint16_t Sat() { return map(_sat, 0, 1023, 0, 255); }

        // uint16_t Val() { return _UFP16_TO_U8(_val, 2); }
        // uint16_t Val() { return _UFP16_TO_U8(_val, 4); }
        uint16_t Val() { return map(_val, 0, 4095, 0, 255); }

        uint16_t _hue;
        uint16_t _sat;
        uint16_t _val;
    };

    static RGB hsv2rgb(HSV hsv);
    static HSV rgb2hsv(RGB rgb);
};