// Test 17 : commande des roues depuis la page de pilotage, modes hold ET latch,
// avec affichage des paquets envoyes au Sabertooth a chaque commande.
// Programme complet. Base : test_16 (meme montage, memes sujets MQTT).
//
// But : savoir d'ou vient l'absence de mouvement sous 20 % de la page.
//   Pour chaque commande recue, le moniteur serie affiche la vitesse demandee,
//   l'octet de vitesse calcule (0..127) et les octets exacts des paquets envoyes.
//   Si les paquets portent un octet non nul et que les roues ne tournent pas,
//   la commande arrive bien : c'est le moteur qui ne demarre pas a cette valeur.
//
// Montage : le meme que les tests 11, 12 et 16.
//   - Sabertooth, interrupteurs B B B H H H (mode paquets, adresse 128, 3 en bas : coupure lithium a 9,0 V), 9600 bauds.
//   - S1 : GPIO 18 -> 10 kOhm -> base du 2N3904 ; collecteur -> S1 et 10 kOhm vers 5V ; emetteur -> 0 V commun.
//   - S2 : 2N3904, emetteur au 0 V commun, collecteur a S2, 10 kOhm de S2 vers 5V, 4,7 kOhm de la base vers 5V,
//          GPIO 19 directement sur la base. GPIO 19 en sortie a 0 V = roues debloquees.
//          GPIO 19 libre (ESP32 eteinte, debranchee ou en redemarrage) = S2 a 0 V = arret.
//   - Roues EN L'AIR. Pile a portee de main.
//
// Regles de ce programme :
//   - Les deux modes sont acceptes, au choix dans la page.
//   - Mode "hold" : la commande doit etre renvoyee au moins toutes les 0,5 s, sinon arret.
//   - Mode "latch" : un seul ordre suffit, la vitesse est gardee tant que des signes de vie
//     (cmd/hb) ou des commandes arrivent. Sans rien pendant 2 s : arret ET commande oubliee
//     (il faut un nouvel ordre). Dans la page : cliquer sur "Envoyer les signes de vie".
//   - Aucune commande n'est appliquee si le broker MQTT n'est pas connecte.
//   - Delai de perte de liaison porte a 2 s pour ce test (1 s au cahier des charges, a trancher avec Tristan).
//   - Vitesse limitee : 100 % de la page = DATA_MAX (40 sur 127), comme au test 16.
//   - Au demarrage : ordres d'arret envoyes pendant 1 s avant de relacher S2 (journal du 7 novembre, essai 5).
//   - Le 170 est renvoye avec chaque paquet (journal du 7 novembre, essai 8).
//   - Minuterie de surveillance de 10 s.
//
// Format d'un paquet (mode paquets du Sabertooth) : adresse, commande, donnee, somme de controle.
//   commande 0 = moteur 1 en avant, commande 4 = moteur 2 en avant ; donnee 0..127 ;
//   somme de controle = (adresse + commande + donnee) & 127.
//
// Sujets MQTT (racine : rover/rover-01) :
//   cmd/drive  JSON {"mode":"hold"|"latch","speed":0..100}
//   cmd/hb     signe de vie (texte quelconque), envoye toutes les 0,3 s par la page
//   status     JSON publie toutes les 2 s et a chaque changement de vitesse
//   online     "true" / "false" (testament)
//
// secrets.h : copier a cote de ce fichier celui qui a servi au test 16 (il n'est jamais publie).

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "secrets.h"

const char *VERSION_PROGRAMME = "2026-11-10-test17";

const int PIN_TX = 18;                        // vers S1, par le transistor
const int PIN_S2 = 19;                        // arret de securite
const byte ADRESSE = 128;                     // adresse du mode paquets
const int DATA_MAX = 40;                      // octet de vitesse pour 100 % (0..127 possible)

const char *RACINE = "rover/rover-01";
const unsigned long DELAI_HOLD_MS = 500;      // hold : arret 0,5 s apres la derniere commande
const unsigned long DELAI_LIEN_MS = 2000;     // arret 2 s apres le dernier signe de vie (2 s pour ce test ; 1 s prevu au cahier des charges)
const unsigned long PERIODE_PAQUET_MS = 100;
const unsigned long PERIODE_STATUT_MS = 2000;

WiFiClient reseau;
PubSubClient mqtt(reseau);

char topicDrive[64], topicHb[64], topicStatus[64], topicOnline[64];

int vitesseVoulue = 0;                        // 0..100, demandee par la page
bool modeMaintenu = true;                     // true = hold, false = latch
int vitesseEnvoyee = -1;                      // derniere vitesse envoyee au Sabertooth
bool lienVivant = false;
unsigned long tDerniereCommande = 0;
unsigned long tDernierSigne = 0;
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

byte octetVitesse(int vitesse) {
  return (byte)(vitesse * DATA_MAX / 100);
}

byte sommeControle(byte cmd, byte data) {
  return (ADRESSE + cmd + data) & 0x7F;
}

void paquet(byte cmd, byte data) {
  Serial1.write(ADRESSE);
  Serial1.write(cmd);
  Serial1.write(data);
  Serial1.write(sommeControle(cmd, data));
}

// Envoie la vitesse (0..100 %) aux deux moteurs, vers l'avant : commande 0 = moteur 1, commande 4 = moteur 2.
void envoyerVitesse(int vitesse) {
  byte data = octetVitesse(vitesse);
  Serial1.write(170);                         // renvoye avec chaque paquet
  paquet(0, data);
  paquet(4, data);
}

// Affiche les octets exacts que envoyerVitesse() met sur la ligne pour cette vitesse.
void afficherPaquets(const char *raison, int vitesse) {
  byte data = octetVitesse(vitesse);
  Serial.printf("[%lu ms] [PAQUETS] %s : vitesse %d %% -> octet %d/127 | envoye : 170 | M1 : %d %d %d %d | M2 : %d %d %d %d\n",
                millis(), raison, vitesse, data,
                ADRESSE, 0, data, sommeControle(0, data),
                ADRESSE, 4, data, sommeControle(4, data));
}

void publierStatut() {
  if (!mqtt.connected()) return;
  StaticJsonDocument<320> doc;
  doc["version"] = VERSION_PROGRAMME;
  doc["uptime"] = millis() / 1000;
  doc["reset"] = causeRedemarrage();
  doc["network"] = "wifi";
  doc["rssi"] = WiFi.RSSI();
  doc["link"] = lienVivant ? "ok" : "lost";
  doc["speed"] = vitesseEnvoyee < 0 ? 0 : vitesseEnvoyee;
  doc["mode"] = modeMaintenu ? "hold" : "latch";
  char tampon[320];
  serializeJson(doc, tampon);
  mqtt.publish(topicStatus, tampon);
}

void surMessage(char *topic, byte *payload, unsigned int longueur) {
  unsigned long maintenant = millis();

  if (strcmp(topic, topicHb) == 0) {          // signe de vie
    tDernierSigne = maintenant;
    return;
  }
  if (strcmp(topic, topicDrive) != 0) return;

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, payload, longueur)) {
    Serial.println("[CMD] JSON invalide, ignore");
    return;
  }
  int vitesse = doc["speed"] | -1;
  const char *mode = doc["mode"] | "hold";
  bool modeValide = (strcmp(mode, "hold") == 0 || strcmp(mode, "latch") == 0);
  if (vitesse < 0 || vitesse > 100 || !modeValide) {
    Serial.println("[CMD] speed (0..100) ou mode (hold/latch) invalide, ignore");
    return;
  }
  bool nouvelle = (vitesse != vitesseVoulue) || (modeMaintenu != (strcmp(mode, "hold") == 0));
  vitesseVoulue = vitesse;
  modeMaintenu = (strcmp(mode, "hold") == 0);
  tDerniereCommande = maintenant;
  tDernierSigne = maintenant;                 // une commande prouve aussi que le lien est vivant

  // En hold, la page repete la commande toutes les 0,15 s : on n'affiche que les changements.
  // En latch, chaque ordre est affiche.
  if (!modeMaintenu || nouvelle) {
    Serial.printf("[%lu ms] [CMD] recue : mode %s, vitesse %d %%\n", maintenant, mode, vitesse);
    afficherPaquets("commande", vitesse);
  }
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
    mqtt.subscribe(topicHb);
    vitesseVoulue = 0;                        // apres une (re)connexion, rien ne repart seul
  } else {
    Serial.printf("echec, code %d\n", mqtt.state());   // 4 = mauvais identifiants, 5 = non autorise, -2 = injoignable
  }
}

// Vitesse reellement permise : zero sans broker, sans commande recente (hold) ou sans signe de vie (latch).
int vitessePermise() {
  unsigned long maintenant = millis();
  if (!mqtt.connected()) { vitesseVoulue = 0; return 0; }

  bool lien = (maintenant - tDernierSigne <= DELAI_LIEN_MS);
  if (lien != lienVivant) {
    lienVivant = lien;
    if (!lien) {
      Serial.printf("[%lu ms] [LIEN] perdu : aucun signe de vie depuis %lu ms, commande oubliee\n",
                    maintenant, maintenant - tDernierSigne);
      vitesseVoulue = 0;                      // la commande est oubliee : il faut un nouvel ordre
    } else {
      Serial.printf("[%lu ms] [LIEN] ok\n", maintenant);
    }
    publierStatut();
  }
  if (!lien) return 0;
  if (modeMaintenu && (maintenant - tDerniereCommande > DELAI_HOLD_MS)) return 0;
  return vitesseVoulue;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[ROVER] test 17 : commande des roues par MQTT (hold et latch), paquets affiches");
  Serial.printf("[ROVER] version %s, dernier redemarrage : %s\n", VERSION_PROGRAMME, causeRedemarrage());

  pinMode(PIN_S2, INPUT);                                  // S2 en arret
  Serial1.begin(9600, SERIAL_8N1, -1, PIN_TX, true);
  Serial.println("[ROVER] ordres d'arret pendant 1 s, S2 encore en arret");
  for (int t = 0; t < 1000; t += 100) { paquet(0, 0); paquet(4, 0); delay(100); }
  pinMode(PIN_S2, OUTPUT);                                 // roues debloquees
  digitalWrite(PIN_S2, LOW);
  Serial.println("[ROVER] S2 relache : roues debloquees, vitesse 0");

  snprintf(topicDrive, sizeof(topicDrive), "%s/cmd/drive", RACINE);
  snprintf(topicHb, sizeof(topicHb), "%s/cmd/hb", RACINE);
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
      afficherPaquets("vitesse envoyee au Sabertooth", v);
      publierStatut();                                     // apres l'envoi : la vitesse affichee est celle envoyee
    }
  }

  if (millis() - tStatut >= PERIODE_STATUT_MS) {
    tStatut = millis();
    publierStatut();
  }
}
