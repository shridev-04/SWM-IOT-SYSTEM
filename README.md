<div align="center">
  <h1>🌡️ SWM IoT System</h1>
  
  <p>An intelligent, dynamic, and automated water temperature control system powered by ESP32, Cloud MQTT, and a native Android App.</p>
</div>

---

## 📖 About The Project
Traditional water heating and cooling systems (like geysers or refrigerators) use static temperature limits. They don't care about the outside weather. 

**The SWM IoT System** solves this by fetching real-time weather data (temperature & humidity) from the internet and dynamically adjusting the target water temperature using a custom inverse-relationship algorithm. It features a stunning **Cyberpunk-themed Android App** for real-time monitoring and control.

### ✨ Key Features
- **Dynamic Weather Algorithm:** Automatically calculates ideal water temperature based on outside heat and humidity.
- **2°C Hysteresis Deadband:** Saves energy and extends hardware life by preventing compressor short-cycling.
- **Lightning Fast Sync:** Powered by HiveMQ MQTT Cloud for real-time two-way communication.
- **Premium Android UI:** Built with Kotlin and Glassmorphism design principles.
- **Emergency Fail-safe:** 1-Click shutdown protocol right from the app.

---

## 🛠️ Hardware Requirements
- **ESP32 Development Board** (30-Pin Module V1 recommended)
- **DS18B20** Waterproof Digital Temperature Sensor
- **2-Channel Relay Module** (For Compressor/Cooling and Geyser/Heating)
- **4.7kΩ Resistor** (For DS18B20 Data Line)

### 🔌 Circuit & Pin Configuration
| Component | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **DS18B20 Sensor Data** | `GPIO 4` | Needs a 4.7k pull-up resistor to 3.3V |
| **Compressor Relay** | `GPIO 5` | Active HIGH (Change in code if module is Active LOW) |
| **Geyser Relay** | `GPIO 18` | Active HIGH |
| **WiFi Status LED (Red)** | `GPIO 19` | Indicates disconnected status |
| **WiFi Status LED (Green)**| `GPIO 15` | Indicates connected status |
| **MQTT Data Blinker** | `GPIO 2` | Onboard Blue LED (Blinks rapidly on data transfer) |

---

# 🚀 Getting Started

This repository is divided into two parts. Follow the guide based on your technical expertise:

## 🟢 Category 1: Quick Setup (For Normal Users)
*Follow this if you just want to run the project without diving into the app source code.*

### Step 1: Upload the ESP32 Code
1. Download and install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install ESP32 board manager in Arduino IDE.
3. Install the required libraries via Library Manager:
   - `PubSubClient` (for MQTT)
   - `OneWire` & `DallasTemperature` (for DS18B20)
   - `ArduinoJson` (for Weather API)
4. Open the `ESP32_Code` (the `.ino` file) from this repository.
5. Update your WiFi SSID, Password, and your **OpenWeather API Key** in the code.
6. Connect your ESP32 via USB and click **Upload**.

### Step 2: Install the Android App
1. Download the pre-built `Base_APK/SWM_IoT_System.apk` file from this repository to your Android phone.
2. Install the APK (You may need to allow "Install from Unknown Sources").
3. Open the app, enter your MQTT credentials (or use the default public test server provided in the code).
4. **Done!** You can now monitor and control your hardware from the app.

---

## 🔴 Category 2: Advanced Setup (For Developers & Students)
*Follow this if you want to modify the App UI, change the Kotlin logic, or build the APK yourself.*

### Step 1: App Development (Android Studio)
1. Download and install [Android Studio](https://developer.android.com/studio).
2. Clone this repository to your local machine:
   ```bash
   git clone https://github.com/YourUsername/Your-Repo-Name.git
   ```
3. Open Android Studio -> Click **Open** -> Select the `App_Source_Code` folder.
4. Let Gradle sync and download all required dependencies (Material Components, HiveMQ MQTT Client, Lottie Animations).
5. Open `DashboardActivity.kt` to view the core logic of MQTT subscriptions and UI updates.
6. Connect your Android phone via USB (enable USB Debugging) and hit the **Run (Play)** button in Android Studio to build and install the app.

### Step 2: Modifying the ESP32 Firmware
- The firmware uses a custom **Feedback Sync Engine**. If you add a new slider or button in the app, ensure you subscribe to its MQTT topic in `setupMQTT()` inside the `.ino` file.
- The Core Temperature logic is located inside `calculateIdealWaterTemp()`. Students can modify this math equation to create their own custom climate algorithms!

---

## 🧠 How the Algorithm Works (For Students/Judges)
The ESP32 calculates the `Ideal Target Temp` using an inverse relationship. 
- **Example Scenario:** If outside is 35°C (Hot) and Humidity is 60%.
- System detects it's hot (>= 25°C) and enters **Cooling Mode**.
- It calculates Heat Intensity: `(35-25)/25 = 40%`.
- It maps 40% intensity to a base cooling target (e.g., 9.2°C).
- It subtracts an extra 1.8°C to compensate for high humidity discomfort.
- **Final Target = 7.4°C**. All calculated completely offline on the edge node after receiving weather data!

---
> **Note:** Please ensure you respect electrical safety guidelines when wiring 220V/110V appliances to relays.

*Made with ❤️ for IoT Enthusiasts and Students.*
Follow this if you just want to run the project without diving into the app source code.
**Step 1: HiveMQ Cloud Configuration**
To enable real-time communication between the ESP32 hardware and the Android app, an MQTT broker setup is required.
1. Navigate to the **HiveMQ Cloud** website (console.hivemq.cloud) and sign up for a free account.
2. Once logged into the dashboard, access the **"Clusters"** section and create a **"New Serverless Cluster"** (Free tier).
3. After the cluster is successfully generated, locate your **Cluster URL**. This functions as your **MQTT Server Address**. Copy and securely store this URL.
4. Within the same dashboard, navigate to the **"Access Management"** or **"Credentials"** tab.
5. Click **"Add New Credentials"** to create a custom **Username** and a secure **Password**.
6. Keep these three core credentials ready for the next setup phase: the MQTT Server Address, Username, and Password.
**Step 2: Upload the ESP32 Code**
1. Download and install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the ESP32 board manager in the Arduino IDE.
3. Install the required libraries via the Library Manager: `PubSubClient`, `OneWire`, `DallasTemperature`, and `ArduinoJson`.
4. Open the `ESP32_Code` (the `.ino` file) from this repository.
5. In the configuration section of the code, update the parameters with your specific network and cloud details. Ensure you replace `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_SERVER`, `MQTT_USER`, `MQTT_PASS`, and the `WEATHER_API_KEY` with your actual secure credentials (currently set to `"***"` for privacy in the repository).
6. Connect your ESP32 via USB and click **Upload**.
**Step 3: Install the Android App**
1. Download the pre-built `Base_APK/SWM_IoT_System.apk` file from this repository to your Android device.
2. Install the APK (You may need to allow "Install from Unknown Sources" in your device settings).
3. Open the app, and when prompted, enter the exact **MQTT Server Address**, **Username**, and **Password** that you configured in Step 1.
4. **Done!** You can now monitor and control your hardware wirelessly from the app.
<br>
---
<br>
### 🔴 Category 2: Advanced Setup (For Developers)
Follow this if you want to modify the App UI, change the Kotlin logic, or build the APK yourself.
**Step 1: App Development (Android Studio)**
1. Download and install [Android Studio](https://developer.android.com/studio).
2. Clone this repository to your local machine using git.
3. Open Android Studio, click **Open**, and select the `App_Source_Code` folder.
4. Let Gradle sync and download all required dependencies (Material Components, HiveMQ MQTT Client, Lottie Animations).
5. Open `DashboardActivity.kt` to view the core logic of MQTT subscriptions and UI updates.
6. Connect your Android phone via USB (enable USB Debugging) and hit the **Run (Play)** button in Android Studio to build and install the app.
**Step 2: Modifying the ESP32 Firmware**
* The firmware uses a custom **Feedback Sync Engine**. If you add a new slider or button in the app, ensure you subscribe to its MQTT topic in `setupMQTT()` inside the `.ino` file.
* The Core Temperature logic is located inside `calculateIdealWaterTemp()`. Students can modify this math equation to create their own custom climate algorithms!
<br>
---
<br>
## 🧠 How the Algorithm Works
The ESP32 calculates the `Ideal Target Temp` using an inverse relationship. 
* **Example Scenario:** If outside is 35°C (Hot) and Humidity is 60%.
* System detects it's hot (>= 25°C) and enters **Cooling Mode**.
* It calculates Heat Intensity: `(35-25)/25 = 40%`.
* It maps 40% intensity to a base cooling target (e.g., 9.2°C).
* It subtracts an extra 1.8°C to compensate for high humidity discomfort.
* **Final Target = 7.4°C**. All calculated completely offline on the edge node after receiving weather data!
<br>
---
<br>
> **Note:** Please ensure you respect electrical safety guidelines when wiring 220V/110V appliances to relays.
*Made with ❤️ for IoT Enthusiasts and Students.*
### Step 1: Upload the ESP32 Code
1. Download and install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install ESP32 board manager in Arduino IDE.
3. Install the required libraries via Library Manager:
   - `PubSubClient` (for MQTT)
   - `OneWire` & `DallasTemperature` (for DS18B20)
   - `ArduinoJson` (for Weather API)
4. Open the `ESP32_Code` (the `.ino` file) from this repository.
5. Update your WiFi SSID, Password, and your **OpenWeather API Key** in the code.
6. Connect your ESP32 via USB and click **Upload**.

### Step 2: Install the Android App
1. Download the pre-built `Base_APK/SWM_IoT_System.apk` file from this repository to your Android phone.
2. Install the APK (You may need to allow "Install from Unknown Sources").
3. Open the app, enter your MQTT credentials (or use the default public test server provided in the code).
4. **Done!** You can now monitor and control your hardware from the app.

---

## 🔴 Category 2: Advanced Setup (For Developers & Students)
*Follow this if you want to modify the App UI, change the Kotlin logic, or build the APK yourself.*

### Step 1: App Development (Android Studio)
1. Download and install [Android Studio](https://developer.android.com/studio).
2. Clone this repository to your local machine:
   ```bash
   git clone https://github.com/YourUsername/Your-Repo-Name.git
   ```
3. Open Android Studio -> Click **Open** -> Select the `App_Source_Code` folder.
4. Let Gradle sync and download all required dependencies (Material Components, HiveMQ MQTT Client, Lottie Animations).
5. Open `DashboardActivity.kt` to view the core logic of MQTT subscriptions and UI updates.
6. Connect your Android phone via USB (enable USB Debugging) and hit the **Run (Play)** button in Android Studio to build and install the app.

### Step 2: Modifying the ESP32 Firmware
- The firmware uses a custom **Feedback Sync Engine**. If you add a new slider or button in the app, ensure you subscribe to its MQTT topic in `setupMQTT()` inside the `.ino` file.
- The Core Temperature logic is located inside `calculateIdealWaterTemp()`. Students can modify this math equation to create their own custom climate algorithms!

---

## 🧠 How the Algorithm Works (For Students/Judges)
The ESP32 calculates the `Ideal Target Temp` using an inverse relationship. 
- **Example Scenario:** If outside is 35°C (Hot) and Humidity is 60%.
- System detects it's hot (>= 25°C) and enters **Cooling Mode**.
- It calculates Heat Intensity: `(35-25)/25 = 40%`.
- It maps 40% intensity to a base cooling target (e.g., 9.2°C).
- It subtracts an extra 1.8°C to compensate for high humidity discomfort.
- **Final Target = 7.4°C**. All calculated completely offline on the edge node after receiving weather data!

---
> **Note:** Please ensure you respect electrical safety guidelines when wiring 220V/110V appliances to relays.

*Made with ❤️ for IoT Enthusiasts and Students.*

**Hardware Wiring Instructions:**
*   **Temperature Sensor:** Connect the data wire of the DS18B20 sensor to the GPIO 4 pin on the ESP32. Ensure a 4.7kΩ resistor is securely placed between the data line and the 3.3V pin for proper sensor operation.
*   **Relay Connections:** Wire the cooling/compressor relay to GPIO 5 and the heating/geyser relay to GPIO 18. 
*   **Status Indicators:** Connect the Red LED to GPIO 19 (to indicate WiFi disconnected status) and the Green LED to GPIO 15 (to indicate successful WiFi connection). 
*   **Data Transfer Indicator:** No external LED is required for MQTT data syncing; the system natively utilizes the ESP32's onboard Blue LED on GPIO 2 to blink rapidly during data transfers.
---
# 🚀 Getting Started
This repository is divided into two parts. Follow the guide based on your technical expertise:
## 🟢 Category 1: Quick Setup
*Follow this if you just want to run the project without diving into the app source code.*
### Step 1: HiveMQ Cloud Configuration
To enable real-time communication between the ESP32 hardware and the Android app, an MQTT broker setup is required.
1. Navigate to the **HiveMQ Cloud** website (console.hivemq.cloud) and sign up for a free account.
2. Once logged into the dashboard, access the **"Clusters"** section and create a **"New Serverless Cluster"** (Free tier).
3. After the cluster is successfully generated, locate your **Cluster URL** (e.g., *****************.eu.hivemq.cloud). This functions as your **MQTT Server Address**. Copy and securely store this URL.
4. Within the same dashboard, navigate to the **"Access Management"** or **"Credentials"** tab.
5. Click **"Add New Credentials"** to create a custom **Username** (e.g.,******* ) and a secure **Password** (e.g., *********).
6. Keep these three core credentials ready for the next setup phase: the **MQTT Server Address**, **Username**, and **Password**.
### Step 2: Upload the ESP32 Code
1. Download and install Arduino IDE.
2. Install ESP32 board manager in Arduino IDE.
3. Install the required libraries via Library Manager:
   - PubSubClient (for MQTT)
   - OneWire & DallasTemperature (for DS18B20)
   - ArduinoJson (for Weather API)
4. Open the ESP32_Code (the .ino file) from this repository.
5. In the configuration section of the code, update the following parameters with your specific network and cloud details:
   - WIFI_SSID: Enter your local WiFi network name.
   - WIFI_PASSWORD: Enter your local WiFi password.
   - MQTT_SERVER: Paste the **Cluster URL** copied from HiveMQ.
   - MQTT_USER: Enter the HiveMQ **Username** you created.
   - MQTT_PASS: Enter the HiveMQ **Password** you created.
   - Update your **OpenWeather API Key** in the code.
6. Connect your ESP32 via USB and click **Upload**.
### Step 3: Install the Android App
1. Download the pre-built Base_APK/SWM_IoT_System.apk file from this repository to your Android device.
2. Install the APK (You may need to allow "Install from Unknown Sources" in your device settings).
3. Open the app, and when prompted, enter the exact **MQTT Server Address**, **Username**, and **Password** that you configured in Step 1 (or use the default public test server provided in the code).
4. **Done!** You can now monitor and control your hardware wirelessly from the app.
---
## 🔴 Category 2: Advanced Setup (For Developers & Students)
*Follow this if you want to modify the App UI, change the Kotlin logic, or build the APK yourself.*
### Step 1: App Development (Android Studio)
1. Download and install Android Studio.
2. Clone this repository to your local machine:
   git clone https://github.com/YourUsername/Your-Repo-Name.git
3. Open Android Studio -> Click **Open** -> Select the App_Source_Code folder.
4. Let Gradle sync and download all required dependencies (Material Components, HiveMQ MQTT Client, Lottie Animations).
5. Open DashboardActivity.kt to view the core logic of MQTT subscriptions and UI updates.
6. Connect your Android phone via USB (enable USB Debugging) and hit the **Run (Play)** button in Android Studio to build and install the app.
### Step 2: Modifying the ESP32 Firmware
- The firmware uses a custom **Feedback Sync Engine**. If you add a new slider or button in the app, ensure you subscribe to its MQTT topic in setupMQTT() inside the .ino file.
- The Core Temperature logic is located inside calculateIdealWaterTemp(). Students can modify this math equation to create their own custom climate algorithms!
---
## 🧠 How the Algorithm Works (For Students/Judges)
The ESP32 calculates the Ideal Target Temp using an inverse relationship.
- **Example Scenario:** If outside is 35°C (Hot) and Humidity is 60%.
- System detects it's hot (>= 25°C) and enters **Cooling Mode**.
- It calculates Heat Intensity: (35-25)/25 = 40%.
- It maps 40% intensity to a base cooling target (e.g., 9.2°C).
- It subtracts an extra 1.8°C to compensate for high humidity discomfort.
- **Final Target = 7.4°C**. All calculated completely offline on the edge node after receiving weather data!
---
> **Note:** Please ensure you respect electrical safety guidelines when wiring 220V/110V appliances to relays.
*Made with ❤️ for IoT Enthusiasts and Students.*
