# Plan solveur Sokoban

## 1. Normalisation de la région joueur
Un état = positions des caisses + zone atteignable par le joueur sans pousser de caisse.
Deux configurations avec les mêmes caisses mais le joueur dans la même zone = même état.

### 1.1 Tests unitaires (QTest)
BFS sur les cases libres pour identifier les zones de chaque niveau.
Pour chaque niveau (32 au total) :
- Deux positions dans la même zone + mêmes caisses → même état normalisé
- Deux positions dans des zones différentes + mêmes caisses → états différents
- Une caisse inaccessible (non poussable depuis la zone joueur) est ignorée dans la comparaison d'états
- Une caisse accessible à une position différente → nouvel état, même si le mouvement est inutile

## 2. Corner deadlocks
Une caisse (pas sur un goal) contre deux murs perpendiculaires → état élagué.

## 3. Adjacent deadlocks
Deux caisses côte à côte contre un mur → aucune des deux ne peut bouger → état élagué.
