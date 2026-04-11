// WIFI / MQTT
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
// PHOTODIODE
#include <IRrecv.h>
#include <IRutils.h>
// SCREEN
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // Ширина екрану в пікселях
#define SCREEN_HEIGHT 64 // Висота екрану в пікселях

// Створюємо об'єкт дисплея (вказуємо ширину, висоту, шину I2C і пін скидання (-1 означає, що його немає))
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define LIGHT_PER 10000
#define PHOTORESISTIOR_PIN 32
unsigned long lightLevelTimer = 0;

const uint16_t kRecvPin = 18;  // Пін, куди підключено DAT/OUT приймача
IRrecv irrecv(kRecvPin);
decode_results results;


const char* ssid = "Kramar";
const char* password = "73779706";
const char* mqtt_server = "banew.duckdns.org";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// -------- MELODY --------

const int BUZZER_PIN = 5;

// Частоти нот (в Герцах)
#define NOTE_A4  440
#define NOTE_F4  349
#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_F5  698

// Масив з нотами (0 - це пауза)
int melody[] = {
  NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_C5,
  NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4
};

// Тривалість нот (у мілісекундах)
int durations[] = {
  500, 500, 500, 350, 150,
  500, 350, 150, 1000
};

void playMelody() {
  int numNotes = sizeof(melody) / sizeof(melody[0]);
  
  for (int i = 0; i < numNotes; i++) {
    if (melody[i] == 0) {
      // Якщо в масиві 0 - просто мовчимо (пауза)
      delay(durations[i]);
    } else {
      // Граємо ноту!
      // Формат: tone(ПІН, ЧАСТОТА, ТРИВАЛІСТЬ)
      tone(BUZZER_PIN, melody[i], durations[i]);
      
      // Щоб ноти не зливалися в одну кашу, робимо маленьку паузу між ними
      delay(durations[i] * 1.30); 
    }
  }
}

// ------------------------

void setupScreen() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("ОЛЕДа не знайдено! Перевір дроти."));
    for(;;); // Зупиняємо програму, якщо дисплея нема
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("BANYAK OS BOOTING...");
  display.display();
}

void printScreen(String str) {
  display.clearDisplay(); // Очищаємо старий текст
  display.setCursor(0, 10);
  display.print(str);     // Пишемо новий
  display.display();      // Виводимо на екран
}

// Функція, яка викликається, коли прилітає повідомлення
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Прилетіло в топік: ");
  Serial.println(topic);

  // 1. РОБИМО EQUALS ДЛЯ ТОПІКА (strcmp повертає 0, якщо рядки однакові)
  if (strcmp(topic, "jidai/capital/sensor/esp32main/print") == 0) {
    
    // 2. БЕЗПЕЧНО ПЕРЕТВОРЮЄМО BYTE* В STRING
    String message = "";
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    
    printScreen(message);
  }
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  espClient.setInsecure();
}

void reconnect() {
  // Крутимося, поки не підключимось до MQTT
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Відправляємо тестове повідомлення на бекенд
      client.publish("jidai/capital/sensor/esp32main/status", "Banyak ESP32 is online!");
      // Підписуємось на команди від бекенду
      client.subscribe("jidai/capital/sensor/esp32main/print");
    } else {
      delay(5000);
    }
  }
}

void photodiodeProcess() {

  if (millis() - lightLevelTimer > LIGHT_PER) {
    lightLevelTimer = millis();
    float lightLevel = (((float) analogRead(PHOTORESISTIOR_PIN)) / 4096) * 100;
    client.publish("jidai/capital/sensor/esp32main/light", ("{\"light\": " + String(lightLevel) + "}").c_str());
  }

  if (irrecv.decode(&results)) {
    // Друкуємо код кнопки у форматі HEX
    String hexCode = String((uint32_t)results.value, HEX);
    client.publish("jidai/capital/sensor/esp32main/photodiode", ("{\"hex_code\": \"" + hexCode + "\"}").c_str());

    Serial.print("Зловлено код: 0x");
    serialPrintUint64(results.value, HEX);
    Serial.println();

    // Перезапускаємо приймач для наступної кнопки
    irrecv.resume();
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setupScreen();
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);
  irrecv.enableIRIn();
  playMelody();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // Важливо! Тримає з'єднання живим
  photodiodeProcess();
}