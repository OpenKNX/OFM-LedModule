#include "Schedule.h"
#include "SingleChannel.h"
#include "knxprod.h"
#include <knx.h>

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

    _points[0].mode       = ParamLED_SC_ChSchedulePoint1_Mode;
    _points[0].hour       = ParamLED_SC_ChSchedulePoint1_Hour;
    _points[0].minute     = ParamLED_SC_ChSchedulePoint1_Minute;
    _points[0].brightness = ParamLED_SC_ChSchedulePoint1_Brightness;

    _points[1].mode       = ParamLED_SC_ChSchedulePoint2_Mode;
    _points[1].hour       = ParamLED_SC_ChSchedulePoint2_Hour;
    _points[1].minute     = ParamLED_SC_ChSchedulePoint2_Minute;
    _points[1].brightness = ParamLED_SC_ChSchedulePoint2_Brightness;

    _points[2].mode       = ParamLED_SC_ChSchedulePoint3_Mode;
    _points[2].hour       = ParamLED_SC_ChSchedulePoint3_Hour;
    _points[2].minute     = ParamLED_SC_ChSchedulePoint3_Minute;
    _points[2].brightness = ParamLED_SC_ChSchedulePoint3_Brightness;

    _points[3].mode       = ParamLED_SC_ChSchedulePoint4_Mode;
    _points[3].hour       = ParamLED_SC_ChSchedulePoint4_Hour;
    _points[3].minute     = ParamLED_SC_ChSchedulePoint4_Minute;
    _points[3].brightness = ParamLED_SC_ChSchedulePoint4_Brightness;

    _points[4].mode       = ParamLED_SC_ChSchedulePoint5_Mode;
    _points[4].hour       = ParamLED_SC_ChSchedulePoint5_Hour;
    _points[4].minute     = ParamLED_SC_ChSchedulePoint5_Minute;
    _points[4].brightness = ParamLED_SC_ChSchedulePoint5_Brightness;

    _points[5].mode       = ParamLED_SC_ChSchedulePoint6_Mode;
    _points[5].hour       = ParamLED_SC_ChSchedulePoint6_Hour;
    _points[5].minute     = ParamLED_SC_ChSchedulePoint6_Minute;
    _points[5].brightness = ParamLED_SC_ChSchedulePoint6_Brightness;

    _points[6].mode       = ParamLED_SC_ChSchedulePoint7_Mode;
    _points[6].hour       = ParamLED_SC_ChSchedulePoint7_Hour;
    _points[6].minute     = ParamLED_SC_ChSchedulePoint7_Minute;
    _points[6].brightness = ParamLED_SC_ChSchedulePoint7_Brightness;

    _points[7].mode       = ParamLED_SC_ChSchedulePoint8_Mode;
    _points[7].hour       = ParamLED_SC_ChSchedulePoint8_Hour;
    _points[7].minute     = ParamLED_SC_ChSchedulePoint8_Minute;
    _points[7].brightness = ParamLED_SC_ChSchedulePoint8_Brightness;

    _points[8].mode       = ParamLED_SC_ChSchedulePoint9_Mode;
    _points[8].hour       = ParamLED_SC_ChSchedulePoint9_Hour;
    _points[8].minute     = ParamLED_SC_ChSchedulePoint9_Minute;
    _points[8].brightness = ParamLED_SC_ChSchedulePoint9_Brightness;

    _points[9].mode       = ParamLED_SC_ChSchedulePoint10_Mode;
    _points[9].hour       = ParamLED_SC_ChSchedulePoint10_Hour;
    _points[9].minute     = ParamLED_SC_ChSchedulePoint10_Minute;
    _points[9].brightness = ParamLED_SC_ChSchedulePoint10_Brightness;

    _active = true;
    _suspendedByManual = false;
    _lastLevelSent = -1;
    _lastEvalMillis = 0;
    publishStatus(true);
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
            _lastLevelSent = -1;  // force resend on next eval
            publishStatus(true);
        }
    }

    if (!_active || _suspendedByManual) return;

    // Don't actively re-write while lamp is off — would turn it on unintentionally.
    // The Schedule resumes writing as soon as Switch ON brings the lamp back on.
    if (!_pChannel->isOn()) return;

    // Throttle re-evaluation to 1 Hz. Linear interpolation produces sub-percent
    // changes per second over multi-hour spans; faster ticks add load without
    // perceptible benefit.
    if (millis() - _lastEvalMillis < 1000) return;
    _lastEvalMillis = millis();

    if (!openknx.time.isValid()) return;
    auto tinfo = openknx.time.getLocalTime();
    uint16_t nowMin = (uint16_t)tinfo.hour * 60 + tinfo.minute;

    uint8_t newLevel = computeInterpolatedLevel(nowMin);
    if ((int16_t)newLevel != _lastLevelSent)
    {
        // Write via SingleChannel::setBrightness — same path as KO_Brightness.
        // Goes through Min/Max-clamp + ramp; respects Sperrfunktion via getLock().
        // setBrightness will set _sceneNumberActive=0 (intended — schedule is
        // "active source", same as a manual brightness command).
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
            // Per OffBehavior: stop tracking only, OR turn lamp off entirely
            if (_offBehavior == OffBehavior::Ausschalten)
                _pChannel->setSwitch(false);
            publishStatus(false);
        }
        else
        {
            // Re-arming clears manual-suspend state.
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
    if (!_active) return;  // already inactive, nothing to suspend
    if (_fallbackMinutes == 0)
    {
        // No automatic fallback — pause indefinitely until Schedule_Active KO is re-sent.
        _suspendedByManual = true;
        _suspendedSinceMillis = millis();
        publishStatus(false);
    }
    else
    {
        _suspendedByManual = true;
        _suspendedSinceMillis = millis();
        publishStatus(false);
    }
}

void Schedule::notifyLampOff()
{
    // Lamp off: don't publish Status=0 (Schedule itself is still "active",
    // just waiting for lamp to come back on). Reset _lastLevelSent so next
    // resume forces a write.
    _lastLevelSent = -1;
}

uint8_t Schedule::computeInterpolatedLevel(uint16_t nowMin) const
{
    // Find the two bracketing points (largest a.timeMin ≤ nowMin, smallest
    // b.timeMin > nowMin). Wrap-aware: if no point ≤ nowMin, the "previous"
    // point is the one with the largest timeMin overall (yesterday). If no
    // point > nowMin, the "next" point is the smallest timeMin overall (today
    // wraps to tomorrow).

    int8_t prevIdx = -1;
    int8_t nextIdx = -1;
    uint16_t prevDelta = 0xFFFF;  // |nowMin - prev.timeMin| (positive)
    uint16_t nextDelta = 0xFFFF;  // |next.timeMin - nowMin| (positive)

    for (uint8_t i = 0; i < _numPoints; i++)
    {
        uint16_t pTime = _points[i].timeMin();
        if (pTime <= nowMin)
        {
            uint16_t d = nowMin - pTime;
            if (d < prevDelta)
            {
                prevDelta = d;
                prevIdx = i;
            }
        }
        else  // pTime > nowMin
        {
            uint16_t d = pTime - nowMin;
            if (d < nextDelta)
            {
                nextDelta = d;
                nextIdx = i;
            }
        }
    }

    // Wrap handling
    if (prevIdx == -1)
    {
        // No point ≤ nowMin → previous is the latest point of the previous day
        uint16_t latestTime = 0;
        for (uint8_t i = 0; i < _numPoints; i++)
        {
            if (_points[i].timeMin() >= latestTime)
            {
                latestTime = _points[i].timeMin();
                prevIdx = i;
            }
        }
    }
    if (nextIdx == -1)
    {
        // No point > nowMin → next is the earliest point of the next day
        uint16_t earliestTime = 1440;
        for (uint8_t i = 0; i < _numPoints; i++)
        {
            if (_points[i].timeMin() <= earliestTime)
            {
                earliestTime = _points[i].timeMin();
                nextIdx = i;
            }
        }
    }

    // If prev == next (only one point active, or all points identical), constant.
    if (prevIdx == nextIdx) return _points[prevIdx].brightness;

    // Compute span and position relative to prev, wrap-aware.
    uint16_t prevTime = _points[prevIdx].timeMin();
    uint16_t nextTime = _points[nextIdx].timeMin();
    uint16_t span = (nextTime > prevTime) ? (nextTime - prevTime)
                                          : (1440 + nextTime - prevTime);
    uint16_t pos = (nowMin >= prevTime) ? (nowMin - prevTime)
                                        : (1440 + nowMin - prevTime);

    if (span == 0) return _points[prevIdx].brightness;

    int32_t deltaB = (int32_t)_points[nextIdx].brightness - (int32_t)_points[prevIdx].brightness;
    int32_t level = (int32_t)_points[prevIdx].brightness + (deltaB * (int32_t)pos) / (int32_t)span;
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
