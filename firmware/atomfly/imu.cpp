#include "imu.h"

#include "AtomFly.h"
#include "M5Atom.h"
#include "MadgwickAHRS.h"

/*
  Atom MPU6886 (USB connector and LEDs facing to you)
  x: left+ right-
  y: front+ back-
  z: down+ up-

  Remap to the robot:
  x: front+ (+Y)
  y: left+ (+X)
  z: up+ (-Z)
*/

namespace {

static struct {
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

void Calibrate() {
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

void AddCalibPoint(float x, float y, float z) {
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
    Calibrate();
  }
}

void GetGyroData(float *gyroX, float *gyroY, float *gyroZ) {
  float x, y, z;
  fly.getGyroData(&x, &y, &z);  // [deg/s]
  *gyroX = y;
  *gyroY = -x;
  *gyroZ = z;
}

void GetAccelData(float *accX, float *accY, float *accZ) {
  float x, y, z;
  fly.getAccelData(&x, &y, &z);  // [g]
  *accX = y;
  *accY = -x;
  *accZ = z;
}

}  // namespace

void SetupIMU() {
  // M5.IMU.Init();
  fly.SetGyroFsr(AtomFly::GFS_2000DPS);
  fly.SetAccelFsr(AtomFly::AFS_2G);
  calib_data.count = 0;
  calib_data.ready = false;
}

void StartCalibration() {
  calib_data.count = 0;
  calib_data.ready = false;
}

bool GetCalibrationState() { return calib_data.ready; }

bool ImuTask(float sampleInterval, IMUData *result) {
  float gyroX, gyroY, gyroZ;
  GetGyroData(&gyroX, &gyroY, &gyroZ);  // [deg/s]
  AddCalibPoint(gyroX, gyroY, gyroZ);
  gyroX -= origin.gyro_x;
  gyroY -= origin.gyro_y;
  gyroZ -= origin.gyro_z;
  float accX, accY, accZ;
  GetAccelData(&accX, &accY, &accZ);

  // TODO: change to use deg/s unit
  // emulate full-range=250 degrees/s | 131 LSB/deg/s
  const float f = 100;

  result->rotation[0] = gyroX * f;
  result->rotation[1] = gyroY * f;
  result->rotation[2] = gyroZ * f;
  result->accel[0] = accX * 1000.0;
  result->accel[1] = accY * 1000.0;
  result->accel[2] = accZ * 1000.0;

  // Madgwick.begin() just sets the sampling frequency used in updateIMU().
  madgwick.begin(1.0f / sampleInterval);
  madgwick.updateIMU(gyroX, gyroY, gyroZ, accX, accY, accZ);
  result->pitch = madgwick.getPitchRadians();
  result->roll = madgwick.getRollRadians();
  result->yaw = madgwick.getYawRadians();
  return true;
}
