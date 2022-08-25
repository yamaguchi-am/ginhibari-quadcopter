#ifndef IMU_H_
#define IMU_H_

struct IMUData {
  float yaw;
  float pitch;
  float roll;
  int rotation[3];
  int accel[3];
};

void SetupIMU();
bool ImuTask(float sampleInterval, IMUData* result);
void StartCalibration();
bool GetCalibrationState();

#endif  // IMU_H_
