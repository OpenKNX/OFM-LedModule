// Phase A + B — Uhrzeitabhängiges Dimmen (per-channel time-based dimming evaluator).
// MDT 4.4.14 parity: continuous tracking of an N-point linear-interpolated
// brightness curve. Stützpunkt times are either fixed clock hours OR
// sunrise/sunset-relative offsets (channel-level mode).
#pragma once

#include "OpenKNX.h"
#include "HWDimmer/HWDimmer.h"
#include <stdint.h>

class SingleChannel;

class Schedule
{
  public:
    Schedule(uint8_t channelIndex, SingleChannel *pChannel, HWDimmer *pDimmer);

    void setup();
    void loop();

    // Returns true if the KO was handled (Schedule_Active).
    bool processInputKo(GroupObject &ko, int16_t relKo);

    // Manual override: pause tracking; resume after fallback minutes (if set).
    void notifyManualChange();

    // Lamp turned off externally: don't actively re-write while off, resume on next ON.
    void notifyLampOff();

    bool isActive() const { return _active && !_suspendedByManual; }

  private:
    static constexpr uint8_t MAX_POINTS = 10;

    enum class OffBehavior : uint8_t
    {
        SequenzStoppen = 0,
        Ausschalten = 1
    };

    enum class Schaltzeiten : uint8_t
    {
        Uhrzeit = 0,
        Sonne = 1
    };

    // For Schaltzeiten=Uhrzeit: timeByte holds hour (0..23).
    // For Schaltzeiten=Sonne:    timeByte holds enum 0..33; decoded via
    //   isSunrise() + sunOffsetMinutes() (uses SUN_OFFSETS[idx % 17]).
    struct StuetzPunkt
    {
        uint8_t timeByte;
        uint8_t brightness;  // 0..100 percent
    };

    uint8_t _channelIndex;
    SingleChannel *_pChannel;
    HWDimmer *_pDimmer;

    // ETS-loaded parameters
    StuetzPunkt _points[MAX_POINTS];
    uint8_t _numPoints = 4;
    OffBehavior _offBehavior = OffBehavior::Ausschalten;
    uint16_t _fallbackMinutes = 0;
    Schaltzeiten _schaltzeiten = Schaltzeiten::Uhrzeit;

    // Runtime state
    bool _active = true;
    bool _suspendedByManual = false;
    uint32_t _suspendedSinceMillis = 0;
    int16_t _lastLevelSent = -1;
    uint32_t _lastEvalMillis = 0;
    bool _lastStatusPublished = false;

    // Returns minutes-since-midnight (0..1439) for a stützpunkt under current
    // Schaltzeiten mode. For Sonne mode, returns -1 if the sun calc isn't valid
    // yet (caller must skip the point).
    int16_t pointTimeMinutes(const StuetzPunkt &p) const;

    // Computes interpolated brightness 0..100 for nowMin (minutes-since-midnight),
    // wrap-aware. Returns 0 if no valid points.
    uint8_t computeInterpolatedLevel(uint16_t nowMin) const;

    // Send Schedule_Status KO if state changed.
    void publishStatus(bool active);
};
