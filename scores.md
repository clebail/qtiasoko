# Scores

Un tableau par nouvelle progression. Rien d'autre que les chiffres : le
contexte, les verdicts et les discussions restent dans `plan.md`.

Règle (cf. `plan.md` §1, 2026-07-23) : un chiffre sans commit à côté ne se
distingue pas d'un chiffre jamais vérifié. En cas de rebase (hash introuvable),
on ne corrige jamais une ligne à la main — on relance la mesure et on ajoute
un nouveau tableau.

## Référence — A* macro

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* macro | 2026-07-23 | c54d7d7 |
| 1 | 14 | 97 | A* macro | 2026-07-23 | c54d7d7 |
| 2 | 433 | 131 | A* macro | 2026-07-23 | c54d7d7 |
| 3 | 509 | 134 | A* macro | 2026-07-23 | c54d7d7 |
| 4 | 4 430 969 | 355 | A* macro | 2026-07-23 | c54d7d7 |
| 5 | 71 339 | 143 | A* macro | 2026-07-23 | c54d7d7 |
| 6 | 821 | 110 | A* macro | 2026-07-23 | c54d7d7 |
| 7 | 210 849 | 90 | A* macro | 2026-07-23 | c54d7d7 |
| 17 | 202 053 | 213 | A* macro | 2026-07-23 | c54d7d7 |

## Backtrack sur les forks (promu en défaut à `f5ceb0e`)

`macroVersButBacktrack`, désormais appelé sans condition (cf. `plan.md` §6.3).

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| 1 | 14 | 97 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| 2 | 433 | 131 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| 3 | 509 | 134 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| 4 | 4 413 543 | 355 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| 5 | 38 594 | 143 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| 6 | 821 | 110 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| 7 | 210 925 | 90 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
| **8** | **11 721 760** | **238** | **A\* macro + backtrack** | **2026-07-23** | **d7eeef5** |
| **9** | **1 364 579** | **237** | **A\* macro + backtrack** | **2026-07-23** | **f5ceb0e** |
| 17 | 202 053 | 213 | A* macro + backtrack | 2026-07-23 | f5ceb0e |

## But du couplage (`AstarMacroCouplage`, régime d'ESSAI — pas le défaut)

La macro pousse en priorité la caisse que le couplage hongrois destine au but
actif (cf. `plan.md` §6.3, 2026-07-24). `AstarMacro` reste le défaut, inchangé.
Poussées identiques au régime par défaut sur tous les niveaux mesurés.

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* macro couplage | 2026-07-24 | 706a801 |
| 1 | 14 | 97 | A* macro couplage | 2026-07-24 | 706a801 |
| 2 | 433 | 131 | A* macro couplage | 2026-07-24 | 706a801 |
| 3 | 509 | 134 | A* macro couplage | 2026-07-24 | 706a801 |
| 4 | 4 413 156 | 355 | A* macro couplage | 2026-07-24 | 706a801 |
| 5 | 37 172 | 143 | A* macro couplage | 2026-07-24 | 706a801 |
| 6 | 799 | 110 | A* macro couplage | 2026-07-24 | 706a801 |
| 7 | 210 824 | 90 | A* macro couplage | 2026-07-24 | 706a801 |
| 8 | 11 719 844 | 238 | A* macro couplage | 2026-07-24 | 706a801 |
| 9 | 1 296 392 | 237 | A* macro couplage | 2026-07-24 | 706a801 |
| 17 | 202 053 | 213 | A* macro couplage | 2026-07-24 | 706a801 |

Bancs d'essai (hors des 33) :

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 190 | 268 579 | 220 | A* macro couplage | 2026-07-24 | 706a801 |
| 191 | 15 | 250 | A* macro couplage | 2026-07-24 | 706a801 |

## Corral unitaire (promu en défaut à `ef150d3`)

Élagage deadlock corral unitaire actif sans condition (cf. `plan.md` §6.1, 2026-07-27).
Forme incrémentale O(1) prouvée équivalente au balayage complet. Poussées identiques
au canari sur tous les niveaux ; états réduits là où le motif est présent (×6,6 sur le
4, ×6,8 sur le 7), inchangés ailleurs.

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 1 | 14 | 97 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 2 | 433 | 131 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 3 | 509 | 134 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 4 | 665 967 | 355 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 5 | 34 711 | 143 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 6 | 821 | 110 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 7 | 31 166 | 90 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 8 | 11 721 759 | 238 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 9 | 1 215 113 | 237 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |
| 17 | 202 053 | 213 | A* macro + backtrack + corral | 2026-07-27 | ef150d3 |

## Corral pince — motif 2 (promu en défaut à `66db7ca`)

Deuxième motif du corral unitaire : la « pince » (deux caisses scellant S, chacune ne
pouvant qu'entrer dans S — cf. `plan.md` §6.1, 2026-07-27 suite). Règle générale
LIBRE/CAPTIVE/IMMOBILE, 0 faux positif au juge `fp` sur les 11 résolus. Poussées
identiques au canari. Touche une famille de morts que le motif 1 ratait : le **8**
(×1,98) et le **17** (que le corral seul laissait à 202 053).

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 1 | 14 | 97 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 2 | 433 | 131 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 3 | 509 | 134 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 4 | 665 967 | 355 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 5 | 30 510 | 143 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 6 | 698 | 110 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 7 | 29 725 | 90 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 8 | 5 905 757 | 238 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 9 | 1 215 113 | 237 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |
| 17 | 190 635 | 213 | A* macro + backtrack + corral pince | 2026-07-27 | 66db7ca |

## Anchor prod `e77b98a` (merge de `gain-deadlock` en master)

Passe bench de contrôle après merge en prod, sur `e77b98a` (« Optim mémoires +
macro étendu + corrals static »). **Tous les compteurs identiques à `66db7ca`** :
`optim mémoire` + `macro étendu` + `corrals static` n'ont régressé ni les états ni
les poussées d'aucun résolu — le canari a tenu à travers le merge et ces trois évols.

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 1 | 14 | 97 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 2 | 433 | 131 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 3 | 509 | 134 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 4 | 665 967 | 355 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 5 | 30 510 | 143 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 6 | 698 | 110 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 7 | 29 725 | 90 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 8 | 5 905 757 | 238 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 9 | 1 215 113 | 237 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |
| 17 | 190 635 | 213 | A* macro + backtrack + corral pince | 2026-07-27 | e77b98a |

## Corral-N — strip + A* borné mémoïsé (régime d'ESSAI `CORRAL_DETECT=3 CORRAL_BUDGET=150`)

Item B (§6.1, 2026-07-27 suite 3). Le vrai levier `f<C*` : détection d'enclos → gate
(filtre) → **strip + BFS borné** (preuve par exhaustion) → prune, mémoïsé par frontière.
Sound par construction, **poussées identiques au canari**. Régime d'essai derrière
l'interrupteur `CORRAL_DETECT=3`, PAS le défaut (net gain sur les gros, légère perte sur
les petits déjà rapides — cf. `plan.md` pour l'USok). Gros gains d'états : 4 ×9,9,
17 ×7,7, 9 ×3,4, 5 ×3,3.

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 1 | 14 | 97 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 2 | 412 | 131 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 3 | 499 | 134 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 4 | 67 224 | 355 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 5 | 9 123 | 143 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 6 | 570 | 110 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 7 | 24 376 | 90 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 8 | 4 376 070 | 238 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 9 | 354 622 | 237 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |
| 17 | 24 786 | 213 | mode3 (corral-N strip+A*) @150 | 2026-07-27 | 6bdd55c |

## Corral-N PROMU EN DÉFAUT (`cb4780c`)

Le corral-N n'est plus derrière un interrupteur : `CORRAL_DETECT` retiré, budget figé à 150,
appel inconditionnel à l'enfilage (cf. `plan.md` §6.1, session du 2026-07-28). **Vérifié binaire
contre binaire** : le défaut reproduit **à l'unité** le régime d'essai `CORRAL_DETECT=3
CORRAL_BUDGET=150` mesuré sur le binaire d'avant (tableau ci-dessus). Poussées identiques au canari.

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 1 | 14 | 97 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 2 | 412 | 131 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 3 | 499 | 134 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 4 | 67 224 | 355 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 5 | 9 123 | 143 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 6 | 570 | 110 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 7 | 24 376 | 90 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 8 | 4 376 070 | 238 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| 9 | 354 622 | 237 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |
| **11** | **87 085 967** | **241** | **A\* macro couplage + corral-N** | **2026-07-28** | **cb4780c** |
| 17 | 24 786 | 213 | A* macro + corral-N (défaut) | 2026-07-28 | cb4780c |

🎉 **PREMIÈRE RÉSOLUTION DU NIVEAU 11** — la cible historique du projet, jamais finie jusqu'ici
(record précédent : 11/14 caisses posées, arrêt manuel à 57,7 M dépilements le 2026-07-24).
Mené au bout sans budget, en régime `couplage`. **12 niveaux résolus sur 33.**
Mémoire à l'arrivée : 123,98 M clés en arène, file 41,5 M — de l'ordre de 8 Go (§6.5).

## Plongeon sur record (`AstarMacroCouplagePlongeon`, régime d'ESSAI — pas le défaut)

Dès qu'un état bat le max de caisses posées, on tente de le COMPLÉTER par une recherche
gloutonne (best-first sur `h` seul) dont le **budget vaut 1/100 des états déjà développés**
(cf. `plan.md` §6.0, 2026-07-28). `AstarMacro` reste le défaut, inchangé.

Régime mesuré : **couplage + corral-N + plongeon**. Le corral-N est le défaut depuis `cb4780c`,
donc actif dans toutes ces mesures ; le couplage est neutre ou meilleur partout (×1,06 sur le 5,
×1,04 sur le 6, ×1,03 sur le 9, identique ailleurs, poussées inchangées) — il n'y a donc aucune
raison de mesurer le plongeon sans lui.

⚠️ **Régime SOUS-OPTIMAL assumé** : les poussées ne sont plus le canari. Elles restent pourtant
identiques à l'optimal sur 8 niveaux sur 12 ; écarts sur le 4 (+4), le 5 (+8), le 8 (+2) et le
11 (+2).

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| 1 | 14 | 97 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| 2 | 431 | 131 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| 3 | 502 | 134 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| **4** | **2 238** | 359 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| 5 | 8 623 | 151 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| 6 | 559 | 110 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| 7 | 24 667 | 90 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| **8** | **159 484** | 240 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| **9** | **82 998** | 237 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| **11** | **13 918 468** | **243** | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |
| 17 | 24 813 | 213 | A* couplage + corral-N + plongeon | 2026-07-28 | c21b112 |

🎯 **Le 11 en couplage + plongeon : ×6,3 contre la force brute** (13 918 468 états contre
87 085 967 en `couplage` seul), pour **+2 poussées** (243 contre 241). Le plongeon gagnant part de
**10/14 caisses posées** et coûte **11 états**, sur un budget de 139 184 — 10 plongeons tentés au
total sur tout le run. C'est le résultat le plus net du régime : il ne se contente pas d'accélérer
des niveaux déjà résolus, il rend abordable le plus dur.

## Plongeon — diviseur du budget FIGÉ à 1/50 (balayé le 2026-07-28)

Le premier réglage (1/100) était à la limite basse de la plage utile et laissait **×6,9 sur le 8**.
Balayage complet (cf. `plan.md` §6.0) : plage sûre **[1/20, 1/100]**, bornée en haut par la
dégradation du niveau 2 (133 poussées à 1/15) et en bas par la perte du 4 et du 8 (à 1/500).
**1/50 est au centre**, et l'interrupteur `PLONGEON_DIV` est retiré.

Régime : **couplage + corral-N + plongeon**. `AstarMacro` reste le défaut.

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| 0 | 4 | 4 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| 1 | 14 | 97 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| 2 | 449 | 131 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| 3 | 512 | 134 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| **4** | **2 115** | 359 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| 5 | 8 629 | 151 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| 6 | 570 | 110 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| 7 | 24 969 | 90 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| **8** | **22 991** | 240 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| **9** | **83 014** | 237 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| **11** | **13 919 578** | **243** | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |
| 17 | 24 813 | 213 | A* couplage + corral-N + plongeon @1/50 | 2026-07-28 | d627ea6 |

**Contre le défaut `A* macro` : ×190 sur le 8** (4 376 070 → 22 991), **×31,8 sur le 4**
(67 224 → 2 115), **×4,3 sur le 9**, **×6,3 sur le 11**. Neutre ailleurs (−9 % au pire, sur le 2 qui
fait 449 états). **Poussées identiques à l'optimal sur 8 niveaux sur 12** ; écarts : 4 (+4), 5 (+8),
8 (+2), 11 (+2). Le 11 est insensible au diviseur (13 919 578 à 1/50 contre 13 918 468 à 1/100).

## 🎉 Niveau 21 — 13ᵉ résolu (2026-07-29)

Tombé pendant une session de relances, **sans aucune modification de code** — le binaire du
2026-07-28 (corral-N promu + plongeon @1/50) suffisait, personne ne l'avait relancé depuis.
Le 21 ne faisait partie d'aucune des cibles travaillées (8, 11, 12).

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| **21** | **2 923 006** | **165** | A* couplage + corral-N + plongeon @1/50 | 2026-07-29 | `f94c8bf` + instrum. |

✅ **Relevé et REPRODUIT hors de l'app** : `bench 21 coupl-plongeon` rend **2 923 006 états à
l'unité près du run de l'app**, et 165 poussées. Binaire = `f94c8bf` + l'instrumentation de mesure
`CACHE_JOUEUR` (coupée par défaut, vérifiée identique à l'unité sur 10 niveaux) — elle ne touche
aucun verdict.

**165 poussées est sans référence** (premier solve du 21, régime plongeon donc sous-optimal
assumé). Pour borner l'écart : `passages 21` donne les trajets solos (`plan.md` §3). ⚠️ La mesure
`CORRAL=0` ci-dessous en trouve **147**, donc 165 est au moins 18 au-dessus de l'atteignable.

**Le plongeon, encore lui — et il réussit de très bas.**

```
[plongeon 7] record 7/13 a 2923006 depiles | budget 58459 -> REUSSI en 53 etats
             cumul plongeons 1915 (0,0655 % du travail)
```

- **Déclenché à 7/13 = 54 % de remplissage**, réussi en **53 états** sur un budget de 58 459 —
  soit **0,09 % du budget accordé**. Même profil que le 11 (11 états sur 139 184 = 0,008 %).
- **6 records réfutés avant** (7 plongeons au total), pour ~1 860 états à eux tous. C'est le motif
  déjà documenté : « les records MORTS dominent la phase initiale, partout ».
- **Surcoût total 0,065 % du travail** — la garantie a priori du budget relatif tient une fois de
  plus (au plus 1/50 du travail fait par record, au plus un record par but).
- ⚠️ **54 % ici, 22 % sur le 8, records morts jusqu'à 79 % sur le 9** : troisième confirmation
  indépendante qu'**aucun seuil en % de remplissage** n'aurait attrapé ces trois cas (§6.3).

**⚠️ SIGNAL — le corral-N coûte ici 7,3× la recherche qu'il sert.**

```
durs juges = 4 016 397   MORTS = 328 757 (8,2 %)   vivants = 834 411   inconnus = 2 853 229 (71 %)
gate : 34,278 % des enfilages (contre 10-21 % documentés)
etats de sous-solve = 21 442 865   contre   2 923 006 etats explores
```

La **fraction de durs prouvés morts** est le prédicteur du gain corral (§6.1) : 40,6 % sur le 4,
24,5 % sur le 9, ~10 % sur 7/17 — et à ~10 % le corral-N était déjà une **perte** en USok. **Le 21
est à 8,2 %, sous le pire cas connu**, et son gate laisse passer trois fois plus de candidats que
partout ailleurs. Mesure `CORRAL=0` en cours pour trancher.
✅ **TRANCHÉ le 2026-07-31** (section en fin de fichier) : le corral y fait **perdre ×2,13 en temps**,
et une fois les deux étages séparés, la perte est **entièrement** celle de l'étage N (×2,08) —
l'unitaire+pince est neutre ici (−2 %), son motif étant absent du 21. ⚠️ Le prédicteur invoqué
ci-dessus est par ailleurs **réfuté** depuis (le 8 gagne ×3,14 à 7,9 % de durs morts).

### ⚠️ Le corral-N sur le 21 : ×1,66 en états, mais **+18 poussées** et 7,3× le travail

| régime | états | **poussées** | états de sous-solve |
|---|---|---|---|
| corral-N **ON** (défaut) | **2 923 006** | 165 | **21 442 865** |
| `CORRAL=0` | 4 861 308 | **147** | 0 |

Deux surprises, en sens opposés.

1. **Le corral-N élague bel et bien** : ×1,66 en états (4,86 M → 2,92 M), alors que sa fraction de
   durs prouvés morts (8,2 %) est sous le pire cas connu. Le prédicteur du §6.1 annonçait une perte
   en états — **il se trompe ici**, et c'est la première fois qu'on le prend en défaut.
2. **Mais il coûte 18 POUSSÉES** (165 contre 147). Ce n'est pas une régression du corral : c'est
   l'**instabilité du plongeon** annoncée au §6.3 — « toute modif décalant le compteur d'états
   décale le moment du plongeon, donc le record d'où il part, donc le nombre de poussées ».
   Première observation directe de cet effet, et il est ample (+12 %).
3. **Le VOLUME DE TRAVAIL penche dans l'autre sens** : 2,92 M états + 21,4 M états de sous-solve
   contre 4,86 M états seuls. Un état de sous-solve (mini-BFS strippé) est bien moins cher qu'un
   état de recherche, donc rien n'est tranché sans chronomètre — **mesure USok à faire**.
   ✅ **FAITE le 2026-07-31** : **38,99 USok avec, 18,35 sans** — le volume de travail avait raison,
   le corral **perd ×2,13** sur le 21. Un état de sous-solve y coûte ~8,3 µs contre ~24,8 µs pour un
   état de recherche : le rapport de coût (≈ 0,33) ne rattrape pas le ×7,3 de volume.

⚠️ Conséquence de méthode : **en régime plongeon, les poussées ne sont plus un canari du tout**,
même approximatif. Elles dépendent du compteur d'états, donc de tout ce qui l'affecte. C'est
exactement l'argument qui avait fait refuser de promouvoir le plongeon en défaut (§6.3) — il est
maintenant chiffré : ±18 poussées sur un niveau, pour une modif qui ne touche pas la qualité.

## 🎉 Niveau 10 — 14ᵉ résolu (2026-07-29)

Tombé le même jour que le 21, toujours **sans modification de code**. Contrairement au 21, le 10
était une **cible identifiée de longue date** : c'est le niveau emblématique du §6.2 multi-salles
(28 buts + 4), celui dont le plan disait que « la macro rebondit entre salles et se mure ».

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| **10** | **2 175 724** | **544** | A* couplage + corral-N + plongeon @1/50 | 2026-07-29 | `703f851` ⚠️ **LINUX** |

✅ **CONFIRMÉ le 2026-07-31** (la colonne commit disait « à confirmer » depuis deux jours) : rejoué
sur la machine **Linux**, `bench 10 coupl-plongeon` rend **2 175 724 états** à l'unité, plongeon
gagnant compris (13/32, 4 339 états, 9,72 % de cumul).
⚠️ **Mais ce chiffre est propre à LINUX.** Sur macOS, au même commit, le même run rend **2 160 492
états** (−0,70 %) — l'ordre de dépilement départage autrement les ex æquo entre libc++ et libstdc++.
Les poussées (544) et toutes les grandeurs géométriques sont identiques ; cf. `plan.md` §1, règle
« le nombre d'états n'est pas portable ».

⚠️ **LE CORRECTIF MULTI-SALLES N'A PAS ÉTÉ NÉCESSAIRE.** Le §6.2 ouvre sur un « correctif minimal »
jamais codé — grouper les buts par composante connexe pour ne jamais entrelacer les salles — présenté
comme la condition pour débloquer 10 / 18 / 24 / 25 / 26. **Le 10 tombe sans.** Ça ne réfute pas le
correctif (les quatre autres restent non résolus), mais ça le **déclasse en priorité** : ce n'est pas
lui qui bloquait le 10.

**Le plongeon réussit depuis 13/32 = 40,6 %** — encore plus bas que le 21 (54 %) et le 8 (22 %).
Quatrième confirmation qu'aucun seuil en pourcentage n'aurait attrapé ces cas.

⚠️ **Mais ici le plongeon COÛTE, pour la première fois** : 4 339 états pour le plongeon gagnant
(contre 11 sur le 11, 53 sur le 21), et **9,72 % du travail total** en cumul sur 12 tentatives —
contre 0,065 % sur le 21 et ~4 % sur le 4. On reste très loin de la borne a priori (au plus 1/50 du
travail par record), mais le régime n'est plus « gratuit » : sur un niveau à 32 buts, il y a 32
records possibles, donc 32 plongeons à payer.

**❌ SIGNAL RETIRÉ le 2026-07-31 — « le corral-N coûte 25× la recherche qu'il sert » était FAUX sur
le 10.** Le bloc de statistiques inscrit ici ne vient **pas de ce run** :

```
❌ CE BLOC EST ÉTRANGER AU RUN DU 10 — conservé pour mémoire, ne pas s'en servir
durs juges = 22 337 966   MORTS = 5 266 282 (23,6 %)   inconnus = 15 337 619 (68,7 %)
etats de sous-solve = 55 001 399   contre   2 175 724 etats explores
```

Le vrai relevé, obtenu sur les **deux** plateformes au même commit — et identique à **0,01 %** entre
elles, parce que ces grandeurs-là ne dépendent que de la géométrie du plateau :

| | Linux | macOS |
|---|---|---|
| durs jugés | 3 474 427 | 3 456 570 |
| **MORTS** | **14 898 (0,4 %)** | **14 895 (0,4 %)** |
| configs distinctes | 27 943 | 27 947 |
| **états de sous-solve** | **4 182 892** | **4 183 492** |
| **ratio sous-solve / recherche** | **×1,92** | **×1,94** |

**Le ratio est ×1,94, pas ×25** ; la fraction de durs morts est **0,4 %, pas 23,6 %**. L'incohérence
était lisible sans rien relancer : 22 M de durs pour 2,17 M d'états font **10 durs par état** là où
la géométrie du plateau en donne **1,6**. Ce chiffre a pourtant servi de « signal » pour ouvrir un
chantier de deux jours — cf. `plan.md` §7, piège neuf sur les blocs de statistiques recopiés.

⚠️ **Ce que le vrai relevé dit quand même** : **99,5 % d'inconnus** sur le 10, chacun payé au budget
plein (150 états) sans rien prouver. Le corral y perd donc bel et bien du temps — mais **×1,22, pas
×25**, et c'est **l'étage N seul** qui perd, l'unitaire+pince rendant **×1,89** (`plan.md` §6.1,
session du 2026-07-31).

## 🎉 Niveau 32 — 15ᵉ résolu (2026-07-29), le premier dû à une CORRECTION

| Niveau | Nb État | Nb Poussé | Méthode | Date | N° de commit |
|---|---|---|---|---|---|
| **32** | **6 591 365** | **153** | A* couplage + corral-N + plongeon @1/50 | 2026-07-29 | `f94c8bf` + fix ordre |

**Différence avec le 10 et le 21 : celui-ci ne serait pas tombé sans modification de code.** Son
ordre de remplissage était **muré au rang 14** (but (4,9)) ; le backtracking sur la garde
(`ordreParPrecedence`, cf. `plan.md` §6.2) le rend sain, et le niveau tombe. **C'est la première
résolution produite par le chantier goal-ordering depuis le 190** (2026-07-20).

Vérification : sur les 33 niveaux + 190 + 191, **le 32 était le SEUL ordre modifié** par ce
correctif — canari intact par construction, et revérifié au solveur
(4/97/131/134/355/143/110/90/237/213, états inchangés).

⚠️ **Le corral-N y coûte encore ×1,8 la recherche** : 11 706 051 états de sous-solve contre
6 591 365 états explorés, pour 794 986 enfilages prunés. Moins extrême que le 21 (×7,3) — et
comparable au 10, dont le ×25 s'est révélé faux le 2026-07-31 (vrai ratio ×1,94). Le poste reste
lourd, mais **un ratio de volume n'est pas un verdict de temps** : le 10 est à ×1,94 et perd ×1,22,
le 9 est à ×28,9 et gagne ×2,24. Le 32 n'a **pas** été chronométré (`CORRAL=0` d'ordre de grandeur
inconnu, run laissé de côté).

## Corral — ATTRIBUTION PAR ÉTAGE (2026-07-31, `703f851` + interrupteur `CORRAL_N` non commité)

Ce qui ferme l'item ouvert du §6.1 (« solve complet du 10 et du 21, défaut contre `CORRAL=0` ») —
et va plus loin : `CORRAL=0` coupant les **deux** étages, un interrupteur de chantier `CORRAL_N=0`
sépare l'étage N (strip + A\* borné) du corral unitaire + pince. Neutralité du défaut vérifiée
binaire contre binaire (états, poussées et stats `[CORRAL-N]` identiques à l'unité).

**Protocole** : `bench <niv> coupl-plongeon`, solves **complets**, temps **CPU** (`/usr/bin/time -l`,
ligne `user` — le mural est inexploitable, cf. `plan.md` §1), USok avec 1 USok = 6,44 s (étalon
`CORRAL=0 bench 2 astar` = 590 066 états / 131 poussées, meilleur de 3). macOS arm64.

| niv | états défaut | états `CORRAL_N=0` | états `CORRAL=0` | USok défaut | USok `CORRAL_N=0` | USok `CORRAL=0` | **étage N** | **unit.+pince** |
|---|---|---|---|---|---|---|---|---|
| 4 | 40 408 | 1 007 744 | 4 271 256 | 0,256 | 5,306 | 19,936 | **×20,7** | ×3,76 |
| 17 | 24 813 | 190 762 | 202 180 | 0,096 | 0,526 | 0,550 | **×5,5** | ×1,05 |
| 8 | 22 992 | 201 155 | 492 404 | 0,370 | 1,163 | 2,744 | **×3,14** | ×2,36 |
| 9 | 83 029 | 1 159 131 | 1 296 470 | 2,812 | 6,306 | 6,828 | **×2,24** | ×1,08 |
| 5 | 8 630 | 29 270 | 37 207 | 0,090 | 0,104 | 0,130 | ×1,16 | ×1,25 |
| **7** | 24 989 | 29 921 | 211 057 | 0,090 | 0,067 | 0,486 | **÷1,34** | ×7,3 |
| **10** | 2 160 492 | 2 248 468 | 4 318 155 | 45,41 | 37,25 | 70,41 | **÷1,22** | ×1,89 |
| **21** | 2 923 006 | 4 851 620 | 4 861 308 | 38,99 | 18,71 | 18,35 | **÷2,08** | ×0,98 |

**Poussées** — canari intact ; les écarts sont l'instabilité connue du plongeon, pas du corral :
4 → 357/355/355 · 17 → 213 partout · 8 → 240 partout · 9 → 237 partout · 5 → 151/143/143 ·
7 → 90 partout · **10 → 544/544/542** · **21 → 165/147/147**.

**Mémoire (pic, `/usr/bin/time -l`)** : 21 → 279 Mo (défaut) contre 624 Mo (`CORRAL=0`) ;
10 → 1,33 Go contre 3,15–5,18 Go selon le tirage.

**Lecture :**
- **L'étage N perd sur 7, 10 et 21** — les trois niveaux hors de l'échantillon qui avait servi à le
  promouvoir — et gagne massivement sur 4, 9, 17 (l'échantillon d'origine) plus le 8.
- **L'unitaire+pince ne coûte jamais rien** (−2 % au pire) et vaut ×7,3 sur le 7, ×3,76 sur le 4,
  ×1,89 sur le 10 où il fait tout le travail.
- ❌ **Le prédicteur « fraction de durs morts » est réfuté** : le 8 gagne ×3,14 à **7,9 %**, sous le
  21 (8,2 %) qui perd ×2,08. Le ratio sous-solve/recherche ne prédit pas non plus.
- Ce qui reste monotone sur les huit : **états épargnés ÷ états de sous-solve dépensés**, avec un
  seuil qui est le rapport de coût entre les deux sortes d'états (0,14 à 0,34 mesuré). Détail,
  limites et suite dans `plan.md` §6.1, session du 2026-07-31.
