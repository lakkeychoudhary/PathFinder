#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <Arduino.h>
#include "pin_definitions.h"
#include "imu.h"
#include "audio.h"

extern float targetX, targetY;
extern float currentX, currentY;
extern float targetHeading;
extern bool isNavigating;
extern int baseSpeed;
extern int baseSpeed;
extern float kp_yaw;

// Odometry Calibration constants
#define VELOCITY_SCALE    0.002  // Coordinate units per PWM unit per second
#define YAW_RAD(deg)      ((deg) * M_PI / 180.0)

void updateOdometry(int leftPWM, int rightPWM, float dt) {
  // Translate current speed outputs to average linear speed
  float speedLeft = leftPWM;
  float speedRight = rightPWM;
  
  float linearSpeed = (speedLeft + speedRight) * 0.5 * VELOCITY_SCALE;
  
  // Calculate relative step based on yaw heading
  float rad = YAW_RAD(currentYaw);
  float dx = linearSpeed * cos(rad) * dt;
  float dy = linearSpeed * sin(rad) * dt;
  
  currentX += dx;
  currentY += dy;
}

void processPathfinding() {
  if (!isNavigating) return;

  // Calculate distance to target waypoint
  float dx = targetX - currentX;
  float dy = targetY - currentY;
  float distance = sqrt(dx * dx + dy * dy);

  // Check if target reached (tolerance of 0.2 coordinate units)
  if (distance < 0.2) {
    isNavigating = false;
    // Play target reached audio track
    playTrack(TRACK_GOAL_REACHED);
    Serial.println("Target reached successfully. Halting.");
    return;
  }

  // Calculate target angle heading (relative to starting heading 0)
  float angleRad = atan2(dy, dx);
  float angleDeg = angleRad * 180.0 / M_PI;

  // Update target heading for the yaw lock controller
  targetHeading = angleDeg;
}

#endif
