#ifndef IMU_H
#define IMU_H

#include <Wire.h>
#include <Preferences.h>
#include "pin_definitions.h"

extern const int MPU_addr;
extern float currentYaw;
extern float currentPitch;
extern float currentRoll;
extern float gyroBiasZ;
extern unsigned long lastImuTime;

Preferences preferences;

void initMPU() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // wake up MPU6050
  Wire.endTransmission(true);

  // Set Gyro Full Scale Range to +-250 deg/s
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1B); // GYRO_CONFIG
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Set Accel Full Scale Range to +-2g
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C); // ACCEL_CONFIG
  Wire.write(0x00);
  Wire.endTransmission(true);
}

void calibrateIMU() {
  Serial.println("Calibrating IMU... Keep vehicle flat and stationary.");
  long sumGyZ = 0;
  int samples = 500;

  for (int i = 0; i < samples; i++) {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x47); // GYRO_ZOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 2, true);
    
    int16_t rawGyZ = Wire.read() << 8 | Wire.read();
    sumGyZ += rawGyZ;
    delay(3);
  }

  gyroBiasZ = (float)sumGyZ / samples;

  // Save to Preferences (Flash)
  preferences.begin("calibration", false);
  preferences.putFloat("gyroBiasZ", gyroBiasZ);
  preferences.end();

  Serial.print("Calibration complete. Gyro Z Bias: ");
  Serial.println(gyroBiasZ);
}

void loadCalibration() {
  preferences.begin("calibration", true);
  gyroBiasZ = preferences.getFloat("gyroBiasZ", 0.0);
  preferences.end();
  Serial.print("Loaded Gyro Z Bias from flash: ");
  Serial.println(gyroBiasZ);
}

void updateIMU() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); // Start with ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);

  int16_t rawAcX = Wire.read() << 8 | Wire.read();
  int16_t rawAcY = Wire.read() << 8 | Wire.read();
  int16_t rawAcZ = Wire.read() << 8 | Wire.read();
  
  int16_t rawTemp = Wire.read() << 8 | Wire.read(); // Skip temp
  
  int16_t rawGyX = Wire.read() << 8 | Wire.read();
  int16_t rawGyY = Wire.read() << 8 | Wire.read();
  int16_t rawGyZ = Wire.read() << 8 | Wire.read();

  unsigned long now = micros();
  float dt = (now - lastImuTime) / 1000000.0;
  lastImuTime = now;

  // Accel conversions
  float ax = rawAcX / 16384.0;
  float ay = rawAcY / 16384.0;
  float az = rawAcZ / 16384.0;

  // Calculate Pitch and Roll from Accelerometer
  float pitchAcc = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / M_PI;
  float rollAcc = atan2(-ax, az) * 180.0 / M_PI;

  // Gyro conversions (LSB sensitivity is 131 LSB/deg/s for +-250deg/s range)
  float gx = rawGyX / 131.0;
  float gy = rawGyY / 131.0;
  float gz = (rawGyZ - gyroBiasZ) / 131.0;

  // Complementary filter for Pitch and Roll
  currentPitch = 0.96 * (currentPitch + gx * dt) + 0.04 * pitchAcc;
  currentRoll = 0.96 * (currentRoll + gy * dt) + 0.04 * rollAcc;

  // Yaw calculation (Pure integration, gyro drift managed during straight line checks)
  if (abs(gz) > 0.15) { // Deadzone filter to suppress sensor noise
    currentYaw += gz * dt;
  }
}

#endif
