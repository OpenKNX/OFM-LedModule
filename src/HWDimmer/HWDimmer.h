#pragma once
#include <OpenKNX.h>

//#define VALUE_KNX_COUNT 256
#define VALUE_KNX_COUNT 101

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
    void processFrontInput();
    void processFrontOutput();

    virtual bool setLevel(uint16_t level, uint8_t channel);
    virtual uint16_t getLevel(uint8_t channel);
    virtual uint16_t scale(uint8_t level, HWDimmer::DimLUTType lutType) = 0;
    virtual uint16_t getScaleMax(HWDimmer::DimLUTType lutType) = 0;
    virtual std::string logPrefix();

    virtual bool checkConnection() { return true; } // TODO: try to remove this functionality from base class
    virtual void reconnect() {}

  protected:
    uint8_t numChannels;
    uint16_t *levels;

    template<int N> class LUT
    {
      public:
        constexpr LUT(uint16_t range, float power) : values(), len(N)
        {
            for (auto i = 0; i < N; ++i)
            {
                values[i] = round(range * pow((float)i / float(N - 1), power));
                //HWDimmer::table(i,values[i]);
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
};