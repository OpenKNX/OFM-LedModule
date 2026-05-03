// Phase A — Uhrzeitabhängiges Dimmen (per-channel time-based dimming evaluator).
// MDT 4.4.14 parity: continuous tracking of an N-point linear-interpolated
// brightness curve over the day. Active while lamp is on. Manual dim/switch
// pauses tracking; optional fallback time auto-resumes.
#pragma once

#include "OpenKNX.h"
#include "HWDimmer/HWDimmer.h"
#include <stdint.h>

class SingleChannel;  // forward declaration

class Schedule
{
  public:
    Schedule(uint8_t channelIndex, SingleChannel *pChannel, HWDimmer *pDimmer);

    // Read parameters from ETS, initialize state. Call once after construction.
    void setup();

    // Tick once per loop. Internally throttled to 1 Hz.
    void loop();

    // Channel's processInputKo passes here for relative-KO offsets we own
    // (LED_SC_KoChScheduleActive). Returns true if KO was handled.
    bool processInputKo(GroupObject &ko, int16_t relKo);

    // Called by the channel when a manual KO arrives that should pause tracking
    // (Brightness absolute, Dim relative, Scene). NOT called for Switch ON.
    void notifyManualChange();

    // Called by the channel when a Switch OFF KO arrives. Schedule will not
    // actively write while lamp is off; will resume when lamp is switched on
    // again (next setup/loop tick after Switch ON).
    void notifyLampOff();

    bool isActive() const { return _active && !_suspendedByManual; }

  private:
    static constexpr uint8_t MAX_POINTS = 10;

    enum class OffBehavior : uint8_t
    {
        SequenzStoppen = 0,
        Ausschalten = 1
    };

    struct StuetzPunkt
    {
        uint8_t mode;        // Phase A: only 0 = Fixed
        uint8_t hour;        // 0..23
        uint8_t minute;      // 0/5/10/.../55
        uint8_t brightness;  // 0..100 percent
        uint16_t timeMin() const { return hour * 60 + minute; }
    };

    uint8_t _channelIndex;
    SingleChannel *_pChannel;
    HWDimmer *_pDimmer;

    // ETS-loaded parameters
    StuetzPunkt _points[MAX_POINTS];
    uint8_t _numPoints = 4;
    OffBehavior _offBehavior = OffBehavior::Ausschalten;
    uint16_t _fallbackMinutes = 0;

    // Runtime state
    bool _active = true;
    bool _suspendedByManual = false;
    uint32_t _suspendedSinceMillis = 0;
    int16_t _lastLevelSent = -1;
    uint32_t _lastEvalMillis = 0;
    bool _lastStatusPublished = false;

    // Returns 0..100 percent for the given minutes-since-midnight, linear-
    // interpolated between the two bracketing active points (wrap-aware).
    uint8_t computeInterpolatedLevel(uint16_t nowMin) const;

    // Sends Schedule_Status KO if state changed since last publish.
    void publishStatus(bool active);
};
