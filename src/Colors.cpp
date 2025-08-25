#include "Colors.h"

// Function based on https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.cpp GNU LGPL
Colors::RGB Colors::hsv2rgb(HSV hsv)
{
    uint32_t r=0, g=0, b=0;
    // Map HUE range to 6 times range of single RGB color. This simplifies the math needed for conversion.
    uint32_t hue = (((uint32_t) hsv._hue) * 6 * VAL_RANGE + H_PART / 2) >> 14; // div by max HUE

    if(hsv._sat != 0)
    {
        if (hue < 2 * VAL_RANGE) { // Red to Green-1
            b = 0;
            if (hue < VAL_RANGE) { //   Red to Yellow-1
                r = VAL_RANGE;
                g = hue;
            } else {         //   Yellow to Green-1
                r = (2 * VAL_RANGE) - hue;
                g = VAL_RANGE;
            }
        } else if (hue < 4 * VAL_RANGE) { // Green to Blue-1
            r = 0;
            if (hue < 3 * VAL_RANGE) { //   Green to Cyan-1
                g = VAL_RANGE;
                b = hue - 2 * VAL_RANGE;
            } else {          //   Cyan to Blue-1
                g = 4 * VAL_RANGE - hue;
                b = VAL_RANGE;
                }
        } else if (hue < 6 * VAL_RANGE) { // Blue to Red-1
            g = 0;
            if (hue < 5 * VAL_RANGE) { //   Blue to Magenta-1
                r = hue - 4 * VAL_RANGE;
                b = VAL_RANGE;
            } else { //   Magenta to Red-1
                r = VAL_RANGE;
                b = 6 * VAL_RANGE - hue;
            }
        } else { // Last 0.5 Red (quicker than % operator)
            r = VAL_RANGE;
            g = b = 0;
        }
    }

    int32_t s2 = (VAL_RANGE - (int32_t)hsv._sat) * VAL_RANGE;

    return RGB(
        (((r * hsv._sat) + s2) * hsv._val + (VAL_RANGE * VAL_RANGE / 2)) / (VAL_RANGE * VAL_RANGE), 
        (((g * hsv._sat) + s2) * hsv._val + (VAL_RANGE * VAL_RANGE / 2)) / (VAL_RANGE * VAL_RANGE), 
        (((b * hsv._sat) + s2) * hsv._val + (VAL_RANGE * VAL_RANGE / 2)) / (VAL_RANGE * VAL_RANGE)
);
}


// Based on instructions found on: https://www.geeksforgeeks.org/program-change-rgb-color-model-hsv-color-model/
Colors::HSV Colors::rgb2hsv(RGB rgb)
{
    int32_t cMin = min(rgb._red, min(rgb._green, rgb._blue));
    int32_t cMax = max(rgb._red, max(rgb._green, rgb._blue));
    int32_t cDelta = cMax - cMin;
    int32_t h = 0, s = 0;

    if(cMax == 0)
    {
        s = 0;
    }
    else
    {
        s = (cDelta * VAL_RANGE + (cMax>>1)) / cMax;
    }

    if(cDelta == 0)
    {
        h = 0;
    }
    else if(cMax == rgb._red)
    {
        if(rgb._green > rgb._blue)
        {
            h = (H_PART * ((int32_t) rgb._green - (int32_t) rgb._blue)) / cDelta;
        }
        else
        {
            h =  H_PART * 6 - (H_PART * ((int32_t) rgb._blue - (int32_t) rgb._green)) / cDelta;
        }
    }
    else if(cMax == rgb._green)
    {
        h = (H_PART * ((int32_t) rgb._blue - (int32_t) rgb._red)) / cDelta + (H_PART) * 2;
    }
    else // --> cMax = blue
    {
        h = (H_PART * ((int32_t) rgb._red - (int32_t) rgb._green)) / cDelta + (H_PART) * 4;
    }

    h = (h + 3) / 6;
    if(h == H_PART)
    {
        h = 0;
    }

    return HSV(h, s, cMax);
}
    