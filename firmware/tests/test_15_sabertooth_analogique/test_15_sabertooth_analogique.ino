// Test 15 : Sabertooth en mode analogique, sensibilite x4 (1,875 V a 3,125 V)
// Interrupteurs (a confirmer sur la fiche) : H H B B H B
// S1 : interface a 2 transistors (schema_analogique) ; S2 : potentiometre regle a 2,50 V
// GPIO 19 -> 1k -> base Q_up ; GPIO 18 -> 10k -> base Q_dn
#include <esp_task_wdt.h>

const bool TEST_GEL = false;   // true : fige le programme quand v >= 0.3 (test E2)
const int PIN_DN = 18;         // Q_dn : fait descendre S1 (recul)
const int PIN_UP = 19;         // Q_up : haut en continu = arret ; moins de temps haut = avance
const int FREQ = 20000;
const int RES = 10;
const int MAXD = 1023;

void commande(float v) {       // v de -1.0 (recul max) a +1.0 (avance max)
  int up = MAXD, dn = 0;
  if (v > 0) up = (int)(MAXD * (1.0 - v));
  else if (v < 0) dn = (int)(MAXD * (-v));
  ledcWrite(PIN_UP, up);
  ledcWrite(PIN_DN, dn);
}

void attendre(int ms) {
  for (int t = 0; t < ms; t += 50) {
    esp_task_wdt_reset();
    delay(50);
  }
}

void palier(float v, int ms) {
  Serial.printf("v = %.2f\n", v);
  if (TEST_GEL && v >= 0.3) {
    Serial.println("GEL du programme");
    while (true) {}
  }
  commande(v);
  attendre(ms);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_UP, INPUT);      // GPIO libres = arret par le montage
  pinMode(PIN_DN, INPUT);
  delay(3000);
  ledcAttach(PIN_UP, FREQ, RES);
  ledcAttach(PIN_DN, FREQ, RES);
  commande(0);
  esp_task_wdt_config_t cfg = {2000, 0, true};
  esp_task_wdt_reconfigure(&cfg);
  esp_task_wdt_add(NULL);
  Serial.println("Demarrage, arret tenu (Sabertooth laissee allumee)");
}

void loop() {
  Serial.println("--- debut du cycle ---");
  palier(0, 4000);
  for (int i = 1; i <= 20; i++) palier(i * 0.05, 2500);   // avance par pas de 5 %
  palier(0, 3000);
  for (int i = 1; i <= 20; i++) palier(-i * 0.05, 2500);  // recul par pas de 5 %
  palier(0, 3000);
}
