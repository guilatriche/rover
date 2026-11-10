// Test 10 : Sabertooth 2x5, mode serie a paquets, adresse 128 (7 nov 2026)
// Interrupteurs : 1 B, 2 B, 3 H (ou B), 4 H, 5 H, 6 H
// Montage : meme adaptateur NPN que le test 09 sur S1 ; 10 kOhm de S2 vers le 5V de la Sabertooth.
// Resultat : M1 avance (commande 0) et recule (commande 1) a 38400 et a 9600 bauds.
// Il faut couper la Sabertooth au moins 5 s quand on change de debit.
// Paquet : adresse, commande, donnee, (adresse + commande + donnee) & 0x7F.

const int PIN_TX = 18;
const byte ADRESSE = 128;

void paquet(byte cmd, byte data) {
  Serial1.write(ADRESSE);
  Serial1.write(cmd);
  Serial1.write(data);
  Serial1.write((ADRESSE + cmd + data) & 0x7F);
}

void tenir(const char* nom, byte cmd, byte data, int duree_ms) {
  Serial.println(nom);
  for (int t = 0; t < duree_ms; t += 100) { paquet(cmd, data); delay(100); }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(38400, SERIAL_8N1, -1, PIN_TX, true);  // true = inverse (transistor). Essaye aussi a 9600.
  delay(2000);
  Serial.println("Envoi du 170 pendant 20 s");
  for (int i = 0; i < 40; i++) { Serial1.write(170); delay(500); }
  Serial.println("Debut des commandes");
}

void loop() {
  tenir("M1 arret", 0, 0, 4000);
  tenir("M1 avance", 0, 40, 4000);
  tenir("M1 arret", 0, 0, 4000);
  tenir("M1 recule", 1, 40, 4000);
}
