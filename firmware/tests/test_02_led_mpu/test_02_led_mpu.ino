// TEST 02 (2026-10-24) : LED sur GPIO 14 et MPU6050 (bibliothèque Adafruit).
// Test matériel : LED sur GPIO 14 et MPU6050 sur I2C (SDA 21, SCL 22).
// Moniteur série : 115200 bauds.
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

const int PIN_LED = 14;
const int PIN_SDA = 21;
const int PIN_SCL = 22;

Adafruit_MPU6050 mpu;
bool mpuOk = false;

// Cherche les appareils I2C branchés et affiche leur adresse.
void scanI2C() {
  Serial.println("[I2C] Recherche d'appareils...");
  int trouves = 0;
  for (uint8_t adresse = 1; adresse < 127; adresse++) {
    Wire.beginTransmission(adresse);
    if (Wire.endTransmission() == 0) {
      Serial.print("[I2C] Appareil trouve a l'adresse 0x");
      Serial.println(adresse, HEX);
      trouves++;
    }
  }
  if (trouves == 0) Serial.println("[I2C] Aucun appareil : verifiez SDA, SCL, 3,3 V et GND.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[TEST] LED + MPU6050");

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);
  scanI2C();

  if (mpu.begin()) {
    mpuOk = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    Serial.println("[MPU] MPU6050 pret.");
  } else {
    Serial.println("[MPU] MPU6050 introuvable.");
  }
}

void loop() {
  static unsigned long dernier = 0;
  static bool ledAllumee = false;

  if (millis() - dernier >= 500) {
    dernier = millis();

    ledAllumee = !ledAllumee;
    digitalWrite(PIN_LED, ledAllumee ? HIGH : LOW);

    if (mpuOk) {
      sensors_event_t a, g, t;
      mpu.getEvent(&a, &g, &t);
      float roulis  = atan2(a.acceleration.y, a.acceleration.z) * 180 / PI;
      float tangage = atan2(-a.acceleration.x,
                            sqrt(a.acceleration.y * a.acceleration.y +
                                 a.acceleration.z * a.acceleration.z)) * 180 / PI;
      Serial.print("LED ");
      Serial.print(ledAllumee ? "ON " : "OFF");
      Serial.print(" | roulis ");
      Serial.print(roulis, 1);
      Serial.print(" deg | tangage ");
      Serial.print(tangage, 1);
      Serial.println(" deg");
    }
  }
}
