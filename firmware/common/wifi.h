#ifndef WIFI_H_
#define WIFI_H_

typedef void (*CommCallback)(long current_millis, unsigned char[], int,
                             unsigned char* retData, int* retSize);

bool ConnectToAP(const char* ssid, const char* password);
void PrintIPAddress();
void SetupUdpReceiver(CommCallback cb);

#endif  // WIFI_H_
