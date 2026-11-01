#include <cassert>
#include <cstdio>
#include "Brightness.h"
#include "LedStatusOutput.h"
int main()
{
    using namespace LedBrightness;
    assert(knxToLevel(0) == 0);
    assert(knxToLevel(1) == 16);
    assert(knxToLevel(128) == 2056);
    assert(knxToLevel(255) == 4095);
    assert(parameterToLevel(0) == 0);
    assert(parameterToLevel(1) == 16);     // 0.4 % -> raw 1
    assert(parameterToLevel(25) == 418);   // 10 % -> raw 26 (half up)
    assert(parameterToLevel(125) == 2056); // 50 % -> raw 128 (half up)
    assert(parameterToLevel(188) == 3083); // 75.2 % -> raw 192
    assert(parameterToLevel(250) == 4095);
    for (unsigned raw = 0; raw <= 255; ++raw)
    {
        assert(levelToKnx(knxToLevel(raw)) == raw);
        if (raw > 0) assert(knxToLevel(raw) > knxToLevel(raw - 1));
    }
    for (unsigned step = 1; step <= 250; ++step)
    {
        assert(parameterToLevel(step) > parameterToLevel(step - 1));
        assert(parameterToLevel(step) <= 4095);
    }
    for (unsigned level = 0; level <= 4095; ++level)
    {
        const int error = int(knxToLevel(levelToKnx(level))) - int(level);
        assert(error >= -8 && error <= 8);
    }
    assert(parameterToKnx(1) == 1);
    assert(parameterToKnx(125) == 128);
    assert(parameterToKnx(250) == 255);
    GroupObject status;
    StatusValueState state;
    TestClock::now = 100;
    StatusOutput::sendValue<uint8_t>(status, DPT_Value_1_Ucount, true, false, 255, state, 0, 0, parameterToKnx(3), 0);
    assert(status.sent == 1 && status.stored == 255);
    StatusOutput::sendValue<uint8_t>(status, DPT_Value_1_Ucount, true, true, 254, state, 0, 0, parameterToKnx(3), 0);
    assert(status.sent == 1 && status.stored == 254); // refresh without transmission
    StatusOutput::sendValue<uint8_t>(status, DPT_Value_1_Ucount, true, true, 252, state, 0, 0, parameterToKnx(3), 0);
    assert(status.sent == 2 && status.stored == 252); // cumulative change reached 1.2 %
    StatusOutput::sendValue<uint8_t>(status, DPT_Value_1_Ucount, true, true, 251, state, 0, 0, parameterToKnx(3), 0);
    assert(status.sent == 2);
    StatusOutput::sendValue<uint8_t>(status, DPT_Value_1_Ucount, true, false, 251, state, 0, 0, parameterToKnx(3), 0);
    assert(status.sent == 3 && status.stored == 251); // final endpoint always transmitted
    TestClock::now += 1000;
    StatusOutput::sendValue<uint8_t>(status, DPT_Value_1_Ucount, true, false, 251, state, 1000, 100, 255, 0);
    assert(status.sent == 4); // cyclic report independent of change thresholds
    std::puts("PASS: 256 KNX values, 251 ETS steps, 4096 feedback levels and cumulative status thresholds");
}
