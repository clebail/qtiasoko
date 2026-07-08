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
- [x] ~~`coins[NB_DIRECTION][NB_COIN_TO_CHECK]`~~ **remplacé** par `casesMortes` (§4) : table statique par niveau (calculée une fois, indépendante des caisses réelles) qui généralise le coin classique — capture aussi les culs-de-sac non géométriques et le cas "caisse contre un seul mur sans but atteignable le long de ce mur" (jamais détecté par le simple test de coin). `checkDefaite()` teste juste `casesMortes[idx]` pour chaque caisse `tcCaisse`. Pas de test de bornes : bordure toujours en murs.
- [x] Tests (`cornerDeadlock`, data-driven) : 4 orientations de coin, détection parmi plusieurs caisses, et non-défaites (coin sur goal, un seul mur — avec but atteignable le long du mur —, centre — avec but atteignable). `checkDefaite()` testé en direct via `friend class TestGetEtat`.

## 3. Adjacent deadlocks
Deux caisses côte à côte contre un mur → aucune des deux ne peut bouger → état élagué.
- [x] Intégré à `checkDefaite()` (après le bloc corner) : pour chaque caisse `tcCaisse`, si une voisine (dans une des 4 directions) est une caisse (`tcCaisse` ou `tcGoalCaisse`) et que les deux sont bloquées du même côté perpendiculaire (mur/caisse/goalcaisse) → `perdu`. Condition actuelle `(hautA ∧ hautB) ∨ (basA ∧ basB)` : correcte et sans faux positif (mur→paire adossée, caisse→bloc 2×2, mixte→L gelé). **Incomplète** : rate le cas diagonal `(hautA ∨ basA) ∧ (hautB ∨ basB)` — sous-détection, sûr pour le solveur.
- [x] Tests (`adjacentDeadlock`, data-driven) : paires horizontale/verticale adossées à un mur, bloc 2×2, caisse + caisse-sur-goal ; non-défaites : paires en espace ouvert, paire déjà résolue sur goals.

## 4. Solveur haut niveau
Recherche sur les états caisses+zone (§1), avec élagage par `checkDefaite()` (§2-3).

### 4.1 Fait
- [x] `Solveur` (QThread dédié — le solve peut explorer des centaines de milliers d'états, hors de question de bloquer le thread GUI). Constructeur prend l'état de départ par valeur (`Game` est du POD depuis que les sprites en sont sortis vers `WGame`). Résultat par signal (`solutionTrouvee(QList<Game::EDirection>, qint64 etatsExplores)` / `aucuneSolution()`) plutôt que par retour de fonction.
- [x] BFS : `QQueue<QPair<Game,int>>` (état cloné + index dans `noeuds`) + `QSet<QByteArray>` de dédup (`getEtat()`). Reconstruction du chemin via `Noeud{parent, coups}` dans un `QVector` parallèle — `coups` = trajet `AStar` rejoué (marche) + poussée finale.
- [x] `casesMortes` : table de deadlocks statique par niveau (flood-fill à rebours depuis tous les buts simultanément, en simulant des *tirages* — l'inverse d'une poussée — avec la règle des 2 cases : la case d'arrivée du tirage ET la case où le joueur devrait se tenir pour la poussée équivalente doivent être libres). Calculée une fois à la construction (`calculCaseMorte()`), partagée par copie légère entre tous les clones (ne dépend que des murs/buts, jamais des caisses réelles). Intègre §2.
- [x] `distanceButs` + `Game::getHeuristique()` : sous-produit du même flood-fill (distance en tirages depuis le but le plus proche, au lieu d'un simple booléen). Heuristique admissible = somme de `distanceButs[i]` pour chaque caisse actuellement sur le plateau (ignore les autres caisses → ne surestime jamais le coût réel).
- [x] UI (`mainwindow`) : combo des niveaux (scan `level????.xsb`), bouton IA, rejeu automatique par `QTimer`, bouton "Revoir", message box Oui/Non à la victoire, overlay stats (`Etats explores` avec séparateur de milliers).
- [x] **Bug corrigé** : le ctor de copie et `operator=` de `Game` ne copiaient pas `gagne`/`perdu`/`nbDep`/`nbDepCaisse` — un état gagnant cloné (pour être enfilé) perdait son flag `gagne` en route, donnant "aucune solution" alors que `checkVictoire()` avait bien été atteint.

### 4.2 À reprendre plus tard
- [ ] Passer la `QQueue` FIFO en file de priorité ordonnée sur `f = coups_faits + game.getHeuristique()` (A* classique d'abord — changement plus contenu que IDA*, la mémoire n'est pas encore le facteur limitant). Décidé après avoir observé qu'un BFS pur explose sur les niveaux à beaucoup de caisses (ex. level0002 : 10 caisses, file arrêtée à 600k éléments) même avec `casesMortes` (qui réduit le facteur de branchement mais pas la combinatoire liée au nombre de caisses).
- [ ] IDA* envisagé en repli si la mémoire devient le facteur limitant plutôt que le temps.
- [ ] Idée initialement écartée : trier les caisses par distance dans la boucle de génération des enfants — **ne sert à rien seul**, un BFS FIFO explore tout le palier N avant N+1 quel que soit l'ordre d'énumération à l'intérieur d'un état ; le gain ne vient que d'une vraie file de priorité sur l'ensemble du front de recherche.
