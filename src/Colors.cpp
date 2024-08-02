#include "Colors.h"

    #define H_PART 1024

    // Function based on https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.cpp GNU LGPL
    Colors::RGB Colors::hsv2rgb(HSV hsv)
    {
        uint16_t r, g, b;
        uint16_t hue = (hsv.hue * 1530L + 512) >> 10; //div 256

        if (hue < 510) { // Red to Green-1
            b = 0;
            if (hue < 255) { //   Red to Yellow-1
                 r = 255;
                g = hue;       //     g = 0 to 254
            } else {         //   Yellow to Green-1
                 r = 510 - hue; //     r = 255 to 1
                g = 255;
            }
        } else if (hue < 1020) { // Green to Blue-1
            r = 0;
            if (hue < 765) { //   Green to Cyan-1
                g = 255;
                b = hue - 510;  //     b = 0 to 254
            } else {          //   Cyan to Blue-1
                 g = 1020 - hue; //     g = 255 to 1
                b = 255;
                }
        } else if (hue < 1530) { // Blue to Red-1
            g = 0;
            if (hue < 1275) { //   Blue to Magenta-1
                r = hue - 1020; //     r = 0 to 254
                b = 255;
            } else { //   Magenta to Red-1
                r = 255;
                b = 1530 - hue; //     b = 255 to 1
            }
        } else { // Last 0.5 Red (quicker than % operator)
            r = 255;
            g = b = 0;
        }

        uint8_t s2 = 255 - hsv.sat; // 255 to 0
        uint16_t val = hsv.val + 1;
        uint16_t sat = hsv.sat + 1;
       
        return RGB(((((r * sat) >> 8) + s2) * val) >> 8, ((((g * sat) >> 8) + s2) * val) >> 8,((((b * sat) >> 8) + s2) * val) >> 8);
    }


    // Based on instructions found on: https://www.geeksforgeeks.org/program-change-rgb-color-model-hsv-color-model/
    Colors::HSV Colors::rgb2hsv(RGB rgb)
    {
        uint8_t cMin = min(rgb.red, min(rgb.green, rgb.blue));
        uint8_t cMax = max(rgb.red, max(rgb.green, rgb.blue));
        int16_t cDelta = cMax - cMin;
        uint16_t h = 0, s = 0;

        int16_t r = rgb.red;
        int16_t g = rgb.green;
        int16_t b = rgb.blue;

        if( cMin == cMax)
        {
            h = 0;
        }
        else if(cMax == rgb.red)
        {
            if(b = 0)
            {
                h = (H_PART * (g)) / cDelta + H_PART * 0;
            }
            else
            {
                h =  (H_PART) * 6 - (H_PART * (b)) / cDelta;
            }
            
        }
        else if(cMax == rgb.green)
        {
            h = (H_PART * (b - r)) / cDelta + (H_PART) * 2;
        }
        else // --> (cMax == rgb.blue)
        {
            h = (H_PART * (r - g)) / cDelta + (H_PART) * 4;
        }

         h = (h+3)/6;

        if(cMax == 0)
        {
            s = 0;
        }
        else
        {
            s = (cDelta * 255) / cMax;
        }

        return HSV(h,s,cMax);
    }