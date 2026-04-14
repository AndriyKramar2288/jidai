#include "secrets.h"

// WIFI / MQTT / Bluetooth
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <NimBLEDevice.h>

// PHOTODIODE
#include <IRrecv.h>
#include <IRutils.h>

// SCREEN
#include <Wire.h>
#include <U8g2lib.h>

#include <ArduinoJson.h>
#include <NonBlockingRtttl.h>

// ------------- CUSTOMABLE -------------

#define SCREEN_WIDTH 128  // Ширина екрану в пікселях
#define SCREEN_HEIGHT 64  // Висота екрану в пікселях
#define RGB_MAC "BE:27:FA:00:11:6C"

// TOPICS
#define GLOBAL_STATUS_TOPIC "jidai/capital/esp32main/global/lwt"
#define LED_TOPIC           "jidai/capital/esp32main/led/cmd"
#define DISPLAY_TOPIC       "jidai/capital/esp32main/display/cmd"
#define BUZZER_TOPIC        "jidai/capital/esp32main/buzzer/cmd"
#define RELAY_TOPIC         "jidai/capital/esp32main/relay/cmd"
#define RGB_TOPIC           "jidai/capital/esp32main/rgb/cmd"
#define PHOTORESISTOR_TOPIC "jidai/capital/esp32main/photoresistor/telemetry"
#define PHOTOREC_TOPIC      "jidai/capital/esp32main/photorec/telemetry"
#define RADAR_TOPIC         "jidai/capital/esp32main/radar/telemetry"

// PINS
#define LED_PIN 2
#define RADAR_PIN 19
#define RELAY_PIN 23
#define BUZZER_PIN 5           // буззер пердунчик маленький мімімі
#define K_RECV_PIN 18          // Пін, куди підключено DAT/OUT приймача, от-той фотодіод для пультика
#define PHOTORESISTOR_PIN 32  // Фоторезистор (аналогово показує освітленість)

// ------------- CONSTANTS -------------

String currentMelody = "";

bool pendingBleUpdate = false;
uint8_t targetR = 0, targetG = 0, targetB = 0;
bool bleEnabled = false;
NimBLEAddress stripAddress(RGB_MAC, 0);

int currentLedPeriod = 0;
int ledBlinkTotalLeft = 0;
unsigned long lastLedBlinkTime = 0;
bool isLedActive = false;

volatile bool radarTriggered = false;

// Створюємо об'єкт дисплея (вказуємо ширину, висоту, шину I2C і пін скидання (-1 означає, що його немає))
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
unsigned long screenShowingTime = 0;
bool isScreenOn = true;

#define LIGHT_PER 10000
unsigned long lightLevelTimer = 0;

IRrecv irrecv(K_RECV_PIN);
decode_results results;

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ------------- SITUABLE FUNCTIONS -------------

void printScreen(String str) {
  // 1. Будимо екран, якщо він спав
  if (!isScreenOn) {
    display.setPowerSave(0);
    isScreenOn = true;
  }
  
  display.clearBuffer();  
  
  // 2. Налаштування шрифту та геометрії
  int lineHeight = 13; // Висота твого шрифту u8g2_font_6x13
  int y = 13;          // Координата Y для першого рядка (в U8g2 це базова лінія знизу букв)
  String currentLine = "";
  
  // 3. Алгоритм розбиття тексту на слова і автопереносу
  int i = 0;
  while (i < str.length()) {
    // Шукаємо найближчий пробіл
    int spaceIndex = str.indexOf(' ', i);
    if (spaceIndex == -1) {
      spaceIndex = str.length(); // Якщо пробілів більше немає, беремо до кінця
    }
    
    // Вирізаємо слово
    String word = str.substring(i, spaceIndex);
    
    // Формуємо тестовий рядок і міряємо його фізичну ширину в пікселях!
    String testLine = currentLine + word + " ";
    int lineWidth = display.getUTF8Width(testLine.c_str());
    
    // Якщо ширина більша за екран (і ми вже хоч щось додали в цей рядок)
    if (lineWidth > SCREEN_WIDTH && currentLine.length() > 0) {
      // Друкуємо те, що влізло
      display.setCursor(0, y);
      display.print(currentLine);
      
      // Переходимо на новий рядок
      y += lineHeight;
      currentLine = word + " "; // Починаємо новий рядок з поточного слова
    } else {
      // Якщо влазить — просто додаємо слово до рядка
      currentLine = testLine;
    }
    
    i = spaceIndex + 1; // Пересуваємось до наступного слова
  }
  
  // 4. Друкуємо останній рядок (хвіст), який залишився в пам'яті
  display.setCursor(0, y);
  display.print(currentLine);
  
  display.sendBuffer();
  screenShowingTime = millis(); 
}

void reconnect() {
  // Крутимося, поки не підключимось до MQTT
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    Serial.print("Free RAM: ");
    Serial.println(ESP.getFreeHeap());

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(),
                       mqtt_server_user,
                       mqtt_server_password,
                       GLOBAL_STATUS_TOPIC,
                       0,
                       true,  // true означає, що брокер запам'ятає це повідомлення (Retain)
                       "{\"status\":\"offline\"}")) {

      Serial.println("connected");
      //printScreen("MQTT: connected!");
      // Відправляємо тестове повідомлення на бекенд
      client.publish(GLOBAL_STATUS_TOPIC, "{\"status\":\"online\"}", true);
      // Підписуємось на команди від бекенду
      client.subscribe(DISPLAY_TOPIC, 1);
      client.subscribe(BUZZER_TOPIC);
      client.subscribe(RELAY_TOPIC, 1);
      client.subscribe(LED_TOPIC);
      client.subscribe(RGB_TOPIC, 1);

    } else {
      //printScreen("MQTT: retrying...");
      delay(1000);
    }
  }
}

// Функція, яка викликається, коли прилітає повідомлення
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Прилетіло в топік: ");
  Serial.println(topic);

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("Братан, це не JSON! Помилка: ");
    Serial.println(error.c_str());
    return;  // Виходимо з функції, щоб не крашнути плату
  }

  // 1. РОБИМО EQUALS ДЛЯ ТОПІКА (strcmp повертає 0, якщо рядки однакові)
  if (strcmp(topic, DISPLAY_TOPIC) == 0) {

    printScreen(doc["message"].as<String>());

  } else if (strcmp(topic, BUZZER_TOPIC) == 0) {
    currentMelody = doc["melody"].as<String>();
    rtttl::begin(BUZZER_PIN, currentMelody.c_str());

  } else if (strcmp(topic, RELAY_TOPIC) == 0) {
    digitalWrite(RELAY_PIN, doc["enable"].as<bool>());

  } else if (strcmp(topic, LED_TOPIC) == 0) {
    ledBlinkTotalLeft += doc["times"].as<int>() * 2;
    currentLedPeriod = doc["period"].as<int>() / 2;

  } else if (strcmp(topic, RGB_TOPIC) == 0) {
    targetR = doc["red"] | targetR;
    targetG = doc["green"] | targetG;
    targetB = doc["blue"] | targetB;  
    bleEnabled = doc["enable"] | bleEnabled;
    
    pendingBleUpdate = true;
  }
}

void IRAM_ATTR handleRadarInterrupt() {
  radarTriggered = true;
}

// ------------- SETUP FUNCTIONS -------------

void setupScreen() {
  display.begin();
  display.enableUTF8Print(); // ВМИКАЄМО МАГІЮ КИРИЛИЦІ!
  
  // Вибираємо кириличний шрифт (їх там багато різних розмірів)
  display.setFont(u8g2_font_6x13_t_cyrillic); 
  
  display.clearBuffer();
  display.setCursor(0, 10);
  display.print("Jidai MAIN BOOTING...");
  display.sendBuffer();
}

void setupWifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  espClient.setInsecure();
}

// ------------- LOOP FUNCTIONS -------------

void bleProcess() {
  if (pendingBleUpdate) {
    pendingBleUpdate = false; // Скидаємо прапорець
    
    Serial.println("[BLE] Стукаємо до стрічки...");
    
    // Створюємо клієнта
    NimBLEClient* pClient = NimBLEDevice::createClient();
    
    // Спробуємо підключитися
    for (int i = 0; i < 10; i++) {
      if (pClient->connect(stripAddress, false)) {
        Serial.println("[BLE] Підключено! Шукаємо сервіс...");
        
        // Шукаємо поштову скриньку FFF0
        NimBLERemoteService* pService = pClient->getService("FFF0");
        if (pService != nullptr) {
          
          // Шукаємо скриньку для команд FFF3
          NimBLERemoteCharacteristic* pChar = pService->getCharacteristic("FFF3");
          if (pChar != nullptr) {
            
            if (bleEnabled) {
              // --- РЕЖИМ УВІМКНЕННЯ ---
              // 1. КОМАНДА "УВІМКНУТИ" (Power ON)
              uint8_t onPayload[9] = {0x7E, 0x00, 0x04, 0xF0, 0x00, 0x01, 0xFF, 0x00, 0xEF};
              pChar->writeValue(onPayload, 9, false);
              
              // Даємо китайському чіпу 50 мілісекунд, щоб прокинутись
              delay(200); 
              
              // 2. КОМАНДА "КОЛІР"
              uint8_t colorPayload[9] = {0x7E, 0x00, 0x05, 0x03, targetR, targetG, targetB, 0x00, 0xEF};
              pChar->writeValue(colorPayload, 9, false);
              
              Serial.println("[BLE] Дуплет відправлено (ON + Color)!");
            } else {
              // --- РЕЖИМ ВИМКНЕННЯ ---
              // КОМАНДА "ВИМКНУТИ" (Power OFF)
              uint8_t offPayload[9] = {0x7E, 0x00, 0x04, 0x00, 0x00, 0x01, 0xFF, 0x00, 0xEF};
              pChar->writeValue(offPayload, 9, false);
              
              Serial.println("[BLE] Стрічку вимкнено (OFF)!");
            }
          }
        }
        // Відключаємось, щоб стрічка знову стала доступною для телефона
        delay(200);
        pClient->disconnect();
        break;
      } else {
        Serial.print("[BLE] Не вдалося підключитися :(\nСпроба: ");
        Serial.println(i + 1);
        delay(150);
      }
    }
    
    // Звільняємо пам'ять
    NimBLEDevice::deleteClient(pClient);
  }
}

void ledProcess() {
  if (ledBlinkTotalLeft > 0) {
    
    if (millis() - lastLedBlinkTime >= currentLedPeriod) {
      lastLedBlinkTime = millis();
      
      isLedActive = !isLedActive;
      digitalWrite(LED_PIN, isLedActive);
      
      ledBlinkTotalLeft--;
      
      if (ledBlinkTotalLeft == 0) {
        isLedActive = false;
        digitalWrite(LED_PIN, LOW);
      }
    }
  }
}

void screenProcess() {
  if (isScreenOn && (millis() - screenShowingTime > 60000)) {
    
    display.setPowerSave(1);
    isScreenOn = false;
  }
}

void radarProcess() {
  if (radarTriggered) {
    radarTriggered = false;
    client.publish(RADAR_TOPIC, ("{\"active\": " + String(digitalRead(RADAR_PIN)) + "}").c_str());
  }
}

void photodiodeProcess() {

  if (millis() - lightLevelTimer > LIGHT_PER) {
    lightLevelTimer = millis();
    float lightLevel = (((float)analogRead(PHOTORESISTOR_PIN)) / 4096) * 100;
    client.publish(PHOTORESISTOR_TOPIC, ("{\"level\": " + String(lightLevel) + "}").c_str(), true);
  }

  if (irrecv.decode(&results)) {
    // Друкуємо код кнопки у форматі HEX
    String hexCode = String((uint32_t)results.value, HEX);
    client.publish(PHOTOREC_TOPIC, ("{\"hex_code\": \"" + hexCode + "\"}").c_str());

    Serial.print("Зловлено код: 0x");
    serialPrintUint64(results.value, HEX);
    Serial.println();

    // Перезапускаємо приймач для наступної кнопки
    irrecv.resume();
  }
}

// ------------- CORE -------------

void setup() {
  Serial.begin(115200);
  setupScreen();
  setupWifi();
  client.setServer(mqtt_server, 8883);
  client.setBufferSize(1024);
  client.setKeepAlive(60);
  client.setSocketTimeout(60);
  client.setCallback(callback);
  irrecv.enableIRIn();
  NimBLEDevice::init("");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  pinMode(LED_PIN, OUTPUT);
  pinMode(RADAR_PIN, INPUT);
  pinMode(PHOTORESISTOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(RADAR_PIN), handleRadarInterrupt, CHANGE);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  printScreen("Successfuly started!");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Важливо! Тримає з'єднання живим
  
  rtttl::play();

  photodiodeProcess();
  radarProcess();
  ledProcess();
  bleProcess();
  screenProcess();
}