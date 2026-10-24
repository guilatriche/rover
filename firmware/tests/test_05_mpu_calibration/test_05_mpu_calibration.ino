// Test 05 : étalonnage du MPU6050, mémorisé dans la mémoire flash de l'ESP32.
//
// Sujets MQTT (racine : rover/rover-01) :
//   cmd/calib  "go" (ou tout texte) -> mesure 2 s, mémorise la position « à plat »
//              "reset"              -> efface l'étalonnage (retour aux valeurs brutes)
//   calib      résultat de la dernière demande (JSON)
//   imu        {"roll":..,"pitch":..,"calibrated":true|false}, une fois par seconde
//              {"imu":"error"} si le capteur ne répond pas (jamais de faux zéros)
//
// Principe : l'angle publié = angle mesuré - angle de référence mémorisé.
// La référence est enregistrée dans la flash (Preferences) : elle survit aux
// redémarrages et aux coupures d'alimentation.
// Règle : l'étalonnage est refusé si le capteur bouge pendant la mesure.
//
// Branchement : MPU6050 SDA -> GPIO 21, SCL -> GPIO 22, VCC -> 3,3 V, GND -> GND.
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "secrets.h"

const int PIN_SDA = 21;
const int PIN_SCL = 22;
const char *TOPIC_IMU = "rover/rover-01/imu";
const char *TOPIC_CALIB = "rover/rover-01/calib";
const char *TOPIC_CMD_CALIB = "rover/rover-01/cmd/calib";
const unsigned long PERIODE_MS = 1000;
const int NB_MESURES = 100;                   // 100 mesures x 20 ms = 2 s
const float TOLERANCE_MOUVEMENT_DEG = 1.5;    // écart max toléré pendant la mesure

WiFiClient reseau;
PubSubClient mqtt(reseau);
Adafruit_MPU6050 mpu;
Preferences memoire;

bool mpuOk = false;
bool etalonne = false;
float refRoulis = 0, refTangage = 0;
bool demandeCalib = false, demandeReset = false;
unsigned long tPublication = 0, tTentativeMqtt = 0;

// Lit l'inclinaison brute en degrés. Renvoie false si la lecture échoue.
bool lireInclinaison(float &roulis, float &tangage) {
  sensors_event_t a, g, t;
  if (!mpuOk || !mpu.getEvent(&a, &g, &t)) return false;
  roulis  = atan2(a.acceleration.y, a.acceleration.z) * 180 / PI;
  tangage = atan2(-a.acceleration.x,
                  sqrt(a.acceleration.y * a.acceleration.y +
                       a.acceleration.z * a.acceleration.z)) * 180 / PI;
  return true;
}

// Ramène un écart d'angle dans l'intervalle -180..180.
float normaliser(float angle) {
  while (angle > 180) angle -= 360;
  while (angle < -180) angle += 360;
  return angle;
}

void surMessage(char *topic, byte *payload, unsigned int longueur) {
  if (strcmp(topic, TOPIC_CMD_CALIB) != 0) return;
  if (longueur == 5 && memcmp(payload, "reset", 5) == 0) demandeReset = true;
  else demandeCalib = true;
}

void publierResultat(const char *texte) {
  Serial.println(texte);
  mqtt.publish(TOPIC_CALIB, texte);
}

void faireReset() {
  memoire.begin("mpu", false);
  memoire.clear();
  memoire.end();
  etalonne = false;
  refRoulis = refTangage = 0;
  publierResultat("{\"calib\":\"reset\"}");
}

void faireEtalonnage() {
  float minR = 999, maxR = -999, minT = 999, maxT = -999, sommeR = 0, sommeT = 0;
  for (int i = 0; i < NB_MESURES; i++) {
    float r, t;
    if (!lireInclinaison(r, t)) {
      publierResultat("{\"calib\":\"refused\",\"reason\":\"imu_error\"}");
      return;
    }
    sommeR += r; sommeT += t;
    if (r < minR) minR = r;
    if (r > maxR) maxR = r;
    if (t < minT) minT = t;
    if (t > maxT) maxT = t;
    delay(20);
  }
  if ((maxR - minR) > TOLERANCE_MOUVEMENT_DEG || (maxT - minT) > TOLERANCE_MOUVEMENT_DEG) {
    publierResultat("{\"calib\":\"refused\",\"reason\":\"motion\"}");
    return;
  }
  refRoulis = sommeR / NB_MESURES;
  refTangage = sommeT / NB_MESURES;
  etalonne = true;
  memoire.begin("mpu", false);
  memoire.putFloat("refR", refRoulis);
  memoire.putFloat("refT", refTangage);
  memoire.end();
  char msg[96];
  snprintf(msg, sizeof(msg), "{\"calib\":\"ok\",\"refRoll\":%.2f,\"refPitch\":%.2f}", refRoulis, refTangage);
  publierResultat(msg);
}

void connecterMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  if (tTentativeMqtt != 0 && millis() - tTentativeMqtt < 2000) return;
  tTentativeMqtt = millis();
  Serial.print("[MQTT] connexion... ");
  if (mqtt.connect("rover-01-imu", MQTT_USER, MQTT_PASS)) {
    Serial.println("OK");
    mqtt.subscribe(TOPIC_CMD_CALIB);
  } else {
    Serial.printf("echec, code %d\n", mqtt.state());
  }
}

void publierInclinaison() {
  char message[96];
  float r, t;
  if (lireInclinaison(r, t)) {
    float roulis = normaliser(r - refRoulis);
    float tangage = normaliser(t - refTangage);
    snprintf(message, sizeof(message), "{\"roll\":%.1f,\"pitch\":%.1f,\"calibrated\":%s}",
             roulis, tangage, etalonne ? "true" : "false");
  } else {
    snprintf(message, sizeof(message), "{\"imu\":\"error\"}");
  }
  Serial.println(message);
  mqtt.publish(TOPIC_IMU, message);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[TEST 05] MPU6050 etalonne -> MQTT");

  memoire.begin("mpu", true);                  // lecture seule
  etalonne = memoire.isKey("refR") && memoire.isKey("refT");
  if (etalonne) {
    refRoulis = memoire.getFloat("refR", 0);
    refTangage = memoire.getFloat("refT", 0);
    Serial.printf("[CALIB] reference chargee : roll %.2f, pitch %.2f\n", refRoulis, refTangage);
  } else {
    Serial.println("[CALIB] aucun etalonnage memorise : valeurs brutes");
  }
  memoire.end();

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
  mqtt.setCallback(surMessage);
}

void loop() {
  connecterMqtt();
  mqtt.loop();

  // Les demandes sont traitées ici, pas dans la fonction de réception.
  if (demandeReset) { demandeReset = false; faireReset(); }
  if (demandeCalib) { demandeCalib = false; faireEtalonnage(); }

  if (mqtt.connected() && millis() - tPublication >= PERIODE_MS) {
    tPublication = millis();
    publierInclinaison();
  }
}
