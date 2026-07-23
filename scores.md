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
| **9** | **1 364 579** | **237** | **A\* macro + backtrack** | **2026-07-23** | **f5ceb0e** |
| 17 | 202 053 | 213 | A* macro + backtrack | 2026-07-23 | f5ceb0e |
