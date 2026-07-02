#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Змінні, які будуть заповнені з UART
String wifi_ssid = "";
String wifi_pass = "";
String mqtt_host = "";
String mqtt_user = "";
String mqtt_pass = "";
String sub_topic = "";

bool isInitialized = false;
unsigned long lastInitReq = 0;

// Зручний парсер рядків по роздільнику (наприклад, по '|')
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// Функція, яка ловить MQTT і кидає в UART
// Оновлена функція, яка ловить MQTT і сплющує JSON у лінію
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MSG|");
  Serial.print(topic);
  Serial.print("|");
  
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    // Викидаємо переноси рядків, щоб ESP32 отримала один суцільний рядок
    if (c != '\n' && c != '\r') {
      Serial.print(c);
    }
  }
  Serial.println(); // Ставимо ЄДИНИЙ кінець рядка для ESP32
}

void setup() {
  Serial.begin(115200);

  espClient.setInsecure(); // Ігноруємо перевірку сертифіката
  client.setCallback(callback);
}

void loop() {
  // 1. СТАНДАРТНЕ ЧИТАННЯ UART (Працює завжди)
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim(); // Забираємо зайві пробіли чи \r
    
    // Якщо ми ще не налаштовані і прилетіла команда INIT
    if (!isInitialized && data.startsWith("INIT|")) {
      wifi_ssid   = getValue(data, '|', 1);
      wifi_pass   = getValue(data, '|', 2);
      mqtt_host   = getValue(data, '|', 3);
      mqtt_user   = getValue(data, '|', 4);
      mqtt_pass   = getValue(data, '|', 5);
      sub_topic   = getValue(data, '|', 6);
      
      client.setServer(mqtt_host.c_str(), 8883);
      isInitialized = true;
      Serial.println("SYS|INIT_OK"); // Сигнал для ESP32, що ми все зрозуміли
    }
    // Якщо ми вже в роботі і прилетіла команда PUB
    else if (isInitialized && data.startsWith("PUB|")) {
      String pub_topic = getValue(data, '|', 1);
      String payload   = data.substring(data.indexOf('|', 4) + 1); // Все, що після другого '|'
      
      if (client.connected()) {
        client.publish(pub_topic.c_str(), payload.c_str());
      }
    }
  }

  // 2. ЯКЩО НЕМАЄ ДАНИХ — КРИЧИМО В UART
  if (!isInitialized) {
    if (millis() - lastInitReq > 2000) {
      lastInitReq = millis();
      Serial.println("SYS|REQ_INIT");
    }
    return; // Блокуємо виконання WiFi/MQTT, поки не отримаємо конфіг
  }

  // 3. ПІДТРИМАННЯ ЗВ'ЯЗКУ
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      delay(500); // Тут можна зробити неблокуюче, але для моста піде
    }
    Serial.println("SYS|WIFI_OK");
  }

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    if (client.connect("ESP01_Bridge", mqtt_user.c_str(), mqtt_pass.c_str())) {
      client.subscribe(sub_topic.c_str());
      Serial.println("SYS|MQTT_OK");
    } else {
      delay(1000); // Таймаут перед наступною спробою
    }
  }

  if (client.connected()) {
    client.loop();
  }
}