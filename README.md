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
| [Adafruit HUZZAH32 ESP32 Feather Board](https://www.adafruit.com/product/3405) | 1 | 19.95 | Main microcontroller for path planning and telemetry web hosting |
| [Adafruit MPU-6050 6-DoF Sensor Breakout](https://www.adafruit.com/product/3886) | 1 | 9.95 | IMU to track relative yaw heading angle for straight line driving |
| [Adafruit TB6612 H-Bridge Motor Driver Breakout](https://www.adafruit.com/product/2448) | 1 | 7.50 | Controls speed and direction of the DC gear motors |
| [Adafruit Mini Robot Rover Chassis Kit](https://www.adafruit.com/product/2939) | 1 | 19.95 | Robot chassis structural plate including two DC TT motors and wheels |
| [DFRobot DFPlayer Mini MP3 Module](https://www.dfrobot.com/product-1121.html) | 1 | 5.90 | Sound decoder to play voice alerts and confirmation status clips |
| [Adafruit Mono Enclosed Speaker (3W 4 Ohm)](https://www.adafruit.com/product/3351) | 1 | 3.95 | Compact enclosed speaker for voice playback output |
| [Adafruit HC-SR04 Ultrasonic Distance Sensor](https://www.adafruit.com/product/3942) | 3 | 11.85 | Provides range distances for front/left/right obstacle avoidance routing |
| [Adafruit UBEC 5V 3A Step-Down Buck Converter](https://www.adafruit.com/product/1385) | 1 | 9.95 | Regulates battery power down to stable 5V for the ESP32 and sensors |
| [SparkFun Lithium Ion Battery Pack (7.4V 1000mAh)](https://www.sparkfun.com/products/11855) | 1 | 9.95 | Rechargeable power supply pack for driving the vehicle |
| [Adafruit Half-Size Breadboard](https://www.adafruit.com/product/64) | 1 | 5.00 | Prototype board to plug in sensors and wire connections |
| [Adafruit Premium Male/Male Jumper Wires (20-Pack)](https://www.adafruit.com/product/1956) | 1 | 3.95 | Flexible jumper wire pins to route signals between components |

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
