# Firmware du rover (ESP32)

Programmes Arduino IDE pour la carte LilyGO T-A7670 (ESP32). Chaque dossier est un programme complet ; le nom du dossier est celui du fichier `.ino`.

## Contenu

| Dossier | Date | Rôle |
| --- | --- | --- |
| `tests/test_01_mqtt_bonjour` | 2026-10-23 | L'ESP32 se connecte au Wi-Fi et au broker, et publie « bonjour N » chaque seconde |
| `tests/test_02_led_mpu` | 2026-10-24 | Clignotement d'une LED (GPIO 14) et lecture de l'inclinaison du MPU6050 (bibliothèque Adafruit) |
| `tests/test_03_mpu_brut` | 2026-10-24 | Diagnostic du MPU6050 : identité, valeurs brutes, température |
| `tests/test_04_mpu_mqtt` | 2026-10-24 | Le MPU6050 publie roulis et tangage bruts sur `rover/rover-01/imu`, une fois par seconde |
| `tests/test_05_mpu_calibration` | 2026-10-24 | Étalonnage du MPU6050 sur ordre MQTT (`cmd/calib`), référence mémorisée en flash, publication de l'inclinaison corrigée |
| `rover_cmd_led` | 2026-10-24, diagnostic ajouté le 2026-10-26 | Commande d'une LED par MQTT avec signal de vie et arrêt de sécurité. Version du 26 octobre : le statut contient aussi la cause du dernier redémarrage (`reset`), le silence mesuré (`silence`), le point d'accès (`bssid`) et le canal ; option `FORCER_POINT_ACCES` (désactivée) pour un essai sur un point d'accès précis ; champ `version` (`2026-10-26-diag`) dans le statut et au démarrage (moniteur série). La page de pilotage ne l'affiche pas |

## Branchements utilisés

| Élément | Broche |
| --- | --- |
| LED (avec résistance de 220 à 330 ohms vers GND) | GPIO 14 |
| MPU6050, SDA | GPIO 21 |
| MPU6050, SCL | GPIO 22 |
| MPU6050, alimentation | 3,3 V et GND (masse commune avec la carte) |

Les GPIO 26, 27, 4 et 12 sont utilisées par le modem : ne pas s'en servir.

## Bibliothèques (Gestionnaire de bibliothèques d'Arduino IDE)

- `PubSubClient` (2.8) : tests 01 et `rover_cmd_led`
- `ArduinoJson` : `rover_cmd_led`
- `Adafruit MPU6050` et `Adafruit Unified Sensor` : tests 02, 04 et 05

Carte : **ESP32 Dev Module**, cœur ESP32 3.x.

## Identifiants (jamais publiés)

Les programmes qui se connectent au réseau lisent `secrets.h`, placé à côté du `.ino`. Ce fichier est dans le `.gitignore`.

1. Copier `secrets.h.example` vers `secrets.h` dans le même dossier.
2. Remplir le Wi-Fi (2,4 GHz uniquement), l'adresse de la VM et les identifiants MQTT.

## Commande de la LED (`rover_cmd_led`)

| Sujet | Contenu |
| --- | --- |
| `rover/rover-01/cmd/drive` | `{"mode":"hold","speed":50}` ou `{"mode":"latch","speed":50}` (vitesse de 0 à 100) |
| `rover/rover-01/cmd/hb` | n'importe quel texte : signal de vie |
| `rover/rover-01/status` | état publié toutes les 2 s |
| `rover/rover-01/online` | `true` ou `false` |

- **Signal de vie** : sans aucun message de commande pendant 1 s, la LED s'éteint.
- **Mode maintenu (`hold`)** : la commande doit être renvoyée toutes les 0,5 s, sinon arrêt.
- **Mode mémorisé (`latch`)** : la commande reste active tant que le signal de vie arrive.
- Après une perte de liaison, la commande est effacée : rien ne redémarre tout seul.
