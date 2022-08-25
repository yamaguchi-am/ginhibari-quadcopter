#include "M5Atom.h"
#include "communication.h"
#include "control.h"
#include "hal.h"
#include "imu.h"
#include "storage.h"
#include "wifi.h"

void setup() {
  M5.begin(true, false, true);
  WifiConfig cfg;
  if (!LoadWifiConfig(&cfg)) {
    Serial.println("EEPROM data is invalid");
  }
  SetupPwm();
  while (!ConnectToAP(cfg.ssid, cfg.password)) {
    ConfigureWifiClient(&cfg);
  }
  SetupUdpReceiver(OnUdpReceived);
  SetupIMU();
  InitRegs();
}

void loop() {
  ControlTask();
  M5.update();
}
