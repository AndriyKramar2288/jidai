#include "secrets.h"

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

ADC_MODE(ADC_VCC);

#define RADAR_TOPIC "jidai/alaska/petro-noroshi/radar/telemetry"

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup() {
  // put your setup code here, to run once:
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
  }

  espClient.setInsecure();
  client.setServer(mqtt_server, 8883); 

  while (!client.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (!client.connect(clientId.c_str(), mqtt_server_user, mqtt_server_password)) {
      delay(500);
    }
  }

  WiFi.setSleepMode(WIFI_MODEM_SLEEP);
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  // ===================

  float vcc = ESP.getVcc() / 1000.0;
  String payload = "{\"battery\": " + String(vcc) + ", \"active\": true}";

  client.publish(RADAR_TOPIC, payload.c_str(), true);

  // ===================

  for(int i=0; i<10; i++) {
    client.loop();
    delay(10);
  }

  ESP.deepSleep(0);
}

void loop() {
  // put your main code here, to run repeatedly:

}