// Test 07 : séquence d'impulsions vers S1 (même montage que le test 06).
//
// Au démarrage : 1500 us pendant 5 s. Puis, en boucle :
//   1600 us (avant), 1500 us (arrêt), 1400 us (arrière), 1500 us (arrêt),
//   4 s chacune. La valeur en cours est écrite sur le port série (115200).
//
// Le Sabertooth garde comme centre la largeur reçue à son allumage, et la
// conserve quand seule l'ESP32 redémarre. Allumer le Sabertooth pendant que
// l'ESP32 envoie 1500 us pour que l'arrêt soit à 1500 us.
const int PIN = 18;

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
  delay(5000);
}

void loop() {
  envoyer(1600); delay(4000);
  envoyer(1500); delay(4000);
  envoyer(1400); delay(4000);
  envoyer(1500); delay(4000);
}
