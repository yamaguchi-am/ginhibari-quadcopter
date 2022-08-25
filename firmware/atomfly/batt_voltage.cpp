#include "batt_voltage.h"

/*
 This is a stub implementation.
 AtomFly does not have circuit to measure battery voltage.
*/

namespace {
    constexpr double kSlope = 1.8659;
    constexpr double kOffset = -147.0441;
}

float ToBattVoltageMv(int adc) {
    return adc * kSlope + kOffset;
}
