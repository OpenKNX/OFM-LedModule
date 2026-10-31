#include "Colors.h"

// Function based on https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.cpp GNU LGPL
Colors::RGB Colors::hsv2rgb(HSV hsv)
{
    uint32_t r = 0, g = 0, b = 0;
    // Map HUE range to 6 times range of single RGB color. This simplifies the math needed for conversion.
    uint32_t hue = (((uint32_t)hsv._hue) * 6 * VAL_RANGE + H_PART / 2) >> 14; // div by max HUE

    if (hsv._sat != 0)
    {
        if (hue < 2 * VAL_RANGE)
        { // Red to Green-1
            b = 0;
            if (hue < VAL_RANGE)
            { //   Red to Yellow-1
                r = VAL_RANGE;
                g = hue;
            }
            else
            { //   Yellow to Green-1
                r = (2 * VAL_RANGE) - hue;
                g = VAL_RANGE;
            }
        }
        else if (hue < 4 * VAL_RANGE)
        { // Green to Blue-1
            r = 0;
            if (hue < 3 * VAL_RANGE)
            { //   Green to Cyan-1
                g = VAL_RANGE;
                b = hue - 2 * VAL_RANGE;
            }
            else
            { //   Cyan to Blue-1
                g = 4 * VAL_RANGE - hue;
                b = VAL_RANGE;
            }
        }
        else if (hue < 6 * VAL_RANGE)
        { // Blue to Red-1
            g = 0;
            if (hue < 5 * VAL_RANGE)
            { //   Blue to Magenta-1
                r = hue - 4 * VAL_RANGE;
                b = VAL_RANGE;
            }
            else
            { //   Magenta to Red-1
                r = VAL_RANGE;
                b = 6 * VAL_RANGE - hue;
            }
        }
        else
        { // Last 0.5 Red (quicker than % operator)
            r = VAL_RANGE;
            g = b = 0;
        }
    }

    int32_t s2 = (VAL_RANGE - (int32_t)hsv._sat) * VAL_RANGE;

    return RGB(
        (((r * hsv._sat) + s2) * hsv._val + (VAL_RANGE * VAL_RANGE / 2)) / (VAL_RANGE * VAL_RANGE),
        (((g * hsv._sat) + s2) * hsv._val + (VAL_RANGE * VAL_RANGE / 2)) / (VAL_RANGE * VAL_RANGE),
        (((b * hsv._sat) + s2) * hsv._val + (VAL_RANGE * VAL_RANGE / 2)) / (VAL_RANGE * VAL_RANGE));
}

// Based on instructions found on: https://www.geeksforgeeks.org/program-change-rgb-color-model-hsv-color-model/
Colors::HSV Colors::rgb2hsv(RGB rgb)
{
    int32_t cMin = min(rgb._red, min(rgb._green, rgb._blue));
    int32_t cMax = max(rgb._red, max(rgb._green, rgb._blue));
    int32_t cDelta = cMax - cMin;
    int32_t h = 0, s = 0;

    if (cMax == 0)
        s = 0;
    else
        s = (cDelta * VAL_RANGE + (cMax >> 1)) / cMax;

    if (cDelta == 0)
        h = 0;
    else if (cMax == rgb._red)
    {
        if (rgb._green > rgb._blue)
            h = (H_PART * ((int32_t)rgb._green - (int32_t)rgb._blue)) / cDelta;
        else
            h = H_PART * 6 - (H_PART * ((int32_t)rgb._blue - (int32_t)rgb._green)) / cDelta;
    }
    else if (cMax == rgb._green)
        h = (H_PART * ((int32_t)rgb._blue - (int32_t)rgb._red)) / cDelta + (H_PART) * 2;
    else // --> cMax = blue
        h = (H_PART * ((int32_t)rgb._red - (int32_t)rgb._green)) / cDelta + (H_PART) * 4;

    h = (h + 3) / 6;
    if (h == H_PART)
        h = 0;

    return HSV(h, s, cMax);
}

// Approximated black body colour (8 bit RGB, 0xRRGGBB) for a colour temperature in Kelvin.
uint32_t Colors::temp2rgb(uint16_t temp)
{
    uint8_t r, g, b = 0;
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
    // above 6500 = 255 (clamped, as the formula exceeds 255 from ~6513 K and would wrap around)
    if (temp <= 1900)
        b = 0;
    if (temp >= 1900)
        b = (uint8_t)min(round(477.5 * log10(temp) - 1565.6), 255.0);
    if (temp >= 6800)
        b = 255;

    return (uint32_t)r << 16 | g << 8 | b;
}

// Inverse of temp2rgb(): the colour temperature (1000..10000 K) whose colour is closest to the given
// 8 bit RGB (0xRRGGBB) at full brightness. Searches a 100 K, then 10 K, then 1 K grid around the best
// match, which needs ~130 evaluations of temp2rgb() instead of thousands for a full 1 K scan.
uint16_t Colors::rgb2temp(uint32_t rgb)
{
    const int32_t rTarget = (rgb >> 16) & 0xFF;
    const int32_t gTarget = (rgb >> 8) & 0xFF;
    const int32_t bTarget = rgb & 0xFF;

    uint16_t bestTemp = 1000;
    uint32_t minError = UINT32_MAX;
    uint16_t low = 1000;
    uint16_t high = 10000;
    for (uint16_t step = 100; step >= 1; step /= 10)
    {
        for (uint16_t temp = low; temp <= high; temp += step)
        {
            uint32_t tempRgb = temp2rgb(temp);
            int32_t dr = (int32_t)((tempRgb >> 16) & 0xFF) - rTarget;
            int32_t dg = (int32_t)((tempRgb >> 8) & 0xFF) - gTarget;
            int32_t db = (int32_t)(tempRgb & 0xFF) - bTarget;
            uint32_t error = dr * dr + dg * dg + db * db;
            if (error < minError)
            {
                minError = error;
                bestTemp = temp;
            }
        }
        low = bestTemp - step < 1000 ? 1000 : bestTemp - step;
        high = bestTemp + step > 10000 ? 10000 : bestTemp + step;
    }

    return bestTemp;
}
