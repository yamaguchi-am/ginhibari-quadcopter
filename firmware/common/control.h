#ifndef CONTROL_H_
#define CONTROL_H_

#include "imu.h"

struct Control {
  int throttle;
  int rudder;
  int elevator;
  int aileron;
};

typedef struct {
  float yaw, pitch, roll;
} YawPitchRoll;

void InitRegs();
void ControlTask();

#endif  // CONTROL_H_