# Comparatif des modes de commande du Sabertooth 2x5

> **Projet** : 243-6J7-LI. **Auteur** : Guillaume Retier. **Créé** : 2026-11-07.
> **But** : choisir le mode de commande du Sabertooth selon des critères mesurés, identiques pour chaque mode.

**Contexte de la liaison** : l'ESP32 est commandé par Wi-Fi (V1), puis éventuellement par antenne radio et 4G (phase 2). L'arrêt à la perte de liaison est assuré par le firmware (signes de vie `cmd/hb` toutes les 0,3 s, expiration à 1 s, arrêt de sécurité au plus 1 s d'après le cahier des charges). Le mode du Sabertooth doit couvrir les pannes **en aval** de l'ESP32 (ESP32 éteint ou figé, fil détaché, redémarrage du Sabertooth) et supporter une arrivée irrégulière des commandes.

**Modes à comparer** (les cinq de la fiche Sabertooth 2X5 Quick Start Guide, décembre 2006) : (1) analogique indépendant linéaire (0 à 5 V, 2,5 V = arrêt), (2) radiocommande différentielle exponentielle avec autocalibration (S1 avance et recule, S2 rotation ; interrupteurs de la fiche B H H H B H, avec 3 en bas pour la pile lithium : B H B H B H), (3) impulsions microcontrôleur, indépendant linéaire (1000 à 2000 µs, 1500 µs = arrêt ; fiche B H H B H B, avec 3 en bas : B H B B H B), (4) série simplifiée 38400 bauds, (5) série à paquets, adresse 128. La combinaison « RC standard indépendante » (B H B B H H) n'est pas une ligne de la fiche : elle a été essayée le 7 novembre mais n'est plus comparée (résultats conservés plus bas).

**Règles d'essai** : S1 et S2 toujours reliées à un signal défini (aucune entrée libre) ; roues en l'air d'abord, puis au sol avec charge ; même pile, mêmes moteurs, même ESP32 ; couper le Sabertooth 5 s entre deux modes ; noter la version d'Arduino-ESP32 (3.3.12).

## Critères éliminatoires (un échec écarte le mode)

| # | Critère | Pourquoi | Méthode | Seuil proposé |
| --- | --- | --- | --- | --- |
| E1 | Arrêt quand l'ESP32 est éteint, débranché ou en réinitialisation | Panne d'alimentation, creux de tension pendant l'émission Wi-Fi / 4G | Retirer l'alimentation de l'ESP32 en marche ; chronométrer (vidéo avec moniteur série) | Roues arrêtées en 1 s au plus (à valider avec le cahier des charges) |
| E2 | Arrêt quand le programme de l'ESP32 est figé | Blocage de la pile réseau ou d'une bibliothèque | Programme volontairement bloqué avec minuterie de surveillance de 2 s ; chronométrer | Arrêt au plus 3 s après le blocage ; roues immobiles pendant le redémarrage |
| E3 | Aucun mouvement non commandé à la mise sous tension du Sabertooth, au redémarrage de l'ESP32 et à l'armement | Sécurité des personnes | Allumer le Sabertooth pendant que l'ESP32 envoie : arrêt, puis avance, puis recule ; 5 essais | 0 mouvement parasite |
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
| I3 | Charge de l'ESP32 et risque de gêne pour Wi-Fi / MQTT | Une tâche d'envoi périodique ralentit le reste ; l'ESP32 traitera aussi MQTT, l'inclinaison, la batterie, et plus tard le modem 4G | Mesurer la durée de la boucle et le jitter des signes de vie, moteurs en marche | Pas de signe de vie en retard |
| I4 | Complexité du code moteur | Plus il y a d'états (armement, 170, ordres d'arrêt, rampes, reprise), plus il y a de risques d'erreur et d'appels bloquants (`delay`) | Compter les états et les lignes ; relever la plus longue attente bloquante ; vérifier que la commande moteur tient dans une tâche à part avec sa minuterie de surveillance | Aucune attente bloquante de plus de 100 ms |

## Méthode de décision

1. Un mode qui échoue à un critère éliminatoire (E1 à E4) est écarté.
2. Les modes restants sont notés de 0 à 3 sur P1 à P5, R1 à R4 et I1 à I4.
3. Pondération proposée : sécurité et robustesse (R) × 3, performance (P) × 2, intégration (I) × 1. À ajuster avec le cahier des charges.
4. Le mode avec le meilleur total est retenu ; les écarts faibles sont tranchés par la fiabilité mécanique et la simplicité du montage.

## Tableau des résultats

Les critères éliminatoires sont les quatre premières lignes. Un échec à l'un d'eux écarte le mode, quelles que soient ses performances.

- **E1** : arrêt des roues quand l'ESP32 est éteinte, débranchée ou en réinitialisation (panne d'alimentation, câble débranché). Seuil : arrêt en 1 s au plus.
- **E2** : arrêt des roues quand le programme de l'ESP32 est figé alors qu'elle reste alimentée (blocage du réseau ou d'une bibliothèque). Seuil : arrêt au plus 3 s après le blocage.
- **E3** : aucun mouvement non commandé quand la Sabertooth est allumée ou redémarrée, quand l'ESP32 redémarre ou quand on arme le robot. Seuil : zéro mouvement parasite.
- **E4** : reprise des commandes sans intervention manuelle après un état anormal (redémarrage de l'ESP32, coupure de courte durée). Seuil : reprise en 10 s au plus.
- **P1 / P2** : P1 = commande séparée des côtés gauche et droit ; P2 = résolution de vitesse (au moins 10 vitesses distinctes perceptibles) et zone morte.


| Critère | Analogique | RC différentiel exponentiel | RC microcontrôleur | Série simplifiée | Série à paquets |
| --- | --- | --- | --- | --- | --- |
| E1 : arrêt si ESP32 coupée |  | réussi (immédiat) | **échec** (dernière commande gardée) |  | réussi |
| E2 : arrêt si programme figé |  | réussi (≈ 3 s) | non testé (éliminé) |  | réussi |
| E3 : pas de mouvement au démarrage |  | **échec** (centre décalé si la Sabertooth redémarre en marche) | non testé (éliminé) |  | réussi |
| E4 : reprise sans intervention |  | réussi pour l'ESP32, échec si la Sabertooth redémarre | réussi pour l'ESP32 |  | réussi |
| P1 : côtés indépendants |  | réussi | réussi |  |  |
| P2 : résolution et zone morte |  | réussi (15 paliers ; zone morte ±55 µs, se décale) | réussi (≈ 18 paliers ; zone morte ≈ 1440 à 1488 µs) |  |  |
| P3 | | | | | |
| P4 | | | | | |
| P5 | | | | | |
| R1 | | | | | |
| R2 | | | | | |
| R3 | | | | | |
| R4 | | | | | |
| I1 | | | | | |
| I2 | | | | | |
| I3 | | | | | |
| I4 | | | | | |

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
- **Conséquence** : une panne de l'ESP32 en marche laisse le robot rouler indéfiniment. Mode éliminé ; E2 (programme figé) donnerait le même résultat d'après la déduction (la réinitialisation par le chien de garde libère les broches comme le débranchement), mais n'a pas été testé car le mode est éliminé.

### Points faibles sans échec

- **RC différentiel exponentiel, P2** : la zone morte se décale d'une itération à l'autre (démarrage vers 1555 µs puis 1600 µs), probablement parce que la Sabertooth apprend les extrêmes d'impulsion (hypothèse).
- **RC microcontrôleur, centre** : les roues sont immobiles de 1485 à 1490 µs environ au lieu de 1500 µs (impulsions vues plus longues d'environ 30 à 35 µs, estimation) ; une valeur d'arrêt calibrée serait nécessaire dans le firmware.
- **Fils détachés** : voir la partie « R1 » de chaque mode ci-dessous ; la décision est de tout passer par le circuit imprimé avec des cavaliers fixés.

## Ce qui est déjà connu (7 novembre, S2 sans branchement dans les modes RC)

- Série à paquets : E1 oui (S2 par transistor), E2 oui (minuterie), R3 oui, R4 oui avec le 170 renvoyé, R1 : S1 et masse non protégés, S2 et 5V protégés.
- RC différentiel exponentiel (7 novembre, B H B H B H) : S1 avant / arrière : les quatre roues avancent / reculent ; S2 : rotation ; câblage moteur correct ; roues tout à fait immobiles à 1500 µs, aucun micro-mouvement ; centre pris à l'allumage (1500 µs doit être la première commande). E1 : arrêt immédiat à la coupure de l'ESP32 ; E2 : arrêt environ 3 s après le blocage (minuterie de 2 s). E3 : échec : Sabertooth rallumée pendant « S1 avant », le centre se décale à 1600 µs ; la commande d'arrêt (1500 µs) n'arrête plus les roues, qui bougent. E4 : réussi après panne de l'ESP32, échec si le Sabertooth redémarre. R1 (S1 à 1600 µs, 14:44) : fil de S1 retiré : M1 s'arrête, M2 continue ; fil de S2 retiré : M2 s'arrête, M1 erratique ; masse retirée : tout s'arrête et repart à la remise (correction 14:46) ; 5V retiré : tout s'arrête et repart à la remise ; R2 : fil de S1 débranché approché de la borne de S2 (et inversement) : les roues bougent de façon erratique ; doigt posé sur les fils de S1 et de S2 branchés, programme en marche (15:20) : aucune réaction des roues (détail des touchers à préciser : S1, S2, les deux, main à proximité). P2 : zone morte d'environ ±55 µs (roues à 1555 µs vers l'avant, 1445 µs vers l'arrière), vitesse maximale : 1re itération 1800 µs (avant) et 1250 µs (arrière), 2e et 3e itérations 1950 et 1000 µs ; début de rotation 1555 µs (1re itération) puis 1600 µs (2e) : la réponse se décale d'une itération à l'autre puis se stabilise aux extrémités (autocalibration probable, non vérifiée) ; avant : 15 paliers de 25 µs perceptibles entre 1600 et 1950 µs (chaque palier accélère un peu, perçu au moteur) ; arrière : à relever. R3 (mise à jour à distance) : fonctionne tant qu'on ne touche pas à l'alimentation du Sabertooth (conclusion de Guillaume, 14:59) ; un essai de 14:54 fait peu après les retraits de masse et de 5V n'avait pas été suivi (lien non établi) : programme de balayage téléversé sans couper le Sabertooth : réponse à 1600 µs seulement, rien à 1400 µs ; après coupure du Sabertooth et téléversement, les roues réagissent comme prévu. À 14:58, la séquence précédente a été suivie sans coupure. Cause non établie. Autres critères : à faire.
- RC indépendant, combinaison hors fiche (essai du 7 novembre, S1 et S2 pilotées, interrupteurs B H B B H H) : fonctionne à condition que 1500 µs soit la première commande envoyée à l'allumage du Sabertooth ; la paire de S2 avance légèrement vers l'avant à 1500 µs (une roue retenue puis relâchée repart comme à environ 5 % de puissance) ; la paire de S1 bouge un peu de façon irrégulière. Décalage de centre à mesurer. E4 : réussi après panne de l'ESP32, échec si le Sabertooth redémarre. R1 (S1 à 1600 µs, 14:44) : fil de S1 retiré : M1 s'arrête, M2 continue ; fil de S2 retiré : M2 s'arrête, M1 erratique ; masse retirée : tout s'arrête et repart à la remise (correction 14:46) ; 5V retiré : tout s'arrête et repart à la remise ; R2 : fil de S1 débranché approché de la borne de S2 (et inversement) : les roues bougent de façon erratique ; bruit au doigt sur des lignes branchées : non testé. R3 (mise à jour à distance) : fonctionne tant qu'on ne touche pas à l'alimentation du Sabertooth (conclusion de Guillaume, 14:59) ; un essai de 14:54 fait peu après les retraits de masse et de 5V n'avait pas été suivi (lien non établi) : programme de balayage téléversé sans couper le Sabertooth : réponse à 1600 µs seulement, rien à 1400 µs ; après coupure du Sabertooth et téléversement, les roues réagissent comme prévu. À 14:58, la séquence précédente a été suivie sans coupure. Cause non établie. Autres critères : à faire.
- RC indépendant : arrêt en moins d'une seconde quand l'ESP32 est coupée (5 novembre). Le reste est à refaire avec S2 reliée.
- RC microcontrôleur : pas de délai d'attente (les roues continuent).
- Série simplifiée : pas de délai d'attente, mémoire par moteur.
- Analogique : non essayé.
- RC microcontrôleur (7 novembre, B H B B H B : fiche B H H B H B avec 3 en bas pour le lithium, mode indépendant linéaire ; S1 = M1, S2 = M2) : à l'allumage de la Sabertooth pendant l'envoi de 1500 µs, les roues tournent un peu (15:28). Recherche du point d'arrêt (S1 et S2 ensemble, de 1450 à 1550 µs par pas de 10 µs) : roues immobiles de 1470 à 1490 µs (15:32), soit un centre réel d'environ 1480 µs et non 1500 µs. Zone d'arrêt d'environ 20 µs, contre ±55 µs en mode à autocalibration. Les côtés M1 et M2 n'ont pas été distingués. Hypothèse non vérifiée : les impulsions vues par la Sabertooth sont plus longues d'environ 20 µs que celles commandées (montage à transistor et résistance de 10 kΩ), ce que l'autocalibration masque ; cela expliquerait l'avancement léger des roues de S2 à 1500 µs relevé en mode indépendant. À mesurer à l'oscilloscope. Autres critères : à faire.
- RC microcontrôleur (suite, 15:38) : seuil d'avance S1 1490 µs, S2 1495 µs (recherche fine, 15:41 ; arrêt jusqu'à 1485 et 1490 µs). Avant : démarrage à 1505 µs, vitesse maximale à 1930 µs (à l'oreille), soit environ 18 paliers de 25 µs. Arrière : démarrage à 1430 µs, vitesse maximale à 1005 µs, environ 18 paliers. Même comportement pour S1 et S2. P2 : réussi (seuil de 10 paliers dépassé). Zone morte estimée d'environ 1440 à 1488 µs (seuil d'arrière non resserré, entre 1430 et 1455 µs), centre d'environ 1465 µs ; 1480 µs est à environ 10 µs du seuil d'avance. Dérive entre deux cycles : non rapportée. Autres critères : à faire (E1 critique : ce mode n'a pas de minuterie de sécurité d'après la documentation).
- RC microcontrôleur (suite, 15:42) : E1 : échec. ESP32 hors tension (USB retiré) : la dernière commande est gardée, les roues continuent. Dans les modes RC avec l'interrupteur 6 en haut, l'arrêt était immédiat (minuterie de sécurité) ; avec l'interrupteur 6 en bas, cette minuterie n'existe plus. E2 : non testé, même résultat attendu (déduction : la réinitialisation par le chien de garde libère les broches comme le débranchement). Bilan P2 : bon, E1 : éliminatoire.
- RC microcontrôleur (suite, 15:52) : E1 refait avec une commande constante (S1 à 1600 µs) : USB débranché, les roues continuent ; échec confirmé. Au rebranchement, arrêt d'environ 1 s puis reprise de la commande sans intervention (E4 pour l'ESP32 : réussi). Mode éliminé par E1 ; E2, E3, R1 à R4, P3 à P5 : non testés (mode éliminé).
