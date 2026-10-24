// TEST 03 (2026-10-24) : diagnostic du MPU6050 par lecture directe des registres.
// Diagnostic brut du MPU6050 : lecture directe des registres, sans bibliothèque.
// SDA = GPIO 21, SCL = GPIO 22. Moniteur série : 115200 bauds.
#include <Wire.h>

const int PIN_SDA = 21;
const int PIN_SCL = 22;
uint8_t adresse = 0x68;   // 0x69 si la broche AD0 est reliée au 3,3 V

// Écrit une valeur dans un registre. Retourne 0 si tout va bien.
uint8_t ecrire(uint8_t reg, uint8_t valeur) {
  Wire.beginTransmission(adresse);
  Wire.write(reg);
  Wire.write(valeur);
  return Wire.endTransmission();
}

// Lit n octets à partir d'un registre. Retourne le nombre d'octets lus.
int lire(uint8_t reg, uint8_t *tampon, int n) {
  Wire.beginTransmission(adresse);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  int lus = Wire.requestFrom((int)adresse, n);
  for (int i = 0; i < lus; i++) tampon[i] = Wire.read();
  return lus;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[BRUT] Diagnostic MPU6050");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  // Quelles adresses répondent ?
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print("[BRUT] Appareil a l'adresse 0x");
      Serial.println(a, HEX);
    }
  }

  // Identité du composant (registre WHO_AM_I = 0x75).
  uint8_t id = 0;
  if (lire(0x75, &id, 1) == 1) {
    Serial.print("[BRUT] WHO_AM_I = 0x");
    Serial.println(id, HEX);
    Serial.println("       (0x68 = MPU6050, 0x70 = MPU6500, 0x71 = MPU9250, 0x98 = MPU6886)");
  } else {
    Serial.println("[BRUT] Lecture WHO_AM_I impossible : probleme de bus.");
  }

  // Sortir le capteur du mode veille (registre PWR_MGMT_1 = 0x6B).
  uint8_t code = ecrire(0x6B, 0x00);
  Serial.print("[BRUT] Reveil du capteur, code I2C = ");
  Serial.println(code);
  delay(100);
}

void loop() {
  uint8_t d[14];
  int lus = lire(0x3B, d, 14);   // 14 octets : accel (6), temperature (2), gyro (6)
  if (lus != 14) {
    Serial.print("[BRUT] Lecture incomplete : ");
    Serial.print(lus);
    Serial.println(" octets");
    delay(500);
    return;
  }

  int16_t ax = (d[0] << 8) | d[1];
  int16_t ay = (d[2] << 8) | d[3];
  int16_t az = (d[4] << 8) | d[5];
  int16_t tr = (d[6] << 8) | d[7];
  int16_t gx = (d[8] << 8) | d[9];
  int16_t gy = (d[10] << 8) | d[11];
  int16_t gz = (d[12] << 8) | d[13];

  float temperature = tr / 340.0 + 36.53;   // formule de la fiche technique

  Serial.print("accel brut ");
  Serial.print(ax); Serial.print(" "); Serial.print(ay); Serial.print(" "); Serial.print(az);
  Serial.print(" | gyro brut ");
  Serial.print(gx); Serial.print(" "); Serial.print(gy); Serial.print(" "); Serial.print(gz);
  Serial.print(" | temp ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  delay(300);
}
