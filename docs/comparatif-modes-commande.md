# Comparatif des modes de commande du Sabertooth 2x5

> **Projet** : 243-6J7-LI. **Auteur** : Guillaume Retier. **Créé** : 2026-11-07.
> **But** : choisir le mode de commande du Sabertooth selon des critères mesurés, identiques pour chaque mode.

**Contexte de la liaison** : l'ESP32 reçoit les ordres par Wi-Fi (version 1), puis plus tard par antenne et réseau 4G (phase 2). Quand la liaison est perdue, c'est le programme de l'ESP32 qui arrête les roues : l'opérateur envoie un « signe de vie » toutes les 0,3 s et, sans signe pendant 1 s (durée indicative du cahier des charges), les roues s'arrêtent. Le mode de commande de la Sabertooth doit donc couvrir les pannes que ce programme ne peut pas corriger, celles qui surviennent après l'ESP32 dans la chaîne : ESP32 éteinte ou figée, fil détaché, Sabertooth qui redémarre. Il doit aussi bien supporter des ordres qui arrivent de façon irrégulière.

**Modes à comparer** : les cinq modes de la fiche de la Sabertooth 2x5 (Quick Start Guide, décembre 2006). Les positions des interrupteurs sont données avec l'interrupteur 3 en bas, qui règle la coupure pour la pile au lithium.

| Mode | Comment l'ESP32 donne ses ordres | Interrupteurs (H = haut, B = bas) |
| --- | --- | --- |
| 1. Analogique indépendant linéaire | Une tension de 0 à 5 V sur chaque entrée ; 2,5 V = arrêt | H H H B H H |
| 2. Radiocommande différentielle exponentielle | Des impulsions de 1000 à 2000 µs ; S1 avance et recule, S2 fait tourner le robot ; la Sabertooth apprend elle-même le centre et les extrémités (autocalibration) | B H B H B H (fiche : B H H H B H) |
| 3. Impulsions de microcontrôleur, indépendant linéaire | Des impulsions de 1000 à 2000 µs ; 1500 µs = arrêt ; chaque entrée commande un côté | B H B B H B (fiche : B H H B H B) |
| 4. Série simplifiée | Des octets envoyés à 38400 bauds | H B H B B H |
| 5. Série à paquets | Des messages complets envoyés à l'adresse 128 | B B H H H H |

Une sixième combinaison, « radiocommande indépendante » (B H B B H H), n'est pas une ligne de la fiche. Elle a été essayée le 7 novembre mais n'est plus comparée (résultats conservés plus bas).

**Règles d'essai** : S1 et S2 toujours reliées à un signal défini (aucune entrée libre) ; roues en l'air d'abord, puis au sol avec charge ; même pile, mêmes moteurs, même ESP32 ; couper la Sabertooth 5 s entre deux modes (ses interrupteurs ne sont lus qu'à l'allumage, et il faut 5 s hors tension pour qu'elle reparte de zéro) ; noter la version d'Arduino-ESP32 (3.3.12).

## Critères éliminatoires (un échec écarte le mode)

| # | Critère | Pourquoi | Méthode | Seuil proposé |
| --- | --- | --- | --- | --- |
| E1 | Arrêt quand l'ESP32 est éteint, débranché ou en réinitialisation | Panne d'alimentation, creux de tension pendant l'émission Wi-Fi / 4G | Retirer l'alimentation de l'ESP32 en marche ; chronométrer (vidéo avec moniteur série) | Roues arrêtées en 1 s au plus (à valider avec le cahier des charges) |
| E2 | Arrêt quand le programme de l'ESP32 est figé | Blocage de la pile réseau ou d'une bibliothèque | Programme volontairement bloqué, avec une minuterie de surveillance de 2 s (elle redémarre l'ESP32 quand son programme ne répond plus) ; chronométrer | Arrêt au plus 3 s après le blocage ; roues immobiles pendant le redémarrage |
| E3 | Aucun mouvement non commandé à la mise sous tension du Sabertooth, au redémarrage de l'ESP32 et au moment de débloquer les roues | Sécurité des personnes | Allumer le Sabertooth pendant que l'ESP32 envoie : arrêt, puis avance, puis recule ; 5 essais | 0 mouvement parasite |
| E4 | Récupération sans intervention manuelle après un état anormal (bruit, redémarrage de l'ESP32, coupure de courte durée) | Le robot ne sera pas toujours accessible | Provoquer le défaut, puis vérifier que les commandes reprennent sans toucher à l'alimentation | Reprise dans les 10 s, 5 essais sur 5 |

## Critères de performance

| # | Critère | Pourquoi | Méthode | Seuil proposé |
| --- | --- | --- | --- | --- |
| P1 | Contrôle indépendant des deux côtés | Virage sur place, trajectoire | Moteurs 1 et 2 commandés séparément, deux sens | Les 4 combinaisons obtenues |
| P2 | Résolution et zone morte de la vitesse | Pilotage précis dans un lieu étroit | Balayer la commande ; compter les paliers de vitesse visibles et la zone sans mouvement | Au moins 10 paliers utiles |
| P3 | Délai entre l'ordre et le mouvement, ajouté par le mode | Retour vidéo avec latence au plus 1 s | Marqueur sur le moniteur série, vidéo image par image | Moins de 100 ms |
| P4 | Tolérance à l'arrivée irrégulière des commandes (Wi-Fi, 4G) | Pertes et retards de paquets | Envoyer les commandes avec écarts de 50 ms à 1 s ; observer les à-coups | Pas d'à-coup tant que les signes de vie respectent l'expiration |
| P5 | Rampe d'accélération et courant | Fusible de 3 A | Mesurer le courant de pointe au démarrage avec la charge | Sous 3 A |

## Critères de robustesse

| # | Critère | Pourquoi | Méthode | Seuil proposé |
| --- | --- | --- | --- | --- |
| R1 | Fil détaché (S1, S2, masse, 5V) | Vibrations, borne qui lâche | Retirer chaque fil en marche | Noter : arrêt ou maintien (non éliminatoire : voir décision sur le câblage) |
| R2 | Sensibilité au bruit et aux parasites | Moteurs, et plus tard antenne et 4G | Entrée touchée au doigt ; essais moteurs chargés ; plus tard, modem en émission à proximité | Aucun mouvement non commandé |
| R3 | Mise à jour du firmware à distance | L'ESP32 redémarre, le Sabertooth reste alimenté | Téléverser un programme modifié sans couper le Sabertooth | Nouvelle commande prise en compte |
| R4 | Redémarrage du Sabertooth seul (changement de pile) | Intervention d'un technicien | Couper 5 s puis rallumer, ESP32 en marche | Reprise sans redémarrer l'ESP32 |

## Critères d'intégration

| # | Critère | Pourquoi | Méthode | Seuil proposé |
| --- | --- | --- | --- | --- |
| I1 | Ressources de l'ESP32 (broches, périphériques) | Les broches du modem 4G (25, 26, 27, 12, 4, 5, 33) et la batterie (35) sont déjà prises | Compter broches et périphériques par mode | Aucune broche du modem utilisée |
| I2 | Nombre de composants et complexité du PCB | Fiabilité, coût, montage | Compter les pièces par mode | Le moins possible |
| I3 | Charge de l'ESP32 et risque de gêne pour Wi-Fi / MQTT | Une tâche d'envoi périodique ralentit le reste ; l'ESP32 traitera aussi MQTT, l'inclinaison, la batterie, et plus tard le modem 4G | Mesurer la durée de la boucle et le retard des signes de vie, moteurs en marche | Pas de signe de vie en retard |
| I4 | Complexité du code moteur | Plus il y a d'états (déblocage des roues, 170, ordres d'arrêt, rampes, reprise), plus il y a de risques d'erreur et d'attentes bloquantes (l'instruction `delay` fige le programme) | Compter les états et les lignes ; relever la plus longue attente bloquante ; vérifier que la commande des moteurs tient dans une partie du programme séparée, surveillée par sa propre minuterie | Aucune attente bloquante de plus de 100 ms |

## Méthode de décision

1. Un mode qui échoue à un critère éliminatoire (E1 à E4) est écarté.
2. Les modes restants sont notés de 0 à 3 sur P1 à P5, R1 à R4 et I1 à I4.
3. Pondération proposée : sécurité et robustesse (R) × 3, performance (P) × 2, intégration (I) × 1. À ajuster avec le cahier des charges.
4. Le mode avec le meilleur total est retenu ; les écarts faibles sont tranchés par la fiabilité mécanique et la simplicité du montage.

## Tableau des résultats

Les critères éliminatoires sont les quatre premières lignes. Un échec à l'un d'eux écarte le mode, quelles que soient ses performances, et les critères suivants ne sont plus testés pour ce mode (« non testé (éliminé) »). Les cases vides sont les essais encore à faire : le montage analogique reste à construire et la série simplifiée reste à essayer.

- **E1** : arrêt des roues quand l'ESP32 est éteinte, débranchée ou en réinitialisation (panne d'alimentation, câble débranché). Seuil : arrêt en 1 s au plus.
- **E2** : arrêt des roues quand le programme de l'ESP32 est figé alors qu'elle reste alimentée (blocage du réseau ou d'une bibliothèque). Seuil : arrêt au plus 3 s après le blocage.
- **E3** : aucun mouvement non commandé quand la Sabertooth est allumée ou redémarrée, quand l'ESP32 redémarre ou quand on débloque les roues. Seuil : zéro mouvement parasite.
- **E4** : reprise des commandes sans intervention manuelle après un état anormal (redémarrage de l'ESP32, coupure de courte durée). Seuil : reprise en 10 s au plus.
- **P1 / P2** : P1 = commande séparée des côtés gauche et droit ; P2 = résolution de vitesse (au moins 10 vitesses distinctes perceptibles) et zone morte (plage de commandes où les roues ne bougent pas).
- **P3 à I4** : définis dans les tableaux de critères plus haut.

| Critère | Analogique | RC différentiel exponentiel | RC microcontrôleur | Série simplifiée | Série à paquets |
| --- | --- | --- | --- | --- | --- |
| E1 : arrêt si ESP32 coupée |  | réussi (immédiat) | **échec** (dernière commande gardée) |  | réussi |
| E2 : arrêt si programme figé |  | réussi (≈ 3 s) | non testé (éliminé) |  | réussi |
| E3 : pas de mouvement au démarrage |  | **échec** (centre décalé si la Sabertooth redémarre) | non testé (éliminé) |  | réussi |
| E4 : reprise sans intervention |  | réussi pour l'ESP32, échec si la Sabertooth redémarre | réussi pour l'ESP32 |  | réussi |
| P1 : côtés indépendants |  | réussi | réussi |  |  |
| P2 : résolution et zone morte |  | réussi (15 paliers ; zone morte ±55 µs, se décale) | réussi (≈ 18 paliers ; zone morte ≈ 1440 à 1488 µs) |  |  |
| P3 : délai ordre → mouvement |  | non testé (éliminé) | non testé (éliminé) |  |  |
| P4 : ordres qui arrivent irrégulièrement |  | non testé (éliminé) | non testé (éliminé) |  |  |
| P5 : courant au démarrage |  | non testé (éliminé) | non testé (éliminé) |  |  |
| R1 : fil détaché |  | relevé (voir les observations) | non testé (éliminé) |  | S1 et masse non protégés ; S2 et 5V protégés |
| R2 : bruit, parasites |  | relevé (voir les observations) | non testé (éliminé) |  |  |
| R3 : mise à jour à distance |  | réussi tant qu'on ne touche pas à l'alimentation de la Sabertooth | non testé (éliminé) |  | réussi |
| R4 : redémarrage de la Sabertooth seule |  | échec (voir E3 et E4) | non testé (éliminé) |  | réussi (avec le 170 renvoyé) |
| I1 : broches de l'ESP32 |  | non testé (éliminé) | non testé (éliminé) |  |  |
| I2 : nombre de composants |  | non testé (éliminé) | non testé (éliminé) |  |  |
| I3 : charge de l'ESP32 |  | non testé (éliminé) | non testé (éliminé) |  |  |
| I4 : complexité du code |  | non testé (éliminé) | non testé (éliminé) |  |  |

## Détail des échecs et des points faibles

Chaque échec du tableau est expliqué ici : ce qui a été fait, ce qui a été observé, la cause (établie ou supposée) et la conséquence pour le robot.

### RC différentiel exponentiel (interrupteur 6 en haut) : E3 échec

- **Essai** : l'ESP32 envoie « S1 avant » (1600 µs) ; la Sabertooth est éteinte puis rallumée pendant l'envoi.
- **Observation (constat de Guillaume, fait établi)** : les ordres sont inversés. À la commande d'arrêt (1500 µs), les roues avancent ; à la commande d'avance (1600 µs), elles s'arrêtent.
- **Explication du mécanisme (hypothèse ; seul le mécanisme est supposé, pas le constat)** : dans ce mode, la Sabertooth apprend le point neutre à partir des premières impulsions reçues après l'allumage. Elle a démarré pendant que l'ESP32 envoyait 1600 µs et a pris 1600 µs pour le neutre ; 1500 µs devient alors une commande « en arrière » de ce neutre.
- **Cas qui fonctionne** : si la Sabertooth est allumée quand l'ESP32 envoie déjà 1500 µs, le comportement est normal. Une remise sous tension avec 1500 µs envoyé rétablit le comportement.
- **Conséquence** : une micro-coupure ou un creux de tension de la Sabertooth pendant que le robot roule le fait repartir avec un mauvais neutre : l'arrêt n'arrête plus et le robot réagit à l'inverse des ordres. C'est le risque pour les personnes que le critère E3 doit exclure.

### RC différentiel exponentiel : E4 échec si la Sabertooth redémarre

- **Observation** : après une panne de l'ESP32, les commandes reprennent seules (réussi). Après un redémarrage de la Sabertooth pendant un mouvement, elles ne reprennent pas correctement.
- **Cause** : la même que pour E3 (neutre mal appris).
- **Conséquence** : sans intervention (arrêt complet de l'alimentation de la Sabertooth), le robot reste inutilisable après une micro-coupure de sa batterie.

### RC microcontrôleur (interrupteur 6 en bas) : E1 échec

- **Essai** : programme du test 14 (S1 tenu à 1600 µs, S2 à 1465 µs) ; le câble USB de l'ESP32 est débranché pendant la marche.
- **Observation** : les roues continuent d'avancer ; aucun arrêt observé. Au rebranchement, les roues s'arrêtent environ 1 s puis reprennent la commande d'elles-mêmes.
- **Cause supposée** : sans impulsions, la Sabertooth garde la dernière commande ; la minuterie de sécurité du mode RC avec interrupteur 6 en haut n'existe pas avec l'interrupteur 6 en bas (documentation).
- **Conséquence** : une panne de l'ESP32 en marche laisse le robot rouler indéfiniment. Mode éliminé ; E2 (programme figé) donnerait le même résultat d'après la déduction (le redémarrage automatique de l'ESP32 par la minuterie de surveillance libère les broches comme le débranchement), mais n'a pas été testé car le mode est éliminé.

### Points faibles sans échec

- **RC différentiel exponentiel, P2** : la zone morte se décale d'une itération à l'autre (démarrage vers 1555 µs puis 1600 µs), probablement parce que la Sabertooth apprend les extrêmes d'impulsion (hypothèse).
- **RC microcontrôleur, centre** : les roues sont immobiles de 1485 à 1490 µs environ au lieu de 1500 µs (impulsions vues plus longues d'environ 30 à 35 µs, estimation) ; une valeur d'arrêt calibrée serait nécessaire dans le firmware.
- **Fils détachés** : voir la partie « R1 » de chaque mode ci-dessous ; la décision est de tout passer par le circuit imprimé avec des cavaliers fixés.

## Observations détaillées par mode (5 et 7 novembre)

Ce qui a été observé, mode par mode. Les échecs sont expliqués plus haut dans « Détail des échecs et des points faibles ». « Côté S1 » et « côté S2 » désignent les roues commandées par chacune des entrées (moteurs M1 et M2).

### Série à paquets (B B H H H H)

- **E1, réussi** : l'arrêt de sécurité sur S2 (transistor) arrête les roues quand l'ESP32 est éteinte, débranchée ou en redémarrage. Sans lui, la Sabertooth garderait la dernière commande reçue.
- **E2, réussi** : la minuterie de surveillance redémarre l'ESP32 quand son programme se fige, et l'arrêt de sécurité garde les roues immobiles pendant ce temps.
- **R3, réussi** : la mise à jour du programme de l'ESP32 se fait sans couper l'alimentation de la Sabertooth.
- **R4, réussi** à condition de renvoyer le nombre 170 (message d'initialisation) avec les ordres.
- **R1, fil détaché** : si le fil de S1 ou de masse se détache, la Sabertooth garde la dernière commande (non protégé) ; si celui de S2 ou du 5V se détache, les roues s'arrêtent (protégé). Décision : tout passer par le circuit imprimé et fixer mécaniquement les fils vers la Sabertooth.

### Radiocommande différentielle exponentielle (B H B H B H)

- **Fonctionnement** : S1 vers l'avant ou l'arrière fait avancer ou reculer les quatre roues ; S2 les fait tourner sur place. Le câblage des moteurs est correct. À 1500 µs, les roues sont tout à fait immobiles, sans aucun micro-mouvement.
- **Centre pris à l'allumage** : la Sabertooth prend pour centre la largeur d'impulsion qu'elle reçoit juste après son allumage. Il faut donc que 1500 µs soit la première commande envoyée. C'est la cause de l'échec E3 (voir plus haut).
- **E1** : arrêt immédiat quand l'ESP32 est coupée. **E2** : arrêt environ 3 s après le blocage du programme (minuterie de surveillance de 2 s). **E3** et **E4** : voir « Détail des échecs ».
- **R1, fils retirés un par un pendant la marche avant** (S1 tenue à 1600 µs) :
  - fil de S1 retiré : les roues du côté S1 s'arrêtent, celles du côté S2 continuent ;
  - fil de S2 retiré : les roues du côté S2 s'arrêtent, celles du côté S1 accélèrent et ralentissent de façon irrégulière ;
  - fil de masse retiré : tout s'arrête, et tout repart à la remise du fil ;
  - fil du 5V retiré : tout s'arrête, et tout repart à la remise du fil.
- **R2, bruit** : un fil de S1 débranché, approché de la borne de S2 (et inversement), fait bouger les roues de façon irrégulière. Un doigt posé sur les fils de S1 et de S2 branchés, programme en marche, ne provoque aucune réaction (quels fils ont été touchés exactement : à préciser).
- **P2, résolution** : zone morte d'environ ±55 µs (les roues avancent dès 1555 µs, reculent dès 1445 µs). Vitesse maximale à 1800 µs (avant) et 1250 µs (arrière) lors du premier passage, puis à 1950 et 1000 µs aux deuxième et troisième. Le début de rotation passe de 1555 à 1600 µs entre les deux premiers passages, puis la réponse se stabilise aux extrémités : probablement l'autocalibration (non vérifiée). Vers l'avant, 15 paliers de 25 µs sont perceptibles entre 1600 et 1950 µs (chaque palier accélère un peu, perçu au moteur). Vers l'arrière : à relever.
- **R3, mise à jour à distance** : un programme de balayage téléversé sans couper la Sabertooth n'a pas été suivi (réponse à 1600 µs seulement, rien à 1400 µs). Après coupure de la Sabertooth et nouveau téléversement, tout fonctionnait. Un autre programme téléversé ensuite sans couper la Sabertooth a été pris en compte. Conclusion de Guillaume : la mise à jour fonctionne tant qu'on ne touche pas à l'alimentation de la Sabertooth. L'essai raté a eu lieu peu après les retraits des fils de masse et de 5V ; le lien entre les deux n'est pas établi.
- Les autres critères ne sont pas testés : le mode est éliminé.

### Radiocommande microcontrôleur (B H B B H B)

- **À l'allumage** de la Sabertooth pendant l'envoi de 1500 µs, les roues tournent un peu.
- **Point d'arrêt** : en envoyant la même largeur aux deux entrées, de 1450 à 1550 µs par pas de 10 µs, les roues sont immobiles de 1470 à 1490 µs. Une recherche plus fine donne : S1 avance à partir de 1490 µs, S2 à partir de 1495 µs. Le véritable point d'arrêt est donc environ 1480 µs et non 1500 µs.
- **P2, résolution** : vers l'avant, la vitesse maximale est atteinte à 1930 µs (à l'oreille), soit environ 18 paliers de 25 µs. Vers l'arrière, le démarrage est à 1430 µs et la vitesse maximale à 1005 µs, environ 18 paliers aussi. S1 et S2 se comportent de la même façon. Le seuil de 10 paliers est dépassé : P2 réussi.
- **Zone morte estimée** : d'environ 1440 à 1488 µs, centre vers 1465 µs (le seuil de recul n'a pas été resserré, entre 1430 et 1455 µs). À 1480 µs, on est à environ 10 µs du seuil d'avance.
- **Hypothèse (non vérifiée)** : les impulsions vues par la Sabertooth sont plus longues de 30 à 35 µs que celles commandées, à cause du montage à transistor et de la résistance de 10 kΩ. Cela expliquerait le décalage du centre et l'avance légère des roues à 1500 µs. À mesurer à l'oscilloscope. L'autocalibration des autres modes masquait ce décalage.
- **E1, échec** (confirmé par un second essai avec une commande constante à 1600 µs) : voir « Détail des échecs ». Au rebranchement de l'ESP32, les roues s'arrêtent environ 1 s puis reprennent seules (E4 réussi pour l'ESP32).
- **Dérive entre deux cycles** : non rapportée. Les autres critères ne sont pas testés : le mode est éliminé.

### Radiocommande indépendante (B H B B H H) : combinaison hors fiche

- Essayée le 7 novembre, avec S1 et S2 pilotées. Elle fonctionne à condition que 1500 µs soit la première commande envoyée à l'allumage de la Sabertooth.
- Les roues du côté S2 avancent légèrement vers l'avant à 1500 µs (une roue retenue puis relâchée repart comme à environ 5 % de puissance). Celles du côté S1 bougent un peu, de façon irrégulière. Le décalage du centre reste à mesurer.
- **E1** : arrêt en moins d'une seconde quand l'ESP32 est coupée (essai du 5 novembre, S2 sans branchement). **E4** : réussi après une panne de l'ESP32, échec si la Sabertooth redémarre.
- Le reste n'a pas été refait avec S2 reliée : la combinaison n'est pas sur la fiche et n'est plus comparée.

### Série simplifiée

- Pas encore essayée. D'après sa description, la Sabertooth n'a pas de délai d'attente (elle ne s'arrête pas d'elle-même quand les ordres cessent) et chaque moteur garde sa dernière commande : à confirmer lors de l'essai.

### Analogique

- Non essayé. Le montage à construire est présenté dans le journal du 7 novembre (essais et schéma de l'interface analogique).
