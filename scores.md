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
