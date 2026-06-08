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
| [ESP32 NodeMCU Development Board](https://www.amazon.com/dp/B08286C22B) | 1 | 7.99 | Main controller (Logic and web interface) |
| [MPU6050 IMU Gyroscope Module](https://www.amazon.com/dp/B008BOPN40) | 1 | 5.99 | Heading/yaw tracking sensor |
| [L298N H-Bridge Motor Driver](https://www.amazon.com/dp/B014KMHSW6) | 1 | 5.99 | Dual motor driver for differential steering |
| [DC Gear Motors with Wheels](https://www.amazon.com/dp/B07DNCPX65) | 4 | 12.99 | 4WD chassis drive motors |
| [DFPlayer Mini MP3 Player](https://www.amazon.com/dp/B08G1F2YV5) | 1 | 6.49 | Audio module for pre-recorded status voice clips |
| [Small 8 Ohm 2W Speaker](https://www.amazon.com/dp/B0738NLFTG) | 1 | 5.99 | Voice output |
| [HC-SR04 Ultrasonic Sensors](https://www.amazon.com/dp/B01MA4O5G5) | 3 | 8.99 | Front, Left, and Right distance tracking |
| [LM2596 Buck Converter](https://www.amazon.com/dp/B01GD81H6U) | 1 | 6.99 | Step-down battery voltage to 5V |
| [3S 11.1V LiPo Battery](https://www.amazon.com/dp/B0754M57C7) | 1 | 18.99 | Drive power source |
| [4WD Robot Chassis Plate Kit](https://www.amazon.com/dp/B07G155RW4) | 1 | 15.99 | Structural baseplate frame |
| [Jumper Wires & Breadboard](https://www.amazon.com/dp/B01EV70C78) | 1 | 5.99 | Interface wiring |

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
