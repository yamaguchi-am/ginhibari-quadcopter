#ifndef STORAGE_H_
#define STORAGE_H_

#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64

struct WifiConfig {
  bool valid;
  char ssid[MAX_SSID_LENGTH];
  char password[MAX_PASSWORD_LENGTH];

  unsigned char checksum;
  bool Valid() const;
  int CalculateChecksum() const;
  void Reset();
};

bool SaveWifiConfig(WifiConfig& config);
bool LoadWifiConfig(WifiConfig* cfg);
void ConfigureWifiClient(WifiConfig* config);

#endif  // STORAGE_H_
