#include "secrets.h"

// BLUETOOTH (Dual Mode)
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include "BluetoothSerial.h"

// PHOTODIODE
#include <IRrecv.h>
#include <IRutils.h>

// SCREEN
#include <Wire.h>
#include <U8g2lib.h>

#include <ArduinoJson.h>
#include <NonBlockingRtttl.h>

// SHT20
#include <DFRobot_SHT20.h>

// ------------- CUSTOMABLE -------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define RGB_MAC "BE:27:FA:00:11:6C"

// ПІНИ ДЛЯ ESP-01 МОСТА
#define ESP01_RX_PIN 27
#define ESP01_TX_PIN 26

// TOPICS
#define GLOBAL_STATUS_TOPIC  "jidai/capital/esp32main/global/lwt"
#define PHOTORESISTOR_TOPIC  "jidai/capital/esp32main/photoresistor/telemetry"
#define PHOTOREC_TOPIC       "jidai/capital/esp32main/photorec/telemetry"
#define RADAR_TOPIC          "jidai/capital/esp32main/radar/telemetry"
#define SHT20_TOPIC          "jidai/capital/esp32main/sht20/telemetry"

#define LED_TOPIC            "jidai/capital/esp32main/led/cmd"
#define DISPLAY_TOPIC        "jidai/capital/esp32main/display/cmd"
#define BUZZER_TOPIC         "jidai/capital/esp32main/buzzer/cmd"

#define RELAY_TOPIC          "jidai/capital/esp32main/relay/cmd"
#define RELAY_TOPIC_STATE    "jidai/capital/esp32main/relay/state"
#define RGB_TOPIC            "jidai/capital/esp32main/rgb/cmd"
#define RGB_TOPIC_STATE      "jidai/capital/esp32main/rgb/state"
#define CWQRP_TOPIC          "jidai/capital/esp32main/cwqrp/cmd"

// PINS
#define LED_PIN 2
#define RADAR_PIN 19
#define RELAY_PIN 23
#define BUZZER_PIN 5
#define K_RECV_PIN 18
#define PHOTORESISTOR_PIN 32

// ------------- CONSTANTS -------------

String currentMelody = "";
bool isBuzzerPlaying = false;

// Змінні для BLE (RGB стрічка)
bool pendingBleUpdate = false;
uint8_t targetR = 0, targetG = 0, targetB = 0;
bool bleEnabled = false;
BLEAddress stripAddress(RGB_MAC);
TaskHandle_t bleTaskHandle = NULL;
volatile bool bleUpdateFinished = false;

// Змінні для Bluetooth Classic (CWQRP міст -> HC-06)
BluetoothSerial SerialBT;
uint8_t cwqrpMac[6] = {0x00, 0x22, 0x09, 0x02, 0x74, 0xC3};
bool pendingBtcUpdate = false;
TaskHandle_t btcTaskHandle = NULL;
String btcPayloadQueue = "";

int currentLedPeriod = 0;
int ledBlinkTotalLeft = 0;
unsigned long lastLedBlinkTime = 0;
bool isLedActive = false;

volatile bool radarTriggered = false;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
unsigned long screenShowingTime = 0;
bool isScreenOn = true;

DFRobot_SHT20 sht20;
#define SHT20_PER 10000
unsigned long sht20Timer = 0;

#define LIGHT_PER 10000
unsigned long lightLevelTimer = 0;

IRrecv irrecv(K_RECV_PIN);
decode_results results;

// ------------- UART BRIDGE HELPER -------------

// Єдина функція для відправки даних на ESP-01 з логуванням
bool isBridgeReady = false;

void sendToBridge(String message, bool force = false) {
  if (!isBridgeReady && !force) return;
  
  Serial2.println(message);
  Serial.println("[UART TX] " + message);
}

// ------------- STATE UPDATE -------------

void relayStateUpdate(bool state) {
  String payload = "{\"enable\": " + String(state ? "true" : "false") + "}";
  sendToBridge("PUB_RET|" + String(RELAY_TOPIC_STATE) + "|" + payload);
}

void rgbStateUpdate() {
  String payload = "{\"enable\": " + String(bleEnabled ? "true" : "false") +
                   ", \"red\": " + String(targetR) +
                   ", \"green\": " + String(targetG) +
                   ", \"blue\": " + String(targetB) + "}";
  sendToBridge("PUB_RET|" + String(RGB_TOPIC_STATE) + "|" + payload);
}

// ------------- SITUABLE FUNCTIONS -------------

void printScreen(String str) {
  if (!isScreenOn) {
    display.setPowerSave(0);
    isScreenOn = true;
  }

  display.clearBuffer();
  int lineHeight = 13;
  int y = 13;
  String currentLine = "";

  int i = 0;
  while (i < str.length()) {
    int spaceIndex = str.indexOf(' ', i);
    if (spaceIndex == -1) {
      spaceIndex = str.length();
    }

    String word = str.substring(i, spaceIndex);
    String testLine = currentLine + word + " ";
    int lineWidth = display.getUTF8Width(testLine.c_str());

    if (lineWidth > SCREEN_WIDTH && currentLine.length() > 0) {
      display.setCursor(0, y);
      display.print(currentLine);
      y += lineHeight;
      currentLine = word + " ";
    } else {
      currentLine = testLine;
    }
    i = spaceIndex + 1;
  }

  display.setCursor(0, y);
  display.print(currentLine);
  display.sendBuffer();
  screenShowingTime = millis();
}

// Функція обробки вхідних повідомлень
void processMqttMessage(String topic, String message) {
  Serial.print("Прилетіло в топік: ");
  Serial.println(topic);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("Братан, це не JSON! Помилка: ");
    Serial.println(error.c_str());
    return;
  }

  if (topic == DISPLAY_TOPIC) {
    printScreen(doc["message"].as<String>());

  } else if (topic == BUZZER_TOPIC) {
    currentMelody = doc["melody"].as<String>();
    rtttl::begin(BUZZER_PIN, currentMelody.c_str());
    isBuzzerPlaying = true;

  } else if (topic == RELAY_TOPIC) {
    bool state = doc["enable"].as<bool>();
    digitalWrite(RELAY_PIN, state);
    relayStateUpdate(state);

  } else if (topic == LED_TOPIC) {
    ledBlinkTotalLeft += doc["times"].as<int>() * 2;
    currentLedPeriod = doc["period"].as<int>() / 2;

  } else if (topic == RGB_TOPIC) {
    targetR = doc["red"] | targetR;
    targetG = doc["green"] | targetG;
    targetB = doc["blue"] | targetB;
    bleEnabled = doc["enable"] | bleEnabled;
    pendingBleUpdate = true;

  } else if (topic == CWQRP_TOPIC) {
    btcPayloadQueue = "";

    if (doc.containsKey("icons")) {
      JsonArray icons = doc["icons"];
      for (int i = 0; i < icons.size() && i < 8; i++) {
        btcPayloadQueue += "I:" + String(i) + ":";
        JsonArray iconBytes = icons[i];
        for (int j = 0; j < iconBytes.size() && j < 8; j++) {
          int b = iconBytes[j].as<int>();
          if (b < 16) btcPayloadQueue += "0";
          btcPayloadQueue += String(b, HEX);
        }
        btcPayloadQueue += "\n";
      }
    }

    if (doc.containsKey("strings")) {
      JsonArray strings = doc["strings"];
      for (int i = 0; i < strings.size(); i++) {
        String s = strings[i].as<String>();
        for (int b = 1; b < 8; b++) {
          s.replace(String((char)b), "~" + String(b));
        }
        btcPayloadQueue += "S:" + String(i) + ":" + s + "\n";
      }
      btcPayloadQueue += "R:" + String(strings.size()) + "\n";
    }

    if (btcPayloadQueue.length() > 0) {
      pendingBtcUpdate = true;
    }
  }
}

// ------------- TASKS -------------

void bleTaskCode(void * parameter) {
  Serial.println("[BLE Task] Стукаємо до стрічки на фоні...");
  BLEClient* pClient = BLEDevice::createClient();

  for (int attempt = 0; attempt < 10; attempt++) {
    if (pClient->connect(stripAddress)) {
      Serial.println("[BLE Task] Підключено! Шукаємо сервіс...");
      BLERemoteService* pService = pClient->getService(BLEUUID((uint16_t)0xFFF0));
      if (pService != nullptr) {
        BLERemoteCharacteristic* pChar = pService->getCharacteristic(BLEUUID((uint16_t)0xFFF3));
        if (pChar != nullptr) {
          if (bleEnabled) {
            uint8_t onPayload[9]    = {0x7E, 0x00, 0x04, 0xF0, 0x00, 0x01, 0xFF, 0x00, 0xEF};
            uint8_t colorPayload[9] = {0x7E, 0x00, 0x05, 0x03, targetR, targetG, targetB, 0x00, 0xEF};
            pChar->writeValue(onPayload, 9, false);
            delay(200);
            pChar->writeValue(colorPayload, 9, false);
            Serial.println("[BLE Task] Дуплет відправлено!");
          } else {
            uint8_t offPayload[9] = {0x7E, 0x00, 0x04, 0x00, 0x00, 0x01, 0xFF, 0x00, 0xEF};
            pChar->writeValue(offPayload, 9, false);
            Serial.println("[BLE Task] Стрічку вимкнено!");
          }
        }
      }
      delay(200);
      pClient->disconnect();
      break;

    } else {
      Serial.printf("[BLE Task] Не вдалося :(\nСпроба: %d\n", attempt + 1);
      delay(200);
    }
  }
  delete pClient;
  bleUpdateFinished = true;
  bleTaskHandle = NULL;
  vTaskDelete(NULL);
}

void btcTaskCode(void * parameter) {
  Serial.println("[BTC Task] Підключаємось до HC-06...");
  bool connected = SerialBT.connect(cwqrpMac);

  if (connected) {
    SerialBT.println();
    delay(70);
    Serial.println("[BTC Task] З'єднано! Починаємо відправку по рядках...");
    
    // Парсимо нашу велику чергу по рядках і відправляємо з паузами
    int startIndex = 0;
    while (startIndex < btcPayloadQueue.length()) {
      int endIndex = btcPayloadQueue.indexOf('\n', startIndex);
      
      // Якщо \n більше немає, беремо залишок до кінця рядка
      if (endIndex == -1) {
        endIndex = btcPayloadQueue.length();
      }

      // Вирізаємо один рядок (без \n)
      String singleCommand = btcPayloadQueue.substring(startIndex, endIndex);
      
      if (singleCommand.length() > 0) {
        SerialBT.println(singleCommand); // println сам додасть \n
        Serial.println("[BTC TX]: " + singleCommand);
        
        // КРИТИЧНИЙ МОМЕНТ: Пауза, щоб буфер Arduino Uno встиг переварити команду
        delay(70); 
      }

      // Зсуваємо індекс для наступного проходу
      startIndex = endIndex + 1;
    }

    // Даємо HC-06 час виплюнути останні байти в Ардуїну перед відключенням
    delay(200); 
    SerialBT.disconnect();
    Serial.println("[BTC Task] Відправлено успішно, відключились.");
  } else {
    Serial.println("[BTC Task] HC-06 не знайдено (Ардуїна вимкнена?).");
  }
  
  btcTaskHandle = NULL;
  vTaskDelete(NULL);
}

void IRAM_ATTR handleRadarInterrupt() {
  radarTriggered = true;
}

// ------------- LOOP FUNCTIONS -------------

// Функція, яка слухає ESP-01
void bridgeProcess() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();

    // 1. ЛОГУЄМО ВСЕ, ЩО ПРИЛІТАЄ
    if (data.length() > 0) {
      Serial.println("[UART RX] " + data);
    }

    // 2. ОБРОБЛЯЄМО СИСТЕМНІ ПОВІДОМЛЕННЯ
    if (data == "SYS|REQ_INIT") {
      String initStr = "INIT|" + String(ssid) + "|" + String(password) + "|" + 
                       String(mqtt_server) + "|" + String(mqtt_server_user) + "|" + 
                       String(mqtt_server_password) + "|jidai/capital/esp32main/+/cmd|" + 
                       String(GLOBAL_STATUS_TOPIC);
      
      isBridgeReady = true;
      sendToBridge(initStr); // Відправляємо з логуванням
      printScreen("NET: Sending Config...");
    } 
    else if (data == "SYS|WIFI_OK") {
      printScreen("NET: WiFi OK");
    }
    else if (data == "SYS|MQTT_OK") {
      printScreen("NET: MQTT Connected!");
      // Публікуємо статус онлайн
      sendToBridge("PUB_RET|" + String(GLOBAL_STATUS_TOPIC) + "|{\"status\":\"online\"}");
      relayStateUpdate(false);
    }
    // 3. ОБРОБЛЯЄМО КОМАНДИ З ІНТЕРНЕТУ
    else if (data.startsWith("MSG|")) {
      int firstPipe = data.indexOf('|');
      int secondPipe = data.indexOf('|', firstPipe + 1);
      
      if (firstPipe > 0 && secondPipe > 0) {
        String topic = data.substring(firstPipe + 1, secondPipe);
        String payload = data.substring(secondPipe + 1);
        processMqttMessage(topic, payload);
      }
    }
  }
}

void sht20Process() {
  if (millis() - sht20Timer > SHT20_PER) {
    sht20Timer = millis();
    float temp = sht20.readTemperature();
    float hum = sht20.readHumidity();
    String payload = "{\"temperature\": " + String(temp, 1) +
                     ", \"humidity\": " + String(hum, 1) + "}";
    sendToBridge("PUB|" + String(SHT20_TOPIC) + "|" + payload);
  }
}

void buzzerProcess() {
  rtttl::play();
  if (isBuzzerPlaying && !rtttl::isPlaying()) {
    noTone(BUZZER_PIN);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    isBuzzerPlaying = false;
  }
}

void bleProcess() {
  if (pendingBleUpdate) {
    pendingBleUpdate = false;
    if (bleTaskHandle == NULL && btcTaskHandle == NULL) {
      xTaskCreate(bleTaskCode, "BLE_Task", 8192, NULL, 1, &bleTaskHandle);
    } else {
      pendingBleUpdate = true; 
    }
  }
  if (bleUpdateFinished) {
    bleUpdateFinished = false;
    rgbStateUpdate();
  }
}

void btcProcess() {
  if (pendingBtcUpdate) {
    pendingBtcUpdate = false;
    if (btcTaskHandle == NULL && bleTaskHandle == NULL) {
      xTaskCreate(btcTaskCode, "BTC_Task", 8192, NULL, 1, &btcTaskHandle);
    } else {
      pendingBtcUpdate = true; 
    }
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
    String payload = "{\"active\": " + String(digitalRead(RADAR_PIN)) + "}";
    sendToBridge("PUB|" + String(RADAR_TOPIC) + "|" + payload);
  }
}

void photodiodeProcess() {
  if (millis() - lightLevelTimer > LIGHT_PER) {
    lightLevelTimer = millis();
    float lightLevel = (((float)analogRead(PHOTORESISTOR_PIN)) / 4096) * 100;
    String payload = "{\"level\": " + String(lightLevel) + "}";
    sendToBridge("PUB|" + String(PHOTORESISTOR_TOPIC) + "|" + payload);
  }

  if (irrecv.decode(&results)) {
    String hexCode = String((uint32_t)results.value, HEX);
    String payload = "{\"hex_code\": \"" + hexCode + "\"}";
    sendToBridge("PUB|" + String(PHOTOREC_TOPIC) + "|" + payload);
    
    Serial.print("Зловлено ІЧ код: 0x");
    serialPrintUint64(results.value, HEX);
    Serial.println();
    irrecv.resume();
  }
}

// ------------- SETUP -------------

void setupScreen() {
  display.begin();
  display.enableUTF8Print();
  display.setFont(u8g2_font_6x13_t_cyrillic);
  display.clearBuffer();
  display.setCursor(0, 10);
  display.print("Jidai MAIN BOOTING...");
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  
  // ЗАПУСКАЄМО МІСТ ДО ESP-01
  Serial2.begin(115200, SERIAL_8N1, ESP01_RX_PIN, ESP01_TX_PIN);

  setupScreen();

  sht20.initSHT20();
  delay(100);
  sht20.checkSHT20();

  irrecv.enableIRIn();

  BLEDevice::init("");
  SerialBT.begin("ESP32_Jidai_Master", true); 
  SerialBT.setPin("1234", 4); // Пароль для HC-06
  Serial.println("Bluetooth stacks initialized!");

  pinMode(LED_PIN, OUTPUT);
  pinMode(RADAR_PIN, INPUT);
  pinMode(PHOTORESISTOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(RADAR_PIN), handleRadarInterrupt, CHANGE);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  printScreen("Hardware ready!");
  Serial.println("Waiting for ESP-01 to request config...");
}

// ------------- CORE LOOP -------------

void loop() {
  bridgeProcess();     // Слухаємо ESP-01 та парсимо команди
  buzzerProcess();     // Крутимо музику
  photodiodeProcess(); // Слухаємо світло та ІЧ-пульт
  radarProcess();      // Перевіряємо рух
  ledProcess();        // Блимаємо діодом
  bleProcess();        // Керуємо стрічкою
  btcProcess();        // Кидаємо дані на екран
  screenProcess();     // Вимикаємо дисплей за таймером
  sht20Process();      // Міряємо температуру
}