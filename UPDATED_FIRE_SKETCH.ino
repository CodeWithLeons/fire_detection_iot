#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

// Wi-Fi credentials
const char* ssid = "LLEONS A11S";         
const char* password = "NEWYEAR2025"; 

// Phone numbers and API keys
String phone_numbers[] = {"2348111789339", "2349027954105", "2348024235740", "2348142746383"};
String api_keys[] = {"3787375", "2244974", "2697445", "9736196"};
const int num_phone_numbers = sizeof(phone_numbers) / sizeof(phone_numbers[0]);

// Sensor thresholds
const int smokeThreshold = 1450;
const int highSmokeThreshold = 1480;
const float tempThreshold = 60.0;

// Sensor pins
const int smokeSensorPin = 34;
const int flameSensorPin = 14;
const int dhtPin = 32;
const int wifiConnectedLED = 2;
const int fireDetectedLED = 4;
const int buzzerPin = 5;
const int powerIndicatorLED = 15; // Power indicator LED

// DHT sensor setup
#define DHTTYPE DHT22
DHT dht(dhtPin, DHTTYPE);

// Timing variables
unsigned long lastAlertTime = 0;
const unsigned long alertCooldown = 45000; // 1-minute cooldown
unsigned long lastWiFiAttempt = 0;
const unsigned long wifiRetryInterval = 30000; // 30 seconds retry

void setup() {
    Serial.begin(9600);
    pinMode(smokeSensorPin, INPUT);
    pinMode(flameSensorPin, INPUT);
    pinMode(wifiConnectedLED, OUTPUT);
    pinMode(fireDetectedLED, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(powerIndicatorLED, OUTPUT); // Set power indicator LED as output
    
    dht.begin(); // Initialize DHT22 sensor

    // Turn off all indicators initially
    digitalWrite(wifiConnectedLED, LOW);
    digitalWrite(fireDetectedLED, LOW);
    digitalWrite(buzzerPin, LOW);
    
    delay(1000); // 1-second delay before turning on power indicator LED
    digitalWrite(powerIndicatorLED, HIGH); // Power indicator LED ON when powered
    
    connectToWiFi();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiAttempt > wifiRetryInterval) {
        connectToWiFi();
    }
    
    int smokeLevel = analogRead(smokeSensorPin);
    int flameDetected = digitalRead(flameSensorPin);
    float temperature = dht.readTemperature();
    
    Serial.print("Smoke Level: "); Serial.println(smokeLevel);
    Serial.print("Temperature: "); Serial.println(temperature);
    Serial.print("Flame Detected: "); Serial.println(flameDetected);
    
    bool fireCondition = (smokeLevel > highSmokeThreshold) || (temperature > tempThreshold) || (flameDetected == 1 && (temperature > tempThreshold || smokeLevel > highSmokeThreshold));

    if (fireCondition && millis() - lastAlertTime > alertCooldown) {
        sendWarningMessage();
        triggerAlarm();
        lastAlertTime = millis();
    }
    
    delay(2000);
}

void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    lastWiFiAttempt = millis();
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi!");
        digitalWrite(wifiConnectedLED, HIGH);
    } else {
        Serial.println("\nWi-Fi Connection Failed!");
        digitalWrite(wifiConnectedLED, LOW); // Turn off WiFi LED when disconnected
    }
}

void sendWarningMessage() {
    if (WiFi.status() == WL_CONNECTED) { 
        digitalWrite(wifiConnectedLED, HIGH); // Turn on WiFi LED when WiFi is back
        HTTPClient http;
        
        for (int i = 0; i < num_phone_numbers; i++) {
            String apiURL = "https://api.callmebot.com/whatsapp.php?phone=" + phone_numbers[i] 
            + "&text=Warning:+Fire+Detected!+Evacuate+Immediately!&apikey=" + api_keys[i];
            http.begin(apiURL);
            int httpResponseCode = http.GET();
            
            if (httpResponseCode > 0) {
                Serial.print("Message sent successfully to ");
                Serial.println(phone_numbers[i]);
            } else {
                Serial.print("Error sending message to ");
                Serial.println(phone_numbers[i]);
            }
            http.end();
            delay(1000);
        }
    } else {
        Serial.println("Error: Wi-Fi not connected!");
        digitalWrite(wifiConnectedLED, LOW); // Ensure WiFi LED is off when disconnected
    }
}

void triggerAlarm() {
    unsigned long alarmStart = millis();
    while (millis() - alarmStart < 60000) { // Run for 60 seconds
        digitalWrite(fireDetectedLED, HIGH);
        digitalWrite(buzzerPin, HIGH);
        delay(100);
        digitalWrite(fireDetectedLED, LOW);
        digitalWrite(buzzerPin, LOW);
        delay(100);
    }
}
