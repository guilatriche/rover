// Commande d'une LED par MQTT avec arrêt de sécurité (banc d'essai avant les moteurs).
//
// Sujets MQTT (racine : rover/rover-01) :
//   cmd/drive  JSON {"mode":"hold"|"latch","speed":0..100}  -> luminosité de la LED
//   cmd/hb     texte quelconque                              -> signal de vie
//   status     JSON publié toutes les 2 s (lien, vitesse, mode, rssi)
//   online     "true" / "false" (retenu ; "false" est publié par le testament
//              du broker si l'ESP32 disparaît)
//
// Règles de sécurité :
//   - aucun message cmd/drive ni cmd/hb pendant 1 s   -> arrêt (liaison perdue)
//   - mode "hold" : aucune nouvelle commande pendant 0,5 s -> arrêt
//   - à chaque perte de liaison, la commande mémorisée est effacée : au retour
//     de la liaison, on repart de zéro (rien ne redémarre tout seul)
//
// Branchement : LED + résistance de 220 à 330 ohms entre GPIO 14 et GND.
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

const int PIN_LED = 14;
const int PWM_FREQ = 5000;
const int PWM_RES = 8;                        // 0..255

// Essai : forcer le robot sur un point d'accès précis (adresse BSSID et canal).
// Mettre FORCER_POINT_ACCES à 0 pour revenir au choix automatique.
#define FORCER_POINT_ACCES 0
const uint8_t BSSID_CIBLE[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};   // à remplir seulement pour un essai
const int CANAL_CIBLE = 1;

// Version du programme : à changer à chaque modification publiée (date + mot-clé).
const char *VERSION_PROGRAMME = "2026-10-26-diag";

const char *RACINE = "rover/rover-01";
const unsigned long DELAI_LIEN_MS = 1000;     // signal de vie
const unsigned long DELAI_HOLD_MS = 500;      // mode maintenu
const unsigned long PERIODE_STATUT_MS = 2000;

WiFiClient reseau;
PubSubClient mqtt(reseau);

char topicDrive[64], topicHb[64], topicStatus[64], topicOnline[64];

int vitesseDemandee = 0;                      // 0..100
bool modeMaintenu = true;
unsigned long tDerniereCommande = 0;
unsigned long tDernierSigne = 0;              // 0 = aucun signe depuis la connexion
bool lienOk = false;
int vitesseAppliquee = -1;
unsigned long tStatut = 0;
unsigned long tTentativeMqtt = 0;

// Cause du dernier redémarrage : "poweron" (mise sous tension), "brownout" (tension trop basse),
// "panic" (plantage du programme), "watchdog" (programme bloqué), "software", etc.
const char *causeRedemarrage() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "poweron";
    case ESP_RST_EXT:       return "ext";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "watchdog_int";
    case ESP_RST_TASK_WDT:  return "watchdog_task";
    case ESP_RST_WDT:       return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT:  return "brownout";
    default:                return "autre";
  }
}

void appliquerLed(int vitesse) {
  if (vitesse == vitesseAppliquee) return;
  vitesseAppliquee = vitesse;
  ledcWrite(PIN_LED, vitesse * 255 / 100);
  Serial.printf("[LED] vitesse %d %%\n", vitesse);
}

void publierStatut() {
  if (!mqtt.connected()) return;
  StaticJsonDocument<384> doc;
  doc["version"] = VERSION_PROGRAMME;
  doc["uptime"] = millis() / 1000;
  doc["reset"] = causeRedemarrage();          // cause du dernier redémarrage
  doc["silence"] = tDernierSigne == 0 ? -1 : (long)(millis() - tDernierSigne);  // ms depuis le dernier signe de vie
  doc["network"] = "wifi";
  doc["rssi"] = WiFi.RSSI();
  doc["bssid"] = WiFi.BSSIDstr();             // point d'accès auquel le robot est connecté
  doc["canal"] = WiFi.channel();
  doc["link"] = lienOk ? "ok" : "lost";
  doc["speed"] = vitesseAppliquee < 0 ? 0 : vitesseAppliquee;
  doc["mode"] = modeMaintenu ? "hold" : "latch";
  char tampon[384];
  serializeJson(doc, tampon);
  mqtt.publish(topicStatus, tampon);
}

void surMessage(char *topic, byte *payload, unsigned int longueur) {
  unsigned long maintenant = millis();

  if (strcmp(topic, topicHb) == 0) {          // signal de vie
    tDernierSigne = maintenant;
    return;
  }

  if (strcmp(topic, topicDrive) == 0) {       // commande
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, payload, longueur)) {
      Serial.println("[CMD] JSON invalide, ignore");
      return;
    }
    int vitesse = doc["speed"] | -1;          // -1 si absent ou pas un entier
    const char *mode = doc["mode"] | "hold";
    bool modeValide = (strcmp(mode, "hold") == 0 || strcmp(mode, "latch") == 0);
    if (vitesse < 0 || vitesse > 100 || !modeValide) {
      Serial.println("[CMD] speed (0..100) ou mode (hold/latch) invalide, ignore");
      return;
    }
    vitesseDemandee = vitesse;
    modeMaintenu = (strcmp(mode, "hold") == 0);
    tDerniereCommande = maintenant;
    tDernierSigne = maintenant;               // une commande prouve aussi que le lien est vivant
    Serial.printf("[CMD] mode %s, vitesse %d %%\n", mode, vitesse);
  }
}

void connecterMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  unsigned long maintenant = millis();
  if (tTentativeMqtt != 0 && maintenant - tTentativeMqtt < 2000) return;
  tTentativeMqtt = maintenant;

  Serial.print("[MQTT] connexion... ");
  // Testament : si l'ESP32 disparaît, le broker publie "false" sur le sujet online.
  if (mqtt.connect("rover-01-led", MQTT_USER, MQTT_PASS, topicOnline, 0, true, "false")) {
    Serial.println("OK");
    mqtt.publish(topicOnline, "true", true);
    mqtt.subscribe(topicDrive);
    mqtt.subscribe(topicHb);
    tDernierSigne = 0;                        // on attend un premier signe de vie
    vitesseDemandee = 0;
  } else {
    // 4 = mauvais identifiants, 5 = non autorisé, -2 = broker injoignable
    Serial.printf("echec, code %d\n", mqtt.state());
  }
}

void surveillerLien() {
  unsigned long maintenant = millis();
  bool vie = mqtt.connected() && tDernierSigne != 0 &&
             (maintenant - tDernierSigne <= DELAI_LIEN_MS);

  bool changement = (vie != lienOk);
  if (changement) {
    lienOk = vie;
    if (vie) Serial.println("[LIEN] etabli");
    else Serial.printf("[LIEN] perdu : arret (silence %lu ms, mqtt %s, wifi rssi %d, bssid %s, canal %d)\n",
                       maintenant - tDernierSigne, mqtt.connected() ? "connecte" : "deconnecte", WiFi.RSSI(),
                       WiFi.BSSIDstr().c_str(), WiFi.channel());
  }

  if (!vie) {                                 // liaison perdue : arrêt et oubli de la commande
    vitesseDemandee = 0;
    appliquerLed(0);
    if (changement) publierStatut();          // publié après l'arrêt, pour afficher la vraie vitesse
    return;
  }

  if (modeMaintenu && vitesseDemandee != 0 &&
      maintenant - tDerniereCommande > DELAI_HOLD_MS) {
    vitesseDemandee = 0;
    Serial.println("[HOLD] commande expiree : arret");
  }
  appliquerLed(vitesseDemandee);
  if (changement) publierStatut();            // publié après appliquerLed : vitesse à jour
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[ROVER] commande LED par MQTT");
  Serial.printf("[ROVER] version %s\n", VERSION_PROGRAMME);
  Serial.printf("[ROVER] cause du dernier redemarrage : %s\n", causeRedemarrage());

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);                 // évite la lueur du démarrage (pull-up interne)
  ledcAttach(PIN_LED, PWM_FREQ, PWM_RES);     // API des cœurs ESP32 3.x
  ledcWrite(PIN_LED, 0);

  snprintf(topicDrive, sizeof(topicDrive), "%s/cmd/drive", RACINE);
  snprintf(topicHb, sizeof(topicHb), "%s/cmd/hb", RACINE);
  snprintf(topicStatus, sizeof(topicStatus), "%s/status", RACINE);
  snprintf(topicOnline, sizeof(topicOnline), "%s/online", RACINE);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                       // pas d'économie d'énergie Wi-Fi : moins de retards d'arrivée
#if FORCER_POINT_ACCES
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, CANAL_CIBLE, BSSID_CIBLE);
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(surMessage);
  mqtt.setKeepAlive(5);
  mqtt.setBufferSize(512);                    // statut plus long avec la version (défaut : 256 octets)
}

void loop() {
  static bool wifiAvant = false;
  bool wifiMaintenant = (WiFi.status() == WL_CONNECTED);
  if (wifiMaintenant != wifiAvant) {
    wifiAvant = wifiMaintenant;
    if (wifiMaintenant) Serial.printf("[WIFI] connecte, adresse %s, rssi %d, bssid %s, canal %d\n", WiFi.localIP().toString().c_str(),
                                        WiFi.RSSI(), WiFi.BSSIDstr().c_str(), WiFi.channel());
    else Serial.println("[WIFI] deconnecte");
  }

  connecterMqtt();
  mqtt.loop();
  surveillerLien();

  if (millis() - tStatut >= PERIODE_STATUT_MS) {
    tStatut = millis();
    publierStatut();
  }
}
