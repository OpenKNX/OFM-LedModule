#pragma once
#include <OpenKNX.h>

#ifdef LEDMODULE_CURRENT_ADDR
    #include <Adafruit_INA238.h>
#endif

// #define VALUE_KNX_COUNT 256
// #define VALUE_KNX_COUNT 101
#define VALUE_KNX_COUNT 4096
#define VALUE_KNX_MULTIPLY 40.95

#define FRONT_INPUT_DEBOUNCE 250
#define POWER_SUPPLY_REQUEST_DELAY 1000
#define VOLTAGE_MIN_DIFFERENCE 500

class HWDimmer
{
  public:
    HWDimmer(uint8_t numChannels);
    ~HWDimmer();

    enum DimLUTType
    {
        Linear = 0,
        Log1_5 = 1,
        Log2_0 = 2
    };

    void loop();

    bool setLevel(uint16_t level, uint8_t channel);
    uint16_t getLevel(uint8_t channel);
    float getTemperature(uint8_t channel);
    float getTemperatureAvg();
    float getCurrent(uint8_t channel);
    float getTotalCurrent();
    float getVoltage(uint8_t channel);
    float getTotalPower();
    virtual uint16_t scale(uint16_t level, HWDimmer::DimLUTType lutType) = 0;
    virtual uint16_t getScaleMax(HWDimmer::DimLUTType lutType) = 0;
    std::string logPrefix();
    virtual void outputLUT() = 0;
    virtual void runTestMode() = 0;
    virtual bool checkConnection() = 0;
    virtual void reconnect() {}

    bool powerSupplyAvailableOrRequest();

  protected:
    bool updateLevelValue(uint16_t level, uint8_t channel);
    virtual void setLevelInternal(uint16_t level, uint8_t channel) = 0;

    uint8_t numChannels;
    uint16_t *levels;

    bool _currentManualMode[LEDMODULE_MAX_LIGHT_CHANNELS] = {false};

    template <uint16_t N>
    class LUT
    {
      public:
        constexpr LUT(uint16_t range, float power) : values(), len(N)
        {
            for (auto i = 0; i < N; ++i)
            {
                values[i] = round(range * pow((float)i / float(N - 1), power));
            }
            values[0] = 0;
            values[N - 1] = range;
        }

        uint16_t Val(uint16_t id)
        {
            uint16_t val = 0;
            if (id < len)
            {
                val = values[id];
            }
            return val;
        }

        uint16_t Max()
        {
            return values[len - 1];
        }

      private:
        uint16_t values[N];
        uint16_t len;
    };

    static LUT<VALUE_KNX_COUNT> dimLUT[3];

  private:
    void checkPowerSupply();
    void processCurrentSense();
    void processFrontInput();
    void processFrontOutput();

    uint32_t _currentManualModeLastChange[LEDMODULE_MAX_LIGHT_CHANNELS] = {0};

#ifdef LEDMODULE_CURRENT_ADDR
    Adafruit_INA238 _currentSense[LEDMODULE_MAX_LIGHT_CHANNELS];

    uint8_t currentInaAddr[LEDMODULE_MAX_LIGHT_CHANNELS] = {LEDMODULE_CURRENT_ADDR};
    float _currentValues[LEDMODULE_MAX_LIGHT_CHANNELS] = {0.0f};
    float _voltageValues[LEDMODULE_MAX_LIGHT_CHANNELS] = {0.0f};
    float _temperatureValues[LEDMODULE_MAX_LIGHT_CHANNELS] = {0.0f};

    uint32_t _currentSenseDebugTimer = 0;
#endif

    float _powerSupplyVoltage = 0;
    uint32_t _powerSupplyLastRequest = 0;
    uint32_t _powerShutdownTimer = 0;
    uint16_t _lastVoltageSent = 0;
    uint32_t _voltageSendTimer = 0;
};