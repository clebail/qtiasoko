# Plan solveur Sokoban

## 1. Normalisation de la région joueur
Un état = positions des caisses + zone atteignable par le joueur sans pousser de caisse.
Deux configurations avec les mêmes caisses mais le joueur dans la même zone = même état.

### 1.1 Clé d'état
`QByteArray` = `[ids des N caisses triés par id de case] + [id min de la zone atteignable par le joueur]`.
- **Toutes** les caisses figurent dans la clé, y compris celles non poussables. L'accessibilité/poussabilité d'une caisse est temporaire (elle dépend de la position des autres caisses) et ne sert qu'à la détection de deadlocks (§2-3), jamais à l'identité de l'état. Ignorer une caisse fusionnerait des états distincts et pourrait couper une branche menant à la solution.
- Caisses triées par id de case → canonicalise le fait que les caisses sont indistinguables.
- Case joueur = min des ids de la zone atteignable → représentant canonique de la zone.
- Longueur fixe (N+1 cases) → pas de délimiteur, aucune ambiguïté.
- Encodage : `short` par case (2 octets). Niveaux 16×16 max = 256 cases (ids 0-255), un `char` non signé déborderait à la moindre sentinelle/bordure ; le `short` laisse de la marge. Clé = 2·(N+1) octets, endianness fixe et constante.

### 1.2 État d'avancement
- [x] `getEtat()` : clé caisses (scan y/x → tri implicite) + `getMinIdx()`.
- [x] `getMinIdx()` : BFS 4-directions depuis le joueur, `QVector<bool> visite`, bornes de bord corrigées (wrap horizontal via `% largeur`), renvoie le min des ids atteignables.
- [x] Tests unitaires (§1.3) — cible QTest dans `tests/` (`qmake && make && ./tst_getetat`). **94 passés / 5 ignorés** (niveaux 10, 12, 13, 21, 32 : joueur coincé sur une seule case → invariance triviale).

### 1.3 Tests unitaires (QTest)
Cible séparée `tests/tests.pro` (n'inclut que `game/level/sprite/*`, pas la GUI ; `QTEST_GUILESS_MAIN`, aucune ressource ni `QApplication`). Chemin des `.xsb` injecté via `DEFINES += LEVELS_DIR`.
Chaque `.xsb` est parsé en carte canonique (murs/goals/caisses/joueur) ; les variantes sont re-générées dans un `.xsb` temporaire puis chargées comme un vrai niveau — `getEtat()` ne dépend que de la grille.

BFS sur les cases libres (murs **et** caisses = obstacles) pour identifier les zones de chaque niveau.
Pour chaque niveau (32 au total) :
- Deux positions dans la même zone + mêmes caisses → même état normalisé (`memeZone`, testé sur toute la composante connexe du joueur)
- Deux positions dans des zones différentes + mêmes caisses → états différents (`zonesDifferentes` : niveau synthétique 2 salles + poches inaccessibles des niveaux réels)
- Une caisse à une position différente → nouvel état, même si le mouvement est inutile et même si la caisse n'est pas poussable dans cet état (`caisseDeplacee`)
- Sanity : grille re-générée == chargement direct du `.xsb`, et déterminisme de `getEtat()` (`renduFidele`)

## 2. Corner deadlocks
Une caisse (pas sur un goal) contre deux murs perpendiculaires → état élagué.
- [x] `checkDefaite()` : pour chaque caisse `tcCaisse` (goal exclu), teste les 4 coins (`coins[NB_DIRECTION][NB_COIN_TO_CHECK]`) ; si un coin a ses 2 cases perpendiculaires en mur → `perdu`. Appelée par `move()` après une poussée (si pas déjà gagné). Pas de test de bornes : bordure toujours en murs.
- [x] Tests (`cornerDeadlock`, data-driven) : 4 orientations de coin, détection parmi plusieurs caisses, et non-défaites (coin sur goal, un seul mur, centre). `checkDefaite()` testé en direct via `friend class TestGetEtat`.

## 3. Adjacent deadlocks
Deux caisses côte à côte contre un mur → aucune des deux ne peut bouger → état élagué.
- [x] Intégré à `checkDefaite()` (après le bloc corner) : pour chaque caisse `tcCaisse`, si une voisine (dans une des 4 directions) est une caisse (`tcCaisse` ou `tcGoalCaisse`) et que les deux sont bloquées du même côté perpendiculaire (mur/caisse/goalcaisse) → `perdu`. Condition actuelle `(hautA ∧ hautB) ∨ (basA ∧ basB)` : correcte et sans faux positif (mur→paire adossée, caisse→bloc 2×2, mixte→L gelé). **Incomplète** : rate le cas diagonal `(hautA ∨ basA) ∧ (hautB ∨ basB)` — sous-détection, sûr pour le solveur.
- [x] Tests (`adjacentDeadlock`, data-driven) : paires horizontale/verticale adossées à un mur, bloc 2×2, caisse + caisse-sur-goal ; non-défaites : paires en espace ouvert, paire déjà résolue sur goals.
