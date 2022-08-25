#include "batt_voltage.h"

namespace {
constexpr double kSlope = 1.8659;
constexpr double kOffset = -147.0441;
}  // namespace

float ToBattVoltageMv(int adc) { return adc * kSlope + kOffset; }
