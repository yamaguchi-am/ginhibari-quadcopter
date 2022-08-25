#ifndef AUTO_PILOT_H_
#define AUTO_PILOT_H_

// Manages throttle control status for automated landing.
class AutoPilot {
 public:
  AutoPilot(int throttle_for_landing)
      : throttle_for_landing_(throttle_for_landing),
        low_battery_(false),
        descend_time_millis_(2000) {}

  void SetThrottleForLanding(int throttle_for_landing) {
    throttle_for_landing_ = throttle_for_landing;
  }

  void SetDescendTime(int millis) { descend_time_millis_ = millis; }

  void Init() { low_battery_ = false; }

  void EnterLowBatteryMode(long current_time_millis) {
    if (low_battery_) {
      return;
    }
    low_battery_begin_time_ = current_time_millis;
    low_battery_ = true;
  }

  // Limits the throttle value to enforce landing when the battery level is low.
  int GetThrottle(long current_time_millis, int manual_throttle);

 private:
  int throttle_for_landing_;
  int descend_time_millis_;
  bool low_battery_;
  long low_battery_begin_time_;
};

#endif  // AUTO_PILOT_H_
