// Test 14 : RC microcontroleur (B H B B H B), commande constante pour les tests de panne
// S1 = roues M1 tenues a S1_TENUE ; S2 = roues M2 a l'arret (CENTRE2)
// Montage : GPIO 18 -> 10k -> base NPN -> S1 ; GPIO 19 -> 10k -> base NPN -> S2
#include <esp_task_wdt.h>

const bool TEST_GEL = false;   // true : fige le programme 10 s apres le demarrage (test E2)
const int S1_TENUE = 1600;     // 1600 = avance ; mettre 1465 pour tester l'arret (E3)
const int CENTRE1 = 1465;
const int CENTRE2 = 1465;
const int PIN_S1 = 18;
const int PIN_S2 = 19;

void impulsion(int pin, int us) {
  uint32_t duty = (uint32_t)(65535UL * (20000 - us) / 20000);  // inverse par le transistor
  ledcWrite(pin, duty);
}

void setup() {
  Serial.begin(115200);
  ledcAttach(PIN_S1, 50, 16);
  ledcAttach(PIN_S2, 50, 16);
  impulsion(PIN_S1, CENTRE1);
  impulsion(PIN_S2, CENTRE2);
  delay(3000);
  esp_task_wdt_config_t cfg = {2000, 0, true};
  esp_task_wdt_reconfigure(&cfg);
  esp_task_wdt_add(NULL);
  impulsion(PIN_S1, S1_TENUE);
  Serial.printf("S1 tenu a %d us, S2 a %d us\n", S1_TENUE, CENTRE2);
}

void loop() {
  static unsigned long debut = millis();
  esp_task_wdt_reset();
  if (TEST_GEL && millis() - debut > 10000) {
    Serial.println("GEL du programme");
    while (true) {}
  }
  Serial.printf("%lu s\n", (millis() - debut) / 1000);
  delay(1000);
}
