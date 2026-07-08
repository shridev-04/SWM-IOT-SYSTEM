// ╔══════════════════════════════════════════════════════════════════╗
// ║  SmartFridge ESP32 — OLED 0.96" • 60s Countdown • UI Sync        ║
// ║  SWM IOT SYSTEM • 2°C Hysteresis • Fast Switching • WiFi LED     ║
// ╚══════════════════════════════════════════════════════════════════╝

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define PIN_SRV_G       2
#define PIN_WIFI_R      19
#define PIN_WIFI_G      15

#define DEFAULT_MAX_HOT     40
#define DEFAULT_MIN_COLD    15
#define DEFAULT_FORCE_TEMP  25
#define TEMP_MIN_RANGE      5
#define TEMP_MAX_RANGE      60

#define TEMP_READ_INTERVAL    2000
#define MQTT_PUBLISH_INTERVAL 3000
#define DISPLAY_ROTATE_INTERVAL 3000
#define CONTROL_INTERVAL      500
#define WIFI_CHECK_INTERVAL   10000

#define TEMP_HYSTERESIS        2.0    

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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Preferences prefs;

float currentWaterTemp   = 0.0;
int   maxHotLimit        = DEFAULT_MAX_HOT;
int   minColdLimit       = DEFAULT_MIN_COLD;
bool  isForceMode        = false;
int   forceTemp          = DEFAULT_FORCE_TEMP;
float targetWaterTemp    = 25.0;
String currentOpMode     = "NORMAL";
bool  emergencyShutdown  = false;

bool  compressorRunning  = false;
bool  geyserRunning      = false;

unsigned long minRuntimeEnd      = 0;
unsigned long switchDelayEnd     = 0;

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

int   displayPage        = 0; 
bool  displayNeedsUpdate = true;

unsigned long lastMqttDataTime   = 0;
int   pendingSrvBlinks           = 0;
unsigned long lastSrvToggle      = 0;
bool  srvBlinkState              = false;

unsigned long lastTempRead       = 0;
unsigned long lastMqttPublish    = 0;
unsigned long lastDisplayRotate  = 0;
unsigned long lastWeatherFetch   = 0;
unsigned long lastControlRun     = 0;
unsigned long lastWifiCheck      = 0;

void loadSettings();
void saveSettings();
void publishAppFeedback();
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
void updateWifiLed();
void updateServerLed();

// ╔══════════════════════════════════════════════════════════════════╗
// ║  SETUP                                                           ║
// ╚══════════════════════════════════════════════════════════════════╗
void setup() {
    Serial.begin(115200);

    // FIX: Set relay pins LOW before setting as OUTPUT to prevent startup glitch
    digitalWrite(PIN_COMPRESSOR, LOW); 
    digitalWrite(PIN_GEYSER, LOW);
    pinMode(PIN_COMPRESSOR, OUTPUT);
    pinMode(PIN_GEYSER, OUTPUT);

    pinMode(PIN_SRV_G, OUTPUT);
    pinMode(PIN_WIFI_R, OUTPUT);
    pinMode(PIN_WIFI_G, OUTPUT);
    
    digitalWrite(PIN_SRV_G, LOW);
    digitalWrite(PIN_WIFI_R, HIGH);
    digitalWrite(PIN_WIFI_G, LOW);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.display();

    sensors.begin();
    sensors.setResolution(12);
    readWaterTemp();

    loadSettings();

    executeStrictBootSequence();

    configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
    targetWaterTemp = calculateIdealWaterTemp();
    
    display.clearDisplay();
    displayNeedsUpdate = true;
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
                publishAppFeedback();
            }
        }
    }

    if (appAlertActive && (now - appAlertStartTime > 3000)) {
        appAlertActive = false;
        displayNeedsUpdate = true;
    }

    if (now - lastTempRead > TEMP_READ_INTERVAL) {
        float oldTemp = currentWaterTemp;
        readWaterTemp();
        if (abs(currentWaterTemp - oldTemp) >= 0.1) {
            displayNeedsUpdate = true; 
        }
        lastTempRead = now;
    }

    if (now - lastWeatherFetch > WEATHER_FETCH_INTERVAL) {
        fetchWeather();
        lastWeatherFetch = now;
    }

    // Force display update if countdown is active
    if ((minRuntimeEnd > 0 && now < minRuntimeEnd) || (switchDelayEnd > 0 && now < switchDelayEnd)) {
        displayNeedsUpdate = true;
    }

    if (!appAlertActive && (now - lastDisplayRotate > DISPLAY_ROTATE_INTERVAL)) {
        displayPage = (displayPage + 1) % 3; 
        displayNeedsUpdate = true;
        lastDisplayRotate = now;
    }

    if (displayNeedsUpdate) {
        updateDisplay();
        displayNeedsUpdate = false;
    }

    updateWifiLed();
    updateServerLed();

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
            
            // New Publishers
            char targetStr[10];
            dtostrf(targetWaterTemp, 4, 1, targetStr);
            mqtt.publish("fridge/targetTemp", targetStr);
            mqtt.publish("fridge/opMode", currentOpMode.c_str());
            
            String relayStat = "IDLE (OFF)";
            if (compressorRunning) relayStat = "COMPRESSOR ON";
            else if (geyserRunning) relayStat = "GEYSER ON";
            mqtt.publish("fridge/relayStatus", relayStat.c_str());
            
            String cdownMsg = "0";
            if (minRuntimeEnd > now) {
                cdownMsg = "RUN:" + String((minRuntimeEnd - now) / 1000);
            } else if (switchDelayEnd > now) {
                cdownMsg = "SWITCH:" + String((switchDelayEnd - now) / 1000);
            }
            mqtt.publish("fridge/countdown", cdownMsg.c_str());
        }
        lastMqttPublish = now;
    }
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  STRICT HANDSHAKE BOOT SEQUENCER                                 ║
// ╚════════════════════════════════════════════════════════════════════╝
void oledPrintCenter(String text, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, y);
    display.print(text);
}

void executeStrictBootSequence() {
    display.clearDisplay();
    display.setTextSize(1);
    oledPrintCenter("SWM IOT SYSTEM", 20);
    display.display();
    delay(2500);

    display.clearDisplay();
    oledPrintCenter("Checking WiFi...", 20);
    display.display();
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    bool initialDisconnectedMsgShown = false;
    while (WiFi.status() != WL_CONNECTED) {
        if (!initialDisconnectedMsgShown) {
            display.clearDisplay();
            oledPrintCenter("Please Connect", 20);
            oledPrintCenter("WiFi...", 40);
            display.display();
            initialDisconnectedMsgShown = true;
            digitalWrite(PIN_WIFI_R, HIGH);
            digitalWrite(PIN_WIFI_G, LOW);
        }
        delay(500);
    }
    
    display.clearDisplay();
    oledPrintCenter("WiFi Connected!", 20);
    display.display();
    digitalWrite(PIN_WIFI_R, LOW);
    digitalWrite(PIN_WIFI_G, HIGH);
    delay(1500);

    display.clearDisplay();
    oledPrintCenter("Checking Server...", 20);
    display.display();
    setupMQTT();
    
    if (checkMQTTConnection()) {
        display.clearDisplay();
        oledPrintCenter("Ready with", 20);
        oledPrintCenter("Server", 40);
        display.display();
        delay(200);
    } else {
        display.clearDisplay();
        oledPrintCenter("Server Error!", 20);
        display.display();
    }
    delay(2000);

    display.clearDisplay();
    oledPrintCenter("Checking Weather", 20);
    display.display();
    
    if (fetchWeatherBoot()) {
        display.clearDisplay();
        oledPrintCenter("Ready with", 20);
        oledPrintCenter("Weather", 40);
        display.display();
    } else {
        display.clearDisplay();
        oledPrintCenter("Weather Error!", 20);
        oledPrintCenter("Using Offline Temp", 40);
        display.display();
    }
    delay(2000);

    for (int count = 3; count > 0; count--) {
        display.clearDisplay();
        display.setTextSize(3);
        oledPrintCenter(String(count), 20);
        display.display();
        delay(1000);
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    oledPrintCenter("START!", 20);
    display.display();
    delay(1500);
    
    display.setTextSize(1);
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
    displayNeedsUpdate = true;
}

void publishAppFeedback() {
    if (mqtt.connected()) {
        mqtt.publish("fridge/feedback/hotLimit", String(maxHotLimit).c_str());
        mqtt.publish("fridge/feedback/coldLimit", String(minColdLimit).c_str());
        mqtt.publish("fridge/feedback/forceMode", isForceMode ? "ON" : "OFF");
        mqtt.publish("fridge/feedback/forceTemp", String(forceTemp).c_str());
        mqtt.publish("fridge/feedback/emergency", emergencyShutdown ? "ON" : "OFF");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    lastMqttDataTime = millis();
    pendingSrvBlinks += length; // Blink green LED based on received payload size

    if (strcmp(topic, "fridge/setZip") == 0) {
        if (msg.length() == 6) {
            String oldZip = currentZipCode;
            currentZipCode = msg;
            saveSettings();
            triggerAppAlert("CHECKING ZIP...", "Please Wait");
            
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
        publishAppFeedback();
        triggerAppAlert("APP UPDATED!", "Max Temp: " + String(maxHotLimit) + "C");
    }
    else if (strcmp(topic, "fridge/setColdLimit") == 0) {
        minColdLimit = constrain(msg.toInt(), TEMP_MIN_RANGE, TEMP_MAX_RANGE);
        saveSettings();
        publishAppFeedback();
        triggerAppAlert("APP UPDATED!", "Min Temp: " + String(minColdLimit) + "C");
    }
    else if (strcmp(topic, "fridge/forceMode") == 0) {
        isForceMode = (msg == "ON");
        saveSettings();
        
        if (!isForceMode) {
            loadSettings(); 
            publishAppFeedback(); 
        }
        triggerAppAlert("APP UPDATED!", isForceMode ? "Force Mode: ON" : "Force Mode: OFF");
    }
    else if (strcmp(topic, "fridge/forceTemp") == 0) {
        forceTemp = constrain(msg.toInt(), TEMP_MIN_RANGE, TEMP_MAX_RANGE);
        saveSettings();
        triggerAppAlert("APP UPDATED!", "Force Tmp: " + String(forceTemp) + "C");
    }
    else if (strcmp(topic, "fridge/emergency") == 0) {
        if (msg == "ON") {
            emergencyShutdown = true;
            if(compressorRunning || geyserRunning) {
                switchDelayEnd = millis() + 3000;
            }
            digitalWrite(PIN_COMPRESSOR, LOW);
            digitalWrite(PIN_GEYSER, LOW);
            compressorRunning = false;
            geyserRunning = false;
        } else {
            emergencyShutdown = false;
        }
        triggerAppAlert("EMERGENCY", emergencyShutdown ? "SHUTDOWN ACTIVE" : "SYSTEM RESTORED");
    }
    displayNeedsUpdate = true;
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
// ║  WEATHER OPERATIONS & OFFLINE FALLBACK                            ║
// ╚══════════════════════════════════════════════════════════════════╗
bool fetchWeatherBoot() {
    if (WiFi.status() != WL_CONNECTED) {
        weatherTemp = 27.0;
        weatherFeelsLike = 27.0;
        weatherValid = false;
        return false;
    }
    
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
    
    if (!success) {
        // Offline / Error Fallback
        weatherTemp = 27.0;
        weatherFeelsLike = 27.0;
        weatherValid = false;
    }
    http.end();
    return success;
}

void fetchWeather() {
    fetchWeatherBoot();
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  TEMPERATURE ENGINE SUBSYSTEMS                                   ║
// ╚══════════════════════════════════════════════════════════════════╗
void readWaterTemp() {
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C && temp > -50.0 && temp < 100.0) {
        currentWaterTemp = temp;
    }
}

float calculateIdealWaterTemp() {
    if (isForceMode) {
        currentOpMode = "FORCE";
        return (float)forceTemp;
    }
    
    if (!weatherValid) {
        currentOpMode = "NORMAL";
    } else {
        currentOpMode = "FEELS LIKE";
    }
    
    float effective = max(weatherTemp, weatherFeelsLike);
    float target;

    if (effective >= 25.0) {
        float rangeLow = 5.0, rangeHigh = 18.0; 
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
        minRuntimeEnd = 0;
        switchDelayEnd = 0;
        return;
    }
    
    unsigned long now = millis();
    float target = targetWaterTemp;
    
    bool needCooling = currentWaterTemp > (target + TEMP_HYSTERESIS);
    bool needHeating = currentWaterTemp < (target - TEMP_HYSTERESIS);
    bool atTarget = (currentWaterTemp <= target && compressorRunning) || (currentWaterTemp >= target && geyserRunning);

    // If currently running, check if we need to turn off
    if (compressorRunning) {
        if (now >= minRuntimeEnd) { // Only turn off if 1-min safety is done
            if (atTarget || needHeating) {
                digitalWrite(PIN_COMPRESSOR, LOW);
                compressorRunning = false;
                switchDelayEnd = now + 3000; // Start 3-second switch delay
            }
        }
    } 
    else if (geyserRunning) {
        if (now >= minRuntimeEnd) {
            if (atTarget || needCooling) {
                digitalWrite(PIN_GEYSER, LOW);
                geyserRunning = false;
                switchDelayEnd = now + 3000;
            }
        }
    }

    // If not running, check if we need to turn on
    if (!compressorRunning && !geyserRunning) {
        if (now >= switchDelayEnd) { // Only turn on if 3-sec switch delay is done
            if (needCooling) {
                digitalWrite(PIN_COMPRESSOR, HIGH);
                compressorRunning = true;
                minRuntimeEnd = now + 60000; // Start 1-min safety runtime
            } else if (needHeating) {
                digitalWrite(PIN_GEYSER, HIGH);
                geyserRunning = true;
                minRuntimeEnd = now + 60000;
            }
        }
    }
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  SMOOTH OLED DISPLAY CORE MATRIX                                  ║
// ╚══════════════════════════════════════════════════════════════════╗
void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    
    if (emergencyShutdown) {
        oledPrintCenter("!!! EMERGENCY !!!", 20);
        oledPrintCenter("SYSTEM SHUTDOWN", 40);
        display.display();
        return;
    }

    if (appAlertActive) {
        oledPrintCenter(appAlertLine1, 20);
        oledPrintCenter(appAlertLine2, 40);
        display.display();
        return;
    }

    if (displayPage == 0) showDataPage1(); 
    else if (displayPage == 1) showDataPage2();
    else showDataPage3(); 
    
    // Countdown Overlay
    unsigned long nowTime = millis();
    if (minRuntimeEnd > nowTime) {
        int secondsLeft = (minRuntimeEnd - nowTime) / 1000;
        display.fillRect(0, 54, 128, 10, SSD1306_BLACK);
        display.setCursor(0, 56);
        display.print("Min Run: ");
        display.print(secondsLeft);
        display.print("s");
    } else if (switchDelayEnd > nowTime) {
        int secondsLeft = (switchDelayEnd - nowTime) / 1000;
        display.fillRect(0, 54, 128, 10, SSD1306_BLACK);
        display.setCursor(0, 56);
        display.print("Wait: ");
        display.print(secondsLeft);
        display.print("s");
    }
    
    display.display();
}

void showDataPage1() {
    display.setCursor(0, 0);
    display.print("Water: "); 
    display.print(currentWaterTemp, 1);
    display.print(" C");

    display.setCursor(0, 20);
    if (isForceMode) {
        display.print("Force Mode: "); display.print(forceTemp); display.print(" C");
    } else {
        display.print("Min: "); display.print(minColdLimit); display.print(" C");
        display.setCursor(0, 32);
        display.print("Max: "); display.print(maxHotLimit); display.print(" C");
    }
}

void showDataPage2() {
    display.setCursor(0, 0);
    display.print("ZIP: ");
    display.print(currentZipCode);

    display.setCursor(0, 20);
    display.print("Out Temp: ");
    display.print(weatherTemp, 1);
    display.print(" C");
}

void showDataPage3() {
    display.setCursor(0, 0);
    display.print("Feels Like: ");
    display.print(weatherFeelsLike, 1);
    display.print(" C");

    display.setCursor(0, 20);
    display.print("Humidity: ");
    display.print((int)weatherHumidity);
    display.print(" %");
}

// ╔══════════════════════════════════════════════════════════════════╗
// ║  LED MULTI-THEME MANAGEMENT                                       ║
// ╚══════════════════════════════════════════════════════════════════╝
void updateWifiLed() {
    if (WiFi.status() != WL_CONNECTED || !mqtt.connected()) {
        digitalWrite(PIN_WIFI_R, HIGH);
        digitalWrite(PIN_WIFI_G, LOW);
    } else {
        digitalWrite(PIN_WIFI_R, LOW);
        digitalWrite(PIN_WIFI_G, HIGH);
    }
}

void updateServerLed() {
    unsigned long now = millis();
    if (!mqtt.connected()) {
        digitalWrite(PIN_SRV_G, LOW);
        return;
    }
    
    if (pendingSrvBlinks > 0) {
        if (now - lastSrvToggle >= 30) { 
            srvBlinkState = !srvBlinkState;
            digitalWrite(PIN_SRV_G, srvBlinkState ? HIGH : LOW);
            lastSrvToggle = now;
            if (!srvBlinkState) pendingSrvBlinks--;
        }
    } else {
        digitalWrite(PIN_SRV_G, LOW);
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