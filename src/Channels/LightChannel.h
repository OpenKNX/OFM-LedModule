#pragma once

#include "Colors.h"
#include "HWDimmer/HWDimmer.h"
#include "OpenKNX.h"
#include "LedStatusOutput.h"
#include "hardware.h"
#include <Arduino.h>

#define DIMLOOP_DELAY 20 // ms
#define DEBUG_DELAY 500  // ms
#define N_SCENES 8

#define LED_INVALID_HW_CHANNEL 0xFF

#define CABLE_RESISTANCE_PER_METER 0.0175f // Copper 1.75 Ohm mm² / m

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
    int32_t getCurrentBrightnessValue() { return _brightness.value(); }
    virtual void setSwitch(bool switchOn) = 0;
    virtual void setSwitchNoDim(bool switchOn) = 0;
    bool getNight();
    virtual void setNight(bool night) = 0;
    bool getLock();
    void setLock(bool lock);
    bool isActive() { return _channelActive; };
    virtual bool getCO1() = 0;
    virtual bool getCO2() = 0;
    virtual bool getCO3() = 0;

    // Recalls a scene from this channel's own scene table (e.g. forwarded by a central object).
    void callScene(uint8_t sceneNr)
    {
        handleScene(sceneNr);
        _sceneNumberActive = sceneNr + 1;
    }

    // A command just (re)set this channel's demand; LedModule arbitrates the combined
    // max-total-brightness budget after dispatch. No-op on boards without the budget define,
    // so call sites need no #ifdef guard.
    void markBudgetRequest(uint16_t dimTime)
    {
#ifdef LEDMODULE_MAX_TOTAL_BRIGHTNESS
        _budgetRequester = true;
        _budgetDimTime = dimTime;
#else
        (void)dimTime;
#endif
    }

#ifdef LEDMODULE_MAX_TOTAL_BRIGHTNESS
    // --- Combined max-total-brightness budget (see LEDMODULE_MAX_TOTAL_BRIGHTNESS) ---------
    // Sum of this channel's physical PWM component fractions at its TARGET, in channel-worths
    // (1.0 = one pin at 100 %). Overridden per channel type to mirror its loop() output.
    virtual float budgetFootprint() { return 0.0f; }
    // Scale the channel's brightness target (preserves colour/CCT). Does NOT mark a request,
    // so the arbiter can call it without recursing back into arbitration.
    virtual void applyBudgetScale(float factor, uint16_t dimTime)
    {
        _brightness.setTargetValue((uint16_t)((float)_brightness.target() * factor + 0.5f), dimTime);
        _sceneNumberActive = 0;
    }
    bool budgetRequester() const { return _budgetRequester; }
    uint16_t budgetDimTime() const { return _budgetDimTime; }
    void clearBudgetRequest() { _budgetRequester = false; }
#endif

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

        // true while a fade/transition towards the target value is in progress
        bool dimming() const
        {
            return isDimming != 0;
        }

        // Relative dimming (DPT 3.007) towards limit: the step code (1..7) selects the step width
        // (100 %, 50 %, ... 1.56 % of fullRange) and the fade runs at a constant speed, i.e. the
        // full range always takes fullRangeTime. Previously every telegram restarted a fade over
        // the full time to the limit, so repeated telegrams only approached min/max asymptotically.
        // Never moves against the requested direction. Returns the fade time used.
        uint16_t relDim(bool up, uint8_t stepCode, uint16_t limit, uint16_t fullRange, uint16_t fullRangeTime)
        {
            const int32_t current = (uint16_t)currentValue;
            int32_t target = current;
            if (fullRange > 0 && stepCode >= 1 && stepCode <= 7)
            {
                int32_t step = (int32_t)fullRange >> (stepCode - 1);
                if (step < 1)
                    step = 1;

                if (up)
                    target = current + step < limit ? current + step : limit;
                else
                    target = current - step > limit ? current - step : limit;

                if (up ? target < current : target > current)
                    target = current;
            }

            const uint32_t distance = target >= current ? target - current : current - target;
            const uint16_t dimTime = fullRange > 0 ? (uint32_t)fullRangeTime * distance / fullRange : 0;
            setTargetValue((uint16_t)target, dimTime);
            return dimTime;
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
        // 8 bit RGB from ETS -> internal HSV ranges (hue 0..H_PART-1, sat 0..VAL_RANGE, val 0..VALUE_KNX_COUNT-1)
        Colors::HSV HSV() { return Colors::rgb2hsv(Colors::RGB((uint32_t)value[0] << 16 | (uint32_t)value[1] << 8 | value[2])); }
        // hue and saturation scenes store a single 8 bit value in value[0] (same scaling as the HSV KO)
        uint16_t Hue() { return _UFP16(value[0], 6); }
        uint16_t Saturation() { return map(value[0], 0, 255, 0, VAL_RANGE); }
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

    StatusValueState _statusCurrent;
    StatusValueState _statusPower;
    uint32_t _deviceProtOverloadTimer = 0;
    uint32_t _lampProtOverloadTimer = 0;

    int32_t _lastOnValue = VALUE_KNX_COUNT - 1;
    unsigned long _stairTime = 0;
    bool _stairTrigger = 0;
    bool _isNight = false;
    bool _isLocked = false;
    uint8_t _sceneNumberActive = 0;

    uint32_t _statusSendOnOffTimer = 0;
    StatusValueState _statusBrightness;

#ifdef LEDMODULE_MAX_TOTAL_BRIGHTNESS
    bool _budgetRequester = false;
    uint16_t _budgetDimTime = 0;
#endif

    SceneConfig *_scenes;

    DimmableValue<uint16_t> _brightness;

    virtual void update() = 0;
    virtual void handleScene(uint8_t sceneNr) = 0;

    // Keep status nonzero while the light is on, including fine intermediate fade levels.
    static uint8_t brightnessRaw(uint16_t brightness)
    {
        const uint8_t raw = LedBrightness::levelToKnx(brightness);
        return (raw == 0 && brightness > 0) ? 1 : raw;
    }

    void processDeviceProtection(GroupObject &koConstCurrent, GroupObject &koOverload, bool active, uint8_t constCurrent, uint8_t overloadPercent, uint32_t overloadTimeMS, uint32_t &overloadTimer, bool cutOff, float current);
    void processLampProtection(GroupObject &koConstCurrent, GroupObject &koOverload, bool active, uint16_t cableLength, uint8_t cableCrossSect, uint8_t constPower, uint8_t overloadPercent, uint32_t overloadTimeMS, uint32_t &overloadTimer, bool cutOff, float current, float voltage, uint8_t channelCount = 1);

    uint32_t _debugTimer = 0;

  private:
    const std::string name() override;
};
