#pragma once
#include <stdint.h>
// Minimal boundary for the real status-output template: no Arduino or bus hardware needed.
struct Dpt { unsigned main, sub; };
constexpr Dpt DPT_State{1, 1};
constexpr Dpt DPT_Value_1_Ucount{5, 10};
namespace TestClock { inline uint32_t now = 0; }
inline uint32_t delayTimerInit() { return TestClock::now; }
inline bool delayCheckMillis(uint32_t since, uint32_t interval) { return TestClock::now - since >= interval; }
struct GroupObject
{
    unsigned sent = 0;
    uint32_t stored = 0;
    uint32_t value(Dpt) const { return stored; }
    template <typename T> bool value(T next, Dpt) { stored = next; ++sent; return true; }
    template <typename T> bool valueNoSend(T next, Dpt) { stored = next; return true; }
};
