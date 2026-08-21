#include "secrets.h"

// WIFI / MQTT
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>

// ------------- CUSTOMABLE -------------

// TOPICS
#define GLOBAL_STATUS_TOPIC "jidai/capital/sandout/global/lwt"
#define SOIL_MOISTURE_TOPIC "jidai/capital/sandout/soil/telemetry"
#define RADAR_TOPIC         "jidai/capital/sandout/radar/telemetry"
#define MIC_TOPIC           "jidai/capital/sandout/mic/telemetry"

// PINS
#define SOIL_MOISTURE_PIN 0 // Аналоговий вхід для датчика вологості
#define MIC_PIN 1           // Аналоговий вхід для мікрофона
#define RADAR_PIN 4

// ------------- CONSTANTS -------------

volatile bool radarTriggered = false;

// Таймери
#define SOIL_PER 10000
unsigned long soilTimer = 0;

// Мікрофон
#define MIC_THRESHOLD 2500
#define MIC_COOLDOWN 500
unsigned long lastMicTrigger = 0;

WiFiClientSecure espClient;
MQTTClient client(1024); // Створюємо клієнта з буфером 1024 байти

// ------------- INTERRUPTS -------------

void IRAM_ATTR handleRadarInterrupt() {
  radarTriggered = true;
}

// ------------- SETUP FUNCTIONS -------------

void setupWifi() {
  delay(10);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.printf("Сигнал Wi-Fi (RSSI): %d dBm\n", WiFi.RSSI());
  
  espClient.setInsecure();
}

void reconnect() {
  if (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    
    String clientId = "sandout-Jidai";

    // У цій бібліотеці Last Will and Testament налаштовується ДО команди connect
    client.setWill(GLOBAL_STATUS_TOPIC, "{\"status\":\"offline\"}", true, 1);

    // Підключаємося (clientID, username, password)
    if (client.connect(clientId.c_str(), mqtt_server_user, mqtt_server_password)) {
      Serial.println("MQTT: connected!");
      
      // Надсилаємо статус online (retain = true, qos = 1)
      client.publish(GLOBAL_STATUS_TOPIC, "{\"status\":\"online\"}", true, 1);
      
    } else {
      // Функція lastError() поверне конкретний код помилки, якщо щось піде не так
      Serial.print("MQTT Failed, error = ");
      Serial.print(client.lastError());
      Serial.println(" ...trying again in 5 seconds");
      
      delay(5000); 
    }
  }
}

// ------------- LOOP FUNCTIONS -------------

void soilMoistureProcess() {
  if (millis() - soilTimer > SOIL_PER) {
    soilTimer = millis();
    
    int rawValue = analogRead(SOIL_MOISTURE_PIN);
    
    String payload = "{\"soil_raw\": " + String(rawValue) + "}";
    client.publish(SOIL_MOISTURE_TOPIC, payload.c_str());
  }
}

void radarProcess() {
  if (radarTriggered) {
    radarTriggered = false;
    client.publish(RADAR_TOPIC, ("{\"active\": " + String(digitalRead(RADAR_PIN)) + "}").c_str());
  }
}

void micProcess() {
  int rawMic = analogRead(MIC_PIN);
  
  if (rawMic > MIC_THRESHOLD && (millis() - lastMicTrigger > MIC_COOLDOWN)) {
    lastMicTrigger = millis();
    
    String payload = "{\"loudness\": " + String(rawMic) + "}";
    client.publish(MIC_TOPIC, payload.c_str());
    
    Serial.printf("Mic triggered! Loudness: %d\n", rawMic);
  }
}

// ------------- CORE -------------

void setup() {
  Serial.begin(115200);
  delay(3000); // Затримка для ініціалізації USB-порту комп'ютером
  
  setupWifi();
  
  // Ініціалізуємо MQTT-брокер на порту 8883 з використанням TLS-клієнта
  client.begin(mqtt_server, 8883, espClient);

  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(MIC_PIN, INPUT);
  pinMode(RADAR_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(RADAR_PIN), handleRadarInterrupt, CHANGE);

  Serial.println("sandout BOOTING...");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop(); // Крутимо цикл тільки коли підключені
    
    radarProcess();
    soilMoistureProcess();
    micProcess();
  }
}