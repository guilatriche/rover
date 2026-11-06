// Test 06 : impulsion de largeur fixe (50 Hz) vers l'entrée S1 du Sabertooth,
// à travers un transistor 2N3904 (adaptateur de niveau logique).
//
// Branchement :
//   GPIO 18 -> 10 kOhm -> base du 2N3904
//   émetteur -> masse commune (GND de l'ESP32 = 0V du Sabertooth)
//   collecteur -> S1 du Sabertooth, et -> 10 kOhm -> borne 5V du Sabertooth
// Sabertooth : interrupteurs 1 bas, 2 haut (radiocommande), 3 bas (lithium),
//   4 bas (indépendant), 5 haut (linéaire), 6 haut (radiocommande standard).
//
// Le signal est inversé par le transistor : broche haute -> S1 à 0 V,
// broche basse -> S1 à 5 V. Pour une impulsion de PULSE_US microsecondes
// à 5 V sur S1, la broche reste basse PULSE_US et haute le reste des 20 ms.
//
// Le Sabertooth prend pour centre (arrêt) la largeur reçue à son allumage.
// Pour que le centre soit 1500 us : PULSE_US = 1500 avant d'allumer le
// Sabertooth (ou avant de couper puis de rallumer son alimentation).
const int PIN = 18;
const int PULSE_US = 1500;   // 1000 à 2000 ; 1500 = arrêt si le centre est 1500

void setup() {
  ledcAttach(PIN, 50, 16);   // 50 Hz, résolution 16 bits (Arduino-ESP32 3.x)
  uint32_t duty = (uint32_t)(65535UL * (20000 - PULSE_US) / 20000);
  ledcWrite(PIN, duty);
}

void loop() {}
