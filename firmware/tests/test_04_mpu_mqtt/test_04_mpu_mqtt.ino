// Test 04 : le MPU6050 publie son inclinaison brute sur MQTT, une fois par seconde.
// Aucun étalonnage, aucune alarme : on veut seulement voir les valeurs arriver au broker.
//
// Sujet : rover/rover-01/imu   exemple : {"roll":-1.2,"pitch":0.8}
// Si le capteur ne répond pas : {"imu":"error"} (jamais de faux zéros).
//
// Branchement : MPU6050 SDA -> GPIO 21, SCL -> GPIO 22, VCC -> 3,3 V, GND -> GND.
// Moniteur série : 115200 bauds.
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "secrets.h"

const int PIN_SDA = 21;
const int PIN_SCL = 22;
const char *TOPIC_IMU = "rover/rover-01/imu";
const unsigned long PERIODE_MS = 1000;

WiFiClient reseau;
PubSubClient mqtt(reseau);
Adafruit_MPU6050 mpu;
bool mpuOk = false;
unsigned long tPublication = 0;
unsigned long tTentativeMqtt = 0;

void connecterMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  if (tTentativeMqtt != 0 && millis() - tTentativeMqtt < 2000) return;
  tTentativeMqtt = millis();
  Serial.print("[MQTT] connexion... ");
  if (mqtt.connect("rover-01-imu", MQTT_USER, MQTT_PASS)) {
    Serial.println("OK");
  } else {
    Serial.printf("echec, code %d\n", mqtt.state());   // 4 ou 5 : identifiants refusés
  }
}

void publierInclinaison() {
  char message[80];
  sensors_event_t a, g, t;
  // getEvent renvoie false si la lecture échoue
  if (mpuOk && mpu.getEvent(&a, &g, &t)) {
    float roulis  = atan2(a.acceleration.y, a.acceleration.z) * 180 / PI;
    float tangage = atan2(-a.acceleration.x,
                          sqrt(a.acceleration.y * a.acceleration.y +
                               a.acceleration.z * a.acceleration.z)) * 180 / PI;
    snprintf(message, sizeof(message), "{\"roll\":%.1f,\"pitch\":%.1f}", roulis, tangage);
  } else {
    snprintf(message, sizeof(message), "{\"imu\":\"error\"}");
  }
  Serial.println(message);
  mqtt.publish(TOPIC_IMU, message);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[TEST 04] MPU6050 -> MQTT");

  Wire.begin(PIN_SDA, PIN_SCL);
  mpuOk = mpu.begin();
  if (mpuOk) {
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    Serial.println("[MPU] MPU6050 pret.");
  } else {
    Serial.println("[MPU] MPU6050 introuvable : verifiez SDA, SCL, 3,3 V et GND.");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
}

void loop() {
  connecterMqtt();
  mqtt.loop();

  if (mqtt.connected() && millis() - tPublication >= PERIODE_MS) {
    tPublication = millis();
    publierInclinaison();
  }
}
