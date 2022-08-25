#include "wifi.h"

#include <EEPROM.h>
#include <WiFi.h>

#include "AsyncUDP.h"

namespace {

AsyncUDP udp;
CommCallback cb_;

}  // namespace

bool ConnectToAP(const char* ssid, const char* password) {
  Serial.print("Connecting to WiFi AP [");
  Serial.print(ssid);
  Serial.println("]...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (Serial.available()) {
      WiFi.disconnect();
      Serial.read();
      Serial.println("interrupted");
      return false;
    }
  }
  Serial.println("Connected");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());
  return true;
}

void onPacket(AsyncUDPPacket packet) {
  int returnSize;
  uint8_t returnPacket[256];
  cb_(millis(), packet.data(), packet.length(), returnPacket, &returnSize);
  if (returnSize > 0) {
    packet.write(returnPacket, returnSize);
  }
}

void SetupUdpReceiver(CommCallback cb) {
  if (udp.listen(1234)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    cb_ = cb;
    udp.onPacket(onPacket);
  }
}

void PrintIPAddress() { Serial.println(WiFi.localIP()); }
