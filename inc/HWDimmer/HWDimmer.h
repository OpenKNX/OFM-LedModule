#pragma once
#include <OpenKNX.h>

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
    void dimToLevel(uint8_t channel, uint8_t level);

    virtual bool checkConnection(){ return true; }
    virtual void reconnect(){}

    protected:
    uint8_t numChannels;
    uint8_t *levels;

    enum DimTask
    {
        DIM_IDLE,
        DIM_STOP,
        DIM_ON,
        DIM_OFF,
        DIM_SOFTON,
        DIM_SOFTOFF,
        DIM_UP,
        DIM_DOWN,
        DIM_SET
    };

    struct DimmChannel
    {
        uint8_t channelId;
        uint8_t level;
        uint8_t targetLevel;
        DimTask task;

        uint32_t currentTick;
        uint32_t dimmDurationAbs;
        uint32_t dimmDurationRel;
    };

    DimmChannel *channels;
};