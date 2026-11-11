// Test 16 : commande des roues depuis la page de pilotage, par MQTT (mode hold seulement).
// Programme complet. Il reunit rover_cmd_led (Wi-Fi, MQTT, commandes) et le test 12 (Sabertooth en mode paquets,
// arret de securite sur S2).
//
// Question de l'essai : une commande envoyee depuis la page de pilotage fait-elle tourner les roues,
// et les roues s'arretent-elles quand on relache le bouton ?
//
// Montage : le meme que les tests 11 et 12.
//   - Sabertooth, interrupteurs B B H H H H (mode paquets, adresse 128, 3 en bas pour la pile lithium), 9600 bauds.
//   - S1 : GPIO 18 -> 10 kOhm -> base du 2N3904 ; collecteur -> S1 et 10 kOhm vers 5V ; emetteur -> 0 V commun.
//   - S2 : 2N3904, emetteur au 0 V commun, collecteur a S2, 10 kOhm de S2 vers 5V, 4,7 kOhm de la base vers 5V,
//          GPIO 19 directement sur la base. GPIO 19 en sortie a 0 V = roues autorisees.
//          GPIO 19 libre (ESP32 eteinte, debranchee ou en redemarrage) = S2 a 0 V = arret.
//   - Roues EN L'AIR. Pile a portee de main.
//
// Regles de ce programme :
//   - Seul le mode "hold" est accepte. Dans la page, choisir "Rester appuye" (hold).
//     La commande doit etre renvoyee au moins toutes les 0,5 s. Sans nouvelle commande pendant 0,5 s : arret.
//   - Aucune commande n'est appliquee si le broker MQTT n'est pas connecte.
//   - Les signes de vie (cmd/hb) ne sont pas utilises dans cet essai.
//   - Vitesse limitee : 100 % de la page = DATA_MAX (40 sur 127), la valeur deja essayee aux tests 11 et 12.
//   - Au demarrage : ordres d'arret envoyes pendant 1 s avant de relacher S2 (journal du 7 novembre, essai 5).
//   - Le 170 est renvoye avec chaque paquet (journal du 7 novembre, essai 8).
//   - Minuterie de surveillance de 10 s (la connexion Wi-Fi ou MQTT peut bloquer quelques secondes) :
//     si le programme se fige, l'ESP32 redemarre, GPIO 19 redevient libre et les roues s'arretent.
//
// Sujets MQTT (racine : rover/rover-01) :
//   cmd/drive  JSON {"mode":"hold","speed":0..100}  -> vitesse des quatre roues, vers l'avant
//   status     JSON publie toutes les 2 s et a chaque changement de vitesse
//   online     "true" / "false" (testament)
//
// secrets.h : copier celui du dossier rover_cmd_led a cote de ce fichier (il n'est jamais publie).

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "secrets.h"

const char *VERSION_PROGRAMME = "2026-11-10-test16";

const int PIN_TX = 18;                        // vers S1, par le transistor
const int PIN_S2 = 19;                        // arret de securite
const byte ADRESSE = 128;                     // adresse du mode paquets
const int DATA_MAX = 40;                      // octet de vitesse pour 100 % (0..127 possible)

const char *RACINE = "rover/rover-01";
const unsigned long DELAI_HOLD_MS = 500;
const unsigned long PERIODE_PAQUET_MS = 100;
const unsigned long PERIODE_STATUT_MS = 2000;

WiFiClient reseau;
PubSubClient mqtt(reseau);

char topicDrive[64], topicStatus[64], topicOnline[64];

int vitesseVoulue = 0;                        // 0..100, demandee par la page
int vitesseEnvoyee = -1;                      // derniere vitesse envoyee au Sabertooth
unsigned long tDerniereCommande = 0;
unsigned long tDernierPaquet = 0;
unsigned long tStatut = 0;
unsigned long tTentativeMqtt = 0;

const char *causeRedemarrage() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "poweron";
    case ESP_RST_EXT:       return "ext";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "watchdog_int";
    case ESP_RST_TASK_WDT:  return "watchdog_task";
    case ESP_RST_WDT:       return "watchdog";
    case ESP_RST_BROWNOUT:  return "brownout";
    default:                return "autre";
  }
}

void paquet(byte cmd, byte data) {
  Serial1.write(ADRESSE);
  Serial1.write(cmd);
  Serial1.write(data);
  Serial1.write((ADRESSE + cmd + data) & 0x7F);
}

// Envoie la vitesse (0..100 %) aux deux moteurs, vers l'avant : commande 0 = moteur 1, commande 4 = moteur 2.
void envoyerVitesse(int vitesse) {
  byte data = (byte)(vitesse * DATA_MAX / 100);
  Serial1.write(170);                         // renvoye avec chaque paquet
  paquet(0, data);
  paquet(4, data);
}

void publierStatut() {
  if (!mqtt.connected()) return;
  StaticJsonDocument<320> doc;
  doc["version"] = VERSION_PROGRAMME;
  doc["uptime"] = millis() / 1000;
  doc["reset"] = causeRedemarrage();
  doc["network"] = "wifi";
  doc["rssi"] = WiFi.RSSI();
  doc["link"] = "ok";                         // ici : le broker MQTT est connecte
  doc["speed"] = vitesseEnvoyee < 0 ? 0 : vitesseEnvoyee;
  doc["mode"] = "hold";
  char tampon[320];
  serializeJson(doc, tampon);
  mqtt.publish(topicStatus, tampon);
}

void surMessage(char *topic, byte *payload, unsigned int longueur) {
  if (strcmp(topic, topicDrive) != 0) return;
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, payload, longueur)) {
    Serial.println("[CMD] JSON invalide, ignore");
    return;
  }
  int vitesse = doc["speed"] | -1;
  const char *mode = doc["mode"] | "hold";
  if (strcmp(mode, "hold") != 0) {
    Serial.printf("[CMD] mode %s refuse : choisir hold dans la page\n", mode);
    return;
  }
  if (vitesse < 0 || vitesse > 100) {
    Serial.println("[CMD] speed (0..100) invalide, ignore");
    return;
  }
  vitesseVoulue = vitesse;
  tDerniereCommande = millis();
  Serial.printf("[%lu ms] [CMD] recue : vitesse %d %%\n", millis(), vitesse);
}

void connecterMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  unsigned long maintenant = millis();
  if (tTentativeMqtt != 0 && maintenant - tTentativeMqtt < 2000) return;
  tTentativeMqtt = maintenant;

  Serial.print("[MQTT] connexion... ");
  if (mqtt.connect("rover-01-moteurs", MQTT_USER, MQTT_PASS, topicOnline, 0, true, "false")) {
    Serial.println("OK");
    mqtt.publish(topicOnline, "true", true);
    mqtt.subscribe(topicDrive);
    vitesseVoulue = 0;
  } else {
    Serial.printf("echec, code %d\n", mqtt.state());   // 4 = mauvais identifiants, 5 = non autorise, -2 = injoignable
  }
}

// Vitesse reellement permise : zero sans broker ou sans commande recente.
int vitessePermise() {
  if (!mqtt.connected()) return 0;
  if (millis() - tDerniereCommande > DELAI_HOLD_MS) return 0;
  return vitesseVoulue;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[ROVER] test 16 : commande des roues par MQTT (hold)");
  Serial.printf("[ROVER] version %s, dernier redemarrage : %s\n", VERSION_PROGRAMME, causeRedemarrage());

  pinMode(PIN_S2, INPUT);                                  // S2 en arret
  Serial1.begin(9600, SERIAL_8N1, -1, PIN_TX, true);
  Serial.println("[ROVER] ordres d'arret pendant 1 s, S2 encore en arret");
  for (int t = 0; t < 1000; t += 100) { paquet(0, 0); paquet(4, 0); delay(100); }
  pinMode(PIN_S2, OUTPUT);                                 // roues autorisees
  digitalWrite(PIN_S2, LOW);
  Serial.println("[ROVER] S2 relache : roues autorisees, vitesse 0");

  snprintf(topicDrive, sizeof(topicDrive), "%s/cmd/drive", RACINE);
  snprintf(topicStatus, sizeof(topicStatus), "%s/status", RACINE);
  snprintf(topicOnline, sizeof(topicOnline), "%s/online", RACINE);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(surMessage);
  mqtt.setKeepAlive(5);
  mqtt.setBufferSize(512);

  esp_task_wdt_config_t cfg = { 10000, 0, true };          // surveillance 10 s
  esp_task_wdt_reconfigure(&cfg);
  esp_task_wdt_add(NULL);
}

void loop() {
  esp_task_wdt_reset();                                    // le programme vit

  static bool wifiAvant = false;
  bool wifiMaintenant = (WiFi.status() == WL_CONNECTED);
  if (wifiMaintenant != wifiAvant) {
    wifiAvant = wifiMaintenant;
    if (wifiMaintenant) Serial.printf("[WIFI] connecte, adresse %s, rssi %d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    else Serial.println("[WIFI] deconnecte");
  }

  connecterMqtt();
  mqtt.loop();

  // Un paquet toutes les 100 ms : la vitesse permise, ou l'arret.
  if (millis() - tDernierPaquet >= PERIODE_PAQUET_MS) {
    tDernierPaquet = millis();
    int v = vitessePermise();
    envoyerVitesse(v);
    if (v != vitesseEnvoyee) {
      vitesseEnvoyee = v;
      Serial.printf("[%lu ms] [ROUES] vitesse envoyee au Sabertooth : %d %%\n", millis(), v);
      publierStatut();                                     // apres l'envoi : la vitesse affichee est celle envoyee
    }
  }

  if (millis() - tStatut >= PERIODE_STATUT_MS) {
    tStatut = millis();
    publierStatut();
  }
}
