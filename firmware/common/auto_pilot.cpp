#include "auto_pilot.h"

int AutoPilot::GetThrottle(long time_millis, int manual_throttle) {
  if (!low_battery_) {
    return manual_throttle;
  }
  if (time_millis > low_battery_begin_time_ + descend_time_millis_) {
    return 0;
  }
  if (manual_throttle > throttle_for_landing_) {
    return throttle_for_landing_;
  }
  return manual_throttle;
}
