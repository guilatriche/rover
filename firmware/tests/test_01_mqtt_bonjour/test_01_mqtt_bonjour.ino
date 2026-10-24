// TEST 01 (2026-10-23) : connexion de l'ESP32 au broker MQTT par le Wi-Fi local.
// Test de connexion : ESP32 -> Wi-Fi -> broker Mosquitto (VM)
// Publie "bonjour N" toutes les secondes sur rover/rover-01/test
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

WiFiClient net;
PubSubClient mqtt(net);
const char *TOPIC_TEST = "rover/rover-01/test";
unsigned long lastMs = 0;
unsigned long counter = 0;

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.printf("Connexion au Wi-Fi %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWi-Fi OK, adresse %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWi-Fi : échec (réseau 2,4 GHz ? mot de passe ?)");
  }
}

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  Serial.print("Connexion MQTT... ");
  if (mqtt.connect("rover-01-esp32", MQTT_USER, MQTT_PASS)) {
    Serial.println("OK");
  } else {
    // 4 = mauvais identifiants, 5 = non autorisé, -2 = broker injoignable
    Serial.printf("échec, code %d\n", mqtt.state());
    delay(2000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  connectWifi();
}

void loop() {
  connectWifi();
  connectMqtt();
  mqtt.loop();
  if (mqtt.connected() && millis() - lastMs >= 1000) {
    lastMs = millis();
    char msg[40];
    snprintf(msg, sizeof(msg), "bonjour %lu", ++counter);
    mqtt.publish(TOPIC_TEST, msg);
    Serial.println(msg);
  }
}
