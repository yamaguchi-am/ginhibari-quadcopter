#include "control.h"

#include "auto_pilot.h"
#include "batt_voltage.h"
#include "communication.h"
#include "filter.h"
#include "hal.h"
#include "registers.h"
#include "wifi.h"

constexpr float kTrimScale = M_PI / 180 * 0.1;
constexpr int kControlLoopIntervalMicros = 5000;
constexpr int kBaseBatteryVoltage = 4200;
// All motor output is shut off when battery voltage is below this level.
constexpr int kShutoffVoltageMV = 2700;
constexpr int kBattFilterWindowSize = 100;

unsigned long last_millis = 0;
unsigned long next_micros = 0;
IMUData origin;
AutoPilot auto_pilot(0);
AverageFilter batt_filter(kBattFilterWindowSize);

namespace {

int ToReg(float radians) { return radians * 1800 / M_PI; }
float ToRadians(int reg) { return reg * M_PI / 1800; }

}  // namespace

void WriteMotorPWM(const struct Control& o) {
  int y[4];
  //     FRONT
  // CW  M4 M2 CCW
  // CCW M1 M3 CW
  //     REAR(switches and connectors)
  const int kMixing[4][4] = {
      {1, 1, 1, 1},    // throttle
      {1, 1, -1, -1},  // rudder   +CW -CCW
      {-1, 1, -1, 1},  // elevator +up -down
      {-1, 1, 1, -1}   // aileron  +left -right
  };
  for (int i = 0; i < 4; i++) {
    y[i] = o.throttle * kMixing[0][i] + o.rudder * kMixing[1][i] +
           o.elevator * kMixing[2][i] + o.aileron * kMixing[3][i];
    int maxOut = reg[LIMITTER];
    y[i] = max(0, min(maxOut, y[i]));
    OutputPwm(i, y[i]);
    reg[OUT_M0 + i] = y[i];
  }
}

void RunMotor(const struct Control& o, int batt_mv) {
  float amp;
  if (batt_mv < kShutoffVoltageMV) {
    amp = 0;
  } else {
    amp = static_cast<float>(kBaseBatteryVoltage) / batt_mv;
  }
  struct Control c;
  c.throttle = o.throttle * amp;
  c.rudder = o.rudder * amp;
  c.elevator = o.elevator * amp;
  c.aileron = o.aileron * amp;
  WriteMotorPWM(c);
}

// Adds PD feedback control output. Throttle is kept unchanged.
void Feedback(const IMUData& cur, const IMUData& origin,
              const YawPitchRoll& target, Control* output) {
  YawPitchRoll d = {
      (cur.rotation[2] - origin.rotation[2]) / 100000.f,
      (cur.rotation[1] - origin.rotation[1]) / 100000.f,
      (cur.rotation[0] - origin.rotation[0]) / 100000.f,
  };
  // Target angles are after adding trim offset.
  YawPitchRoll p = {
      cur.yaw - target.yaw,
      cur.pitch - target.pitch,
      cur.roll - target.roll,
  };
  if (p.yaw > M_PI) {
    p.yaw -= 2 * M_PI;
  }
  if (p.yaw < -M_PI) {
    p.yaw += 2 * M_PI;
  }
  if (p.roll > M_PI) {
    p.roll -= 2 * M_PI;
  }
  if (p.roll < -M_PI) {
    p.roll += 2 * M_PI;
  }
  output->rudder += p.yaw * reg[GAIN_YAW_P] + d.yaw * reg[GAIN_YAW_D];
  output->elevator += p.pitch * reg[GAIN_PITCH_P] + d.pitch * reg[GAIN_PITCH_D];
  output->aileron += p.roll * reg[GAIN_ROLL_P] + d.roll * reg[GAIN_ROLL_D];
}

// Test mode: Directly specfy motor duty(n/1000) by OUT_Mx registers.
void TestMode() {
  for (int i = 0; i < 4; i++) {
    OutputPwm(i, reg[OUT_M0 + i]);
  }
}

void InitRegs() {
  for (int i = 0; i < N_REGISTERS; i++) {
    reg[i] = 0;
  }
  // [(duty) * 1023 / radians]
  reg[GAIN_PITCH_P] = 0;
  reg[GAIN_PITCH_D] = 0;

  reg[TARGET_YAW] = 0;
  reg[TARGET_PITCH] = 0;
  reg[TARGET_ROLL] = 0;

  // angles in 10x degrees
  reg[TRIP_ANGLE_PR] = 300;
  reg[TRIP_ANGLE_YAW] = 300;

  // duty * 1023
  reg[LIMITTER] = 600;
  reg[TEST_MODE] = 0;
}

void ControlTask() {
  unsigned long begin_micros = micros();
  int batt_ad = GetBattAD();
  int batt_mv = ToBattVoltageMv(batt_ad);

  reg[BATT_AD] = batt_ad;
  reg[BATT_VOLTAGE] = batt_mv;

  auto_pilot.SetThrottleForLanding(reg[HOVERING_VOLTAGE]);
  auto_pilot.SetDescendTime(reg[DESCEND_TIME]);

  batt_filter.Put(batt_mv);
  float batt_mv_filtered;
  if (batt_filter.Get(&batt_mv_filtered)) {
    reg[BATT_VOLTAGE_FILTERED] = batt_mv_filtered;
    if (batt_mv_filtered < reg[LOW_BATTERY_THRESHOLD]) {
      // TODO: define status code
      reg[BATT_STATUS] = 1;
      auto_pilot.EnterLowBatteryMode(millis());
    } else {
      reg[BATT_STATUS] = 2;
    }
  }

  if (reg[JS_THROTTLE] == 0) {
    auto_pilot.Init();
  }

  IMUData imu;
  if (reg[CALIBRATE] == 1) {
    StartCalibration();
    reg[CALIBRATE] = 2;
    ShowLED(DisplayState::CALIBRATING);
  }
  if (reg[CALIBRATE] == 2) {
    if (GetCalibrationState() == true) {
      reg[CALIBRATE] = 0;
      ShowLED(DisplayState::READY);
    }
  }

  float sample_interval = kControlLoopIntervalMicros * 1.0e-6;
  if (ImuTask(sample_interval, &imu)) {
    reg[YAW_ANGLE] = ToReg(imu.yaw);
    reg[PITCH_ANGLE] = ToReg(imu.pitch);
    reg[ROLL_ANGLE] = ToReg(imu.roll);
    reg[ROTATION_X] = imu.rotation[0];
    reg[ROTATION_Y] = imu.rotation[1];
    reg[ROTATION_Z] = imu.rotation[2];
    reg[ACCEL_X] = imu.accel[0];
    reg[ACCEL_Y] = imu.accel[1];
    reg[ACCEL_Z] = imu.accel[2];
  }

  while (Serial.available()) {
    int c = Serial.read();
    switch (c) {
      case ' ':
        PrintIPAddress();
        break;
    }
  }
  Control ctrl = {0, 0, 0, 0};
  long current_millis = millis();
  if (CommTimedOut(current_millis)) {
    reg[ENABLE] = 0;
  }

  YawPitchRoll target;
  target.yaw = ToRadians(reg[TARGET_YAW]);
  target.pitch = reg[TRIM_PITCH] * kTrimScale + ToRadians(reg[JS_PITCH]);
  target.roll = reg[TRIM_ROLL] * kTrimScale + ToRadians(reg[JS_ROLL]);
  reg[TARGET_PITCH] = ToReg(target.pitch);
  reg[TARGET_ROLL] = ToReg(target.roll);

  // Throttle is directly given by the command.
  ctrl.throttle = auto_pilot.GetThrottle(current_millis, reg[JS_THROTTLE]);
  // Others are given by the PD feedback controller.
  ctrl.rudder = 0;
  ctrl.aileron = 0;
  ctrl.elevator = 0;

  // IMU feedback
  if (reg[JS_THROTTLE] > 0) {
    Control fb = {0, 0, 0, 0};
    Feedback(imu, origin, target, &fb);
    ctrl.rudder += fb.rudder;
    ctrl.elevator += fb.elevator;
    ctrl.aileron += fb.aileron;
  }

  reg[OUT_YAW] = ctrl.rudder;
  reg[OUT_PITCH] = ctrl.elevator;
  reg[OUT_ROLL] = ctrl.aileron;
  reg[OUT_THROTTLE] = ctrl.throttle;
  if (!reg[ENABLE]) {
    ctrl.throttle = 0;
    ctrl.rudder = 0;
    ctrl.elevator = 0;
    ctrl.aileron = 0;
  }
  if (reg[TEST_MODE] == 0xa5) {
    TestMode();
  } else {
    RunMotor(ctrl, batt_mv_filtered);
  }
  unsigned long current_micros = micros();
  reg[CTRL_INTERVAL] = current_micros - begin_micros;
  next_micros += kControlLoopIntervalMicros;
  if (micros() > next_micros) {
    next_micros = micros();
  }
  while (micros() < next_micros) {
    delayMicroseconds(10);
  }
  last_millis = current_millis;
  *((uint32_t*)&reg[ELAPSED_L]) = current_millis;
}
