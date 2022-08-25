#include "hal.h"

#include <Arduino.h>
#include <M5Atom.h>

namespace {

// The motor ID to OutputPwm (see RunMotor() in control.cpp):
//
// (front)
// 3 1
// 0 2
//
// Ginhibari 3 board:
// (front)
// G22 G23
// G19 G33
constexpr int kMotorPins[] = {19, 23, 33, 22};
constexpr int kPinLed = 27;
constexpr int kPinBatt = 32;  // 1/2 batt voltage
constexpr int kMotorPWMFreq = 10000;
constexpr int kMotorPWMBits = 8;
const CRGB kGreen = CRGB(255, 0, 0);
const CRGB kRed = CRGB(0, 255, 0);
const CRGB kBlack = CRGB(0, 0, 0);
constexpr int kNumLed = 25;

}  // namespace

void SetupPwm() {
  for (int i = 0; i < 4; i++) {
    ledcSetup(i, kMotorPWMFreq, kMotorPWMBits);
    ledcAttachPin(kMotorPins[i], i);
    ledcWrite(i, 0);
  }
  pinMode(kPinBatt, INPUT);

  for (int i = 0; i < kNumLed; i++) {
    M5.dis.drawpix(i, kBlack);
  }
  ShowLED(DisplayState::READY);
}

void OutputPwm(int id, int duty) {
  // duty is 10-bit, PWM is 8-bit
  ledcWrite(id, duty >> (10 - kMotorPWMBits));
}

int GetBattAD() { return analogRead(kPinBatt); }

void ShowLED(DisplayState state) {
  CRGB color = kBlack;
  switch (state) {
    case DisplayState::READY:
      color = kGreen;
      break;
    case DisplayState::CALIBRATING:
      color = kRed;
      break;
  }
  M5.dis.drawpix(0, 0, color);
  M5.dis.drawpix(4, 0, color);
  M5.dis.drawpix(0, 4, color);
  M5.dis.drawpix(4, 4, color);
}
