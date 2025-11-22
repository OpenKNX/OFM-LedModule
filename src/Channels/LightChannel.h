#pragma once

#include "Colors.h"
#include "HWDimmer/HWDimmer.h"
#include "OpenKNX.h"
#include <Arduino.h>

#define DIMLOOP_DELAY 20 // ms
#define DEBUG_DELAY 500 // ms
#define N_SCENES 8

#define LED_INVALID_HW_CHANNEL 0xFF

class LightChannel : public OpenKNX::Channel
{
  public:
    LightChannel(uint8_t channel_number, HWDimmer *pDimmer, uint8_t hwChannels[], uint8_t numChannels);
    virtual void loop();
    virtual void processInputKo(GroupObject &ko) = 0;
    void Save();
    void Restore();
    void setStairTime(unsigned long time) { _stairTime = time; }
    unsigned long getStairTime() { return _stairTime; }
    void setStairTrigger(bool trigger) { _stairTrigger = trigger; }
    bool getStairTrigger() { return _stairTrigger; }
    void setLastOnValue(int32_t lastOnVal) { _lastOnValue = lastOnVal; }
    int32_t getLastOnValue() { return _lastOnValue; }
    bool getNight();
    virtual void setNight(bool night) = 0;
    bool getLock();
    void setLock(bool lock);

    template <typename T>
    class DimmableValue
    {
      public:
        DimmableValue() {}
        DimmableValue(T val) { currentValue = minValue = maxValue = val; }
        DimmableValue(T val, uint16_t min, uint16_t max)
        {
            currentValue = val;
            minValue = min;
            maxValue = max;
        }

        const std::string logPrefix() { return "DimValues:"; }

        void setRange(uint16_t min, uint16_t max)
        {
            minValue = min;
            maxValue = max;
        }

        uint16_t getMin() { return minValue; }
        uint16_t getMax() { return maxValue; }

        void setTargetValue(uint16_t target, uint16_t dimTime)
        {
            lastValue = currentValue;
            /*if (target > maxValue)
            {
                target = maxValue;
            }
            else if (target < minValue)
            {
                target = minValue;
            }*/
            targetValue = target;
            deltaValue = (int32_t)targetValue - (int32_t)lastValue;
            startTimestamp = millis();
            dimDurationAbs = dimTime;
            isDimming = 1;
        }

        uint16_t step(uint32_t timestamp)
        {
            if (isDimming == 1)
            {
                if (dimDurationAbs == 0)
                    currentValue = targetValue;
                else
                    currentValue = ((int32_t)(timestamp - startTimestamp) * (deltaValue)) / dimDurationAbs + lastValue;

                if (deltaValue >= 0 && currentValue >= targetValue || deltaValue < 0 && currentValue <= targetValue)
                {
                    isDimming = 0;
                    currentValue = targetValue;
                }
            }
            return (uint16_t)currentValue;
        }

        T value()
        {
            return (uint16_t)currentValue;
        }

        T target()
        {
            return (uint16_t)targetValue;
        }

      private:
        uint8_t isDimming = 0;

        uint16_t lastValue = 0;
        uint16_t targetValue = 0;
        uint16_t minValue = 0;
        uint16_t maxValue = 0;
        int32_t currentValue = 0;
        int32_t deltaValue = 0;
        uint32_t startTimestamp = 0;
        uint16_t dimDurationAbs = 100;
        uint8_t lastDimmValue = 100;
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
        uint16_t Brightness() { return value[0]; }
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
    bool _channelActive = false;
    uint8_t _numChannels;
    HWDimmer *_pDimmer;
    uint8_t *_pHWChannels;
    uint32_t _lastDimTimestamp = 0;
    uint16_t _lastBrightnessLevel = 0;

    float _lastSentCurrent = 0.0f;
    float _lastSentPower = 0.0f;
    uint32_t _currentCyclicSendTimer = 0;
    uint32_t _powerCyclicSendTimer = 0;

    int32_t _lastOnValue = VALUE_KNX_COUNT-1;
    unsigned long _stairTime = 0;
    bool _stairTrigger = 0;
    bool _isNight = false;
    bool _isLocked = false;
    uint8_t _sceneNumberActive = 0;

    uint32_t _statusSendOnOffTimer = 0;
    uint32_t _statusSendBrightnessTimer = 0;

    SceneConfig *_scenes;

    DimmableValue<uint16_t> _brightness;

    virtual void update() = 0;
    virtual void handleScene(uint8_t sceneNr) = 0;
    void processSendValue(GroupObject& ko, Dpt dpt, bool send, uint8_t sendMinChangePercent, uint16_t sendMinChangeAbsolute, uint32_t sendCyclicTimeMS, uint32_t &cyclicSendTimer, float &lastSentValue, float currentValue, uint16_t checkMultiply = 1);

    uint32_t _debugTimer = 0;

  private:
    const std::string name() override;
};
