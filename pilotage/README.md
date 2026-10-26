# Page de pilotage

Une seule page web (`index.html`, sans installation) pour piloter le robot et voir son état. Elle se connecte au broker MQTT par le tunnel Cloudflare et envoie les commandes décrites dans [`firmware/`](../firmware/README.md).

## Utilisation

1. Ouvrir `index.html` dans un navigateur (double-clic sur le fichier).
2. Vérifier l'adresse (par défaut `wss://rover.gretier.xyz/mqtt`) et l'utilisateur, saisir le mot de passe du broker, puis « Se connecter ». **Le mot de passe n'est jamais enregistré** : il faut le saisir à chaque fois.
3. Cliquer sur « Envoyer les signes de vie » : sans ces messages (toutes les 0,3 s), le robot s'arrête au bout de 1 s.
4. Choisir le mode et la vitesse, puis « Avancer » ; « STOP » (ou la barre d'espace) arrête le robot.

En réseau local, l'adresse peut être `ws://adresse-de-la-VM:9001` ; la page doit alors être ouverte depuis un fichier, pas depuis une page `https`.

## Modes de commande

| Mode | Comportement |
| --- | --- |
| Une pression (`latch`) | Un seul ordre suffit ; le robot garde sa vitesse tant que les signes de vie arrivent |
| Rester appuyé (`hold`) | Le robot avance tant que le bouton est tenu ; il s'arrête 0,5 s après la dernière commande |

## Sécurités côté page

- Perte du focus de la fenêtre, changement d'onglet, fermeture ou changement de mode : arrêt envoyé.
- Une reconnexion efface la commande mémorisée : rien ne repart tout seul.
- Quand la page n'est plus connectée au broker, l'état du robot s'affiche « inconnu » (et non plus le dernier état connu).

## Ce qui est validé, et ce qui ne l'est pas

| Fonction | État |
| --- | --- |
| Connexion, statut, signes de vie, mode `latch` | Validés avec le robot (commande d'une LED) |
| Mode `hold` | Tenu plus de 60 s lors d'un seul essai : pas concluant seul |
| Case « Robot » (« inconnu » hors connexion) | Vérifiée avec un faux broker ; à revérifier avec le vrai robot |
| Affichage de l'inclinaison | **Jamais vu avec de vraies données** : le programme actuel du robot ne lit pas le capteur ; à vérifier après l'intégration de l'inclinaison |
| Commande des moteurs | Pas encore faite : la page ne commande aujourd'hui qu'une LED (vitesse de 0 à 100 %) |

## Limites connues

- La liaison vue par le robot passe parfois à « perdue » par moments (perturbations du réseau mesurées le 25 octobre) ; voir le journal de cette date. Le délai d'arrêt de sécurité (1 s) est provisoire.
- La page charge la bibliothèque MQTT depuis un CDN (`mqtt@5.3.4`) : il faut un accès à Internet.
