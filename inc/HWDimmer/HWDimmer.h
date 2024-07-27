#pragma once
#include <OpenKNX.h>

#define DIMLOOP_DELAY       10 // ms
#define VALUE_MIN           0
#define VALUE_MAX           255
#define VALUE_KNX_COUNT     256

class HWDimmer
{
    public:
    HWDimmer(uint8_t numChannels);
    ~HWDimmer();

    virtual bool setLevel(uint8_t level, uint8_t channel);
    virtual uint8_t getLevel(uint8_t channel);
    virtual std::string logPrefix();
    
    void dimLoop();
    void switchOn(uint8_t channel);
    void switchOff(uint8_t channel);
    void dimToLevel(uint8_t channel, uint8_t level, uint32_t dimTime);

    virtual bool checkConnection(){ return true; } // TODO: try to remove this functionality from base class
    virtual void reconnect(){}

    protected:
    uint8_t numChannels;

    enum DimTask
    {
        DIM_IDLE,
        DIM_TO_VALUE
    };

    struct DimChannel
    {
        DimChannel()
        {
            dimDurationAbs = 1;
        }

        uint8_t dir;
        uint8_t channelId;
        uint8_t level;
        uint8_t lastLevel;
        uint8_t targetLevel;
        int16_t deltaLevel;
        DimTask task;

        uint32_t startTimestamp;
        uint16_t dimDurationAbs;
        uint16_t dimDurationRel;
    };

    DimChannel *dimChannels;

    enum DimLUTType
    {
        Linear = 0,
        Log1_5 = 1
    };

    template<int N>
    class LUT
    {
        public:
        constexpr LUT(uint16_t range, float power):values(), len(N)
        {
            for (auto i = 0; i < N; ++i)
            {
                values[i] = round(range * pow((float)i / float(N), power));
            }
            values[0] = 0;
            values[N] = range;
        }

        uint16_t Val(uint16_t id)
        {
            uint16_t val = 0;
            if(id < len)
            {
                val = values[id];
            }
            return val;
        }

        private:
        uint16_t values[N];
        uint16_t len;
    };

    static LUT<VALUE_KNX_COUNT> dimLUT[2];
};