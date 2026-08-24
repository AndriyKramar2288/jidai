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
#define MIC_TOPIC           "jidai/capital/sandout/mic/telemetry"

// PINS
#define SOIL1_PIN 0 // Датчик 1
#define MIC_PIN   1 // Мікрофон
#define SOIL2_PIN 3 // Датчик 2
#define SOIL3_PIN 4 // Датчик 3

// ------------- CONSTANTS -------------

// Таймери
#define SOIL_PER 10000
unsigned long soilTimer = 0;

// Мікрофон
#define MIC_THRESHOLD 2500
#define MIC_COOLDOWN 500
unsigned long lastMicTrigger = 0;

WiFiClientSecure espClient;
MQTTClient client(1024);

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

    client.setWill(GLOBAL_STATUS_TOPIC, "{\"status\":\"offline\"}", true, 1);

    if (client.connect(clientId.c_str(), mqtt_server_user, mqtt_server_password)) {
      Serial.println("MQTT: connected!");
      client.publish(GLOBAL_STATUS_TOPIC, "{\"status\":\"online\"}", true, 1);
    } else {
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
    
    // Читаємо всі три датчики. 
    // Між зчитуваннями аналогових пінів бажано робити мікрозатримку, 
    // щоб АЦП встиг переключити мультиплексор і стабілізуватися.
    int raw1 = analogRead(SOIL1_PIN);
    delay(50);
    int raw2 = analogRead(SOIL2_PIN);
    delay(50);
    int raw3 = analogRead(SOIL3_PIN);
    
    // Пакуємо всі три значення в один зручний JSON
    String payload = "{\"soil1_raw\": " + String(raw1) + 
                     ", \"soil2_raw\": " + String(raw2) + 
                     ", \"soil3_raw\": " + String(raw3) + "}";
                     
    client.publish(SOIL_MOISTURE_TOPIC, payload.c_str());
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
  delay(3000); 
  
  setupWifi();
  client.begin(mqtt_server, 8883, espClient);

  pinMode(SOIL1_PIN, INPUT);
  pinMode(SOIL2_PIN, INPUT);
  pinMode(SOIL3_PIN, INPUT);
  pinMode(MIC_PIN, INPUT);

  Serial.println("sandout BOOTING (Radar-free edition)...");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop(); 
    
    soilMoistureProcess();
    micProcess();
  }
}