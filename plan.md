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
- [x] Intégré à `checkDefaite()` (après le bloc corner) : pour chaque caisse `tcCaisse`, si une voisine (dans une des 4 directions) est une caisse (`tcCaisse` ou `tcGoalCaisse`) et que les deux sont bloquées du même côté perpendiculaire (mur/caisse/goalcaisse) → `perdu`. Condition actuelle `(hautA ∧ hautB) ∨ (basA ∧ basB)` : correcte et sans faux positif (mur→paire adossée, caisse→bloc 2×2, mixte→L gelé). **Incomplète** : rate le cas diagonal — sous-détection, sûr pour le solveur. → **Remplacé par le test de gel récursif, cf. §3bis** (et attention : la généralisation naïve du cas diagonal introduit des faux positifs).
- [x] Tests (`adjacentDeadlock`, data-driven) : paires horizontale/verticale adossées à un mur, bloc 2×2, caisse + caisse-sur-goal ; non-défaites : paires en espace ouvert, paire déjà résolue sur goals.

## 3bis. Freeze deadlocks — FAIT (gain : ~5 %)

**Résultat mesuré, sans embellissement :**

| niveau | états avant | états après | gain | poussées |
|---|---|---|---|---|
| 0  (BFS) | 111        | 111        | 0 %    | 4 ✅ |
| 1  (BFS) | 1 040 235  | 983 557    | −5,4 % | 97 ✅ |
| 1  (A*)  | 834 329    | 783 474    | −6,1 % | 97 ✅ |
| 17 (A*)  | 29 135 928 | 28 467 673 | **−2,3 %** | 213 ✅ |

Optimalité préservée partout (aucun faux positif). **Mais le gain est dérisoire — et il DÉCROÎT quand le niveau grossit** : −6 % sur le niveau 1, −2,3 % sur le 17. Autant dire rien là où on en aurait le plus besoin.

L'espérance « la détection de deadlocks est le seul levier qui réduit le nombre d'états » était juste dans son principe — elle réduit bien — mais l'ampleur est négligeable. La raison : **`casesMortes` (§2) ramassait déjà l'essentiel.** Le flood-fill à rebours depuis les buts élimine tous les culs-de-sac *statiques*, qui sont la quasi-totalité des deadlocks réels. Le gel ne récupère que les configurations *dynamiques* (caisses qui se bloquent mutuellement), qui sont rares — et d'autant plus rares que le niveau est vaste et ouvert. D'où la décroissance du gain avec la taille.

**Implémentation retenue** (`Game::caisseGelee` / `bloqueeSurAxe` / `estCaisse`) : test de gel récursif, garde de récursion via `QVector<bool> enCours` (la caisse examinée compte comme un mur pour ses voisines). Remplace l'ancien bloc « adjacent » de `checkDefaite()`.

**Piège rencontré, à ne pas refaire.** Première tentative : `bloques[idxCaisse] >= 2`, un décompte du **nombre total** de voisins bloquants. Faux : une caisse dans un **couloir horizontal** (mur au-dessus ET en dessous) a 2 bloqueurs, mais sur le **même axe** — elle glisse librement. Le niveau 1 est devenu *insoluble*. La règle se compte **par axe**, jamais en total. Détecté par le protocole prévu : le nombre de poussées est le canari.

**Tests** (`gel_data` / `gel`, `gelDeadlock_data` / `gelDeadlock`, 16 cas) : `caisseGelee()` est appelée **directement** (via `friend`) et non à travers `checkDefaite()` — sinon `casesMortes` fait passer un test pour une raison qui n'est pas celle qu'on croit éprouver (3 de mes grilles étaient fausses à cause de ça : caisses collées au mur sur une colonne sans but = cases mortes). Couvrent le gel (coin, diagonale à murs opposés, bloc 2×2, paire sous un mur) et surtout le **non**-gel (couloir horizontal = la régression, couloir vertical, contre-exemple `S` sur 3 caisses, un seul mur, espace ouvert), plus la propreté de la garde de récursion.

---

### Spécification d'origine (conservée pour le raisonnement)

**Pourquoi celui-ci d'abord.** A* a montré (§4.3) qu'une heuristique admissible ne peut pas réduire le nombre d'états : une poussée utile fait `g +1` / `h −1`, donc `f` ne bouge pas, et A* est obligé de développer tout état de `f ≤ C*`. **La détection de deadlocks est le SEUL levier qui supprime des états pour de bon** — et c'est exactement le mur du niveau 2 (§4.4). Elle profite à `SolveurBFS` *et* à `SolveurAStar` sans toucher ni l'un ni l'autre : tout est dans `Game::checkDefaite()`.

### Le cas manquant est réel
Paire verticale `A(x,y)` / `B(x,y+1)`, mur à **gauche de A** et mur à **droite de B** :
```
# A
  B #
```
`A` est immobile horizontalement (pousser à droite exige que le joueur soit à sa gauche = mur ; pousser à gauche exige que la destination soit libre = mur — un déplacement sur un axe demande **les deux** cases libres) et verticalement (B en dessous). Idem pour `B`. Blocage mutuel définitif. Le code actuel ne le voit pas.

### ⚠️ Le piège : la généralisation naïve crée des FAUX POSITIFS
Écrire « chaque caisse bloquée d'au moins un côté perpendiculaire, indépendamment » est **faux** dès que le bloqueur est une caisse :
```
C A
  B D
```
`C` bloque `A` à gauche, `D` bloque `B` à droite → deadlock déclaré. Mais `C` peut être poussée vers le haut ou le bas (ses cases haut/bas/gauche sont libres) ; une fois partie, `A` se pousse à droite et tout se débloque. **On aurait élagué une branche valide.**

Un faux positif est **bien plus grave** qu'une non-détection : rater un deadlock coûte du temps, en inventer un fait *manquer la solution* — « aucune solution », ou une solution non optimale, sans aucun signal.

C'est d'ailleurs pour ça que la condition actuelle « même côté pour les deux » est saine : deux murs → paire adossée ; deux caisses → **bloc 2×2**, toujours gelé ; mur + caisse → la géométrie place le mur sous la caisse bloqueuse, ce qui la gèle elle-même. La diagonale casse cette propriété (`A,B,C,D` forment un `S`, pas un bloc).

### L'algorithme correct : test de gel récursif
La distinction qui compte est **« bloquée maintenant » vs « bloquée pour toujours »**.

> Une caisse est **gelée** si elle est bloquée sur **les deux axes**.
> Elle est bloquée sur un axe si :
> 1. l'un des deux voisins de cet axe est un **mur** ; **ou**
> 2. les deux voisins de cet axe sont des **cases mortes** (`casesMortes`, §2) — y pousser mènerait de toute façon à un deadlock, donc l'axe est inutilisable ; **ou**
> 3. l'un des deux voisins est une **caisse elle-même gelée** (récursion).

- [x] **Garde de récursion obligatoire** : `QVector<bool> enCours` — la caisse examinée compte comme un mur pour ses voisines, sinon `A` demande à `B` qui redemande à `A` → récursion infinie.
- [x] **Condition de défaite** : deadlock s'il existe une caisse gelée **qui n'est pas sur un but**. `checkDefaite()` ne boucle que sur les `tcCaisse`, donc un groupe gelé *tout sur des buts* passe — c'est un sous-ensemble résolu.
- [x] Remplace le bloc « adjacent » de `checkDefaite()`. Le test `casesMortes[idx]` de §2 reste en amont, inchangé.

### Vérification — un faux positif doit être impossible à rater
- [x] **Les nombres de poussées n'ont pas bougé** : 4 / 97 / 213. (La première implémentation, elle, rendait le niveau 1 *insoluble* — le canari a chanté.)
- [x] Le nombre d'états baisse — mais peu : −6 % (niveau 1), −2,3 % (niveau 17).
- [x] Tests QTest `gel` + `gelDeadlock` (16 cas) : cas diagonal en défaite, contre-exemple `S` et couloirs en NON-défaite.
- [x] Cas limite : caisse gelée sur un but → **pas** de défaite (`gelDeadlock(coin mais sur goal)`).

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

#### Étape 1 — `Game` : sémantique de déplacement ✅
- [x] Ajouter `Game(Game&&) noexcept` et `Game& operator=(Game&&) noexcept` : voler le pointeur `cases` (puis `other.cases = nullptr`), `std::move` sur `goals`/`casesMortes`/`distanceButs`, copie des scalaires. Ne pas oublier `gagne`/`perdu`/`nbDep`/`nbDepCaisse`/`numNiveau` (cf. le bug de §4.1, même piège).
- **Pourquoi c'est indispensable et pas cosmétique** : une FIFO copie chaque `Game` une fois et n'y retouche jamais ; un tas binaire, lui, réarrange ses éléments — un `pop` fait redescendre un élément sur ~log₂(n) ≈ 18 niveaux et chaque échange est un `tmp = a; a = b; b = tmp`, soit 3 copies profondes. Sans move ctor on passe de ~2 copies profondes par état à plusieurs dizaines.
- **Mesuré** (level0002, 14×10, 300k états, `-O2`, à nombre d'états identique — seul le conteneur varie) : `QQueue<QPair<Game,int>>` = **66 ms**, `std::priority_queue` contenant des `Game` = **567 ms** (×8,6), tas ne contenant que des poignées `{f, idx}` = **27 ms**. Le ×8,6 est un impôt sur le gain d'A*, pas forcément une perte nette — mais sur un niveau où `h` réduit l'espace de moins de 8,6× on verrait *le compteur d'états baisser pendant que le chrono monte*, le pire des signaux à diagnostiquer.

#### Étape 2 — `Game` : poussée directe, sans marche ✅
- [x] Ajouter `bool pousse(int idxCaisse, EDirection dir)` : **téléporte** le joueur sur la case d'appui puis appelle `move(dir)` (qui fait la vraie poussée, `checkVictoire()`/`checkDefaite()` compris).
- Case d'appui = `idxCaisse - directions[dir]` (l'opposé du vecteur de déplacement — cf. `offsetsPousse` dans `getCaissesDeplacable()`). Mise à jour de `cases` : libérer l'ancienne case du joueur (`tcPlayer→tcNone`, `tcGoalPlayer→tcGoal`), occuper la case d'appui (`tcNone→tcPlayer`, `tcGoal→tcGoalPlayer`), déplacer `playerPoint`. Si le joueur est **déjà** sur la case d'appui, ne rien faire avant le `move()`.
- **Légitimité de la téléportation** : `getCaissesDeplacable(zone)` garantit déjà que la case d'appui est dans la zone du joueur — donc joignable sans pousser. La méthode ne *vérifie pas* cette accessibilité, elle la suppose : à documenter dans le header, c'est une précondition de l'appelant. Garde-fou bon marché : retourner `false` si la case d'appui n'est ni libre ni la case courante du joueur.
- `nbDep` ne comptera qu'un coup au lieu de la marche complète. Sans conséquence : `nbDep` n'entre pas dans `getEtat()`, et l'affichage porte sur le `game` de `MainWindow`, piloté par de vrais `deplace()` pendant le rejeu.
- **Pourquoi ça vaut le coup** : aujourd'hui `AStar(&e).getChemin(...)` (un parcours complet de la grille) est appelé pour **chaque enfant généré**, donc *avant* de savoir s'il est un doublon — et sur un niveau à 10 caisses l'écrasante majorité des enfants sont des doublons. Ce chemin de marche ne sert qu'à reconstruire la solution finale, jamais à l'identité de l'état. Gain indépendant d'A*, cumulable avec.

#### Étape 3 — `Solveur` : renommer la table de directions ✅
- [x] `Solveur::directions` (`{{0,1},{-1,0},{0,-1},{1,0}}`) n'est **pas** un vecteur de déplacement : c'est l'opposé, c'est-à-dire l'offset de la **case d'appui** relative à la caisse (il coïncide avec `offsetsPousse` de `game.cpp`, pas avec `directions` de `game.cpp` qui vaut `{{0,-1},{1,0},{0,1},{-1,0}}`). Le renommer (`appuis`) avant d'écrire A*, sinon la poussée directe est un nid à bugs. Répercuter dans `solveurbfs.cpp`.

#### Étape 4 — `SolveurAStar` ✅
- [x] Nouveaux fichiers `solveurastar.h/.cpp`, `class SolveurAStar : public Solveur`, `Q_OBJECT`, seul `run()` à écrire.
  - Bugs rencontrés à l'écriture, tous **silencieux** (ça compilait) : `noeuds` jamais réinitialisé → le premier enfant devenait son propre parent → `reconstruire()` bouclait à l'infini ; le test de dédup ne gardait que l'insertion dans `meilleurG` et **pas l'enfilage** → aucune dédup, explosion ; `reconstruire(cur.f)` au lieu de `reconstruire(cur.idxNoeud)` (un coût passé comme un indice) ; et l'idiome du tas mal formé (`pop_heap` sans `pop_back`, `push_back` sans `push_heap`) → boucle jamais entrée, aucun signal émis, UI figée.
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
  6. `noeuds.append(Noeud{cur.idxNoeud, i, (Game::EDirection)d})` — **aucun `AStar` ici** : `Noeud` ne porte que la poussée, et `Solveur::reconstruire()` (déjà écrit, hérité) recalcule les trajets de marche une seule fois sur la solution retenue.
  7. `file.push_back(Element{gE + e.getHeuristique(), gE, std::move(e), noeuds.size()-1, cle})` + `push_heap`.
- `#include <climits>` pour `INT_MAX`.

#### Étape 5 — Câblage ✅
- [x] `Solveur::EType` : `Astar` — et **pas** `AStar`. La collision annoncée a bien eu lieu : un énumérateur non scopé est injecté dans la portée de `Solveur`, il masquait donc la classe `AStar` d'`astar.h` dans *toutes* les méthodes de `Solveur`, et `reconstruire()` ne compilait plus (« called object type 'Solveur::EType' is not a function »). `MainWindow` n'a rien eu à changer, comme prévu.
- [x] `qtiasoko.pro` : `solveurastar.cpp` / `.h` ajoutés.

#### Étape 6 — Vérification ✅
**Référence BFS mesurée (build `-O2`, M3 Pro 18 Go)** — l'étalon, mesuré *avant* le gel de §3bis :

| niveau | caisses | états | temps | mémoire | solution |
|---|---|---|---|---|---|
| 0  | 3  | 111        | ~1 ms  | —      | 4 poussées / 17 coups   |
| 1  | 6  | 1 040 235  | 7,2 s  | 188 Mo | 97 poussées / 506 coups |
| 17 | 6  | 30 093 130 | 225 s  | 4,0 Go | 213 poussées / 584 coups |
| 2  | 10 | —          | ne termine pas (espace ~2 500× celui du niveau 1) | | |

- Niveau **1** = niveau de mise au point (assez gros pour voir le gain, assez rapide pour itérer). Niveau **17** = niveau de démonstration. Niveau **2** = objectif, mais **aucune référence** pour y valider l'optimalité.
- [x] **Poussées identiques entre BFS et A*** : 4 / 97 / 213. Optimalité confirmée.
- [x] Les *coups* diffèrent (506 en BFS, 560 en A* sur le niveau 1) : normal, l'optimalité porte sur les poussées.
- [x] États **et** temps baissent ensemble — mais très peu (cf. §4.3, c'est là le problème).
- Mémoire du BFS : ~134 o/état, **linéaire**, dominée par le `QSet<QByteArray>` de dédup (une clé de 14 o utiles coûte ~110 o entre l'en-tête `QArrayData` et le nœud de hachage).

### 4.3 A* : résultats réels, et pourquoi le gain n'est PAS venu

**A* est correct — 4 / 97 / 213 poussées, identiques au BFS — et pratiquement inutile.**

| niveau | | états | temps | mémoire | poussées |
|---|---|---|---|---|---|
| 1  | BFS         | 1 040 235  | 7,2 s | 188 Mo | 97 |
| 1  | A*          | 834 329 (−20 %) | 6,9 s | **518 Mo** | 97 |
| 1  | A* pondéré w=2 | **33 890** (−97 %) | **0,23 s** | — | 103 (+6 %) |
| 17 | BFS         | 30 093 130 | 225 s | 4,0 Go | 213 |
| 17 | A*          | 29 135 928 (−3 %) | **259 s** (plus lent) | **4,8 Go** | 213 |
| 17 | A* pondéré w=2 | 13 383 432 (−55 %) | 112 s | 2,5 Go | 217 (+1,9 %) |

**Pourquoi A* n'élague presque rien** — l'erreur d'analyse initiale, à ne pas refaire. Une poussée *utile* fait `g +1` et `h −1` : **`f` reste constant**. Seules les poussées qui *éloignent* une caisse font monter `f`. Or `casesMortes` a déjà supprimé les branches absurdes. Donc la quasi-totalité des états atteignables ont `f ≈ C*`, et A* est *obligé* de développer tout état de `f ≤ C*`. **Une heuristique admissible ne peut pas élaguer ce qui n'est pas mauvais.**

Ce n'est pas un défaut de `h` : mesuré, `h(départ)` vaut 88 pour un optimum de 97 sur le niveau 1 — **tendue à 91 %**, et sans effet. La leçon : *la tension d'une heuristique ne fait pas sa capacité à discriminer.*

Corollaire : le raisonnement « le BFS épuise l'espace (`file = 88`), donc A* a une marge énorme » était un **non-séquitur**. Que le BFS explore tout dit seulement que la solution est profonde, pas qu'il existe des branches à couper.

**Ce qui marche, c'est la pondération** (`f = g + w·h`), qui sacrifie l'optimalité pour plonger : ×30 en temps sur le niveau 1 pour +6 % de poussées. Mais le gain n'est pas constant (seulement ×2 sur le 17, où `h` n'est tendue qu'à 54 %), et elle casse la cohérence de `h` : on observe **1,8× plus de développements que d'états distincts** (re-développements), et un `f` qui *décroît* au fil du dépilement.

### 4.4 Le vrai mur : la mémoire de la file ouverte

Niveau 2 (10 caisses), A* pondéré w=2 : **6,3 Go en 3 minutes**, +2 Go/min, tué avant saturation des 18 Go. Il n'a jamais abouti — ni en BFS, ni en A*.

Diagnostic précis, et il n'est **pas** là où je le croyais : ce n'est pas le `QHash` de dédup (~1 Go pour 8,5 M clés), c'est **la file ouverte** (~3,4 Go pour 4,78 M entrées). Chaque `SElement` transporte un `Game` **complet** : 72 o d'objet + le tableau `cases` de 140 × 4 = **560 o**, soit ~700 o par entrée.

Différence structurelle avec le BFS : sa file restait minuscule (`file = 88` à la fin) parce qu'il épuisait l'espace couche par couche. A*, par construction, maintient un front de *tous* les candidats ouverts.

#### Étape 7 — Alléger `SElement` ✅ (et verdict final sur le niveau 2)

**Fait.** `SElement` = `{f, g, idxNoeud, cle}` (~50 o au lieu de ~700), `Game::appliqueEtat(cle)` reconstruit le plateau au dépilement, sur un `Game` de travail **réutilisé** (hérite de `casesMortes`/`distanceButs` par partage implicite, jamais de `calculCaseMorte()`). Résultats **identiques au bit près** (783 474 / 97 ; 23 072 / 103 ; 13 405 481 / 217) → la reconstruction depuis la clé est exacte.

Gain mémoire réel : **÷2 à ÷3** (niveau 1 A* : 518 → 160 Mo ; niveau 17 pondéré : 2,5 → 1,35 Go). Pas le ×17 annoncé : mon calcul ne portait que sur la file ouverte et ignorait que le `QHash meilleurG` — une clé par état **jamais oublié** — reste intact et devient le poste dominant.

**⚠️ Piège de mesure sur macOS.** `ps rss` est **faux** : le compresseur mémoire sort les pages compressées de la RSS. Observé : RSS qui *baisse* (7,1 → 5,3 Go) pendant que les structures grossissent, alors que le `phys_footprint` réel était à **13 Go**. Utiliser `footprint -p PID`, jamais `ps rss`. Toutes les mesures mémoire antérieures de ce document sont donc **sous-estimées**.

#### 🔴 Niveau 2 : VERDICT — hors d'atteinte de cette approche
> **⚠️ PÉRIMÉ (voir §7.-1) : le niveau 2 A ÉTÉ RÉSOLU** en 591 138 états / 131 poussées, une fois
> la `h` rendue quasi exacte par le couplage hongrois (§7). Le diagnostic ci-dessous — « la cause
> est en amont, `h` ne discrimine pas » — était le **bon**, et sa conclusion (« seule une meilleure
> `h` sauvera le 2 ») s'est vérifiée. Ce n'était pas un mur de combinatoire, c'était un mur
> d'heuristique.
A* pondéré (w=2) + `SElement` allégé + gel + `casesMortes`, build `-O2` :
```
25 min | 119 606 000 états dépilés | 74 352 881 distincts | file ouverte 37 453 122
f = 182 (il REMONTE, il ne converge pas) | pic 20,7 Go (115 % des 18 Go)
→ TUÉ PAR LE SYSTÈME, aucune solution
```
Pour comparaison, le niveau 17 est résolu en 13,4 M états : on est allé **9× plus loin** sans approcher. 83 s de temps *système* = le noyau occupé à compresser pendant qu'on croyait calculer.

**Ce n'est pas qu'un mur mémoire.** Même avec une RAM infinie, `f` qui remonte et 119 M d'états sans convergence disent que le *temps* aurait pris le relais. Ajouter de la mémoire ne fait que déplacer la date de l'échec.

**La cause est en amont : `h` ne discrimine pas** (cf. §4.3). Tant qu'on n'aura pas une heuristique qui sépare vraiment les bons états des mauvais, aucune optimisation de conteneur ne sauvera le niveau 2. Le seul levier restant qui attaque la *cause* est le couplage de coût minimal (§4.6) ; tout le reste (hash 64 bits, IDA*) ne fait que repousser le mur.

#### Spécification (conservée)
Le `Game` est **redondant** dans la file : la clé (`getEtat()`, 22 o pour 10 caisses) détermine entièrement l'état. On ne stocke que la clé, et on reconstruit le `Game` au dépilement.

- [ ] `SElement` = `{int f; int g; int idxNoeud; QByteArray cle;}` — ~40 o au lieu de ~700 (**×17 sur le tas**, ~3,4 Go → ~200 Mo).
- [ ] `Game::appliqueEtat(const QByteArray& cle)` : remet le plateau à nu par un mapping **local**, case par case (`tcCaisse`/`tcPlayer` → `tcNone`, `tcGoalCaisse`/`tcGoalPlayer` → `tcGoal`, le reste inchangé) — surtout **pas** de `goals.contains(i)`, qui serait en O(n) par case. Puis estampille les caisses depuis la clé, puis le joueur sur le dernier index de la clé. **Termine par `checkVictoire()`** : sinon `gagne` reste celui du modèle (faux) et A* ne détecte jamais la solution.
- [ ] Au dépilement : partir d'une copie du `Game` de départ (les `QVector` `casesMortes`/`distanceButs` sont en partage implicite → copie quasi gratuite, et surtout **ne PAS relancer `calculCaseMorte()`**, qui est un flood-fill complet), puis `appliqueEtat(cur.cle)`.
- **Pourquoi la clé suffit** : elle porte les caisses + `getMinIdx()`, la case *canonique* de la zone du joueur. Le joueur reconstruit n'est donc pas forcément où il était — et c'est sans conséquence : `getEtat()` normalise déjà à cette case, `pousse()` téléporte, et `checkVictoire()`/`checkDefaite()` ne dépendent que des caisses. Vérifié aussi pour `getCaissesDeplacable()`, dont le cas spécial `idxDestination == idxPlayer` conclut pareil avec le joueur canonique.
- ⚠️ **Ne réduit PAS le nombre d'états.** 8,5 M distincts après 3 min et en croissance : si le niveau 2 en demande 200 M, on se cogne au même mur, plus tard.

#### Si ça ne suffit pas
- [ ] `QHash<quint64,int>` (hachage 64 bits) au lieu de `QHash<QByteArray,int>` : ~16 o/entrée au lieu de ~110 (**×7**). Prix : risque de collision, qui ferait silencieusement manquer une branche.
- [ ] **IDA*** — recherche en profondeur bornée par `f`, **aucune file ouverte** : mémoire proportionnelle à la profondeur, pas au nombre d'états. C'est la seule réponse *structurelle* au mur mémoire. Prix : perte de la dédup globale, donc revisite d'états.

#### Réalisé en cours de route (hors plan initial)
- [x] **Accélération du BFS** (indépendante d'A*, cumulable) : `pousse()` remplace « `AStar` de marche + `deplace()` » dans la génération des enfants, et `Noeud` passe de `{parent, QList<EDirection> coups}` à `{parent, idxCaisse, dir}` — 12 octets plats, zéro allocation. Le trajet de marche ne sert qu'à l'affichage : `Solveur::reconstruire()` le recalcule une seule fois, en rejouant les poussées depuis `depart`. Niveau 1 : **62,9 s → 7,2 s** (×8,7) et **378 Mo → 188 Mo**, à solution rigoureusement identique. Vérifié : 136 791 poussées comparées sur les 33 niveaux, `pousse()` ≡ marche+poussée, zéro divergence ; et le chemin reconstruit rejoué coup par coup aboutit bien à la victoire.
- [x] **Build en release** : le `.pro` était en `CONFIG += debug` / `-= release`, donc `-O0`. Passé en `release force_debug_info` (`-O2 -g` : optimisé mais toujours profilable). Toutes les mesures ci-dessus sont en `-O2` — un build debug fausse complètement le jugement sur les perfs du solveur.

### 4.5 Les trois solveurs du combo — FAIT

`SolveurAStar` prend un **poids** au constructeur (`f = g + poids · h`) : pas de classe dupliquée pour une simple multiplication. La fabrique instancie `(depart, 1)` ou `(depart, 2)`. `MainWindow` n'a pas changé d'une ligne — ce que la hiérarchie abstraite promettait.

| | niveau 1 (états / poussées) | niveau 17 (états / poussées) |
|---|---|---|
| BFS (optimal)        | 983 557 / 97   | — |
| A* (optimal)         | 783 474 / 97   | 28 467 673 / 213 |
| A* pondéré w=2       | **23 072** / 103 | **13 405 481** / 217 |
| **gain de la pondération** | **×34** (+6 % de poussées) | **×2,1** (+1,9 %) |

**Le gain de la pondération suit la tension de `h`** : 91 % sur le niveau 1 → ×34 ; 54 % sur le 17 → ×2,1 seulement. Gonfler une heuristique qui voit mal ne la fait pas mieux voir. Le 17 reste à ~110 s et 2,5 Go.

**w=2 et pas plus** : w=3 et w=5 explorent PLUS d'états que w=2 (107 k et 172 k contre 23 k sur le niveau 1). Une `h` trop gonflée ne guide plus, elle fait perdre le fil. Commenté dans `creer()` pour que personne ne « l'améliore » en montant le poids.

Libellés volontairement explicites (« optimal » / « rapide, approché ») : cacher la perte d'optimalité derrière un nom technique serait malhonnête.

À noter : l'A* optimal rend 560 coups là où le BFS en rend 506, **à nombre de poussées identique (97)**. Normal — on optimise les poussées, pas la marche ; deux solutions à 97 poussées sont également optimales au sens du solveur.

### 4.6 Le couplage hongrois : ÉCARTÉ, sur mesure
- [x] Mesuré avant d'écrire une ligne (matrice `distance[but][case]` = un flood-fill **par but**, puis affectation de coût minimal) :

| niveau | caisses | `h` actuelle | `h` couplage | coût réel | tension |
|---|---|---|---|---|---|
| 0  | 3  | 3   | **4**   | 4   | 75 % → **100 %** |
| 1  | 6  | 88  | **95**  | 97  | 91 % → **98 %** |
| 17 | 6  | 114 | **121** | 213 | 54 % → **57 %** |
| 2  | 10 | 100 | **119** | ?   | +19 % |

**Le couplage rend `h` exacte sur le niveau 0 et quasi parfaite sur le 1 — et ne fait rien sur le 17 (54 → 57 %), qui est justement celui qui résiste.**

Raison, et elle est décisive : le couplage ne corrige que les **collisions de buts** (N caisses qui prétendent viser le même but). Sur le 17, il ne récupère que 7 poussées sur les 99 manquantes. Les 92 autres sont du **coût de manœuvre** — pousser une caisse *loin* de son but pour dégager un passage, faire le tour, repositionner. C'est de l'interaction caisse↔caisse et caisse↔joueur, et **aucune heuristique fondée sur des distances caisse↔but ne peut la voir** : elle relaxe précisément cette interaction, c'est ce qui la rend admissible.

Coût : ~7× plus cher par état (O(n³) au lieu de O(cases)), pour un seuil de rentabilité à ÷3 du nombre d'états. **Écarté** : le quatrième pansement, et la mesure dit qu'il ne colle pas là où ça saigne.

- [x] ~~Deadlock adjacent diagonal~~ → traité par le test de gel de §3bis (gain réel : 2 à 6 %).
- [x] ~~Trier les caisses par distance dans la génération des enfants~~ → sans objet depuis A*.

**État des lieux, sans complaisance.** Les niveaux 0, 1 et 17 sont résolus, avec un choix explicite entre optimal et rapide. Le **niveau 2 reste hors de portée**. Trois leviers ont été essayés contre lui :
- heuristique admissible (A*) : **~0 %** — raison structurelle, cf. §4.3 ;
- détection de deadlocks (gel) : **2 à 6 %**, et le gain *décroît* avec la taille du niveau ;
- coût unitaire (`pousse()`, move ctor, release) : **×8,7 en temps**, mais **0 %** sur le nombre d'états.

Seule la **pondération** a produit un ordre de grandeur (×34 sur le niveau 1), en renonçant à l'optimalité. Ce n'est pas un hasard : c'est le seul levier qui accepte de *ne pas explorer* des états qu'on ne peut pas prouver mauvais. Tout le reste ne peut couper que ce qui est démontrablement sans issue — et ce gisement est épuisé.

---

## 6. Heuristique joueur-aware — FAIT ✅ (le levier qui a enfin payé)

### 6.A Résultats mesurés

**Niveau 17** (celui qui résistait à tout) :

| solveur | états | poussées |
|---|---|---|
| BFS (référence) | 30 093 130 | 213 |
| A* optimal, ancienne `h` | 29 135 928 | 213 |
| A* pondéré, ancienne `h` | 13 405 481 | 217 |
| **A* optimal, nouvelle `h`** | **14 826 798** | **213** ✅ |
| A* pondéré, nouvelle `h` | 4 264 544 | 225 |
| **A* pondéré, nouvelle `h` + fermeture** | **1 636 218** | 227 |

**×18 contre le BFS**, file ouverte tombée à ~24 000 éléments (le mur mémoire s'effondre). Niveau 0 : 111 → **8** états (×14). Niveau 1 : inchangé (`h` n'y bouge pas, sa géométrie ne piège jamais le joueur) — mais l'optimalité tient partout (4 / 97 / 213).

### 6.B Les quatre pièges rencontrés (tous silencieux)

- [x] **`h += -1`.** `distancePoussee[b][r]` vaut -1 quand la caisse ne peut plus atteindre aucun but *avec le joueur de ce côté*. `casesMortes` ne le voit pas (elle n'est vraie que si la caisse est perdue pour TOUTES les régions). Mesuré : 141 couples (case, région) concernés sur le niveau 1, jusqu'à 270 sur le 14. L'heuristique se mettait à **soustraire**. → Correctif = ajouter ce **deadlock dynamique** dans `checkDefaite()`. Ce n'est pas un bonus optionnel : sans lui, `h` est corrompue.
- [x] **La bordure n'est PAS toujours en murs.** `Level::load()` complète les lignes courtes par des espaces (`tcNone`), et certains `.xsb` commencent leurs lignes par des espaces. Le commentaire « pas de test de bornes, la bordure est en murs » n'est vrai que parce que le joueur ne peut jamais atteindre ces cases de remplissage. Or `calculDistancePoussee()` balaie **toutes** les cases non-mur → index négatif → crash. **Tests de bornes indispensables dans ce précalcul, et là seulement.**
- [x] **`QVector::operator[]` non-const appelle `detach()`.** `checkDefaite()` n'est pas const : chaque lecture de `regions[...]` faisait une **copie profonde de 97 Ko** (le vecteur est partagé par COW entre tous les clones du solveur), à chaque poussée. Coût : ×1,85 sur le temps total. → **`.at()`**, qui est const et ne détache jamais. `getHeuristique()`, `caisseGelee()` et `bloqueeSurAxe()` sont `const`, donc épargnés.
- [x] **`h` n'est plus COHÉRENTE.** Une poussée déplace le joueur, et la contribution de *toutes* les autres caisses dépend de sa position (elle se lit dans leur région) : `h` peut sauter de plusieurs unités quand le coût n'augmente que de 1. Elle reste **admissible** (l'optimalité tient : 4 / 97 / 213), mais la garantie « premier dépilement = `g` optimal » tombe → re-développements massifs (4 264 544 dépilements pour 1 659 245 distincts sur le 17). Symptômes : la file gonfle, `f` fluctue, `vus` stagne.
  → **Fermeture (ensemble des états déjà développés) en mode PONDÉRÉ uniquement** : solution bornée par `w × C*`, ×2,6 récupéré. **Surtout pas en mode optimal** : là, le re-développement n'est pas du gaspillage, c'est LUI qui rétablit l'optimalité face à une `h` incohérente.

### 6.C Disposition mémoire
`regions[CASE * size + CAISSE]` — la **case en index majeur**, pas la caisse. Le chemin chaud interroge toujours avec le joueur fixe et la caisse variable ; cet ordre rend les lectures contiguës. (Mesuré : sans effet en pratique une fois le `detach()` corrigé — le vrai coupable était ailleurs. Gardé quand même, la disposition est plus saine.)

---

### Spécification d'origine

### 6.0 Correction de §4.3 : j'avais tort

§4.3 affirmait que « la tension d'une heuristique ne fait pas sa capacité à discriminer », et en concluait qu'aucune heuristique admissible ne sauverait le projet. **C'était trop pessimiste et théoriquement bancal.**

Un état est élagué si `g + h > C*`. Pour un état qui n'est PAS sur un chemin optimal, le vrai coût qui le traverse dépasse `C*`. Donc **si `h` était parfaite, A\* n'explorerait QUE les chemins optimaux.** Ce qui empêchait l'élagage n'était pas une propriété structurelle du Sokoban — c'était simplement que `h` était trop lâche : avec 46 % de mou sur le niveau 17, n'importe quel état pouvait se faire passer pour prometteur.

### 6.1 Le point aveugle réel

`distanceButs` n'est **pas** une distance à vol d'oiseau (contresens fréquent) : `calculCaseMorte()` fait déjà un BFS sur la grille, en vérifiant que la case d'arrivée *et* la case derrière ne sont pas des murs. C'est le vrai nombre de poussées d'une caisse seule, murs et détours compris.

**Mais elle ignore que le joueur doit pouvoir ALLER derrière la caisse.** Elle vérifie que la case d'appui n'est pas un mur — pas qu'elle est *atteignable*.

Or une caisse dans un couloir **coupe le niveau en deux** : le joueur ne peut plus passer de l'autre côté, donc la caisse n'est plus poussable que dans un seul sens. `distanceButs` croit qu'elle peut revenir en arrière. Elle sous-estime massivement.

### 6.2 L'idée : distance d'une caisse SEULE, avec accessibilité du joueur

L'état d'une caisse seule = **(case de la caisse, RÉGION où se trouve le joueur)**. Selon le côté où le joueur se trouve, la caisse n'est pas poussable dans les mêmes directions. Il n'y a que quelques centaines de tels couples par niveau → **table précalculée une fois**, lecture O(1) ensuite.

**Admissible** : retirer les autres caisses ne fait que *libérer* le joueur (les caisses ne sont que des obstacles), donc la distance calculée seule est toujours ≤ au coût réel. Et la somme sur les caisses reste une borne inférieure : chaque poussée ne déplace qu'une caisse.

**Mesuré — et c'est le meilleur résultat du projet :**

| niveau | `h` actuelle | **`h` joueur** | `h` joueur + couplage | coût réel | tension |
|---|---|---|---|---|---|
| 0  | 3   | 3       | 4       | 4   | 75 % → 75 % → **100 %** |
| 1  | 88  | 88      | 95      | 97  | 91 % → 91 % → **98 %** |
| 17 | 114 | **194** | 201     | 213 | **54 % → 91 % → 94 %** |
| 2  | 100 | 110     | 129     | ?   | +10 % → **+29 %** |
| 6  | 87  | 87      | 104     | ?   | +0 % → +20 % |
| 7  | 61  | 61      | 80      | ?   | +0 % → +31 % |

Le **niveau 17** — celui qui a résisté à A*, au gel et au couplage — passe de 54 % à **91 %** : la relaxation récupère **80 des 99 poussées manquantes**. Les niveaux 1, 6, 7 ne bougent pas : leur géométrie ne piège pas le joueur.

Les deux relaxations sont **orthogonales** (manœuvres du joueur vs collisions de buts) et se composent → 94-100 % de tension partout.

### 6.3 Étape 9 — Implémentation (version joueur-aware seule, sans couplage)
Cette version est **plus informative que l'actuelle ET moins chère** : `getHeuristique()` devient une somme de lectures de table.

**Précalcul, une fois à la construction (dans/à côté de `calculCaseMorte()`) :**
- [ ] `comp[caisse][case]` = id de la composante connexe des cases libres, **la caisse comptant comme un obstacle** (`-1` pour un mur ou la caisse elle-même). Un flood-fill par case candidate → O(size²), soit ~100 k opérations. Négligeable, une seule fois.
- [ ] `distJoueur[caisse][region]` = poussées minimales vers un but quelconque. **BFS à rebours** depuis tous les couples `(but, region)`, distance 0.
- [ ] Transition arrière : le prédécesseur de `(c, rc)` est `(b, r)` avec `b = c - d` (la caisse venait de `b`, poussée vers `c`) et `p = c - 2d` (le joueur s'y tenait). Conditions :
  - `b` et `p` ne sont pas des murs ;
  - après la poussée le joueur est en `b`, qui doit être dans la région `rc` → `comp[c][b] == rc` ;
  - `r = comp[b][p]`.

**À l'exécution :**
- [ ] `getHeuristique()` = `Σ` sur les caisses de `distJoueur[caisse][ comp[caisse][playerPoint] ]`.

**⚠️ Pièges**
- [ ] **`h` doit rester une fonction de l'ÉTAT NORMALISÉ** (§1), sinon deux positions du joueur dans la même zone donneraient des `h` différentes et A* deviendrait incohérent. C'est vrai, mais il faut le savoir : la zone réelle du joueur (toutes caisses en obstacles) est connexe, donc **entièrement contenue dans UNE seule composante** de `plateau-moins-cette-caisse` (qui a moins d'obstacles). `comp[caisse][playerCell]` est donc invariant sur toute la zone. ✅
- [ ] **Mémoire** : `comp` fait `size × size` entiers — 410 Ko pour un niveau 20×16. À **aplatir en un seul `QVector<qint16>`** de `size*size` plutôt qu'un `QVector<QVector<int>>` (allocations imbriquées). Copié par COW entre tous les clones de `Game`, comme `casesMortes` — **surtout ne pas le recalculer par état**.
- [ ] **Bonus deadlock** : `distJoueur[caisse][region] == -1` signifie que cette caisse ne peut plus JAMAIS atteindre un but avec le joueur de ce côté. C'est un deadlock **dynamique**, plus fort que `casesMortes` (qui est statique). Sain pour la même raison que l'admissibilité : retirer les autres caisses ne fait qu'aider. À tester dans `checkDefaite()` — mais **pas** à fusionner dans `casesMortes`, qui ne dépend pas du joueur.

### 6.4 Vérification
- [ ] **Poussées inchangées : 4 / 97 / 213.** Le canari. `h` reste admissible (vérifié : 3 ≤ 4, 88 ≤ 97, 194 ≤ 213), donc l'optimalité doit être préservée. Un écart = bug (table fausse, ou `h` qui surestime).
- [ ] **Le nombre d'états doit chuter**, surtout sur le 17. C'est le seul verdict qui vaille.
- [ ] ⚠️ **La tension au départ est un proxy IMPARFAIT.** Le niveau 1 avait déjà 91 % de tension… et A* n'y élaguait que 20 %. La tension initiale ne dit rien de la tension *en profondeur dans l'arbre*. **Ne rien conclure avant d'avoir compté les états.**

### 6.5 Ensuite, si besoin
- [ ] Ajouter le **couplage hongrois** par-dessus (distance joueur-aware *vers chaque but* → matrice `n×n` → affectation de coût minimal). Orthogonal, se cumule : 91 % → 94 % sur le 17, +29 % sur le 2. Coût : O(n³) par état au lieu de O(n).

---

## 7. Couplage hongrois par-dessus la table joueur-aware — IMPLÉMENTÉ

### 7.-1 Résultats mesurés (couplage vs `h` joueur-aware seule)

Implémentation en place : `distanceParBut[(but·size + case)·maxRegions + region]` (un BFS
à rebours par but, `distancePoussee` en devient le min), et `getHeuristique()` construit la
matrice `n×n` puis résout l'affectation de coût minimal (hongrois par potentiels, O(n³),
`QVarLengthArray` sans allocation tas tant que n < 32).

| niveau | mode | états AVANT | états APRÈS | poussées |
|---|---|---|---|---|
| 1  | A* **optimal**           | 783 474    | **13 208**    | **97 ✅** (canari) |
| 1  | A* pondéré (+fermeture)  | 23 072     | **12 184**    | 103 (inchangé) |
| 17 | A* **optimal**           | 14 826 798 | **1 088 789** | **213 ✅** (canari) |
| 17 | A* pondéré (+fermeture)  | 1 636 218  | **1 634 102** | 227 → 225 |
| **2** | A* **optimal**        | ∞ (jamais résolu) | **591 138** | **131 🏆** |

### 🏆 LE NIVEAU 2 EST RÉSOLU
Le niveau 2 (10 caisses), « hors d'atteinte de cette approche » (§4.4, tué à 20,7 Go / 119 M
d'états sans converger), tombe en **591 138 états, 131 poussées, en A* OPTIMAL**. Détail qui
résume tout : **591 k < 1,09 M** — le niveau 2 est devenu **plus facile que le 17**.

Pourquoi (validation totale du §7.1) : l'erreur du 17 est du **coût de manœuvre** (dur à capturer,
`h` plafonne à 94 %) ; celle du 2 est de la **collision de buts** — dix caisses se disputant les
mêmes buts —, **exactement** ce que le couplage corrige. Tension `h(départ)` = **129 / 131 =
98,5 %** : heuristique quasi exacte → espace effondré. Le §6.0 poussé à son terme.

Optimalité : aucune référence externe n'existait pour le 2, mais `h` est **admissible par
construction** et confirmée sur le 1 (97) et le 17 (213) → **131 est l'optimum**.

**Le gros gain du couplage est en mode OPTIMAL, et il est ÉNORME** — c'est la
**confirmation de la thèse §6.0** : la relation tension→élagage est **très non-linéaire près de
100 %**. À 91 % de tension A* optimal ne coupait que −20 % (§4.3) ; resserrer de quelques points
au sommet fait s'effondrer le nombre d'états (presque plus aucun n'a `f ≤ C*` à tort).
- **niveau 1, optimal** : **×59** (783 474 → 13 208), tension 91 % → 98 %, optimalité préservée.
- **niveau 17, optimal** : **×13,6** (14,8 M → 1,09 M), tension 91 % → **94 %** seulement, et 213.

**⚠️ Correction d'une conclusion trop rapide.** En voyant d'abord le résultat *pondéré* du 17
(~0 % de gain), j'avais écrit « le couplage ne sert à rien sur le 17 ». **Faux — vrai seulement
en pondéré.** Le pondéré (w=2 + fermeture) était déjà écrasé à ~1,6 M par le *plongeon* ; le
petit gain de tension n'y était pas visible. En **optimal**, ce même +3 points de tension vaut
×13,6. Leçon : **le bénéfice d'une `h` plus tendue se lit en mode OPTIMAL**, le pondéré le masque.

**Conséquence : sur le 17, le mode pondéré est devenu inutile** — l'optimal avec couplage
(1,09 M / 213) explore **moins** d'états que le pondéré avec couplage (1,63 M / 225), *et* il est
optimal.

**⚠️ Non encore vérifié :**
- [~] **Canari optimal** : doit rendre **4 / 97 / 213**. **Niveau 1 = 97 ✅, niveau 17 = 213 ✅**
  (admissible confirmé sur les deux gros). Reste le **0 (=4)**, trivial.
- [ ] **Niveau 2** : le vrai juge (10 caisses → collisions dominantes). Pas encore lancé.
- [ ] **Bonus deadlock du couplage** (§7.2, affectation à coût infini) : **pas implémenté**. Les
  paires inatteignables valent `INF_COUPLAGE = 1 000 000` (grand mais fini) → un état sans
  affectation finie reçoit un `h` énorme et n'est jamais développé (correct mais il encombre la
  file au lieu d'être élagué dans `checkDefaite()`).

**Niveaux de test 0100-0109** : dérivés de `level0002` (grille 10 caisses/10 buts), chacun ne
gardant **qu'une caisse + un but** (caisse *k* appariée au but *k* dans l'ordre de balayage,
joueur conservé). Isolent chaque distance caisse↔but individuelle pour valider la table avant
de faire confiance à l'affectation globale. Certains couples sont volontairement insolubles
(caisse coincée / but derrière un mur) → testent au passage la détection de deadlock.

### 7.0 Où en est le niveau 2 (état des lieux après §6) — PÉRIMÉ, résolu depuis (§7.-1)
> Ce qui suit décrit la meilleure tentative *avant* le couplage (pondéré, ne terminait pas). Depuis
> §7.-1, le niveau 2 est **résolu en 591 138 états en optimal**. Conservé pour le raisonnement.


Meilleure tentative à ce jour, A* pondéré w=2 + `h` joueur-aware + fermeture :
```
81 700 000 états découverts | 63 800 000 développés | file ouverte 20 600 000
f = 203 (il REMONTE : 185 il y a une heure) | phys_footprint 14 Go / 18 (pic 15)
→ arrêté avant saturation, aucune solution
```
**Mais la mécanique est enfin saine** : `vus` (81,7 M) > développés (63,8 M), et l'écart correspond exactement à la file. Plus aucun re-développement. On ne meurt plus d'un défaut du solveur — on meurt de la combinatoire.

Et le progrès est réel : la tentative précédente était à 6,3 Go dès **3 minutes** pour 8,5 M d'états distincts, morte à 20,7 Go. Ici, 14 Go pour **81,7 M** — soit **~10× plus d'états pour la même mémoire**.

Repère qui cadre l'échelle : le niveau 17 se résout en **1,6 M** d'états. Le 2 en a exploré 50× plus sans aboutir.

### 7.1 Pourquoi celui-ci maintenant

`h` joueur-aware fait viser à chaque caisse son but **le plus proche**. Sur un niveau à **10 caisses**, dix caisses peuvent réclamer le même but — alors qu'un but n'en accueille qu'une. La sous-estimation est grossière, et elle croît avec le nombre de caisses. C'est *l'erreur dominante du niveau 2*.

Les deux relaxations sont **orthogonales** : la table joueur-aware corrige les **manœuvres du joueur**, le couplage corrige les **collisions de buts**. Mesuré (tension `h(départ)` / `C*`) :

| niveau | `h` actuelle | `h` joueur | **joueur + couplage** | `C*` |
|---|---|---|---|---|
| 0  | 3   | 3   | **4**   | 4   (100 %) |
| 1  | 88  | 88  | **95**  | 97  (98 %) |
| 17 | 114 | 194 | **201** | 213 (94 %) |
| 2  | 100 | 110 | **129** | ?   (+17 % sur la joueur-aware) |

**C'est le seul levier restant qui réduise le NOMBRE d'états** (les clés compactes, §7.4, ne font que mieux les stocker).

### 7.2 Étape 10 — Implémentation
- [x] **Une table de distances PAR BUT** : `distanceParBut[but][case][region]`. Un BFS à rebours par but, au lieu d'un seul depuis tous les buts. Même transition qu'en §6 (double vérification de régions). Aplati en un `QVector<int>` d'index `(but·size + case)·maxRegions + region`, ajouté aux ctors copie/déplacement (partage COW comme `distancePoussee`).
  - Mémoire : `nbButs × size × maxRegions` entiers. Niveau 2 : 10 × 140 × 4 × 4 o ≈ **22 Ko**. Un 20×16 à 20 buts : ~205 Ko. Négligeable, partagé par COW.
- [x] **`distancePoussee` en devient un sous-produit** : `distancePoussee[b][r] = min sur les buts de distanceParBut[j][b][r]`, calculé dans la même boucle. Une seule source de vérité → `casesMortes`/`checkDefaite` inchangés (valeurs identiques à l'ancien BFS multi-but).
- [x] **`getHeuristique()`** : construit la matrice `n×n` (`n` = caisses), `cout[i][j] = distanceParBut[j][ caisse_i ][ regions[joueur][caisse_i] ]`, puis **affectation de coût minimal** (hongrois par potentiels, O(n³)).
  - ✅ Vérifié : sur les 43 niveaux, **nb caisses == nb buts**. La matrice est carrée, pas de cas rectangulaire. Garde-fou en place : si `n != nbButs`, repli sur l'ancienne borne « chaque caisse vise son but le plus proche ».

**⚠️ Pièges**
- [x] **Paires inatteignables** (`distanceParBut == -1`) : coût `INF_COUPLAGE = 1 000 000`, **grand mais FINI** (n ≤ ~30, la somme ne déborde jamais un `int`) — jamais `INT_MAX` qui déborderait dans l'addition du hongrois.
- [ ] **Bonus deadlock, plus fort que celui de §6** : si le couplage optimal contient au moins une paire à coût infini, **aucune affectation bijective n'est possible** → deadlock. Exemple que rien ne détecte aujourd'hui : deux caisses qui ne peuvent atteindre qu'un seul et même but. À tester dans `checkDefaite()`. **PAS ENCORE FAIT** : pour l'instant l'état reçoit juste un `h` énorme (jamais développé, mais pas élagué).
- [x] **`h` reste incohérente** (elle l'était déjà depuis §6) → fermeture en pondéré et re-développement en optimal **conservés à l'identique**. Rien touché dans `solveurastar.cpp`.
- [ ] **Admissible** : toute solution réelle réalise une bijection caisses↔buts ; son coût est ≥ la somme de cette affectation ; on prend le **minimum sur toutes** les affectations. Donc `h_couplage ≤ C*`. Et comme la version « but le plus proche » relâche la contrainte de distinction, `h_joueur ≤ h_couplage` : le couplage **domine**, il ne peut pas être pire.

### 7.3 Vérification
- [ ] **Poussées inchangées en optimal : 4 / 97 / 213.** Le canari. **PAS ENCORE RELANCÉ** depuis le couplage. Un écart = `h` qui surestime (bug du hongrois ou de la matrice).
- [~] **Le pari est un échange temps ↔ états** : mesuré en **pondéré** — niveau 1 ×1,9 d'états (23 072 → 12 184), niveau 17 ~0 % (1,636 M → 1,634 M). Le coût O(n³) par état n'a pas fait exploser le temps (à confirmer au chrono). Reste à mesurer en **optimal**.
- [ ] Puis le niveau 2, en surveillant `footprint -p PID` (**jamais `ps rss`**, cf. §6.B).

### 7.4 Si la mémoire redevient le mur
- [ ] **Clés compactes.** Les 11 Go de `MALLOC_SMALL` du niveau 2 sont 81,7 M de `QByteArray` : 22 octets utiles, mais un en-tête `QArrayData` de 24 o plus un bloc `malloc` arrondi, plus le nœud de hachage. Un **hachage 128 bits** (valeur plate, zéro allocation) ferait tomber ces 11 Go à ~3 → **~3× de piste**. Risque de collision négligeable en 128 bits (en 64 bits : ~1,8·10⁻⁴ pour 82 M de clés — une collision élaguerait silencieusement une branche).
- [ ] ~~Fusionner `ferme` dans `meilleurG`~~ → **ne rapporte que ~2 Go** : les `QByteArray` sont déjà partagés par COW entre les deux tables, seuls les *nœuds* sont dupliqués. Bien moins rentable que les clés compactes.

---

## 8. Découpage « une caisse à la fois » (goal ordering) — ANALYSÉ, EN RÉSERVE

Idée : découper le gros problème en petits (maxime classique « diviser pour régner »).
1. Trouver l'**ordre** de rangement des caisses (se concentrer sur une caisse à la fois).
2. Une fois l'ordre fixé, positionner le joueur pour ranger cette caisse **sans créer de
   deadlock** avec les autres.
3. Recommencer pour chaque caisse restante.

### 8.1 La clause oubliée de « diviser pour régner »
Diviser-pour-régner et la prog. dynamique marchent quand les sous-problèmes sont
**indépendants** et **recombinables** proprement (tri fusion…). **Sokoban est PSPACE-complet
précisément parce que les coups interagissent globalement** : pas de coupe nette. Le découpage
suppose deux indépendances, toutes deux fausses ici.

### 8.2 Couture n°1 — temporelle : « poser puis figer » est INCOMPLET
Ranger chaque caisse définitivement interdit deux manœuvres parfois **obligatoires** :
- **Parquer une caisse sur une case non-but**, temporairement, pour que le joueur passe de
  l'autre côté (il ne traverse pas une caisse).
- **Ressortir une caisse d'un but** : quand un but est sur l'unique passage vers un autre but,
  il faut l'y poser, l'en retirer pour laisser passer la suivante, puis la remettre.

Un algo qui fige toute caisse rangée **ne peut pas exprimer** ces solutions → « aucune solution »
sur des niveaux solubles. Limite d'expressivité, pas bug réparable. **⇒ incomplet.**

### 8.3 Couture n°2 — spatiale : « trouver l'ordre » EST le problème dur
- Une caisse posée devient un **obstacle** — pour le joueur ET pour les caisses suivantes. Donc
  l'étape 2 (« sans deadlock ») n'est pas un test local : un placement peut condamner une caisse
  pas encore traitée.
- Pour savoir qu'un ordre est **valide**, il faut vérifier que le sous-problème restant est
  soluble = **le problème d'origine**. « Trouver l'ordre » cache toute la difficulté.
- `n!` ordres (niveau 2 : 10! = 3,6 M), *fois* la recherche de placement pour chacun.

### 8.4 Ce qui est récupérable
- **Les niveaux de test 011x SONT les sous-problèmes de ce découpage.** Traiter les autres
  caisses comme des murs était faux *en heuristique A\* admissible* (§7 / cf. ci-dessous), mais
  **à l'intérieur d'un sous-solve** — où l'on a décidé de ne pas toucher les autres caisses —
  elles ne bougent pas : les murs sont alors un modèle **correct**. Les 010x/011x sont les tests
  unitaires du « une caisse à la fois » (010x = autres caisses retirées, 011x = autres caisses
  figées en murs).
- **Le goal ordering marche comme GUIDE, pas comme solveur complet** — technique réelle des
  solveurs de compétition. Sortie propre = un **nouveau solveur explicitement approché** (comme
  le « rapide, approché » du §4.5, ou le beam §5) :
  - choisir un ordre gloutonnement (buts en cul-de-sac d'abord, ou distance joueur-aware
    croissante) ;
  - résoudre caisse par caisse avec le petit A\* existant (un sous-solve à 1 caisse est minuscule) ;
  - **backtrack borné sur l'ordre** quand un sous-problème échoue.
  Ni complet ni optimal — mais peut-être ce qui fait enfin **avancer le niveau 2** là où l'A\*
  global se noie. Même renoncement que §4.5/§5, rendu explicite dans le libellé.

### 8.5 Rappel — pourquoi « caisses manquantes = murs » est FAUX comme heuristique
(Piège rencontré en montant les niveaux 011x, à ne pas refaire.)
- **010x (autres caisses = sol)** = ce que fait `distanceParBut`. On *retire* des obstacles → la
  distance ne peut que raccourcir → borne **inférieure** → **admissible**. Optimiste mais sûr.
- **011x (autres caisses = murs)** = on *ajoute* des obstacles → la distance ne peut que
  s'allonger (voire ∞). Or **un mur est permanent, une caisse non** : dans la vraie solution la
  caisse gênante se pousse hors du chemin. `h_murs` peut donc **surestimer `C*`** → A\* élague un
  état du chemin optimal (`g+h > C*`) → **solution non optimale ou "aucune solution"**. Le canari
  4/97/213 tombe. Même famille de faute que le faux positif de deadlock (§3bis) et la `h` qui
  soustrait (§6.B), en pire : ici on **invente une borne fausse**.
- La distinction qui réconcilie tout : *les murs comme obstacles sont valides DANS un sous-solve
  figé (§8.4), invalides comme borne inférieure GLOBALE (ici)*.

---

## 5. Recherche en faisceau (beam search) — EN RÉSERVE
*(Déclassé : §6 attaque la cause — une `h` trop lâche — là où le faisceau renonce à l'optimalité. À reprendre seulement si §6 ne suffit pas.)*

### 5.1 L'erreur de cadrage qu'on vient de comprendre

Depuis le début, on demande à `h` d'être une **borne**. C'est ce qu'exige A* : pour garantir l'optimalité, `h` doit sous-estimer, et A* doit développer *tout* état de `f ≤ C*`. D'où l'impasse de §4.3 — une poussée utile fait `g+1, h−1`, `f` ne bouge pas, rien n'est élaguable.

Mais on peut aussi lui demander d'être un **classeur** : non pas « combien de poussées reste-t-il ? », mais « entre ces deux états, lequel a l'air plus prometteur ? ».

**Ce sont deux exigences très différentes, et on n'a jamais testé la seconde.** Notre `h` est une mauvaise *borne* sur le niveau 17 (54 % de tension, mesuré). Rien ne dit qu'elle est un mauvais *classeur* : une heuristique qui sous-estime systématiquement de 46 % peut parfaitement ordonner correctement les états entre eux.

### 5.2 Le principe

Exploration par **paliers de profondeur** (comme un BFS), mais à chaque palier on ne garde que les **W meilleurs** états selon `h`. Le reste est jeté définitivement.

- **La mémoire devient bornée *a priori*** : elle dépend de `W`, pas du nombre d'états du problème. Le mur des 20,7 Go ne recule pas — il n'existe plus.
- **`h` n'a plus besoin de discriminer globalement**, seulement de séparer les frères d'un même palier.
- **Le prix, franc et total** : ni optimalité, ni **complétude**. Si la solution passe par un état jeté, on ne la trouvera jamais — on peut rendre « aucune solution » sur un niveau soluble. Mais c'est un renoncement *paramétrable* : plus `W` est grand, plus on est prudent.

**Simplification élégante** : toutes les arêtes coûtent 1 poussée, donc **tous les états d'un même palier ont le même `g`**. Trier par `h` ≡ trier par `f`. Pas de `f` à calculer.

### 5.3 Étape 8 — `SolveurFaisceau`
- [ ] `solveurfaisceau.h/.cpp`, `class SolveurFaisceau : public Solveur`, **`Q_OBJECT`** (oublié sur `SolveurAStar`, le `moc` le signale par « No relevant classes found »). Paramètre `W` au constructeur, comme `poids` pour `SolveurAStar`.
- [ ] Boucle, par palier :
  1. `QVector<SCandidat>` des enfants de **tous** les états du palier courant. `SCandidat = {int h; int parent; int idxCaisse; EDirection dir; QByteArray cle;}` — **pas de `Game`** (cf. étape 7) et **pas encore de `Noeud`**.
  2. Élaguer : `isPerdu()` (gel + `casesMortes`), puis l'ensemble des états **déjà vus** (global).
  3. **Dédupliquer par clé À L'INTÉRIEUR du palier** — sinon le faisceau se remplit de W copies du même état et l'exploration s'effondre. Piège n°1.
  4. `std::partial_sort` sur `h` croissant, garder les **W premiers** (`partial_sort`, pas `sort` : on n'a besoin que des W meilleurs sur potentiellement des centaines de milliers de candidats).
  5. **Seulement pour les survivants** : `noeuds.append(Noeud{parent, idxCaisse, dir})`. Sinon `noeuds` grossirait avec tous les candidats jetés. Piège n°2.
  6. Victoire testée à la génération (`e.isGagne()`), pas au dépilement : il n'y a plus de dépilement.
- [ ] **Départage à `h` égal** : beaucoup d'états partageront la même `h`. Sans critère secondaire, le faisceau se remplit d'états quasi identiques et perd sa diversité. Piste : nombre de caisses déjà sur des buts, décroissant.
- [ ] **Relance automatique avec `W` doublé** si le faisceau meurt sans solution (« anytime ») : c'est ce qui rend l'incomplétude acceptable en pratique.
- [ ] Ensemble des vus : borné par `W × profondeur` (W=100 k, profondeur 300 → 30 M clés). Reste le poste mémoire dominant → candidat naturel pour le hachage 64 bits (§5.5).

### 5.4 Vérification
- [ ] Doit **trouver** une solution sur 0 / 1 / 17. Comparer aux optima connus (**4 / 97 / 213**) pour chiffrer la perte de qualité — c'est le prix du renoncement, il doit être mesuré, pas supposé.
- [ ] Mémoire **plate** en fonction du niveau, à `W` fixé : c'est LA propriété à vérifier. Si elle croît avec la taille du problème, l'implémentation a un défaut (ensemble des vus non borné, ou `noeuds` alimenté par les candidats jetés).
- [ ] Puis le **niveau 2**, avec W croissant : 10 k, 50 k, 100 k, 500 k.
- [ ] `footprint -p PID`, **jamais `ps rss`** (cf. étape 7 : le compresseur macOS fausse tout).

### 5.5 Gains annexes, gratuits
- [ ] **Pas de re-développement en A\* pondéré** : mesuré 119 M dépilés pour 74 M distincts, soit **1,6× de travail gaspillé**. Un simple ensemble « déjà développé » (skip au dépilement si l'état a déjà été étendu) l'élimine, au prix d'une sous-optimalité *bornée* — qu'on accepte déjà puisque le solveur est explicitement approché.
- [ ] **`QHash<quint64,int>`** au lieu de `QHash<QByteArray,int>` : ~16-24 o/entrée au lieu de ~80-110. C'est le poste mémoire dominant (74 M clés ≈ 6-8 Go des 20,7 Go du niveau 2). Prix : une collision ferait silencieusement manquer une branche — probabilité ~1,5·10⁻⁴ pour 74 M clés sur 64 bits (paradoxe des anniversaires). Passer à 128 bits la rend négligeable.
