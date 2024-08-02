#pragma once

#include <Arduino.h>
#include "OpenKNX.h"
#include "HWDimmer.h"

#define DIMLOOP_DELAY       20 // ms
#define UPDATE_DELAY        500 // ms


class LightChannel : public OpenKNX::Channel
{
  public:
  
    LightChannel(uint8_t channel_number, HWDimmer* pDimmer, uint8_t hwChannels[], uint8_t numChannels);
    virtual void loop();
    virtual void processInputKo(GroupObject& ko) = 0;
    void Save();
    void Restore();

    template<typename T> class  DimmableValue
    {
      public:
        DimmableValue() { }
        DimmableValue(T val) { currentValue = val; }

        const std::string logPrefix() { return "DimValues:"; }

        void setTargetValue(T target, uint32_t timestamp, uint16_t dimTime)
        {
          lastValue = currentValue;
          targetValue = target;
          deltaValue = (int32_t)targetValue - (int32_t)lastValue;
          startTimestamp = timestamp;
          dimDurationAbs = dimTime;
          isDimming = 1;
        }

        T step(uint32_t timestamp)
        {
          if(isDimming == 1)
          {
            currentValue = ((int32_t)(timestamp - startTimestamp) * (deltaValue)) / dimDurationAbs + lastValue;
            if(deltaValue >= 0 && currentValue >= targetValue || deltaValue < 0 && currentValue <= targetValue)
            {
              isDimming = 0;
              currentValue = targetValue;
            }
          }
          return (T) currentValue;
        }

        T value()
        {
          return (T) currentValue;
        }

        private:
        uint8_t isDimming = 0;
        
        T lastValue = 0;
        T targetValue = 0;
        int32_t currentValue = 0;
        int32_t deltaValue = 0;
        
        uint32_t startTimestamp = 0;
        uint16_t dimDurationAbs = 100;
    };
  
  protected:

    uint8_t _numChannels;
    HWDimmer* _pDimmer;
    uint8_t *_pHWChannels;
    uint32_t _lastTimestamp = 0;
    uint32_t _lastDimTimestamp = 0;
    uint16_t _lastBrightnessLevel = 0;
    bool _isLocked = false;
    
    DimmableValue<uint8_t> _brightness;

    virtual void update() = 0;

  private:

    const std::string name() override;
};
