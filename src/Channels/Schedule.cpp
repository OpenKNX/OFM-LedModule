#include "Schedule.h"
#include "SingleChannel.h"
#include "knxprod.h"
#include <knx.h>

// SUN_OFFSETS table — indexed by (timeByte mod 17). MUST match enum ordering
// in PT-ScheduleSunOffset (LedModule.share.xml). idx<17 ⇒ sunrise, ≥17 ⇒ sunset.
//   Sonnenaufgang/-untergang -5h/-4h/-3h/-2h/-1h/-45min/-30min/-15min/+-0min/+15min/+30min/+45min/+1h/+2h/+3h/+4h/+5h
static constexpr int16_t SUN_OFFSETS[17] = {
    -300, -240, -180, -120, -60,
    -45, -30, -15, 0, 15,
    30, 45, 60, 120, 180,
    240, 300};

Schedule::Schedule(uint8_t channelIndex, SingleChannel *pChannel, HWDimmer *pDimmer)
    : _channelIndex(channelIndex), _pChannel(pChannel), _pDimmer(pDimmer)
{
}

void Schedule::setup()
{
    _numPoints = ParamLED_SC_ChScheduleNumPoints;
    if (_numPoints < 2) _numPoints = 2;
    if (_numPoints > MAX_POINTS) _numPoints = MAX_POINTS;
    _offBehavior = ParamLED_SC_ChScheduleOffBehavior == 1
                       ? OffBehavior::Ausschalten
                       : OffBehavior::SequenzStoppen;
    _fallbackMinutes = ParamLED_SC_ChScheduleFallbackMin;
    _schaltzeiten = ParamLED_SC_ChScheduleSchaltzeiten == 1
                        ? Schaltzeiten::Sonne
                        : Schaltzeiten::Uhrzeit;

    // Stützpunkte: time byte (Uhrzeit/SunOffset share storage), brightness byte.
    // Note: ParamLED_SC_ChSchedulePoint{N}_Uhrzeit is the SAME byte as ..._SunOffset.
    // Reading either macro returns the byte; interpretation depends on _schaltzeiten.
    _points[0].timeByte   = ParamLED_SC_ChSchedulePoint1_Uhrzeit;
    _points[0].brightness = ParamLED_SC_ChSchedulePoint1_Brightness;
    _points[1].timeByte   = ParamLED_SC_ChSchedulePoint2_Uhrzeit;
    _points[1].brightness = ParamLED_SC_ChSchedulePoint2_Brightness;
    _points[2].timeByte   = ParamLED_SC_ChSchedulePoint3_Uhrzeit;
    _points[2].brightness = ParamLED_SC_ChSchedulePoint3_Brightness;
    _points[3].timeByte   = ParamLED_SC_ChSchedulePoint4_Uhrzeit;
    _points[3].brightness = ParamLED_SC_ChSchedulePoint4_Brightness;
    _points[4].timeByte   = ParamLED_SC_ChSchedulePoint5_Uhrzeit;
    _points[4].brightness = ParamLED_SC_ChSchedulePoint5_Brightness;
    _points[5].timeByte   = ParamLED_SC_ChSchedulePoint6_Uhrzeit;
    _points[5].brightness = ParamLED_SC_ChSchedulePoint6_Brightness;
    _points[6].timeByte   = ParamLED_SC_ChSchedulePoint7_Uhrzeit;
    _points[6].brightness = ParamLED_SC_ChSchedulePoint7_Brightness;
    _points[7].timeByte   = ParamLED_SC_ChSchedulePoint8_Uhrzeit;
    _points[7].brightness = ParamLED_SC_ChSchedulePoint8_Brightness;
    _points[8].timeByte   = ParamLED_SC_ChSchedulePoint9_Uhrzeit;
    _points[8].brightness = ParamLED_SC_ChSchedulePoint9_Brightness;
    _points[9].timeByte   = ParamLED_SC_ChSchedulePoint10_Uhrzeit;
    _points[9].brightness = ParamLED_SC_ChSchedulePoint10_Brightness;

    _active = true;
    _suspendedByManual = false;
    _lastLevelSent = -1;
    _lastEvalMillis = 0;
    publishStatus(true);
}

int16_t Schedule::pointTimeMinutes(const StuetzPunkt &p) const
{
    if (_schaltzeiten == Schaltzeiten::Uhrzeit)
    {
        // timeByte = hour 0..23, minute always 0 (Phase A simplification per UX feedback)
        return (int16_t)p.timeByte * 60;
    }
    // Sonne mode: requires valid sun calc
    if (!openknx.sun.isSunCalculatioValid()) return -1;
    bool isSunrise = (p.timeByte < 17);
    int16_t offset = SUN_OFFSETS[p.timeByte % 17];
    auto t = isSunrise ? openknx.sun.sunRiseLocalTime() : openknx.sun.sunSetLocalTime();
    int32_t absMin = (int32_t)t.hour * 60 + t.minute + offset;
    // Wrap into 0..1439
    while (absMin < 0)    absMin += 1440;
    while (absMin >= 1440) absMin -= 1440;
    return (int16_t)absMin;
}

void Schedule::loop()
{
    // Manual-suspend fallback handling — runs even while suspended/inactive
    if (_suspendedByManual && _fallbackMinutes > 0)
    {
        uint32_t elapsedMin = (millis() - _suspendedSinceMillis) / 60000UL;
        if (elapsedMin >= _fallbackMinutes)
        {
            _suspendedByManual = false;
            _lastLevelSent = -1;
            publishStatus(true);
        }
    }

    if (!_active || _suspendedByManual) return;

    if (!_pChannel->isOn()) return;

    if (millis() - _lastEvalMillis < 1000) return;
    _lastEvalMillis = millis();

    if (!openknx.time.isValid()) return;
    auto tinfo = openknx.time.getLocalTime();
    uint16_t nowMin = (uint16_t)tinfo.hour * 60 + tinfo.minute;

    uint8_t newLevel = computeInterpolatedLevel(nowMin);
    if ((int16_t)newLevel != _lastLevelSent)
    {
        _pChannel->setBrightness((uint16_t)(newLevel * VALUE_KNX_MULTIPLY));
        _lastLevelSent = newLevel;
    }
}

bool Schedule::processInputKo(GroupObject &ko, int16_t relKo)
{
    if (relKo == LED_SC_KoChScheduleActive)
    {
        bool turnOn = ko.value(DPT_Switch);
        _active = turnOn;
        if (!turnOn)
        {
            if (_offBehavior == OffBehavior::Ausschalten)
                _pChannel->setSwitch(false);
            publishStatus(false);
        }
        else
        {
            _suspendedByManual = false;
            _lastLevelSent = -1;
            publishStatus(true);
        }
        return true;
    }
    return false;
}

void Schedule::notifyManualChange()
{
    if (!_active) return;
    _suspendedByManual = true;
    _suspendedSinceMillis = millis();
    publishStatus(false);
}

void Schedule::notifyLampOff()
{
    _lastLevelSent = -1;
}

uint8_t Schedule::computeInterpolatedLevel(uint16_t nowMin) const
{
    // Build list of valid (time, brightness) pairs for active points.
    // In Sonne mode, points may be invalid (sun calc not ready yet).
    int16_t times[MAX_POINTS];
    uint8_t brights[MAX_POINTS];
    uint8_t count = 0;
    for (uint8_t i = 0; i < _numPoints; i++)
    {
        int16_t t = pointTimeMinutes(_points[i]);
        if (t < 0) continue;
        times[count] = t;
        brights[count] = _points[i].brightness;
        count++;
    }
    if (count == 0) return 0;
    if (count == 1) return brights[0];

    // Find prev (largest time ≤ nowMin) and next (smallest time > nowMin),
    // wrap-aware.
    int8_t prevIdx = -1, nextIdx = -1;
    uint16_t prevDelta = 0xFFFF, nextDelta = 0xFFFF;
    for (uint8_t i = 0; i < count; i++)
    {
        if (times[i] <= (int16_t)nowMin)
        {
            uint16_t d = (uint16_t)nowMin - (uint16_t)times[i];
            if (d < prevDelta) { prevDelta = d; prevIdx = i; }
        }
        else
        {
            uint16_t d = (uint16_t)times[i] - (uint16_t)nowMin;
            if (d < nextDelta) { nextDelta = d; nextIdx = i; }
        }
    }
    if (prevIdx == -1)
    {
        // No point ≤ nowMin → previous is the latest point of the previous day
        uint16_t latestTime = 0;
        for (uint8_t i = 0; i < count; i++)
            if ((uint16_t)times[i] >= latestTime) { latestTime = (uint16_t)times[i]; prevIdx = i; }
    }
    if (nextIdx == -1)
    {
        // No point > nowMin → next is the earliest point of the next day
        uint16_t earliestTime = 1440;
        for (uint8_t i = 0; i < count; i++)
            if ((uint16_t)times[i] <= earliestTime) { earliestTime = (uint16_t)times[i]; nextIdx = i; }
    }

    if (prevIdx == nextIdx) return brights[prevIdx];

    uint16_t prevTime = (uint16_t)times[prevIdx];
    uint16_t nextTime = (uint16_t)times[nextIdx];
    uint16_t span = (nextTime > prevTime) ? (nextTime - prevTime) : (1440 + nextTime - prevTime);
    uint16_t pos  = (nowMin >= prevTime)  ? (nowMin - prevTime)  : (1440 + nowMin - prevTime);
    if (span == 0) return brights[prevIdx];

    int32_t deltaB = (int32_t)brights[nextIdx] - (int32_t)brights[prevIdx];
    int32_t level = (int32_t)brights[prevIdx] + (deltaB * (int32_t)pos) / (int32_t)span;
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    return (uint8_t)level;
}

void Schedule::publishStatus(bool active)
{
    if (active == _lastStatusPublished) return;
    KoLED_SC_ChScheduleStatus.value(active, DPT_State);
    _lastStatusPublished = active;
}
