/*
 * PathFinder - Intelligent Self-Driving Autonomous Vehicle
 * Developed by: lakkeychoudhary
 * 
 * Hardware:
 * - ESP32 Dev Board
 * - MPU6050 IMU (for Yaw Heading lock)
 * - L298N Motor Driver + 4x DC Gear Motors
 * - 3x HC-SR04 Ultrasonic Distance Sensors
 * - DFPlayer Mini MP3 Player
 * - Dual analog IR phototransistor target sensors
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "pin_definitions.h"

// Task handles for dual-core execution
TaskHandle_t Core0_Sensors_Handle;
TaskHandle_t Core1_Navigation_Handle;

// Web server on Port 80
WebServer server(80);

// MPU6050 I2C Address
const int MPU_addr = 0x68;

// Heading control (Gyro Yaw lock)
float targetHeading = 0.0;
float currentYaw = 0.0;
float yawError = 0.0;
float kp_yaw = 4.5; // Gain to correct motor speeds to run straight
unsigned long lastImuTime = 0;

// Navigation & coordinates
float targetX = 0.0, targetY = 0.0;
float currentX = 0.0, currentY = 0.0;
bool isNavigating = false;
int obstacleThreshold = 25; // cm

// Speed settings
int baseSpeed = 180; // 0-255 PWM range

// System states
enum SystemState {
  STATE_STANDBY,
  STATE_DRIVING,
  STATE_OBSTACLE_AVOIDANCE,
  STATE_BEACON_CHASING
};
SystemState currentState = STATE_STANDBY;

// Setup hardware serial for DFPlayer Mini
HardwareSerial AudioSerial(2);

// Forward declarations
void initMPU();
void updateYaw();
void setMotors(int leftSpeed, int rightSpeed);
void sendAudioCommand(uint8_t command, uint8_t dat1, uint8_t dat2);
long readSensor(int trigPin, int echoPin);
void handleRoot();
void handleSetTarget();

void setup() {
  Serial.begin(115200);
  AudioSerial.begin(9600, SERIAL_8N1, AUDIO_RX, AUDIO_TX);
  
  // Pin Configurations for Motor Driver
  pinMode(ENA_LEFT, OUTPUT);
  pinMode(IN1_LEFT, OUTPUT);
  pinMode(IN2_LEFT, OUTPUT);
  pinMode(IN3_RIGHT, OUTPUT);
  pinMode(IN4_RIGHT, OUTPUT);
  pinMode(ENB_RIGHT, OUTPUT);
  
  setMotors(0, 0); // Stop initially

  // Distance sensor pin modes
  pinMode(FRONT_TRIG, OUTPUT);
  pinMode(FRONT_ECHO, INPUT);
  pinMode(LEFT_TRIG, OUTPUT);
  pinMode(LEFT_ECHO, INPUT);
  pinMode(RIGHT_TRIG, OUTPUT);
  pinMode(RIGHT_ECHO, INPUT);

  // Initialize I2C and IMU
  Wire.begin(IMU_SDA, IMU_SCL, 400000);
  initMPU();

  // Create Local WiFi Access Point
  WiFi.softAP("PathFinder_Tesla_Mini", "admin12345");
  Serial.println("AP Created: PathFinder_Tesla_Mini");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Web endpoints
  server.on("/", handleRoot);
  server.on("/setTarget", HTTP_POST, handleSetTarget);
  server.begin();

  // Create background tasks
  xTaskCreatePinnedToCore(
    IMU_Sensor_Loop,
    "Sensor_Loop",
    4096,
    NULL,
    1,
    &Core0_Sensors_Handle,
    0 // Core 0 processes IMU gyro integration (Yaw tracking)
  );

  xTaskCreatePinnedToCore(
    Navigation_Loop,
    "Nav_Loop",
    4096,
    NULL,
    1,
    &Core1_Navigation_Handle,
    1 // Core 1 handles steering, obstacles, voice, and web server
  );

  // Voice confirmation: "I am here boss, now where else do you want me to go?" (Track 0001)
  delay(1000);
  sendAudioCommand(0x0F, 0x01, 0x01); 
}

void loop() {
  // Executed inside RTOS tasks
}

// ----------------------------------------------------
// CORE 0: IMU integration for Heading Angle (Yaw)
// ----------------------------------------------------
void IMU_Sensor_Loop(void * pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(10); // 100 Hz
  lastImuTime = micros();

  for(;;) {
    updateYaw();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ----------------------------------------------------
// CORE 1: Navigation Logic, Steering & Obstacle Avoidance
// ----------------------------------------------------
void Navigation_Loop(void * pvParameters) {
  for(;;) {
    server.handleClient();

    // Check distance sensors
    long frontDist = readSensor(FRONT_TRIG, FRONT_ECHO);
    long leftDist = readSensor(LEFT_TRIG, LEFT_ECHO);
    long rightDist = readSensor(RIGHT_TRIG, RIGHT_ECHO);

    if (frontDist < obstacleThreshold && frontDist > 0) {
      currentState = STATE_OBSTACLE_AVOIDANCE;
      setMotors(0, 0); // Stop
      
      // Play obstacle alert sound (Track 0002)
      sendAudioCommand(0x0F, 0x01, 0x02);
      vTaskDelay(pdMS_TO_TICKS(500));

      // Backup briefly
      setMotors(-150, -150);
      vTaskDelay(pdMS_TO_TICKS(800));
      setMotors(0, 0);

      // Pivot away from the obstacle
      if (leftDist > rightDist) {
        // Spin Left using Gyro Yaw offset
        targetHeading -= 90.0;
        setMotors(-150, 150);
      } else {
        // Spin Right
        targetHeading += 90.0;
        setMotors(150, -150);
      }
      
      vTaskDelay(pdMS_TO_TICKS(800)); // Allow turning execution time
      setMotors(0, 0);
      currentState = STATE_DRIVING;
    }

    // Target tracking / Beacon chasing
    int leftIR = analogRead(TRACKER_LEFT);
    int rightIR = analogRead(TRACKER_RIGHT);

    if (currentState == STATE_DRIVING || isNavigating) {
      if (leftIR > 800 || rightIR > 800) {
        // Target beacon detected! Chase it
        currentState = STATE_BEACON_CHASING;
        int diff = leftIR - rightIR;
        int steerCorrection = diff / 4;
        setMotors(baseSpeed + steerCorrection, baseSpeed - steerCorrection);
      } else {
        // Gyro-assisted straight line driving using Target Heading Lock
        yawError = targetHeading - currentYaw;
        int yawCorrection = yawError * kp_yaw;
        setMotors(baseSpeed - yawCorrection, baseSpeed + yawCorrection);
      }
    } else {
      setMotors(0, 0); // Standby
    }

    vTaskDelay(pdMS_TO_TICKS(50)); // 20 Hz loop
  }
}

// MPU6050 Helpers
void initMPU() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0);    // Wake up
  Wire.endTransmission(true);
}

void updateYaw() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x47); // GYRO_ZOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 2, true);
  
  int16_t GyZ = Wire.read()<<8|Wire.read();

  unsigned long now = micros();
  float dt = (now - lastImuTime) / 1000000.0;
  lastImuTime = now;

  // Convert raw Z gyro value to degrees/sec (FS_SEL = 0 -> 131 LSB/deg/s)
  float gyroYawRate = GyZ / 131.0;
  
  // Integrate gyro rate to get relative Yaw angle
  if (abs(gyroYawRate) > 0.1) { // Deadzone filter
    currentYaw += gyroYawRate * dt;
  }
}

// Drive Motor Interface
void setMotors(int leftSpeed, int rightSpeed) {
  // Constrain speeds to 8-bit PWM limits
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Left Motor control
  if (leftSpeed >= 0) {
    digitalWrite(IN1_LEFT, HIGH);
    digitalWrite(IN2_LEFT, LOW);
    analogWrite(ENA_LEFT, leftSpeed);
  } else {
    digitalWrite(IN1_LEFT, LOW);
    digitalWrite(IN2_LEFT, HIGH);
    analogWrite(ENA_LEFT, -leftSpeed);
  }

  // Right Motor control
  if (rightSpeed >= 0) {
    digitalWrite(IN3_RIGHT, HIGH);
    digitalWrite(IN4_RIGHT, LOW);
    analogWrite(ENB_RIGHT, rightSpeed);
  } else {
    digitalWrite(IN3_RIGHT, LOW);
    digitalWrite(IN4_RIGHT, HIGH);
    analogWrite(ENB_RIGHT, -rightSpeed);
  }
}

// DFPlayer Mini UART packet sender
void sendAudioCommand(uint8_t command, uint8_t dat1, uint8_t dat2) {
  uint8_t buffer[10] = { 0x7E, 0xFF, 0x06, command, 0x00, dat1, dat2, 0x00, 0x00, 0xEF };
  
  uint16_t sum = 0;
  for (int i=1; i<7; i++) {
    sum += buffer[i];
  }
  sum = -sum;
  buffer[7] = (uint8_t)(sum >> 8);
  buffer[8] = (uint8_t)(sum & 0xFF);
  
  for (int i=0; i<10; i++) {
    AudioSerial.write(buffer[i]);
  }
}

long readSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 25000); // 25ms timeout
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

// Web Server Interface
void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:sans-serif; text-align:center; background:#0f0f15; color:#e0e0eb;}";
  html += "input{padding:10px; margin:5px; font-size:16px; border-radius:5px; border:1px solid #444; background:#222; color:#fff;}";
  html += "button{padding:12px 24px; font-size:16px; background:#e85a4f; border:none; border-radius:5px; color:#fff; cursor:pointer;}";
  html += "button:hover{background:#d84a3f;}";
  html += "</style></head><body>";
  html += "<h1>PathFinder Tesla-Mini Controller</h1>";
  html += "<form action='/setTarget' method='POST'>";
  html += "Target X Coordinate: <input type='number' step='0.1' name='x'><br>";
  html += "Target Y Coordinate: <input type='number' step='0.1' name='y'><br><br>";
  html += "<button type='submit'>Execute Navigation Command</button>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleSetTarget() {
  if (server.hasArg("x") && server.hasArg("y")) {
    targetX = server.arg("x").toFloat();
    targetY = server.arg("y").toFloat();
    isNavigating = true;
    currentState = STATE_DRIVING;
    
    // Play voice confirmation: "Heading to destination, boss" (Track 0003)
    sendAudioCommand(0x0F, 0x01, 0x03);
    
    server.send(200, "text/plain", "Navigation target coordinates confirmed. Driving...");
  } else {
    server.send(400, "text/plain", "Missing navigation variables");
  }
}
