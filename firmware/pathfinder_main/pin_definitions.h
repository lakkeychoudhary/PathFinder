#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

// L298N Dual H-Bridge Motor Driver
#define ENA_LEFT          12  // PWM Pin for Left Motor Speed
#define IN1_LEFT          13  // Direction Pin 1 for Left Motor
#define IN2_LEFT          14  // Direction Pin 2 for Left Motor
#define IN3_RIGHT         27  // Direction Pin 1 for Right Motor
#define IN4_RIGHT         26  // Direction Pin 2 for Right Motor
#define ENB_RIGHT         15  // PWM Pin for Right Motor Speed

// MPU6050 IMU (I2C)
#define IMU_SDA           21
#define IMU_SCL           22

// DFPlayer Mini MP3 Player (Serial2)
#define AUDIO_TX          17
#define AUDIO_RX          16

// HC-SR04 Ultrasonic Sensors
#define FRONT_TRIG        4
#define FRONT_ECHO        5

#define LEFT_TRIG         18
#define LEFT_ECHO         19

#define RIGHT_TRIG         23
#define RIGHT_ECHO         25

// Target Tracker (Analog IR Phototransistors)
#define TRACKER_LEFT      32
#define TRACKER_RIGHT     33

#endif
