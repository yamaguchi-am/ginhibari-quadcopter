#include "imu.h"

#include "M5Atom.h"
#include "MadgwickAHRS.h"

namespace {

struct {
  float gyro_x;
  float gyro_y;
  float gyro_z;
} origin;

#define BUFSIZE 1000
struct {
  float x[BUFSIZE];
  float y[BUFSIZE];
  float z[BUFSIZE];
  int count;
  bool ready;
} calib_data;

Madgwick madgwick;

void calibrate() {
  double sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < BUFSIZE; i++) {
    sumX += calib_data.x[i];
    sumY += calib_data.y[i];
    sumZ += calib_data.z[i];
  }
  origin.gyro_x = sumX / BUFSIZE;
  origin.gyro_y = sumY / BUFSIZE;
  origin.gyro_z = sumZ / BUFSIZE;
}

void addCalib(float x, float y, float z) {
  if (calib_data.ready) {
    return;
  }
  int &i = calib_data.count;
  calib_data.x[i] = x;
  calib_data.y[i] = y;
  calib_data.z[i] = z;
  i++;
  if (i >= BUFSIZE) {
    i = 0;
    calib_data.ready = true;
    calibrate();
  }
}

}  // namespace

void SetupIMU() {
  M5.IMU.Init();
  M5.IMU.SetGyroFsr(MPU6886::GFS_2000DPS);
  M5.IMU.SetAccelFsr(MPU6886::AFS_2G);
  calib_data.count = 0;
  calib_data.ready = false;
}

/*
  Configuration of MPU6886 in ATOM Matrix:
  (TOP view, with the USB connector directed to you)
  x: left+ right-
  y: front+ back-
  z: down+ up-

  We remap it to adjust to the commonly used coordinate system on a robot:
  x: front+ (+Y)
  y: left+ (+X)
  z: up+ (-Z)
*/
void getGyroData(float *gyroX, float *gyroY, float *gyroZ) {
  float x, y, z;
  M5.IMU.getGyroData(&x, &y, &z);  // [deg/s]
  *gyroX = y;
  *gyroY = x;
  *gyroZ = -z;
}

void getAccelData(float *accX, float *accY, float *accZ) {
  float x, y, z;
  M5.IMU.getAccelData(&x, &y, &z);  // [g]
  *accX = y;
  *accY = x;
  *accZ = -z;
}

void StartCalibration() {
  calib_data.count = 0;
  calib_data.ready = false;
}

bool GetCalibrationState() { return calib_data.ready; }

bool ImuTask(float sampleInterval, IMUData *result) {
  float gyroX, gyroY, gyroZ;
  getGyroData(&gyroX, &gyroY, &gyroZ);  // [deg/s]
  addCalib(gyroX, gyroY, gyroZ);
  gyroX -= origin.gyro_x;
  gyroY -= origin.gyro_y;
  gyroZ -= origin.gyro_z;
  float accX, accY, accZ;
  getAccelData(&accX, &accY, &accZ);

  // TODO: change to use deg/s unit
  // emulate full-range=250 degrees/s | 131 LSB/deg/s
  const float f = 100;

  result->rotation[0] = gyroX * f;
  result->rotation[1] = gyroY * f;
  result->rotation[2] = gyroZ * f;
  result->accel[0] = accX * 1000.0;
  result->accel[1] = accY * 1000.0;
  result->accel[2] = accZ * 1000.0;

  constexpr float kDegToRad = M_PI / 180;
  // Madgwick.begin() just sets the sampling frequency used in updateIMU().
  madgwick.begin(1.0f / sampleInterval);
  madgwick.updateIMU(gyroX, gyroY, gyroZ, accX, accY,
                             accZ);
  result->pitch = madgwick.getPitchRadians();
  result->roll = madgwick.getRollRadians();
  result->yaw = madgwick.getYawRadians();
  return true;
}
