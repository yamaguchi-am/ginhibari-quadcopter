#include "storage.h"

#include <EEPROM.h>

#define EEPROM_SIZE 1024

namespace {

void SkipNewline() {
  if (!Serial.available()) {
    return;
  }
  int c = Serial.peek();
  while (Serial.available() && c == '\r' || c == '\n') {
    Serial.read();
    c = Serial.peek();
  }
}

void InputLine(char* buf, int n) {
  int i;
  for (i = 0; i < n - 1; i++) {
    while (!Serial.available()) {
    }
    int x = Serial.read();
    if (x == '\r' || x == '\n') {
      break;
    }
    Serial.print((char)x);
    buf[i] = x;
  }
  buf[i] = 0;
  Serial.println();
  SkipNewline();
}

bool IsTerminated(const char* buf, size_t n) {
  for (int i = 0; i < n; i++) {
    if (buf[i] == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool LoadWifiConfig(WifiConfig* cfg) {
  int sum = 0;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, *cfg);
  unsigned char cs = cfg->checksum;
  Serial.print("stored checksum: ");
  Serial.println((int)cs);
  unsigned char actual_sum = cfg->CalculateChecksum();
  Serial.print("data checksum: ");
  Serial.println((int)actual_sum);
  if (actual_sum != cs) {
    Serial.println("checksum mismatch");
    cfg->Reset();
    return false;
  }
  if (!cfg->Valid()) {
    Serial.println("string not terminated");
    cfg->Reset();
    return false;
  }
  return true;
}

bool SaveWifiConfig(WifiConfig& config) {
  int sum = 0;
  int addr = 0;
  config.checksum = config.CalculateChecksum();
  EEPROM.put(0, config);
  EEPROM.commit();
  Serial.println("Saved.");
  return true;
}

void ConfigureWifiClient(WifiConfig* config) {
  Serial.print("SSID=");
  InputLine(config->ssid, MAX_SSID_LENGTH);
  Serial.print("password=");
  InputLine(config->password, MAX_PASSWORD_LENGTH);
  SaveWifiConfig(*config);
}

int WifiConfig::CalculateChecksum() const {
  int sum = 0;
  for (int i = 0; i < MAX_SSID_LENGTH; i++) {
    sum += (unsigned char)ssid[i];
  }
  for (int i = 0; i < MAX_PASSWORD_LENGTH; i++) {
    sum += (unsigned char)password[i];
  }
  return sum & 0xff;
}

bool WifiConfig::Valid() const {
  if (!IsTerminated(ssid, MAX_SSID_LENGTH)) {
    return false;
  }
  if (!IsTerminated(password, MAX_PASSWORD_LENGTH)) {
    return false;
  }
  return true;
}

void WifiConfig::Reset() {
  ssid[0] = 0;
  password[0] = 0;
}
