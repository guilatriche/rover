// Test 13 : Sabertooth en mode R/C microcontroleur, independant lineaire
// Interrupteurs (fiche + 3 en bas pour le lithium) : B H B B H B
// S1 = roues M1, S2 = roues M2
// Montage : GPIO 18 -> 10k -> base NPN -> S1 ; GPIO 19 -> 10k -> base NPN -> S2
#include <esp_task_wdt.h>

const bool TEST_GEL = false;   // true : fige le programme au premier palier a 1600 us (test E2)
const int CENTRE1 = 1480;      // point d'arret mesure pour S1 (ajuster)
const int CENTRE2 = 1480;      // point d'arret mesure pour S2 (ajuster)
const int PIN_S1 = 18;
const int PIN_S2 = 19;

void impulsion(int pin, int us) {
  uint32_t duty = (uint32_t)(65535UL * (20000 - us) / 20000);  // inverse par le transistor
  ledcWrite(pin, duty);
}

void attendre(int ms) {
  for (int t = 0; t < ms; t += 50) {
    esp_task_wdt_reset();
    delay(50);
  }
}

void palier(int us1, int us2, int ms) {
  Serial.printf("S1=%d  S2=%d\n", us1, us2);
  if (TEST_GEL && us1 == 1600) {
    Serial.println("GEL du programme");
    while (true) {}
  }
  impulsion(PIN_S1, us1);
  impulsion(PIN_S2, us2);
  attendre(ms);
}

void setup() {
  Serial.begin(115200);
  ledcAttach(PIN_S1, 50, 16);
  ledcAttach(PIN_S2, 50, 16);
  impulsion(PIN_S1, CENTRE1);
  impulsion(PIN_S2, CENTRE2);
  for (int i = 3; i > 0; i--) {
    Serial.printf("Demarrage dans %d s (Sabertooth laissee allumee)\n", i);
    delay(1000);
  }
  esp_task_wdt_config_t cfg = {2000, 0, true};
  esp_task_wdt_reconfigure(&cfg);
  esp_task_wdt_add(NULL);
  // Recherche fine du point d'arret, un cote a la fois
  Serial.println("--- point d'arret S1 (S2 a CENTRE2) ---");
  for (int us = 1465; us <= 1495; us += 5) palier(us, CENTRE2, 3000);
  Serial.println("--- point d'arret S2 (S1 a CENTRE1) ---");
  for (int us = 1465; us <= 1495; us += 5) palier(CENTRE1, us, 3000);
  palier(CENTRE1, CENTRE2, 3000);
}

void loop() {
  Serial.println("--- debut du cycle ---");
  palier(CENTRE1, CENTRE2, 4000);
  // S1 (roues M1) : avance de CENTRE1 a 2000 par pas de 25 us
  for (int us = CENTRE1; us <= 2000; us += 25) palier(us, CENTRE2, 2500);
  palier(CENTRE1, CENTRE2, 3000);
  // S1 : recule de CENTRE1 a 1000
  for (int us = CENTRE1; us >= 1000; us -= 25) palier(us, CENTRE2, 2500);
  palier(CENTRE1, CENTRE2, 3000);
  // S2 (roues M2) : avance de CENTRE2 a 2000 par pas de 25 us
  for (int us = CENTRE2; us <= 2000; us += 25) palier(CENTRE1, us, 2500);
  palier(CENTRE1, CENTRE2, 3000);
  // S2 : recule de CENTRE2 a 1000
  for (int us = CENTRE2; us >= 1000; us -= 25) palier(CENTRE1, us, 2500);
  palier(CENTRE1, CENTRE2, 3000);
}
