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
#include "imu.h"
#include "audio.h"
#include "navigation.h"
#include "telemetry.h"

// Task handles for dual-core execution
TaskHandle_t Core0_Sensors_Handle;
TaskHandle_t Core1_Navigation_Handle;

// Web server on Port 80
WebServer server(80);

// MPU6050 Global Variables
const int MPU_addr = 0x68;
float currentYaw = 0.0;
float currentPitch = 0.0;
float currentRoll = 0.0;
float gyroBiasZ = 0.0;
unsigned long lastImuTime = 0;

// Navigation Global Variables
float targetX = 0.0, targetY = 0.0;
float currentX = 0.0, currentY = 0.0;
float targetHeading = 0.0;
bool isNavigating = false;
int obstacleThreshold = 25; // cm
int baseSpeed = 180;        // 0-255 PWM speed
float kp_yaw = 3.5;         // Proportional gain for gyro drift correction
int leftMotorOutput = 0;
int rightMotorOutput = 0;

// System states
enum SystemState {
  STATE_STANDBY,
  STATE_DRIVING,
  STATE_OBSTACLE_AVOIDANCE,
  STATE_BEACON_CHASING,
  STATE_MANUAL_CONTROL
};
SystemState currentState = STATE_STANDBY;
const char* currentStateStr = "STANDBY";

// Distance Sensor Readings
long lastSensorFront = 999;
long lastSensorLeft = 999;
long lastSensorRight = 999;

// Setup hardware serial for DFPlayer Mini
HardwareSerial AudioSerial(2);

// Forward Declarations of RTOS Loop Tasks
void IMU_Sensor_Loop(void * pvParameters);
void Navigation_Loop(void * pvParameters);

// Web Server API Handlers
void handleSetTarget();
void handleManualControl();

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

  // Status Warning LED
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW);

  // Initialize I2C and IMU
  Wire.begin(IMU_SDA, IMU_SCL, 400000);
  initMPU();

  // Load IMU settings or calibrate
  loadCalibration();
  if (gyroBiasZ == 0.0) {
    calibrateIMU();
  }

  // Create Local WiFi Access Point
  WiFi.softAP("PathFinder_Tesla_Mini", "admin12345");
  Serial.println("AP Created: PathFinder_Tesla_Mini");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Web endpoints
  server.on("/", handleRoot);
  server.on("/telemetry", handleTelemetry);
  server.on("/setTarget", HTTP_POST, handleSetTarget);
  server.on("/manual", HTTP_POST, handleManualControl);
  server.begin();

  // Create background tasks
  xTaskCreatePinnedToCore(
    IMU_Sensor_Loop,
    "Sensor_Loop",
    4096,
    NULL,
    1,
    &Core0_Sensors_Handle,
    0 // Core 0 dedicated to real-time gyro integration and odometry calculations
  );

  xTaskCreatePinnedToCore(
    Navigation_Loop,
    "Nav_Loop",
    4096,
    NULL,
    1,
    &Core1_Navigation_Handle,
    1 // Core 1 dedicated to networking & steering algorithms
  );

  // Startup Voice Feedback: "I am here boss..." (Track 0001)
  delay(1000);
  setVolume(25);
  playTrack(TRACK_STARTUP); 
}

void loop() {
  // Executed inside RTOS tasks
}

// ----------------------------------------------------
// CORE 0: IMU gyro calculations & Odometry tracking
// ----------------------------------------------------
void IMU_Sensor_Loop(void * pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(10); // 100 Hz
  lastImuTime = micros();

  for(;;) {
    updateIMU();
    
    // Update odometry with actual motor speeds
    updateOdometry(leftMotorOutput, rightMotorOutput, 0.01);
    
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ----------------------------------------------------
// CORE 1: Navigation Logic, Steering & Obstacle Avoidance
// ----------------------------------------------------
void Navigation_Loop(void * pvParameters) {
  for(;;) {
    server.handleClient();

    // Read range sensor inputs
    lastSensorFront = readSensor(FRONT_TRIG, FRONT_ECHO);
    lastSensorLeft = readSensor(LEFT_TRIG, LEFT_ECHO);
    lastSensorRight = readSensor(RIGHT_TRIG, RIGHT_ECHO);

    // Obstacle Override Check (Safety lock)
    if (lastSensorFront < obstacleThreshold && lastSensorFront > 0 && currentState != STATE_MANUAL_CONTROL) {
      digitalWrite(LED_RED, HIGH); // Turn on warning LED
      currentState = STATE_OBSTACLE_AVOIDANCE;
      currentStateStr = "OBSTACLE DETECTED";
      setMotors(0, 0); // Stop
      
      playTrack(TRACK_OBSTACLE);
      vTaskDelay(pdMS_TO_TICKS(600));

      // Backup
      setMotors(-160, -160);
      vTaskDelay(pdMS_TO_TICKS(800));
      setMotors(0, 0);

      // Pivot to clear side
      if (lastSensorLeft > lastSensorRight) {
        targetHeading -= 90.0;
        setMotors(-150, 150);
      } else {
        targetHeading += 90.0;
        setMotors(150, -150);
      }
      
      vTaskDelay(pdMS_TO_TICKS(800));
      setMotors(0, 0);
      
      if (isNavigating) {
        currentState = STATE_DRIVING;
        currentStateStr = "DRIVING";
      } else {
        currentState = STATE_STANDBY;
        currentStateStr = "STANDBY";
      }
      digitalWrite(LED_RED, LOW); // Turn off warning LED
    }

    // Path Execution steering calculations
    if (currentState == STATE_DRIVING && isNavigating) {
      currentStateStr = "DRIVING";
      processPathfinding();

      // Heading steering mix
      float yawError = targetHeading - currentYaw;
      int yawCorrection = yawError * kp_yaw;
      
      setMotors(baseSpeed - yawCorrection, baseSpeed + yawCorrection);
    } else if (currentState == STATE_STANDBY) {
      currentStateStr = "STANDBY";
      setMotors(0, 0);
    } else if (currentState == STATE_MANUAL_CONTROL) {
      currentStateStr = "MANUAL INTERACTION";
    }

    vTaskDelay(pdMS_TO_TICKS(50)); // 20 Hz loop
  }
}

// Drive Motor Interface
void setMotors(int leftSpeed, int rightSpeed) {
  leftMotorOutput = leftSpeed;
  rightMotorOutput = rightSpeed;

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

// REST Web Server API Route Handlers
void handleSetTarget() {
  if (server.hasArg("x") && server.hasArg("y")) {
    targetX = server.arg("x").toFloat();
    targetY = server.arg("y").toFloat();
    isNavigating = true;
    currentState = STATE_DRIVING;
    
    playTrack(TRACK_NAV_CONFIRM);
    
    server.send(200, "text/plain", "Target confirmed.");
  } else {
    server.send(400, "text/plain", "Missing coordinates.");
  }
}

void handleManualControl() {
  if (server.hasArg("dir")) {
    String dir = server.arg("dir");
    currentState = STATE_MANUAL_CONTROL;
    isNavigating = false;

    if (dir == "forward") {
      setMotors(baseSpeed, baseSpeed);
    } else if (dir == "backward") {
      setMotors(-baseSpeed, -baseSpeed);
    } else if (dir == "left") {
      setMotors(-150, 150);
    } else if (dir == "right") {
      setMotors(150, -150);
    } else if (dir == "stop") {
      setMotors(0, 0);
      currentState = STATE_STANDBY;
    }
    server.send(200, "text/plain", "Manual command executed.");
  } else {
    server.send(400, "text/plain", "Direction parameter missing.");
  }
}
