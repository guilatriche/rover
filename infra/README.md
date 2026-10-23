# Infrastructure du rover

Broker MQTT (Mosquitto) et tunnel Cloudflare, lancés avec Docker Compose dans la VM.

## Démarrage

1. Copier `.env.example` vers `.env` et y mettre le jeton du tunnel Cloudflare.
2. Créer le fichier de mots de passe du broker (non publié) :
   ```
   mkdir -p mosquitto/data mosquitto/log
   docker run --rm -it -v $(pwd)/mosquitto/config:/mosquitto/config eclipse-mosquitto:2 mosquitto_passwd -c /mosquitto/config/passwd rover
   sudo chown 1883:1883 mosquitto/config/passwd && sudo chmod 600 mosquitto/config/passwd
   sudo chown -R 1883:1883 mosquitto/data mosquitto/log
   ```
3. Lancer : `docker compose up -d`
4. Vérifier : `docker compose logs --tail 15 mosquitto` et `docker compose logs cloudflared | grep -i registered`

## Fichiers jamais publiés

`.env`, `mosquitto/config/passwd`, `mosquitto/data/`, `mosquitto/log/`.
