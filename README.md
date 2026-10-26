# Rover : robot de reconnaissance télépiloté

Projet 243-6J7 (préparation 243-5J4), Cégep Limoilou, réseaux et télécommunications. Guillaume Retier.

Robot d'intérieur piloté à distance (châssis Lynxmotion 4WD1, carte ESP32 LilyGO T-A7670E) pour inspecter une pièce ou une zone inaccessible à partir de l'image transmise. Les commandes et les mesures passent par un broker MQTT hébergé dans une machine virtuelle, joignable de l'extérieur par un tunnel Cloudflare.

## Organisation du dépôt

| Dossier | Contenu |
| --- | --- |
| [`infra/`](infra/README.md) | Serveur : Mosquitto, tunnel Cloudflare et Docker Compose, à refaire sur une autre machine |
| [`firmware/`](firmware/README.md) | Programmes de l'ESP32 (Arduino IDE) et programmes d'essai |
| [`pilotage/`](pilotage/README.md) | Page web de pilotage (une seule page, sans installation) |
| `docs/journal/` | Journal de bord : une entrée datée par séance |

## État d'avancement

- Serveur : broker MQTT avec mot de passe, WebSocket chiffré par le tunnel, terminal SSH dans le navigateur.
- Robot : liaison Wi-Fi vers le broker, commande d'une LED avec signes de vie et arrêt de sécurité, inclinaison mesurée, étalonnée et publiée.
- Pilotage : page web de pilotage (connexion, statut, signes de vie, modes `latch` et `hold`), validée avec la commande d'une LED.
- À venir : moteurs et pont en H, caméra, alarme d'inclinaison, 4G et GPS.

## Sécurité

Aucun mot de passe, jeton ni identifiant n'est publié. Les fichiers `secrets.h`, `.env` et `mosquitto/config/passwd` sont ignorés par Git ; des modèles `*.example` montrent le format attendu.
