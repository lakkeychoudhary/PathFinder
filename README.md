# PathFinder

PathFinder is an intelligent, self-driving autonomous vehicle (configured as a 4-wheeled mini-car platform) designed to navigate dynamically to target coordinates, chase active signal beacons, execute smart obstacle avoidance, and provide real-time voice feedback.

The system utilizes an ESP32 microcontroller with dual-core processing. One core is dedicated to high-frequency gyroscope sensor integration for a yaw-heading lock, while the second core runs the navigation control, obstacle routing, audio feedback, and the local web server.

---

## Technical Specifications & Features

- **Gyro-Assisted Straight-Line Driving**: Utilizes an MPU6050 Inertial Measurement Unit (IMU) to track heading angle (yaw). A feedback loop adjusts motor power dynamically to correct wheel mismatches, keeping the vehicle driving perfectly straight.
- **Smart Obstacle Avoidance**: Equipped with three ultrasonic distance sensors (front, left, right) to detect path obstructions, perform emergency halts, backup, and rotate to a clear heading before resuming target navigation.
- **Beacon-Chasing Mode**: Tracks active signal sources (such as an infrared beacon or signal transmitter) using differential analog phototransistors to steer directly toward the source.
- **Wireless Coordinate Dashboard**: Hosts a local WiFi Access Point on the ESP32, serving a web dashboard where users input 2D coordinates $(X, Y)$ to command the vehicle.
- **Voice Synthesis Alerts**: Integrates a DFPlayer Mini MP3 module and a 2W speaker to vocalize status changes (e.g. playing "I am here boss, now where else do you want me to go?" upon startup).

---

## Complexity Level: Tier 3 (200 Bits | 17–33 Hours)

PathFinder fits perfectly into **Tier 3**. While the robot sits on a stable 4-wheeled platform rather than balancing, it maintains high technical complexity due to:
1. **Real-time Yaw Lock**: Using gyroscope integration to perform active straight-line correction and precise 90-degree pivot turns.
2. **Dual-Core Execution**: Splitting I2C sensor reading from web server network cycles to prevent timing lag in motor control.
3. **Multi-Sensor Integration**: Orchestrating distance sensors, target trackers, motor controllers, and UART voice feedback simultaneously.

---

## Bill of Materials (BOM)

The following components are required to assemble PathFinder:

| Component | Quantity | Estimated Price ($) | Function |
| :--- | :---: | :---: | :--- |
| [ESP32 WROOM 38-Pin Development Board](https://robu.in/product/esp32-wroom-32-38pin-development-board/) | 1 | 3.55 | Main microcontroller for path planning and telemetry web hosting |
| [MPU6050 3-Axis Gyroscope and Accelerometer](https://robu.in/product/mpu-6050-3-axis-accelerometer-and-gyro-sensor/) | 1 | 2.08 | IMU to track relative yaw heading angle for straight line driving |
| [L298N 2A Based Motor Driver Module](https://robu.in/product/l298n-2a-based-motor-driver-module/) | 1 | 1.75 | Controls speed and direction of the DC gear motors |
| [4WD Smart Robot Car Chassis Kit](https://robu.in/product/4wd-smart-robot-car-chassis-kits/) | 1 | 6.60 | Robot chassis structural plate including four DC gear motors and wheels |
| [DFPlayer Mini MP3 Player Module](https://robu.in/product/dfrobot-dfplayer-a-mini-mp3-player/) | 1 | 1.86 | Sound decoder to play voice alerts and confirmation status clips |
| [Stereo Enclosed Speaker 3W 8 Ohm (Pair)](https://robu.in/product/stereo-enclosed-speaker-3w-8-with-jst-ph2-0-interface-pair/) | 1 | 2.70 | Compact enclosed speaker for voice playback output |
| [HC-SR04 Ultrasonic Sonar Distance Sensor](https://robu.in/product/hc-sr04-ultrasonic-sensor/) | 3 | 3.06 | Provides range distances for front/left/right obstacle avoidance routing |
| [DC-DC LM2596 Buck Converter Step Down Module](https://robu.in/product/dc-dc-lm2596-buck-converter-step-down-module/) | 1 | 0.78 | Regulates battery power down to stable 5V for the ESP32 and sensors |
| [Orange 1000mAh 3S LiPo Battery Pack](https://robu.in/product/orange-1000mah-3s-30c60c-lipo-battery-pack/) | 1 | 6.95 | Rechargeable power supply pack for driving the vehicle |
| [Solderless Breadboard 830 Points](https://robu.in/product/gl-12-solderless-breadboard-830-points/) | 1 | 1.14 | Prototype board to plug in sensors and wire connections |
| [40pcs 10cm Male to Male Jumper Wire](https://robu.in/product/40pcs-10cm-male-to-male-jumper-wire/) | 1 | 0.66 | Flexible jumper wire pins to route signals between components |


---

## System Architecture & Wiring Diagram

The electrical layout routes power from the 3S LiPo battery to the motor driver, with a buck converter supplying stable 5V power to the ESP32 and logic sensors:

![PathFinder Wiring Diagram](schematics/pathfinder_schematic.png)

---

## Directory Structure

```
PathFinder/
├── .gitignore
├── LICENSE
├── CONTRIBUTING.md
├── bom.csv
├── README.md
├── schematics/
│   └── pathfinder_schematic.png
└── firmware/
    └── pathfinder_main/
        ├── pathfinder_main.ino
        └── pin_definitions.h
```

---

## Operating Instructions

1. **Upload Firmware**: Flash the ESP32 via Arduino IDE or PlatformIO with the files located in the `firmware/` directory.
2. **Access Point Setup**: Connect your mobile phone or computer to the WiFi network `PathFinder_Tesla_Mini` (Password: `admin12345`).
3. **Web Dashboard**: Open a web browser and go to `http://192.168.4.1`.
4. **Target Settings**: Input coordinate targets $(X, Y)$ on the control page to begin autonomous self-driving, or active target beacons for the car to chase.
