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
- [x] Niveaux embarqués dans le `.qrc` (préfixe `/levels`) au lieu d'être lus depuis `QDir::current()` : le répertoire courant n'est pas celui des sources (shadow build de Qt Creator, `/` quand le `.app` est lancé depuis le Finder) et le combo restait vide. Le numéro extrait du nom de fichier est désormais la seule source de vérité (libellé du combo *et* `Game::numNiveau`, qui étaient décalés de 1).
- [x] `Solveur` rendu abstrait (`run()` pur), BFS extrait dans `SolveurBFS`. La base porte le socle `QThread`, les signaux, `depart`, `noeuds`/`reconstruire()` et la table des directions. `Solveur::types()` + `Solveur::creer()` : l'UI peuple son select et instancie sans connaître aucune sous-classe concrète. Ajouter un solveur = 1 entrée d'enum + 1 ligne dans `types()` + 1 cas dans `creer()`.

### 4.2 A* — plan d'exécution

**Décision de fond : on optimise le nombre de POUSSÉES**, pas de déplacements.
C'est déjà ce que fait le BFS actuel : chaque arête de la file est « marche jusqu'à la caisse + 1 poussée », donc le BFS est optimal en poussées — alors que le nombre affiché dans la message box (`chemin.size()`) est un nombre de *coups*. `getHeuristique()` étant une distance en tirages (= en poussées), `g` = nombre de poussées donne un A* homogène et une `h` tendue. Compter les déplacements dans `g` rendrait `h` très lâche (une poussée coûte ≥ 1 déplacement, donc `h_poussées` ≤ `h_coups`) et ferait dégénérer A* vers un Dijkstra. Conséquence assumée : le chemin rejoué peut contenir un peu de marche superflue entre deux poussées.

**`h` est cohérente (monotone), pas seulement admissible.** Une poussée déplace une caisse d'une case ; `distanceButs` étant une distance de graphe sur les tirages, elle ne varie que de ±1 pour cette caisse, les autres ne bougeant pas. `h` décroît donc d'au plus 1 par poussée, qui coûte exactement 1. **Conséquence exploitée : quand A* dépile un état pour la première fois, son `g` est déjà optimal → aucun état fermé à rouvrir.**

#### Étape 1 — `Game` : sémantique de déplacement
- [ ] Ajouter `Game(Game&&) noexcept` et `Game& operator=(Game&&) noexcept` : voler le pointeur `cases` (puis `other.cases = nullptr`), `std::move` sur `goals`/`casesMortes`/`distanceButs`, copie des scalaires. Ne pas oublier `gagne`/`perdu`/`nbDep`/`nbDepCaisse`/`numNiveau` (cf. le bug de §4.1, même piège).
- **Pourquoi c'est indispensable et pas cosmétique** : une FIFO copie chaque `Game` une fois et n'y retouche jamais ; un tas binaire, lui, réarrange ses éléments — un `pop` fait redescendre un élément sur ~log₂(n) ≈ 18 niveaux et chaque échange est un `tmp = a; a = b; b = tmp`, soit 3 copies profondes. Sans move ctor on passe de ~2 copies profondes par état à plusieurs dizaines.
- **Mesuré** (level0002, 14×10, 300k états, `-O2`, à nombre d'états identique — seul le conteneur varie) : `QQueue<QPair<Game,int>>` = **66 ms**, `std::priority_queue` contenant des `Game` = **567 ms** (×8,6), tas ne contenant que des poignées `{f, idx}` = **27 ms**. Le ×8,6 est un impôt sur le gain d'A*, pas forcément une perte nette — mais sur un niveau où `h` réduit l'espace de moins de 8,6× on verrait *le compteur d'états baisser pendant que le chrono monte*, le pire des signaux à diagnostiquer.

#### Étape 2 — `Game` : poussée directe, sans marche
- [ ] Ajouter `bool pousse(int idxCaisse, EDirection dir)` : **téléporte** le joueur sur la case d'appui puis appelle `move(dir)` (qui fait la vraie poussée, `checkVictoire()`/`checkDefaite()` compris).
- Case d'appui = `idxCaisse - directions[dir]` (l'opposé du vecteur de déplacement — cf. `offsetsPousse` dans `getCaissesDeplacable()`). Mise à jour de `cases` : libérer l'ancienne case du joueur (`tcPlayer→tcNone`, `tcGoalPlayer→tcGoal`), occuper la case d'appui (`tcNone→tcPlayer`, `tcGoal→tcGoalPlayer`), déplacer `playerPoint`. Si le joueur est **déjà** sur la case d'appui, ne rien faire avant le `move()`.
- **Légitimité de la téléportation** : `getCaissesDeplacable(zone)` garantit déjà que la case d'appui est dans la zone du joueur — donc joignable sans pousser. La méthode ne *vérifie pas* cette accessibilité, elle la suppose : à documenter dans le header, c'est une précondition de l'appelant. Garde-fou bon marché : retourner `false` si la case d'appui n'est ni libre ni la case courante du joueur.
- `nbDep` ne comptera qu'un coup au lieu de la marche complète. Sans conséquence : `nbDep` n'entre pas dans `getEtat()`, et l'affichage porte sur le `game` de `MainWindow`, piloté par de vrais `deplace()` pendant le rejeu.
- **Pourquoi ça vaut le coup** : aujourd'hui `AStar(&e).getChemin(...)` (un parcours complet de la grille) est appelé pour **chaque enfant généré**, donc *avant* de savoir s'il est un doublon — et sur un niveau à 10 caisses l'écrasante majorité des enfants sont des doublons. Ce chemin de marche ne sert qu'à reconstruire la solution finale, jamais à l'identité de l'état. Gain indépendant d'A*, cumulable avec.

#### Étape 3 — `Solveur` : renommer la table de directions
- [ ] `Solveur::directions` (`{{0,1},{-1,0},{0,-1},{1,0}}`) n'est **pas** un vecteur de déplacement : c'est l'opposé, c'est-à-dire l'offset de la **case d'appui** relative à la caisse (il coïncide avec `offsetsPousse` de `game.cpp`, pas avec `directions` de `game.cpp` qui vaut `{{0,-1},{1,0},{0,1},{-1,0}}`). Le renommer (`appuis`) avant d'écrire A*, sinon la poussée directe est un nid à bugs. Répercuter dans `solveurbfs.cpp`.

#### Étape 4 — `SolveurAStar`
- [ ] Nouveaux fichiers `solveurastar.h/.cpp`, `class SolveurAStar : public Solveur`, `Q_OBJECT`, seul `run()` à écrire.
- **Conteneur : `std::vector<Element>` + `std::push_heap`/`std::pop_heap`, PAS `std::priority_queue`.** Raison : `priority_queue::top()` rend une `const&`, impossible d'en *déplacer* l'état — tout le bénéfice de l'étape 1 serait perdu au dépilement. Avec `pop_heap` on récupère l'élément en fin de vecteur et on fait `Element e = std::move(file.back()); file.pop_back();`.
- `struct Element { int f; int g; Game etat; int idxNoeud; QByteArray cle; };`
  - **`cle` est portée par l'élément**, calculée une seule fois à la génération. Sinon il faut refaire un `getEtat()` — donc un flood-fill complet — à chaque dépilement.
- Comparateur (tas-min sur `f`) : `if (a.f != b.f) return a.f > b.f; return a.g < b.g;` — à `f` égal on préfère le **`g` le plus grand** (état le plus profond, donc le plus proche du but) : départage classique qui fait plonger A* vers la solution au lieu de balayer le plateau.
- **Dédup : `QHash<QByteArray,int> meilleurG`, pas un `QSet` de vus.** Le BFS insère dans `vus` au moment de l'*enfilage*, ce qui est correct pour une FIFO (l'ordre d'enfilage est l'ordre de coût) mais **plus du tout** pour A*, qui dépile par `f` et non par `g`. On n'enfile un enfant que si son état est inconnu ou atteint avec un `g` strictement meilleur.
- Boucle :
  1. `pop_heap` + `std::move` de l'élément.
  2. **Entrée périmée** : si `cur.g > meilleurG.value(cur.cle, INT_MAX)` → `continue` (un meilleur chemin vers cet état a été trouvé depuis son enfilage). Ne pas incrémenter le compteur.
  3. `compteur++`, test `cur.etat.isGagne()` → `emit solutionTrouvee(reconstruire(cur.idxNoeud), compteur)`.
  4. `zone = cur.etat.getZoneJoueur()` **une fois**, puis `getCaissesDeplacable(zone)` (l'overload prenant la zone existe exactement pour ça).
  5. Pour chaque caisse × direction poussable : `Game e(cur.etat)` ; `e.pousse(i, d)` ; écarter si `e.isPerdu()` ; `cle = e.getEtat()` ; `gE = cur.g + 1` ; écarter si `gE >= meilleurG.value(cle, INT_MAX)` ; sinon `meilleurG.insert(cle, gE)`.
  6. **Seulement ici**, sur un enfant réellement retenu : calculer la marche `AStar(&cur.etat).getChemin(cur.etat.getPlayerPoint(), appui)` — **sur l'état PARENT** (avant poussée), depuis la position du joueur parent jusqu'à la case d'appui — puis `coups.append(d)`, `noeuds.append(Noeud{cur.idxNoeud, coups})`.
  7. `file.push_back(Element{gE + e.getHeuristique(), gE, std::move(e), noeuds.size()-1, cle})` + `push_heap`.
- `#include <climits>` pour `INT_MAX`.

#### Étape 5 — Câblage
- [ ] `Solveur::EType` : ajouter `Astar` (**pas** `AStar` : la classe de pathfinding de `astar.h` porte déjà ce nom, collision assurée dans `solveur.cpp`). Une ligne dans `types()` (libellé `"A* (poussées)"`), un cas dans `creer()`. `MainWindow` n'a rien à changer.
- [ ] `qtiasoko.pro` : `solveurastar.cpp` dans `SOURCES`, `solveurastar.h` dans `HEADERS`.

#### Étape 6 — Vérification
- [ ] À solution trouvée, **le nombre de poussées doit être identique entre BFS et A*** sur les niveaux que le BFS boucle (c'est la même garantie d'optimalité) ; seul le nombre d'états explorés doit chuter. Un écart = bug dans `meilleurG` ou dans la cohérence de `h`.
- [ ] Le nombre de *coups* peut différer entre les deux : normal, l'optimalité porte sur les poussées.
- [ ] Mesurer états explorés **et** temps sur level0002 (10 caisses, le BFS y arrête sa file à 600k éléments) — les deux doivent baisser. Si les états baissent mais pas le temps, l'étape 1 a été ratée.

### 4.3 À reprendre plus tard
- [ ] IDA* envisagé en repli si la mémoire devient le facteur limitant plutôt que le temps.
- [ ] `checkDefaite()` rate le deadlock adjacent diagonal `(hautA ∨ basA) ∧ (hautB ∨ basB)` (cf. §3) — sous-détection, donc sûr, mais c'est de l'élagage gratuit laissé sur la table.
- [ ] Idée initialement écartée : trier les caisses par distance dans la boucle de génération des enfants — **ne sert à rien seul**, un BFS FIFO explore tout le palier N avant N+1 quel que soit l'ordre d'énumération à l'intérieur d'un état ; le gain ne vient que d'une vraie file de priorité sur l'ensemble du front de recherche. Devient sans objet une fois A* en place.
