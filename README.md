# Infrastructure du rover

Broker MQTT (Mosquitto) et tunnel Cloudflare, lancés avec Docker Compose dans une machine virtuelle Linux. Ce dossier permet de refaire le serveur sur une autre machine (VM, Raspberry Pi, serveur) sans rien perdre.

## Ce que ça fait

Le robot, les navigateurs et les outils parlent tous au même broker Mosquitto, par trois portes :

| Port | Où | Protocole | Chiffré | Usage |
| --- | --- | --- | --- | --- |
| 1883 | La machine | MQTT | Non | ESP32 et outils (`mosquitto_sub`), en réseau local |
| 9001 | La machine | MQTT sur WebSocket | Non | Navigateur, en réseau local |
| 443 | Cloudflare | MQTT sur WebSocket (`wss`) | Oui | Depuis n'importe où, via le tunnel, qui redirige vers le port 9001 |

Aucun port n'est ouvert sur Internet : le service `cloudflared` établit lui-même la connexion sortante vers Cloudflare.

## Prérequis

- Une machine Linux (testé : Ubuntu Server 26.04 dans VirtualBox, réseau en accès par pont) avec accès Internet.
- Docker, Docker Compose et Git.
- Un compte Cloudflare avec un nom de domaine (exemple ici : `gretier.xyz`).

Installation de Docker et Git :

```
sudo apt update && sudo apt upgrade -y
sudo apt install -y docker.io docker-compose-v2 git
sudo usermod -aG docker $USER
```

Se déconnecter puis se reconnecter (le groupe `docker` n'est pris en compte qu'à la nouvelle session), puis vérifier avec `docker run hello-world`.

## Installation

1. Récupérer le dépôt :

```
git clone https://github.com/guilatriche/rover.git
cd rover/infra
```

2. Créer le tunnel dans Cloudflare (tableau de bord, Networking, Tunnels) :
   - **Create a tunnel**, type **Cloudflared**, nom `rover-vm`.
   - Choisir **Docker** et repérer le jeton : c'est le texte qui suit `--token` dans la commande affichée. Il commence par `eyJ`. Ne pas confondre avec le *Tunnel ID*.
   - Dans **Routes**, **Add route**, **Published application** : sous-domaine `rover`, domaine `gretier.xyz`, URL du service `http://mosquitto:9001` (en `http`, pas `https` : le chiffrement est assuré par Cloudflare).

3. Créer le fichier `.env` avec le jeton :

```
cp .env.example .env
nano .env
chmod 600 .env
```

Le fichier doit contenir une seule ligne, sans espace ni guillemets : `CLOUDFLARE_TUNNEL_TOKEN=eyJ...`

4. Créer le fichier de mots de passe du broker (remplacer `rover` par l'identifiant voulu, un mot de passe long et unique sera demandé) :

```
mkdir -p mosquitto/data mosquitto/log
docker run --rm -it -v $(pwd)/mosquitto/config:/mosquitto/config eclipse-mosquitto:2 mosquitto_passwd -c /mosquitto/config/passwd rover
sudo chown 1883:1883 mosquitto/config/passwd
sudo chmod 600 mosquitto/config/passwd
sudo chown -R 1883:1883 mosquitto/data mosquitto/log
```

5. Lancer :

```
docker compose up -d
```

## Vérifier

```
docker compose ps
docker compose logs --tail 15 mosquitto
docker compose logs cloudflared | grep -i registered
```

- Les journaux de Mosquitto doivent montrer deux lignes « Opening ... listen socket » (ports 1883 et 9001).
- Ceux de `cloudflared` doivent montrer quatre lignes « Registered tunnel connection ». Dans Cloudflare, le tunnel est alors **Healthy**.

## Tester avec un navigateur

1. Ouvrir le client WebSocket de HiveMQ : `https://www.hivemq.com/demos/websocket-client/`.
2. Par le tunnel : hôte `rover.gretier.xyz`, port `443`, **SSL coché**, identifiant et mot de passe du broker, puis **Connect**.
3. En local (sans tunnel) : hôte = adresse de la machine, port `9001`, SSL décoché. Chrome peut exiger d'autoriser le contenu non sécurisé pour le site (icône à gauche de l'adresse, Paramètres du site).
4. **Add New Topic Subscription** : taper le sujet à la main (par exemple `rover/rover-01/#`), puis **Subscribe**.

## Après un redémarrage de la machine

Rien à relancer : les deux services ont `restart: unless-stopped`, Docker les remet en route tout seul. Vérifier avec `docker ps` : `mosquitto` et `cloudflared` doivent être en état Up. Si on les a arrêtés à la main (`docker compose down`), les relancer avec `docker compose up -d`.

## Problèmes fréquents

| Symptôme | Cause probable | Correctif |
| --- | --- | --- |
| `cloudflared` : « Provided Tunnel token is not valid » | Le *Tunnel ID* a été copié à la place du jeton, ou le jeton est coupé | Copier la commande Docker complète, ne garder que le texte après `--token` |
| Mosquitto : « Invalid subscription string » | Sujet saisi avec un espace ou un caractère invisible | Retaper le sujet à la main |
| ESP32 : échec de connexion MQTT, code 5 | Identifiant ou mot de passe MQTT incorrect | Corriger le fichier `secrets.h` du firmware |
| `Permission denied` sur `docker.sock` | Session ouverte avant l'ajout au groupe `docker` | Se reconnecter, ou utiliser `sudo docker` |
| HiveMQ reste rouge en port 9001 | Chrome bloque le `ws://` depuis une page https | Autoriser le contenu non sécurisé, ou passer par le tunnel (port 443) |

## Fichiers jamais publiés

Ils sont dans le `.gitignore` du dépôt :

- `.env` (jeton Cloudflare) : seul `.env.example`, avec une valeur bidon, est publié
- `mosquitto/config/passwd` (mots de passe du broker)
- `mosquitto/data/` et `mosquitto/log/` (données internes du broker)
