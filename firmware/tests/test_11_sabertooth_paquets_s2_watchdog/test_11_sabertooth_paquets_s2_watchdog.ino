// Test 11 : Sabertooth 2x5, mode paquets + arret de securite sur S2 + minuterie de surveillance (7 nov 2026)
// Interrupteurs : B B H H H H (3 en bas pour la pile lithium). Debit 9600.
// Montage S1 : voir test 10. Montage S2 : 2N3904, emetteur au 0 V commun, collecteur a S2,
// 10 kOhm de S2 vers 5V, 4,7 kOhm de la base vers 5V, GPIO 19 directement sur la base.
// GPIO 19 en sortie a 0 V = roues autorisees. GPIO 19 libre (entree), ESP32 eteinte ou en reset = arret.
// Resultats deja observes : debrancher l'ESP32 arrete les roues; sans ordre d'arret avant le relachement
// de S2 la Sabertooth reprend sa derniere commande; avec les ordres d'arret, les roues restent immobiles
// pendant le redemarrage; un nouveau programme se televerse sans couper la Sabertooth (meme debit).
// 12:17 : avec le 170 renvoye dans tenir(), une Sabertooth rallumee apres la fenetre demarre tout de suite;
// sans lui elle ne bouge pas (12:10 et 12:21). TEST_GEL simule un programme bloque : la minuterie (2 s) doit redemarrer l'ESP32.

#include <esp_task_wdt.h>

const bool TEST_GEL = false;   // true = le programme se bloque apres un cycle (test du watchdog)

const int PIN_TX = 18;
const int PIN_S2 = 19;
const byte ADRESSE = 128;

void paquet(byte cmd, byte data) {
  Serial1.write(ADRESSE);
  Serial1.write(cmd);
  Serial1.write(data);
  Serial1.write((ADRESSE + cmd + data) & 0x7F);
}

void tenir(const char* nom, byte cmd, byte data, int duree_ms) {
  Serial.println(nom);
  for (int t = 0; t < duree_ms; t += 100) {
    Serial1.write(170);            // renvoye en permanence : la Sabertooth qui se rallume se reconfigure
    paquet(cmd, data);
    esp_task_wdt_reset();          // le programme vit
    delay(100);
  }
}

void arreter_tout(int duree_ms) {
  for (int t = 0; t < duree_ms; t += 100) { paquet(0, 0); paquet(4, 0); delay(100); }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_S2, INPUT);                                // S2 en arret
  Serial1.begin(9600, SERIAL_8N1, -1, PIN_TX, true);
  delay(2000);
  Serial.println("Ordres d'arret, S2 encore en arret");
  arreter_tout(1000);
  pinMode(PIN_S2, OUTPUT);                               // arme
  digitalWrite(PIN_S2, LOW);
  Serial.println("Arme, 170 + arret pendant 20 s");
  for (int i = 0; i < 40; i++) {
    Serial1.write(170);
    paquet(0, 0);
    paquet(4, 0);
    delay(500);
  }
  Serial.println("Debut des commandes");

  esp_task_wdt_config_t cfg = { 2000, 0, true };         // surveillance 2 s
  esp_task_wdt_reconfigure(&cfg);
  esp_task_wdt_add(NULL);
}

void loop() {
  tenir("M1 arret", 0, 0, 4000);
  tenir("M1 avance", 0, 40, 6000);
  tenir("M1 arret", 0, 0, 4000);
  tenir("M1 recule", 1, 40, 6000);
  if (TEST_GEL) {
    Serial.println("GEL : le programme se bloque maintenant");
    while (true) {}                                      // simule un programme bloque
  }
}
