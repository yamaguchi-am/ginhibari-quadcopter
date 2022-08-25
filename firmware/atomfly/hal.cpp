#include "hal.h"

#include <Arduino.h>

#include "AtomFly.h"

namespace {

// The motor ID to OutputPwm (see RunMotor() in control.cpp):
//
// (front)
// 3 1
// 0 2
//
// AtomFly board:
// (front)
// 0 1
// 2 3
constexpr int kMotorIdMap[] = {2, 1, 3, 0};
constexpr int kPinLed = 27;
const int kMotorPWMFreq = 10000;
const int kMotorPWMBits = 8;
const CRGB kGreen = CRGB(255, 0, 0);
const CRGB kRed = CRGB(0, 255, 0);
const CRGB kBlack = CRGB(0, 0, 0);

}  // namespace

void SetupPwm() {
  fly.begin();
  fly.initFly();

  // FastLED also worked, but using M5.dis following the example.
  // FastLED.addLeds<WS2812, kPinLed>(leds, kNumLed);
  // FastLED.setBrightness(20);
  ShowLED(DisplayState::READY);
}

void OutputPwm(int id, int duty) {
  // duty is 10-bit, PWM is 8-bit
  fly.WritePWM(kMotorIdMap[id], duty >> (10 - kMotorPWMBits));
}

int GetBattAD() {
  // AtomFly does not have circuit to watch battery voltage
  return 2048;
}

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
  M5.dis.drawpix(0, color);
}
