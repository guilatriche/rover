// Test 09 : Sabertooth 2x5, mode serie simplifie (7 nov 2026)
// Interrupteurs : 1 H, 2 B, 3 H (ou B pour la coupure lithium), 4 B, 5 B, 6 H  -> 38400 bauds
// Montage : GPIO 18 -> 10 kOhm -> base 2N3904 ; collecteur -> S1 et 10 kOhm vers 5V Sabertooth ; emetteur -> 0 V commun
// UART inverse (dernier argument true) car le transistor re-inverse le signal.
// Reconstitue apres la seance : valeurs balayees 0, 64, 96, 127, 32, 1, 192, 224, 160.
// Resultat : 1-127 = moteur 1 (64 arret), 128-255 = moteur 2 (192 arret), 0 = arret des deux.
// Chaque moteur garde sa derniere commande, pas de delai d'attente.

const int PIN_TX = 18;

void envoyer(byte v, int ms) {
  Serial.print("Valeur envoyee : ");
  Serial.println(v);
  for (int t = 0; t < ms; t += 100) {
    Serial1.write(v);
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(38400, SERIAL_8N1, -1, PIN_TX, true);
  delay(2000);
}

void loop() {
  envoyer(0, 4000);
  envoyer(64, 4000);
  envoyer(96, 4000);
  envoyer(127, 4000);
  envoyer(32, 4000);
  envoyer(1, 4000);
  envoyer(192, 4000);
  envoyer(224, 4000);
  envoyer(160, 4000);
}
