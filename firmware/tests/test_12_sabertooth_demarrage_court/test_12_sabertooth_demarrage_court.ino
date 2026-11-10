// Test 12 : demarrage court (7 nov 2026). Meme montage que le test 11 (S1 par NPN, S2 par NPN + GPIO 19).
// Le 170 est renvoye avec chaque paquet (toutes les 100 ms) : plus besoin des 20 s de 170 au demarrage.
// Sabertooth eteinte au televersement puis rallumee plusieurs fois : Guillaume rapporte que ca fonctionne.
#include <esp_task_wdt.h>

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
    Serial1.write(170);
    paquet(cmd, data);
    esp_task_wdt_reset();
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_S2, INPUT);                                // S2 en arret
  Serial1.begin(9600, SERIAL_8N1, -1, PIN_TX, true);
  delay(1000);
  for (int t = 0; t < 1000; t += 100) { paquet(0, 0); paquet(4, 0); delay(100); }
  pinMode(PIN_S2, OUTPUT);                               // arme
  digitalWrite(PIN_S2, LOW);
  Serial.println("Arme, debut des commandes");

  esp_task_wdt_config_t cfg = { 2000, 0, true };
  esp_task_wdt_reconfigure(&cfg);
  esp_task_wdt_add(NULL);
}

void loop() {
  tenir("M1 arret", 0, 0, 3000);
  tenir("M1 avance", 0, 40, 4000);
}
