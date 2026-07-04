// ╔══════════════════════════════════════════════════════════════════╗
// ║  SmartFridge ESP32 — 3-Page Smooth 16x2 Dynamic Display         ║
// ║  SWM IOT SYSTEM • 2°C Hysteresis Deadband • App Feedback Sync   ║
// ║  FIXED: Added Dual-Topic App Feedback Synchronization Engine    ║
// ╚══════════════════════════════════════════════════════════════════╝

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Wire.h>
#include <time.h>

// ═══════════════════════════════════════════════════════
// CONFIGURATION SECTION 
// ═══════════════════════════════════════════════════════
#define WIFI_SSID       "SHRIDEV"
#define WIFI_PASSWORD   "1234567898"

#define MQTT_SERVER     "a1b917c769d84be0ace8154f70bf6cdc.s1.eu.hivemq.cloud"
#define MQTT_PORT       8883
#define MQTT_USER       "shridev"
#define MQTT_PASS       "QWcc23@$"
#define MQTT_CLIENT_ID  "ESP32_SmartFridge"
#define MQTT_USE_SSL    true

#define WEATHER_API_KEY "31fd08a0e944420c2323f96d829baecf"
#define WEATHER_FETCH_INTERVAL 600000 

// ═══════════════════════════════════════════════════════
// PIN DEFINITIONS
// ═══════════════════════════════════════════════════════
#define PIN_DS18B20     4
#define PIN_COMPRESSOR  5
#define PIN_GEYSER      18

#define LCD_I2C_ADDR    0x27
#define LCD_COLS        16  
#define LCD_ROWS        2   

#define PIN_SRV_G       2
#define PIN_WIFI_R      19
#define PIN_WIFI_G      21

// No buttons needed

#define DEFAULT_MAX_HOT     40
#define DEFAULT_MIN_COLD    15
#define DEFAULT_FORCE_TEMP  25
#define TEMP_MIN_RANGE      5
#define TEMP_MAX_RANGE      60

#define TEMP_READ_INTERVAL    2000
#define MQTT_PUBLISH_INTERVAL 3000
#define DISPLAY_ROTATE_INTERVAL 3000
#define CONTROL_INTERVAL      5000
#define WIFI_CHECK_INTERVAL   10000

#define PWM_FREQ        5000
#define PWM_RESOLUTION  8

#define TEMP_HYSTERESIS        2.0    

// ═══════════════════════════════════════════════════════
// CITIES DATABASE STRUCTURE
// ═══════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════
// (City Table Removed - Using ZIP Code API)
// ═══════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════
// GLOBAL OBJECTS & CORE STATE
// ═══════════════════════════════════════════════════════
#if MQTT_USE_SSL
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif

PubSubClient mqtt(espClient);
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
Preferences prefs;

// Removed MenuState and OkAction

float currentWaterTemp   = 0.0;
int   maxHotLimit        = DEFAULT_MAX_HOT;
int   minColdLimit       = DEFAULT_MIN_COLD;
bool  isForceMode        = false;
int   forceTemp          = DEFAULT_FORCE_TEMP;
float targetWaterTemp    = 25.0;
bool  emergencyShutdown  = false;

bool  compressorRunning  = false;
bool  geyserRunning      = false;
unsigned long compressorStartTime = 0;
unsigned long geyserStartTime     = 0;
#define MIN_COMPRESSOR_CYCLE   60000  
#define MIN_GEYSER_CYCLE       60000  

String currentZipCode  = "110001";
float weatherTemp        = 30.0;
float weatherFeelsLike   = 32.0;
float weatherHumidity    = 60.0;
bool  weatherValid       = false;
bool  bootCheckPassed    = false; 

bool  appAlertActive     = false;
unsigned long appAlertStartTime = 0;
String appAlertLine1     = "";
String appAlertLine2     = "";

// Removed menu variables

int   displayPage        = 0; 
bool  lcdNeedsUpdate     = true;

unsigned long lastMqttDataTime   = 0;
int   serverBlinkInterval        = 1000;
bool  serverLedGreen             = false;
unsigned long lastServerBlink    = 0;
unsigned long lastWifiBlink      = 0;
bool  wifiLedGreen               = false;
int   wifiBlinkInterval          = 1000;

// Removed button variables

unsigned long lastTempRead       = 0;
unsigned long lastMqttPublish    = 0;
unsigned long lastDisplayRotate  = 0;
unsigned long lastWeatherFetch   = 0;
unsigned long lastControlRun     = 0;
unsigned long lastWifiCheck      = 0;

// Removed button pin array

// Forward Declarations
void loadSettings();
void saveSettings();
void publishAppFeedback(); // New Function to sync app elements
void executeStrictBootSequence();
void setupMQTT();
bool checkMQTTConnection();
bool fetchWeatherBoot();
void fetchWeather();
void readWaterTemp();
float calculateIdealWaterTemp();
void controlTemperature();
void updateDisplay();
void showDataPage1();
void showDataPage2();
void showDataPage3();
void triggerAppAlert(String l1, String l2);
// No menu declarations
void updateServerLed();
void updateWifiLed();

// ╔══════════════════════════════════════════════════════════════════╗
// ║  SETUP                                                           ║
// ╚══════════════════════════════════════════════════════════════════╗
void setup() {
    Serial.begin(115200);

    pinMode(PIN_COMPRESSOR, OUTPUT);
    pinMode(PIN_GEYSER, OUTPUT);
    digitalWrite(PIN_COMPRESSOR, LOW); 
    digitalWrite(PIN_GEYSER, LOW);

    pinMode(PIN_SRV_G, OUTPUT);
    pinMode(PIN_WIFI_R, OUTPUT);
    pinMode(PIN_WIFI_G, OUTPUT);
    
    digitalWrite(PIN_SRV_G, LOW);
    digitalWrite(PIN_WIFI_R, HIGH);
    digitalWrite(PIN_WIFI_G, LOW);

    // Button pins removed

    lcd.init();
    lcd.backlight();
    lcd.clear();

    sensors.begin();
    sensors.setResolution(12);
    readWaterTemp();

    loadSettings();

    executeStrictBootSequence();

    configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
    targetWaterTemp = calculateIdealWaterTemp();
    
    lcd.clear();
    lcdNeedsUpdate = true;
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  MAIN LOOP                                                       ║
// ╚══════════════════════════════════════════════════════════════════╗
void loop() {
    unsigned long now = millis();

    mqtt.loop();
    if (now - lastWifiCheck > WIFI_CHECK_INTERVAL) {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.reconnect();
        }
        lastWifiCheck = now;
    }
    
    if (!mqtt.connected()) {
        digitalWrite(PIN_SRV_G, LOW);
        if (WiFi.status() == WL_CONNECTED) {
            bool connected = (strlen(MQTT_USER) > 0) ? mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS) : mqtt.connect(MQTT_CLIENT_ID);
            if (connected) {
                mqtt.subscribe("fridge/setZip");
                mqtt.subscribe("fridge/setHotLimit");
                mqtt.subscribe("fridge/setColdLimit");
                mqtt.subscribe("fridge/forceMode");
                mqtt.subscribe("fridge/forceTemp");
                mqtt.subscribe("fridge/requestSync");
                mqtt.subscribe("fridge/emergency");
                digitalWrite(PIN_SRV_G, HIGH);
                delay(100); // Quick blink on connect
                digitalWrite(PIN_SRV_G, LOW);
                // On a fresh reconnection, immediately push saved feedback states to mobile UI
                publishAppFeedback();
            }
        }
    }

    if (appAlertActive && (now - appAlertStartTime > 3000)) {
        appAlertActive = false;
        lcdNeedsUpdate = true;
    }

    if (now - lastTempRead > TEMP_READ_INTERVAL) {
        float oldTemp = currentWaterTemp;
        readWaterTemp();
        if (abs(currentWaterTemp - oldTemp) >= 0.1) {
            lcdNeedsUpdate = true; 
        }
        lastTempRead = now;
    }

    if (now - lastWeatherFetch > WEATHER_FETCH_INTERVAL) {
        fetchWeather();
        lastWeatherFetch = now;
    }

    if (!appAlertActive && (now - lastDisplayRotate > DISPLAY_ROTATE_INTERVAL)) {
        displayPage = (displayPage + 1) % 3; 
        lcdNeedsUpdate = true;
        lastDisplayRotate = now;
    }

    if (lcdNeedsUpdate) {
        updateDisplay();
        lcdNeedsUpdate = false;
    }

    updateServerLed();
    updateWifiLed();

    if (bootCheckPassed && (now - lastControlRun > CONTROL_INTERVAL)) {
        targetWaterTemp = calculateIdealWaterTemp();
        controlTemperature();
        lastControlRun = now;
    }

    if (now - lastMqttPublish > MQTT_PUBLISH_INTERVAL) {
        if (mqtt.connected()) {
            char tempStr[10];
            dtostrf(currentWaterTemp, 4, 1, tempStr);
            mqtt.publish("fridge/waterTemp", tempStr);
            
            long rssi = WiFi.RSSI();
            int signalPct = map(constrain(rssi, -100, -50), -100, -50, 0, 100);
            mqtt.publish("fridge/signal", String(signalPct).c_str());
            
            int mockPing = random(18, 35);
            mqtt.publish("fridge/ping", String(mockPing).c_str());
            
            long uptimeSecs = millis() / 1000;
            int h = uptimeSecs / 3600;
            int m = (uptimeSecs % 3600) / 60;
            String uptimeStr = String(h) + "h " + String(m) + "m";
            mqtt.publish("fridge/uptime", uptimeStr.c_str());
            
            mqtt.publish("fridge/weatherData", String(weatherFeelsLike, 1).c_str());
        }
        lastMqttPublish = now;
    }
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  STRICT HANDSHAKE BOOT SEQUENCER                                 ║
// ╚════════════════════════════════════════════════════════════════════╝
void executeStrictBootSequence() {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print(F("SWM IOT SYSTEM"));
    delay(2500);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Checking WiFi..."));
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    bool initialDisconnectedMsgShown = false;
    while (WiFi.status() != WL_CONNECTED) {
        if (!initialDisconnectedMsgShown) {
            lcd.clear();
            lcd.setCursor(1, 0);
            lcd.print(F("Please Connect"));
            lcd.setCursor(4, 1);
            lcd.print(F("WiFi..."));
            initialDisconnectedMsgShown = true;
            digitalWrite(PIN_WIFI_R, HIGH);
            digitalWrite(PIN_WIFI_G, LOW);
        }
        delay(500);
    }
    
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print(F("WiFi Connected!"));
    digitalWrite(PIN_WIFI_R, LOW);
    digitalWrite(PIN_WIFI_G, HIGH);
    delay(1500);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Checking Server..."));
    setupMQTT();
    
    if (checkMQTTConnection()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Ready with"));
        lcd.setCursor(5, 1);
        lcd.print(F("Server"));
        digitalWrite(PIN_SRV_G, HIGH);
        delay(200);
        digitalWrite(PIN_SRV_G, LOW);
    } else {
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print(F("Server Error!"));
        digitalWrite(PIN_SRV_G, LOW);
    }
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Checking Weather"));
    
    if (fetchWeatherBoot()) {
        lcd.clear();
        lcd.setCursor(3, 0);
        lcd.print(F("Ready with"));
        lcd.setCursor(4, 1);
        lcd.print(F("Weather"));
    } else {
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print(F("Weather Error!"));
    }
    delay(2000);

    lcd.clear();
    for (int count = 3; count > 0; count--) {
        lcd.setCursor(7, 0);
        lcd.print(count);
        delay(1000);
    }
    
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print(F("START!"));
    delay(1500);
    
    bootCheckPassed = true; 
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  MQTT ENGINE & CRITICAL FEEDBACK ROUTINES                        ║
// ╚══════════════════════════════════════════════════════════════════╝
void triggerAppAlert(String l1, String l2) {
    appAlertLine1 = l1;
    appAlertLine2 = l2;
    appAlertStartTime = millis();
    appAlertActive = true;
    lcdNeedsUpdate = true;
}

// 🛠️ NEW FEEDBACK SYNC ENGINE (Directly updates Antigravity's App UI Squares)
void publishAppFeedback() {
    if (mqtt.connected()) {
        mqtt.publish("fridge/feedback/hotLimit", String(maxHotLimit).c_str());
        mqtt.publish("fridge/feedback/coldLimit", String(minColdLimit).c_str());
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    lastMqttDataTime = millis();

    if (strcmp(topic, "fridge/setZip") == 0) {
        if (msg.length() == 6) {
            String oldZip = currentZipCode;
            currentZipCode = msg;
            saveSettings();
            triggerAppAlert("CHECKING ZIP...", "Please Wait");
            
            // Validate via fetchWeather. If it fails, revert and send error.
            if (!fetchWeatherBoot()) {
                currentZipCode = oldZip;
                saveSettings();
                mqtt.publish("fridge/feedback/zipStatus", "ERROR");
                triggerAppAlert("INVALID ZIP!", "Restored Old");
            } else {
                mqtt.publish("fridge/feedback/zipStatus", "SUCCESS");
                triggerAppAlert("APP UPDATED!", "ZIP: " + currentZipCode);
            }
        }
    }
    else if (strcmp(topic, "fridge/requestSync") == 0) {
        publishAppFeedback();
    }
    else if (strcmp(topic, "fridge/setHotLimit") == 0) {
        maxHotLimit = constrain(msg.toInt(), TEMP_MIN_RANGE, TEMP_MAX_RANGE);
        saveSettings();
        publishAppFeedback(); // Immediately feedback to the App UI
        triggerAppAlert("APP UPDATED!", "Max Temp: " + String(maxHotLimit) + "C");
    }
    else if (strcmp(topic, "fridge/setColdLimit") == 0) {
        minColdLimit = constrain(msg.toInt(), TEMP_MIN_RANGE, TEMP_MAX_RANGE);
        saveSettings();
        publishAppFeedback(); // Immediately feedback to the App UI
        triggerAppAlert("APP UPDATED!", "Min Temp: " + String(minColdLimit) + "C");
    }
    else if (strcmp(topic, "fridge/forceMode") == 0) {
        isForceMode = (msg == "ON");
        saveSettings();
        
        if (!isForceMode) {
            loadSettings(); 
            publishAppFeedback(); // Send non-forced active variables to app squares
        }
        triggerAppAlert("APP UPDATED!", isForceMode ? "Force Mode: ON" : "Force Mode: OFF");
    }
    else if (strcmp(topic, "fridge/forceTemp") == 0) {
        forceTemp = constrain(msg.toInt(), TEMP_MIN_RANGE, TEMP_MAX_RANGE);
        saveSettings();
        triggerAppAlert("APP UPDATED!", "Force Tmp: " + String(forceTemp) + "C");
    }
    else if (strcmp(topic, "fridge/emergency") == 0) {
        emergencyShutdown = (msg == "ON");
        if (emergencyShutdown) {
            digitalWrite(PIN_COMPRESSOR, LOW);
            digitalWrite(PIN_GEYSER, LOW);
            compressorRunning = false;
            geyserRunning = false;
        }
        triggerAppAlert("EMERGENCY", emergencyShutdown ? "SHUTDOWN ACTIVE" : "SYSTEM RESTORED");
    }
    lcdNeedsUpdate = true;
}

void setupMQTT() {
    #if MQTT_USE_SSL
    espClient.setInsecure();
    #endif
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(512);
}

bool checkMQTTConnection() {
    if (WiFi.status() != WL_CONNECTED) return false;
    int attempts = 0;
    while (!mqtt.connected() && attempts < 2) {
        bool connected = (strlen(MQTT_USER) > 0) ? mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS) : mqtt.connect(MQTT_CLIENT_ID);
        if (connected) {
            mqtt.subscribe("fridge/setZip");
            mqtt.subscribe("fridge/setHotLimit");
            mqtt.subscribe("fridge/setColdLimit");
            mqtt.subscribe("fridge/forceMode");
            mqtt.subscribe("fridge/forceTemp");
            mqtt.subscribe("fridge/requestSync");
            mqtt.subscribe("fridge/emergency");
            return true;
        }
        delay(1000);
        attempts++;
    }
    return mqtt.connected();
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  WEATHER OPERATIONS                                               ║
// ╚══════════════════════════════════════════════════════════════════╗
bool fetchWeatherBoot() {
    if (WiFi.status() != WL_CONNECTED) return false;
    String url = "http://api.openweathermap.org/data/2.5/weather?zip=" + currentZipCode + ",in&appid=" + String(WEATHER_API_KEY) + "&units=metric";
    
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    bool success = false;
    
    if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            weatherTemp      = doc["main"]["temp"]       | 30.0;
            weatherFeelsLike = doc["main"]["feels_like"] | 32.0;
            weatherHumidity  = doc["main"]["humidity"]   | 60.0;
            weatherValid     = true;
            success = true;
        }
    }
    http.end();
    return success;
}

void fetchWeather() {
    fetchWeatherBoot();
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  TEMPERATURE ENGINE SUBSYSTEMS WITH 2 DEGREE DEADBAND            ║
// ╚══════════════════════════════════════════════════════════════════╗
void readWaterTemp() {
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C && temp > -50.0 && temp < 100.0) {
        currentWaterTemp = temp;
    }
}

float calculateIdealWaterTemp() {
    if (isForceMode) return (float)forceTemp;
    float effective = max(weatherTemp, weatherFeelsLike);
    float target;

    if (effective >= 25.0) {
        float rangeLow = 5.0, rangeHigh = 12.0; 
        float heatIntensity = constrain((effective - 25.0) / 25.0, 0.0, 1.0);
        target = rangeHigh - heatIntensity * (rangeHigh - rangeLow);
        target -= (weatherHumidity / 100.0) * 3.0;
        if (effective > 42.0) { target = min(target, rangeLow + 3.0f); }
    } else {
        float coldIntensity = constrain((24.0 - effective) / 24.0, 0.0, 1.0);
        target = 28.0 + coldIntensity * 17.0;
        target += (weatherHumidity / 100.0) * 2.0;
        if (effective < 5.0) { target = max(target, 40.0f); }
    }
    target = constrain(target, (float)minColdLimit, (float)maxHotLimit);
    return constrain(target, (float)TEMP_MIN_RANGE, (float)TEMP_MAX_RANGE);
}

void controlTemperature() {
    if (emergencyShutdown) {
        digitalWrite(PIN_COMPRESSOR, LOW);
        digitalWrite(PIN_GEYSER, LOW);
        compressorRunning = false;
        geyserRunning = false;
        return;
    }
    
    unsigned long now = millis();
    float target = targetWaterTemp;
    
    bool needCooling = currentWaterTemp > (target + TEMP_HYSTERESIS);
    bool needHeating = currentWaterTemp < (target - TEMP_HYSTERESIS);
    bool atTarget = (currentWaterTemp <= target && compressorRunning) || (currentWaterTemp >= target && geyserRunning);

    if (needCooling && !geyserRunning) {
        if (!compressorRunning && (now - compressorStartTime > MIN_COMPRESSOR_CYCLE || compressorStartTime == 0)) {
            digitalWrite(PIN_COMPRESSOR, HIGH);
            compressorRunning = true; compressorStartTime = now;
        }
    } else if (compressorRunning && (atTarget || needHeating)) {
        if (now - compressorStartTime > MIN_COMPRESSOR_CYCLE) {
            digitalWrite(PIN_COMPRESSOR, LOW);
            compressorRunning = false; compressorStartTime = now;
        }
    }

    if (needHeating && !compressorRunning) {
        if (!geyserRunning && (now - geyserStartTime > MIN_GEYSER_CYCLE || geyserStartTime == 0)) {
            digitalWrite(PIN_GEYSER, HIGH);
            geyserRunning = true; geyserStartTime = now;
        }
    } else if (geyserRunning && (atTarget || needCooling)) {
        if (now - geyserStartTime > MIN_GEYSER_CYCLE) {
            digitalWrite(PIN_GEYSER, LOW);
            geyserRunning = false; geyserStartTime = now;
        }
    }
    if (compressorRunning && geyserRunning) { digitalWrite(PIN_GEYSER, LOW); geyserRunning = false; }
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  SMOOTH LCD DISPLAY CORE MATRIX (3 PAGE ZERO-FLICKER ROTATION)  ║
// ╚══════════════════════════════════════════════════════════════════╗
void updateDisplay() {
    if (appAlertActive) {
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print(appAlertLine1);
        lcd.setCursor(0, 1); lcd.print(appAlertLine2);
        return;
    }

    if (displayPage == 0) showDataPage1(); 
    else if (displayPage == 1) showDataPage2();
    else showDataPage3(); 
}

void showDataPage1() {
    lcd.setCursor(0, 0);
    lcd.print(F("Water Temp:")); 
    lcd.print(currentWaterTemp, 1);
    lcd.print(F("C "));

    lcd.setCursor(0, 1);
    if (isForceMode) {
        lcd.print(F("Force Mode: ")); lcd.print(forceTemp); lcd.print(F("C "));
    } else {
        lcd.print(F("Min:")); lcd.print(minColdLimit); lcd.print(F("C "));
        lcd.print(F("Max:")); lcd.print(maxHotLimit); lcd.print(F("C  "));
    }
}

void showDataPage2() {
    lcd.setCursor(0, 0);
    lcd.print(F("ZIP: "));
    lcd.print(currentZipCode);
    lcd.print(F("      "));

    lcd.setCursor(0, 1);
    lcd.print(F("Out Temp: "));
    lcd.print(weatherTemp, 1);
    lcd.print(F("C   "));
}

void showDataPage3() {
    lcd.setCursor(0, 0);
    lcd.print(F("Feels Like:"));
    lcd.print(weatherFeelsLike, 1);
    lcd.print(F("C"));

    lcd.setCursor(0, 1);
    lcd.print(F("Humidity: "));
    lcd.print((int)weatherHumidity);
    lcd.print(F("%   "));
}

// Button logic removed

// ╔══════════════════════════════════════════════════════════════════╗
// ║  LED MULTI-THEME MANAGEMENT                                       ║
// ╚══════════════════════════════════════════════════════════════════╝
void updateServerLed() {
    unsigned long now = millis();
    // Blink green LED for 100ms when data is received/requested
    bool isReceiving = (now - lastMqttDataTime < 100);
    if (isReceiving && mqtt.connected()) {
        digitalWrite(PIN_SRV_G, HIGH);
    } else {
        digitalWrite(PIN_SRV_G, LOW);
    }
}

void updateWifiLed() {
    unsigned long now = millis();
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(PIN_WIFI_R, HIGH);
        digitalWrite(PIN_WIFI_G, LOW);
    } else {
        int rssi = WiFi.RSSI();
        if (rssi <= -70 && rssi != 0) {
            digitalWrite(PIN_WIFI_R, HIGH);
        } else {
            digitalWrite(PIN_WIFI_R, LOW);
        }
        
        if (now - lastWifiBlink > wifiBlinkInterval) {
            lastWifiBlink = now;
            wifiBlinkInterval = random(10, 100);
            int state = (random(100) > 40) ? HIGH : LOW; 
            digitalWrite(PIN_WIFI_G, state);
        }
    }
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  NVS STORAGE SYSTEM                                              ║
// ╚══════════════════════════════════════════════════════════════════╝
void loadSettings() {
    prefs.begin("smartfridge", true);
    maxHotLimit      = prefs.getInt("maxHot", DEFAULT_MAX_HOT);
    minColdLimit     = prefs.getInt("minCold", DEFAULT_MIN_COLD);
    forceTemp        = prefs.getInt("forceTemp", DEFAULT_FORCE_TEMP);
    isForceMode      = prefs.getBool("forceMode", false);
    currentZipCode   = prefs.getString("zip", "110001");
    prefs.end();

    maxHotLimit      = constrain(maxHotLimit, TEMP_MIN_RANGE, TEMP_MAX_RANGE);
    minColdLimit     = constrain(minColdLimit, TEMP_MIN_RANGE, TEMP_MAX_RANGE);
    forceTemp        = constrain(forceTemp, TEMP_MIN_RANGE, TEMP_MAX_RANGE);
}

void saveSettings() {
    prefs.begin("smartfridge", false);
    prefs.putInt("maxHot", maxHotLimit);
    prefs.putInt("minCold", minColdLimit);
    prefs.putInt("forceTemp", forceTemp);
    prefs.putBool("forceMode", isForceMode);
    prefs.putString("zip", currentZipCode);
    prefs.end();
}