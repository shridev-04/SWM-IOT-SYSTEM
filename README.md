<div align="center">

# 🌡️ SWM IoT System

### Intelligent Weather-Based Water Temperature Control System using ESP32, MQTT Cloud & Android

*A Smart IoT solution that automatically adjusts water temperature according to real-time weather conditions.*

![Platform](https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge)
![Android](https://img.shields.io/badge/Android-Kotlin-green?style=for-the-badge)
![MQTT](https://img.shields.io/badge/Protocol-MQTT-orange?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-red?style=for-the-badge)

</div>

---

# 📖 About The Project

Traditional water heating and cooling systems operate using fixed temperature values. They cannot understand changes in outdoor weather conditions, which often results in unnecessary power consumption and reduced comfort.

**SWM IoT System (Smart Weather Manager)** solves this problem by combining IoT, cloud communication, and real-time weather intelligence.

The ESP32 continuously receives outdoor weather information (temperature and humidity) from the internet, processes the data using a custom algorithm, and automatically calculates the most suitable water temperature.

The Android application provides real-time monitoring and wireless control from anywhere through MQTT Cloud.

This project demonstrates how IoT, Cloud Computing, Mobile Applications, and Embedded Systems can work together to build an intelligent automation system.

---

# ✨ Features

## 🌦 Weather-Based Smart Control
Automatically adjusts the target water temperature according to the outside weather.

## 🌡 Intelligent Temperature Algorithm
Uses a custom inverse-relation algorithm instead of fixed temperature values.

## 📡 Real-Time MQTT Communication
ESP32 and Android stay synchronized instantly using HiveMQ Cloud MQTT.

## 📱 Premium Android Application
Modern Cyberpunk-inspired user interface developed completely in Kotlin.

## ⚡ Automatic Heating & Cooling
Controls both Heater and Cooling Compressor without manual intervention.

## 🔄 Two-Way Synchronization
Any change made from the Android App immediately updates the ESP32.

## 💡 WiFi Status Indicators
Dedicated LEDs display WiFi connection status.

## 🚨 Emergency Shutdown
Immediately turns OFF all outputs directly from the mobile application.

## ⚙️ Energy Efficient
Uses a 2°C hysteresis deadband to prevent unnecessary relay switching.

## 🌍 Remote Monitoring
Monitor the system from anywhere with internet connectivity.

---

# 🛠 Hardware Requirements

| Component | Quantity |
|-----------|----------|
| ESP32 Development Board (30-Pin Module V1 Recommended) | 1 |
| DS18B20 Waterproof Temperature Sensor | 1 |
| 2-Channel Relay Module | 1 |
| 4.7kΩ Resistor | 1 |
| Red LED | 1 |
| Green LED | 1 |
| Jumper Wires | As Required |
| Breadboard / PCB | Optional |
| 5V Power Supply | 1 |

---

# 🔌 Circuit & Pin Configuration

| Component | ESP32 GPIO | Description |
|-----------|------------|-------------|
| DS18B20 Data Pin | GPIO 4 | Water Temperature Sensor |
| Compressor Relay | GPIO 5 | Cooling Output |
| Geyser Relay | GPIO 18 | Heating Output |
| Red LED | GPIO 19 | WiFi Disconnected Indicator |
| Green LED | GPIO 15 | WiFi Connected Indicator |
| Blue LED | GPIO 2 | MQTT Data Transfer Indicator |

> **Important:** Connect a **4.7kΩ Pull-up Resistor** between the DS18B20 Data pin and the 3.3V supply.

---

# 📂 Repository Structure

```text
SWM-IOT-SYSTEM/

├── ESP32_Code/
│   └── SWM_IoT_System.ino
│
├── App_Source_Code/
│   └── Android Studio Project
│
├── Base_APK/
│   └── SWM_IoT_System.apk
│
├── Images/
│   ├── Banner.png
│   ├── Circuit_Diagram.png
│   ├── Dashboard.png
│   └── Hardware.png
│
├── LICENSE
│
└── README.md
```

---

# 🚀 Getting Started

This repository is divided into two categories.

Choose the setup method according to your experience level.

---

# 🟢 Category 1 — Quick Setup

This method is recommended for users who simply want to use the project without modifying the Android source code.

The setup requires only three major steps:

- Configure HiveMQ Cloud
- Upload ESP32 Firmware
- Install Android APK

---

# ☁️ Step 1 — HiveMQ Cloud Configuration

The ESP32 and Android application communicate using the MQTT protocol.

Before using the project, create your own free MQTT broker.

### 1. Create a HiveMQ Cloud Account

Visit:

https://console.hivemq.cloud

Create a free account.

---

### 2. Create a Serverless Cluster

After login,

Go to

**Clusters**

↓

Click

**Create New Serverless Cluster**

↓

Wait until the cluster becomes active.

---

### 3. Copy Your Cluster URL

Example

```
xxxxxxxxxxxx.s1.eu.hivemq.cloud
```

This will be your

**MQTT Server Address**

Save it safely.

---

### 4. Create MQTT Credentials

Go to

**Access Management**

↓

**Credentials**

↓

**Add Credentials**

Now create

• Username

• Password

Save both carefully.

---

After completing this step, you should have:

✅ MQTT Server Address

✅ MQTT Username

✅ MQTT Password

These credentials will be required for both the ESP32 firmware and Android application.

---

# 💻 Step 2 — Upload ESP32 Firmware

### Install Arduino IDE

Download and install the latest Arduino IDE.

---

### Install ESP32 Board Package

Open Arduino IDE

↓

Boards Manager

↓

Search

```
ESP32
```

↓

Install the latest stable version.

---

### Install Required Libraries

Open

Library Manager

Install the following libraries.

• PubSubClient

• OneWire

• DallasTemperature

• ArduinoJson

---

### Open the Firmware

Navigate to

```
ESP32_Code/
```

Open

```
SWM_IoT_System.ino
```

---

### Update Configuration

Replace the following values with your own credentials.

```
WIFI_SSID

WIFI_PASSWORD

MQTT_SERVER

MQTT_USER

MQTT_PASS

WEATHER_API_KEY
```

Do not leave any field empty.

---

### Upload the Firmware

Connect the ESP32 using USB.

Select the correct

Board

and

COM Port

Click

**Upload**

Wait until uploading completes successfully.

The ESP32 will automatically restart and connect to WiFi.

---
---

# 📱 Step 3 — Install the Android Application

After successfully uploading the ESP32 firmware, install the Android application.

### Download the APK

Navigate to

```
Base_APK/
```

Download

```
SWM_IoT_System.apk
```

to your Android device.

---

### Install the APK

If prompted,

Enable

**Install from Unknown Sources**

in your Android settings.

Complete the installation.

---

### Configure MQTT

Launch the application.

Enter the following information that you created in **HiveMQ Cloud**.

- MQTT Server Address
- MQTT Username
- MQTT Password

Save the configuration.

---

### Connect

Ensure that

- ESP32 is connected to WiFi.
- Mobile phone has Internet access.
- MQTT credentials are correct.

Once connected, the Dashboard will immediately begin displaying live data.

You can now monitor and control the complete system wirelessly.

---

# 🔴 Category 2 — Advanced Setup

This section is intended for developers, students, and researchers who want to modify the Android application or customize the ESP32 firmware.

---

# 💻 Step 1 — Android Studio Setup

### Install Android Studio

Download and install the latest version of Android Studio.

---

### Clone the Repository

```bash
git clone https://github.com/YourUsername/YourRepository.git
```

or download the ZIP file directly from GitHub.

---

### Open the Project

Launch Android Studio.

Select

**Open Existing Project**

Choose

```
App_Source_Code
```

Wait for Gradle Sync to complete.

---

### Dependencies

The project automatically downloads all required libraries, including

- Material Components
- HiveMQ MQTT Client
- Lottie Animation
- AndroidX Libraries

---

### Run the Application

Enable USB Debugging on your Android phone.

Connect your device using USB.

Press the

▶ Run

button.

Android Studio will build and install the application.

---

# 🔧 Step 2 — ESP32 Firmware Customization

The firmware is fully customizable.

Developers can modify

- MQTT Topics
- Relay Logic
- Weather Algorithm
- Sensor Handling
- Dashboard Synchronization

Important Functions

```
setupWiFi()

setupMQTT()

callback()

calculateIdealWaterTemp()

controlRelays()

publishSensorData()
```

Whenever a new button or slider is added to the Android application,

remember to subscribe to the corresponding MQTT topic inside

```
setupMQTT()
```

---

# 🧠 How the Algorithm Works

Unlike traditional temperature controllers,

SWM IoT System uses a dynamic weather-based algorithm.

Instead of maintaining one fixed temperature,

the target water temperature changes according to

- Outdoor Temperature
- Outdoor Humidity

---

## Example

Outside Temperature

```
35°C
```

Humidity

```
60%
```

Since the outdoor temperature is greater than 25°C,

the system automatically enters

**Cooling Mode**

Heat Intensity

```
(35 - 25) / 25

= 40%
```

The controller maps this intensity to a cooling target.

Example

```
9.2°C
```

Because humidity is relatively high,

an additional

```
1.8°C
```

is reduced.

Final Target Temperature

```
7.4°C
```

All calculations are performed directly on the ESP32 after receiving weather data.

No cloud processing is required.

---

# 🔄 System Workflow

```
Weather API
      │
      ▼
ESP32 downloads Weather Data
      │
      ▼
Temperature Algorithm
      │
      ▼
Ideal Water Temperature
      │
      ▼
Heating / Cooling Decision
      │
      ▼
Relay Control
      │
      ▼
MQTT Publish
      │
      ▼
Android Dashboard Update
```

---

# 📡 MQTT Communication

The project uses MQTT Cloud for instant communication.

Example Topics

```
swm/temp

swm/target

swm/mode

swm/heater

swm/compressor

swm/weather

swm/status
```

The Android application subscribes to these topics and instantly updates the user interface.

---

# 📷 Screenshots

You can add screenshots inside

```
Images/
```

Example

```
Images/

├── Dashboard.png

├── Login.png

├── Settings.png

├── Circuit.png

└── Hardware.png
```

Then display them inside README.

Example

```markdown
## Dashboard

![Dashboard](Images/Dashboard.png)
```

---

# ⚠️ Electrical Safety

Please follow proper electrical safety guidelines.

- Never touch exposed AC wiring while the system is powered.
- Always disconnect the power supply before modifying connections.
- Use properly rated relay modules.
- Ensure secure insulation for all AC wiring.
- Keep the controller away from water.
- Use a regulated power supply for the ESP32.

The author is not responsible for damage caused by improper wiring.

---

# ❓ Troubleshooting

## ESP32 does not connect to WiFi

Check

- WiFi SSID
- Password
- Internet Connection

---

## MQTT Connection Failed

Verify

- Cluster URL
- Username
- Password

Ensure the HiveMQ Cluster is running.

---

## Sensor Not Detected

Check

- DS18B20 Wiring
- GPIO 4 Connection
- 4.7kΩ Pull-up Resistor

---

## Android App Shows No Data

Verify

- Internet Connection
- MQTT Credentials
- ESP32 Connection Status

---

# 🚀 Future Improvements

The project can be extended with

- AI-based Temperature Prediction
- Voice Control
- Google Assistant Integration
- OTA Firmware Updates
- Firebase Cloud Backup
- Power Consumption Monitoring
- Water Level Monitoring
- Multiple Temperature Sensors
- ESP32 Camera Support
- Web Dashboard

---

# 🤝 Contributing

Contributions are always welcome.

If you would like to improve this project,

1. Fork the repository.

2. Create a new feature branch.

3. Commit your changes.

4. Push the branch.

5. Create a Pull Request.

---

# 📜 License

This project is released under the **MIT License**.

You are free to use, modify, and distribute this project while retaining the original license.

---

# ❤️ Author

**Developed by**

**Shridev Pandit**

Electronics & Communication Engineering Student

Passionate about

- Embedded Systems
- Internet of Things (IoT)
- Android Development
- Artificial Intelligence
- Robotics

---

# ⭐ Support

If you found this project helpful,

please consider

⭐ Starring this repository

and sharing it with other developers and students.

Your support motivates future development.

---

<div align="center">

## ❤️ Thank You For Visiting This Repository ❤️

**Happy Coding! 🚀**

</div>
