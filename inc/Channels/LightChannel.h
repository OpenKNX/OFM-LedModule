#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"
#include "Colors.h"

#define DIMLOOP_DELAY       20 // ms
#define UPDATE_DELAY        500 // ms
#define N_SCENES            8
#define BRIGHTNESS_MAX      UINT8_MAX

class LightChannel : public OpenKNX::Channel
{
  public:
    LightChannel(uint8_t channel_number, HWDimmer *pDimmer, uint8_t hwChannels[], uint8_t numChannels);
    virtual void loop();
    virtual void processInputKo(GroupObject &ko) = 0;
    void Save();
    void Restore();

    template<typename T> class DimmableValue
    {
      public:
        DimmableValue() {}
        DimmableValue(T val) { currentValue = minValue = maxValue = val; }
        DimmableValue(T val, T min, T max)
        {
            currentValue = val;
            minValue = min;
            maxValue = max;
        }

        const std::string logPrefix() { return "DimValues:"; }

        void setRange(T min, T max)
        {
            minValue = min;
            maxValue = max;
        }

        T getMin() { return (T)minValue; }
        T getMax() { return (T)maxValue; }

        void setStairTime(unsigned long _t_ime)     { _stairTime = _t_ime; }
        unsigned long getStairTime()                { return _stairTime; }
        void setStairTrigger(bool __trigger)        { _stairTrigger = __trigger; }
        T getStairTrigger()                         { return _stairTrigger; }

        void setLastOnValue(int32_t _lastOnVal)     { lastOnValue = _lastOnVal; }
        int32_t getLastOnValue()                    { return lastOnValue; }

        void setRGBChangingTime(unsigned long _t_ime)
        {
            _rgbChangingTime = _t_ime;
            logInfoP("start change time");
        }
        unsigned long getRGBChangingTime() { return _rgbChangingTime; }
        void setRGBChangingTrigger(bool _t_rigger)
        {
            _rgbChangingTrigger = _t_rigger;
            logDebugP("start change trigger");
        }
        bool getRGBChangingTrigger() { return _rgbChangingTrigger; }

        void setTargetValue(T target, uint32_t timestamp, uint16_t dimTime)
        {
            lastValue = currentValue;
            if (target > maxValue)
            {
                target = maxValue;
            }
            else if (target < minValue)
            {
                target = minValue;
            }
            targetValue = target;
            deltaValue = (int32_t)targetValue - (int32_t)lastValue;
            startTimestamp = timestamp;
            dimDurationAbs = dimTime;
            isDimming = 1;
        }

        T step(uint32_t timestamp)
        {
            if (isDimming == 1)
            {
                currentValue = ((int32_t)(timestamp - startTimestamp) * (deltaValue)) / dimDurationAbs + lastValue;
                if (deltaValue >= 0 && currentValue >= targetValue || deltaValue < 0 && currentValue <= targetValue)
                {
                    isDimming = 0;
                    currentValue = targetValue;
                }
            }
            return (T)currentValue;
        }

        T value()
        {
            return (T)currentValue;
        }

      private:
        uint8_t isDimming = 0;

        T lastValue = 0;
        T targetValue = 0;
        T minValue = 0;
        T maxValue = 0;
        int32_t lastOnValue = 255;
        int32_t currentValue = 0;
        int32_t deltaValue = 0;
        unsigned long _stairTime = 0;
        bool _stairTrigger = 0;
        uint32_t startTimestamp = 0;
        uint16_t dimDurationAbs = 100;
        unsigned long _rgbChangingTime = 0;
        bool _rgbChangingTrigger = 0;
    };

    struct SceneConfig
    {
        uint8_t lockFunc : 1;
        uint8_t lockObj : 1;
        uint8_t valueType : 3;
        uint8_t funcType : 3;
        uint8_t sceneNr : 7;
        uint8_t isFixed : 1;
        uint8_t value[3];

      public:
        uint16_t ColorTemperature() { return value[2] + (value[1] << 8); }
        uint8_t Brightness() { return value[0]; }
        Colors::HSV HSV() { return Colors::rgb2hsv(Colors::RGB(value[0], value[1], value[2])); }
        enum FuncType
        {
            INACTIVE = 0,
            VALUE = 1,
            FUNCTION = 2,
            SEQUENCE = 3,
            LOCKING = 4
        };
    };

  protected:
    uint8_t _numChannels;
    HWDimmer *_pDimmer;
    uint8_t *_pHWChannels;
    uint32_t _lastTimestamp = 0;
    uint32_t _lastDimTimestamp = 0;
    uint16_t _lastBrightnessLevel = 0;
    bool _isLocked = false;

    SceneConfig *_scenes;

    DimmableValue<uint8_t> _brightness;

    virtual void update() = 0;

    virtual void handleScene(uint8_t sceneNr) = 0;

  private:
    const std::string name() override;
};
