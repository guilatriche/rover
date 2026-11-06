// Test 08 : minuterie de surveillance (watchdog) et programme bloqué.
//
// Même montage que le test 06. Le générateur d'impulsions de l'ESP32 émet
// seul : si le programme se fige, les impulsions continuent et les roues
// gardent la dernière vitesse. La minuterie (2 s) redémarre l'ESP32 si elle
// n'est pas remise à zéro, ce qui coupe les impulsions et arrête les roues.
//
// Déroulement : 1500 us pendant 5 s, puis 1600 us. À 10 s, le programme se
// fige volontairement (while (true) {}). Environ 2 s plus tard, la minuterie
// redémarre l'ESP32 (message série : rst:0xc (SW_CPU_RESET)), qui repart à
// 1500 us.
#include <esp_task_wdt.h>

const int PIN = 18;
unsigned long t0;
bool en1600 = false;

void envoyer(int us) {
  uint32_t duty = (uint32_t)(65535UL * (20000 - us) / 20000);
  ledcWrite(PIN, duty);
  Serial.print("Impulsion : ");
  Serial.print(us);
  Serial.println(" us");
}

void setup() {
  Serial.begin(115200);
  ledcAttach(PIN, 50, 16);
  envoyer(1500);
  esp_task_wdt_config_t cfg = { 2000, 0, true };   // délai 2 s
  esp_task_wdt_reconfigure(&cfg);
  esp_task_wdt_add(NULL);
  t0 = millis();
}

void loop() {
  unsigned long t = millis() - t0;
  if (t >= 5000 && !en1600) { envoyer(1600); en1600 = true; }
  if (t < 10000) {
    esp_task_wdt_reset();     // le programme fonctionne : remise à zéro
  } else {
    while (true) {}           // simule un programme bloqué
  }
}
