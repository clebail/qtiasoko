# Plan solveur Sokoban

> **Ce document a été condensé le 2026-07-17.** On y garde : les gains **mesurés** et la
> technique qui les a produits, les pistes **restantes**, et les pièges à ne pas refaire.
> Le récit des impasses et des allers-retours a été coupé (l'historique est dans git).

---

## 0. État réel — la carte des 33

**Les chiffres (états/poussées/commit) sont dans [scores.md](scores.md) — seul ce fichier fait
foi.** Ne plus reporter de tableau états/poussées ici : un nombre copié dans ce document vieillit
en silence pendant que le code bouge (c'est exactement ce qui a fait passer inaperçue la
régression du niveau 9, cf. §6.3 — corrigé par la règle du §1).

- ⚠️ **UN NIVEAU EST « RÉSOLU » SI ET SEULEMENT S'IL A UNE LIGNE DANS [scores.md](scores.md)**
  (solve mené au bout, états/poussées/commit relevés). À ce jour : **11 résolus sur les 33**
  (0-9 et 17). **Les 22 autres — 10, 11, 12, 13 à 16, 18 à 32 — ne sont PAS résolus**, y compris
  ceux dont ce document parle beaucoup (10, 18, 24, 25, 26 au §6.2 ; 13-16 dans les tableaux de
  diagnostic du §6.3). Apparaître dans un tableau de mesure ne veut PAS dire résolu : `mort`,
  `macro` et la jauge `rangees` tournent justement sur des niveaux qu'on ne sait pas finir.
  Hors carte : 190 et 191 sont des **bancs d'essai** (endgame du 11 isolé), résolus mais ils ne
  comptent pas dans les 33.
- ⚠️ **Ne jamais écrire « il ne reste que X et Y » sans dire de QUEL sous-ensemble.** Les
  « cibles » de ce document sont les quelques niveaux travaillés activement (8, 11, 12), pas
  l'ensemble des non-résolus. Le raccourci « il ne reste que 11 et 12 » a été écrit deux fois
  (§6.3) et se lit comme « 31/33 faits », ce qui est faux d'un facteur trois.
- **Le CANARI** — les poussées optimales des niveaux résolus les plus simples, qui ne doivent
  JAMAIS bouger d'une modif à l'autre (valeurs à jour : [scores.md](scores.md)). C'est le juge de
  toute modif : une `h` qui surestime ou un deadlock faux positif ne dégrade pas la solution, il
  fait **manquer l'optimum sans aucun signal**.
- **Le mur mémoire n'existe plus** : pic 599 Mo sur tout le tour (contre 20,7 Go qui tuaient
  le 2 avant la macro). **Ce qui reste est un mur de TEMPS.** Tous les chantiers mémoire
  (hachage 128 bits, blocs pour `noeuds`/file) sont **sans objet**.
- Deux modes d'échec (jauge `rangees`) : **Groupe A** ne démarre pas (la macro ne s'engage
  jamais) ; **Groupe B** plafonne à mi-chemin (l'ordre de remplissage se mure tout seul).

---

## 1. Les outils de mesure — `mesures/`

Harnais en ligne de commande qui compilent le solveur tel quel et l'interrogent de
l'extérieur. Rien n'entre dans `qtiasoko.pro`. Détail dans [mesures/mesure.md](mesures/mesure.md).

| outil | question |
|---|---|
| `bench <niv> [poids]` | états / poussées / mémoire ; avec `INSTRUM_F`, histogramme des `f` au dépilement |
| `mou <niv> [n]` | les états dépilés sont-ils du gaspillage ? (sur chemin / hors chemin / **deadlock**) |
| `mort <niv> …` | **(neuf, 2026-07-17)** taux de deadlocks non détectés sur un niveau qu'on NE sait PAS résoudre |
| **rejeu pas à pas** (dans l'app) | **(neuf, 2026-07-24)** ◀ ▶ + slider + libellé `coup n/N — poussée p/P`, **Maj = saut de poussée à poussée**. Rejoue la solution, mais surtout le chemin du **MEILLEUR ÉTAT d'un run qui n'aboutit pas** (`nouveauMaxCaisses` porte désormais le chemin, pas seulement l'état). C'est lui qui a fait voir les deadlocks non détectés du niveau 4 → §6.1 |
| `fp <niv> [variante]` | **(neuf, 2026-07-21) LE JUGE D'UN ÉLAGAGE** : rejoue une solution GAGNANTE et interroge le test sur chacun de ses états — tous solubles par construction, donc **toute détection est un faux positif prouvé**. À passer AVANT de câbler quoi que ce soit dans `checkDefaite` |
| `macro <niv> [s]` | **(neuf, 2026-07-21)** POURQUOI la goal macro échoue : tentatives/succès, cause de l'échec, **à quel pas** il survient, et la part d'échecs survenus après un choix arbitraire de descente. Tourne à budget de temps → marche sur les niveaux jamais résolus |
| `deltaf <niv> [s]` | **(neuf, 2026-07-24)** la macro **PROMEUT-elle** ses enfants dans la file ? Distribution de `Δf = N + poids·Δh` sur les enfants enfilés, macro contre poussée simple ; `Δf = 0` = promu par le tie-break `g`, `Δf > 0` = relégué d'un palier. Décompose Δh en part **caisses** et part **joueur** |
| `usok <niv> [mode]` | **(neuf, 2026-07-27)** coût en **TEMPS** normalisé par machine (§ règle ci-dessous). Chronomètre l'étalon `bench 2 astar` ET la cible sur le même binaire, rend la cible en **USok**. `CORRAL=1 usok.sh …` = coût d'une feature sur la cible. Script `mesures/usok.sh`, pas un binaire |
| `diverge`, `paires`, `trace`, `passages`, `congestion` | mou de `h`, interactions de paires, solution pas à pas, cartes de trajets |

**Règles de mesure, non négociables :**
- **Comparer un binaire à un AUTRE binaire** (ancien reconstruit depuis `HEAD` via
  `git worktree`), **jamais à un chiffre écrit** dans ce document : il vieillit en silence
  pendant que le code bouge.
- **Noter le commit à côté de CHAQUE chiffre mesuré.** Un chiffre sans commit ne se distingue pas
  d'un chiffre jamais vérifié. Les scores (états/poussées par niveau) vivent dans
  [scores.md](scores.md), un tableau par nouvelle progression, commit en clair sur chaque ligne.
  **En cas de rebase** (hash introuvable, `git cat-file -e <hash>` échoue) → le chiffre est
  présumé périmé, on relance la mesure et on ajoute un nouveau tableau, on ne corrige jamais une
  ligne à la main.
- **Le TEMPS se note en USok, jamais en secondes** (neuf, 2026-07-27). Le projet tourne sur
  plusieurs machines : une seconde écrite ici ne veut rien dire (elle dépend de la machine), c'est
  le piège « jamais à un chiffre écrit » appliqué au temps. **1 USok = temps de `bench 2 astar`**
  (A\* pur, 590 066 états / 131 poussées — invariant du canari, donc un mètre qui ne dérive pas
  quand on touche à la macro/corral/goal-ordering). `mesures/usok.sh <niv> [mode]` re-chronomètre
  l'étalon sur place à chaque appel (aucun état persistant à maintenir entre machines) et rend la
  cible en multiples. Un ratio ≥ ×1,1 est significatif ; en dessous, c'est du bruit best-of-3
  (~3 %). Pour comparer deux régimes, figer la calibration : `USOK_REF=<s> usok.sh …`.
- **`ps rss` ment sur macOS** (le compresseur sort les pages de la RSS). Utiliser
  `/usr/bin/time -l` (« peak memory footprint ») ou `footprint -p PID`.
- **La jauge de progression part sur `stderr`, et un pipe l'avale.** `bench <niv> 2>&1 | tail`
  après un `timeout` ne rend RIEN — rediriger vers un fichier (`2>jauge.txt`). C'est la seule
  façon de mesurer un niveau qu'on ne résout pas (11, 12) : `rangees N (max M)`, dépilements,
  et la tendance de la file.
- **`getEtat()->QByteArray` est en BIG-ENDIAN**, `appliqueEtat(quint16*)` lit du **NATIF**.
  Passer les octets bruts à `appliqueEtat` reconstruit un plateau **vide** (0 caisse), que
  `checkVictoire()` prend pour un état gagné. Tout harnais qui relit une clé DUMP_DEV doit
  la **décoder** (`decodeCle`). ⚠️ **Ce bug a faussé `mou` pendant longtemps — cf. §5.**

---

## 2. Gains mesurés, et la technique qui les a produits

### 2.1 Coût unitaire (temps/mémoire, à espace d'états constant)

| technique | gain | comment |
|---|---|---|
| **`pousse()` + `Noeud` plat** | niveau 1 **62,9 s → 7,2 s (×8,7)**, 378 → 188 Mo | poussée directe qui téléporte le joueur au lieu d'un `AStar` de marche par enfant ; `Noeud{parent, idxCaisse, dir}` (8 o) au lieu d'une `QList` de coups |
| **Build `-O2`** | facteur constant | le `.pro` était en `-O0` ; passé en `release force_debug_info` |
| **Move ctor `noexcept` + tas de poignées** | conteneur level2 567 → 27 ms | le tas ne porte que `{f,g,idx,cle}`, pas un `Game` ; sans `noexcept`, `std::vector` recopie profondément à chaque doublement |
| **`SElement` allégé + `appliqueEtat`** | mémoire ÷2 à ÷3 (niveau 1 : 518 → 160 Mo) | la file ne porte que la clé ; le `Game` est reconstruit au dépilement sur un objet réutilisé |
| **Clé en arène (`cle.h`)** | mémoire ×1,3 à ×1,4 | toutes les clés d'un niveau font `N+1` shorts (N constant) → rangées bout à bout, la file/les tables ne portent qu'un offset 32 bits ; zéro `malloc`/en-tête `QArrayData` par clé |
| **Chemin chaud du flood-fill** (zone passée + pré-test avant copie + tampons réutilisés) | **×1,20 à ×1,53** en temps (niv 11 ×1,53, 7 ×1,50, 5 et 17 ×1,44, 8 ×1,20), **à espace d'états constant** | `getZoneJoueur` est le point le plus appelé du solveur (~10 fois par état). Trois causes : il était refait à l'identique au 1ᵉʳ pas de chaque macro (×5 caisses/état), une tentative sur deux mourait au pas 0 **après** une copie complète de `Game`, et chaque appel allouait un `QVector<bool>` **et** une `QList<short>`. Cf. §6.3 |
| **Adressage ouvert (`TableG`) + `Noeud` 8 o + arène par blocs** | mémoire **×1,4 à ×2,0** (niveau 17 ×2,04, niveau 3 ×1,66) | `meilleurG` en table ouverte (8 o/cellule, sondage linéaire) au lieu d'`unordered_map` (~40 o d'infra) ; arène en blocs de 65536 jamais réalloués → aucun pic de doublement, pointeurs valides à vie |

### 2.2 Réduction du NOMBRE d'états — le vrai levier

| technique | gain | comment |
|---|---|---|
| **`casesMortes`** (deadlocks statiques) | supprime la quasi-totalité des culs-de-sac | flood-fill à rebours depuis tous les buts, en simulant des *tirages* (règle des 2 cases) ; table statique par niveau. Généralise le corner deadlock |
| **Gel récursif** (freeze) | −5,4 % (niv 1), −2,3 % (niv 17) — **décroît avec la taille** | une caisse gelée si bloquée sur les 2 axes (mur, ou 2 cases mortes, ou caisse elle-même gelée — récursion avec garde `enCours`). Ne récupère que les deadlocks *dynamiques*, rares |
| **A\* admissible seul** | **~0 %** (−20 % niv 1, −3 % niv 17) | une poussée utile fait `g+1 / h−1` → `f` constant → A\* doit développer tout `f ≤ C*`. **Une `h` admissible ne coupe pas ce qui n'est pas mauvais** |
| **`h` joueur-aware** | niveau 17 **tension 54 % → 91 %, ×18 vs BFS** ; niveau 0 : 111 → 8 états | `distJoueur[caisse][region]` = distance d'une caisse SEULE vers un but, **en tenant compte de la région où est le joueur** (une caisse coupe le plateau ; selon le côté du joueur elle n'est pas poussable pareil). BFS à rebours, table précalculée |
| **Couplage hongrois** | niveau 1 optimal **×59** (783k→13k) ; niveau 17 **×13,6** (14,8M→1,09M) ; **a résolu le 2** (591k/131) | `cout[caisse][but]` = `distanceParBut` joueur-aware ; affectation de coût minimal (O(n³)). Corrige les **collisions de buts** (N caisses visant le même). Domine « chaque caisse vise son plus proche » |
| **Guidage lexicographique** (tie-break §10.2) | niveau 1 **÷2,8** (15596→5638) | à `f` et `g` égaux, ordre canonique de rangement (distances-restantes par but, via l'appariement hongrois déjà calculé). **Pur tie-break → optimalité intacte.** N'aide QUE le régime `f=C*` (cf. §3) |
| **Goal macro + goal-ordering à rebours** | **×1000 à ×14000** ; **a résolu le 4** (3,69M/355) ; niveau 1 : 5638 → 14 | pousse une caisse jusqu'au but d'un coup (transition composite) le long de son trajet solo ; ne s'engage que si le but actif est atteignable, sinon poussées simples. L'ordre de remplissage vient du **rebours** (vider la salle pleine en tirant les caisses, ordre inversé = ordre de pose : le plus enclavé posé en premier) |

**La pondération** (`f = g + w·h`, w=2) : niveau 1 ×34, niveau 17 ×2,1 — **mais renonce à
l'optimalité** (+6 % / +1,9 % de poussées). Gardée comme mode « rapide, approché » distinct.
Depuis le couplage, l'optimal la bat sur le 17 → utile surtout en secours. w>2 explore PLUS
(une `h` trop gonflée ne guide plus).

---

## 3. Le résultat structurant : `C* = trajets + congestion`

Mesuré (`passages`) : on résout chaque caisse **seule** (BFS, 1 caisse + 1 but) et on somme.

| niveau | trajets solos | complet | écart = **mou** |
|---|---|---|---|
| 1 | 95 | 97 | 2 |
| 2 | 129 | 131 | 2 |
| 3 | 128 | 134 | 6 |
| 17 | 201 | 213 | 12 |

**L'écart vaut EXACTEMENT le mou de `h`** — logique : `h` EST la somme des trajets solos
(couplage). Donc :

```
C*  =  Σ trajets solos  +  coût de congestion
       └── h, EXACT ──┘    └── le mou, 2 à 12 ──┘
```

- **Les artères de trafic sont prédites sans erreur** : une caisse lancée dans une artère
  suit son trajet solo, les autres ne la dévient pas. Tout l'écart est concentré là où les
  caisses se démêlent (zone de départ du 17, zone d'arrivée du 1).
- **La congestion, c'est du DÉBLOCAGE** : écarter une caisse assise sur le trajet d'une autre
  (coûte 2 : elle s'éloigne + devra revenir), ou qui bloque le **joueur**. **Critère PAR
  CAISSE, jamais par case** — une carte de trafic agrégée a perdu l'identité des caisses et ne
  peut structurellement pas voir la congestion (niv 17 : 12 de mou pour 2 passages hors réseau).

### Les DEUX régimes de `f` — décident quel levier mord où

Histogramme des `f` au dépilement (`INSTRUM_F`) :

| niveau | `f < C*` (le mou) | `f == C*` (multiplicité) |
|---|---|---|
| 1 | **0 %** | 100 % |
| 2 | **99,7 %** | 0,3 % |
| 17 | **93,3 %** | 6,7 % |

- **`f = C*`** = multiplicité des entrelacements de chemins optimaux (§9.4). Un **guidage**
  (tie-break) les départage → gain. C'est le niveau 1.
- **`f < C*`** = le mou de `h`. A\* les développe **d'office** (optimalité), quel que soit
  l'ordre. **Aucun guidage n'y touche** — seule une `h` plus serrée OU un **élagage prouvé**
  les enlève. C'est ce qui domine les gros niveaux (2, 11, 17).

---

## 4. Ce qui a été réfuté (avec la raison mesurée)

- **Oracle du mou** (ajouter le mou comme constante) : A\* ne voit que les `f` **relatifs** →
  ajouter `k` partout ne change rien à l'ordre de dépilement.
- **Couper les répétitions** (interdire de remplir dans le désordre) : le désordre valide
  n'est pas de la redondance → niveau 1 rendu **insoluble**.
- **Sous-optimal sur gros niveaux** (greedy / pondéré / beam) : **PIRE** que l'optimal — le
  greedy plonge, tombe dans les manœuvres, backtracke. Ne paie que sur le niveau 1.
- **Décomposition par PAIRES de caisses** (PDB) : la congestion est une **densité** (3+
  caisses), pas une interaction 2-à-2 — nulle sur le 17 (0/15 paires) qui a pourtant 12 de mou.
- **Découpage « une caisse à la fois »** : incomplet (interdit le parking temporaire et le
  ressortir-d'un-but), et « trouver l'ordre » EST le problème (Sokoban est PSPACE-complet).
- **« caisses manquantes = murs » comme `h`** : ajoute des obstacles permanents → surestime →
  élague le chemin optimal. (Valide seulement DANS un sous-solve figé, pas comme borne globale.)
- **Couplage hongrois pur** (avant le joueur-aware) : ne corrige que les collisions de buts,
  ~0 % sur le 17 (dont l'erreur est du coût de manœuvre). C'est le joueur-aware qui l'a rendu
  décisif.

---

## 5. ⚠️ CORRECTION MAJEURE (2026-07-17) : `mou` était cassé, §9.1 est FAUX

**Le bug.** `mou.cpp` relisait chaque clé DUMP_DEV via `appliqueEtat((quint16*)cle.constData())`.
Or `getEtat()->QByteArray` est **big-endian** et `appliqueEtat` lit du **natif** → tous les
index byte-swappés → plateau **vide** → `checkVictoire()` = **gagné**. `mou` prenait donc chaque
garbage pour un état trivialement résolu et rendait **« 100 % sur chemin, 0 deadlock »** sur du
vide.

**Conséquence.** Le **§9.1** (« A\* ne gaspille RIEN — 0 deadlock non détecté ») — le pilier qui a
fait **abandonner la piste deadlock** et **fermer le corral** — reposait sur cette mesure. Il est
**invalide**.

**Corrigé** (`decodeCle`, 1 ligne dans `mou.cpp`) + outil neuf `mesures/mort.cpp` : pour un
niveau non résolu, il échantillonne les états **dépilés** et classe chacun par un **sous-solve
complet borné** (A\* optimal, budget d'états) → **soluble** / **mort** (file vidée sous budget =
deadlock manqué) / **inconnu** (budget atteint). Validé par self-test (`decodeCle` reconstruit le
vrai départ ; le cast brut donne un plateau vide) et par des états morts exportés en `.xsb`,
tous sensés.

**La vraie mesure :**

| (A\* optimal) | niveau 1 | niveau 2 |
|---|---|---|
| sur un chemin optimal | 4 % | 5 % |
| hors chemin, soluble | 24 % | 5 % |
| **DEADLOCK non détecté** | **72 %** | **90 %** |

**72 % (niv 1) à 90 % (niv 2) de ce qu'A\* optimal développe sont des culs-de-sac que
`checkDefaite()` laisse passer.** Mesuré aussi sur le 6, le 11 — partout. **La détection de deadlock est un levier
réel, abandonné à tort.** Couper un état mort supprime aussi sa descendance → gain superlinéaire.

**Le corral, mesuré :**
- **93–100 % des états morts ont une région scellée** (corral) → bonne **couverture** d'un
  détecteur fondé sur le corral. Les morts « globales sans corral » sont rares.
- **MAIS 75 % des états SOLUBLES ont aussi un corral**, de même taille moyenne. Donc
  **« corral > 0 » ne se prune PAS** : on scelle souvent une région temporairement, le joueur
  la rouvre (parking §4, manœuvres §3). Fausse-positif = insoluble en silence.
- **Signal validé** (idée utilisateur) : au départ du 11, la poussée mort-née scelle **35 cases**
  d'un coup (corral 35 → MORT) ; les coups vivants ont **corral 0**. Le classement manuel
  « favoriser les coups qui ouvrent une porte sans en fermer » est un **prédicteur de mort mesuré**.

---

## 6. Pistes à explorer

### 6.0 Feuille de route — ordre de reprise (décidé le 2026-07-17)

Séquence convenue, du plus sûr au plus risqué :

0. **✅ FAIT le 2026-07-20 (suite 2) — l'ordre de remplissage est codé et promu en défaut**
   (§6.2). La règle **précédence par approches + CONTIGUITÉ DE RUN** régénère l'ordre humain
   et fait **27 états sur 191** (bat l'oracle, 28), **résout le 190**, et améliore massivement
   2/3/17 en macro. Oracle et env de debug retirés. Reste ouvert : le **signal de connectivité**
   (poche-derrière-goulot en premier) que le local ne capture pas — bascule sur l'item 3.
1. **Goal-ordering multi-salle** (§6.2) — sûr, LOUD (un mauvais ordre fait échouer la
   macro visiblement, jamais une fausse solution : `ordreButs` guide, il n'est pas une
   borne). Coût nul sur les niveaux à une seule salle (ordre identique).
2. **❌ RÉFUTÉ le 2026-07-21 — le test par-but N'EST PAS SÛR** (§6.1). Câblé, mesuré, retiré :
   le juge neuf `mesures/fp` (rejeu d'une solution GAGNANTE) lui trouve **106 faux positifs sur
   le 17**. Le « 0 FP » de la veille était un artefact d'échantillonnage. Le couplage restait,
   lui, inutilisable (52 % de FP). **Aucun élagage deadlock sûr n'est disponible à ce jour.**
3. **❌ RÉFUTÉ le 2026-07-21 — le guidage par portes ne paie pas** (§6.1). Codé, mesuré, reverté.
   Sûr comme prévu (canari intact partout), mais **le gain suit exactement la masse `f=C*`** :
   ÷3,1 sur le 1 (100 % à `f=C*`), −0,06 % sur le 17 (6,7 %), 6 états sur 590 066 au niveau 2
   (0,3 %). **Zéro sur les cibles 8 et 11**, et la variante forte **fait perdre le 190**.
   Conséquence pour la suite : **plus aucun tie-break ne reste à tenter**. Le §3 est une borne,
   pas une indication — seul un ÉLAGAGE prouvé attaque encore les gros niveaux.
4. **✅ FAIT le 2026-07-27 — Corral unitaire PROMU en défaut** (§6.1). Le cas **taille 1** est
   terminé : O(1) incrémental prouvé équivalent au balayage complet, coût mesuré en USok (~0 sur le
   chemin macro, +6 % en A\* pur), fréquence du motif quantifiée (100 % des morts sur 4/7, 2 % sur
   5/9 — prédit le gain), **0 faux positif** (juge `fp` ET oracle `mort`). C'est le **premier
   élagage deadlock sûr** du projet : ×6,6 sur le 4, ×6,8 sur le 7, mais **zéro** là où son motif
   est absent (1/2/6/17, A\* pur) — un **coin** du problème, pas la masse `f<C*`. **Le corral de
   taille N reste entier**, avec son contrat d'origine (scellé + non-rouvrable + sous-doté en buts
   atteignables). Prochaine étape convenue : structure **« liste de prédicats LOCAUX »** pour
   ajouter d'autres motifs bon marché (localisés à la caisse bougée) sans toucher au point chaud.
5. **Repli anytime de la macro** (§6.3) — en réserve, borne le temps des cas lents (8, 9).
6. **✅ FAIT le 2026-07-23 — backtracking sur les forks de la macro, promu en défaut** (§6.3) :
   `Game::macroVersButBacktrack` remplace `macroVersBut` dans le solveur sans condition. Canari
   intact, gain net sur 5 (÷1,85) et 9 (passe de « ne termine pas » à ~150 s). Neutre sur les
   cibles 11/12 (toujours non résolues) ; **le 8 tombe depuis, sans modif de code, laissé tourner
   sans budget — cf. §6.3**. Reste ouvert : réutiliser la zone du 1ᵉʳ pas (perf,
   cf. §6.3) et le secours de recherche borné gaté par `resteAuBlocage` pour les vrais détours
   non-monotones (aucun cas confirmé à ce jour).

En réserve, pas à trancher : mémoire (mur disparu), sous-optimal (pire sur gros), RN (§6.4).

### 6.1 Détection de deadlock — le levier rouvert par §5

> ⚠️ Terrain du faux positif : le projet s'y est fait avoir 3 fois (gel naïf, `h` qui
> soustrait, caisses=murs). **Juge unique : le canari + les niveaux résolus restent résolus**
> (les 11 de [scores.md](scores.md) — on ne peut juger que là-dessus, un élagage ne peut pas
> « casser » un niveau qu'on ne finit pas de toute façon). Discuter avant de coder.

#### ✅ Session du 2026-07-20 (suite 3) — mesure du gain deadlock (outil `mort`, branche `gain-deadlock`)

**Le taux de deadlock, mesuré (`mort <niv> livraison`, oracle = sous-solve optimal borné) :**
- **Niv 9 : 100 % des états profonds dépilés sont MORTS** (60/60, 120/120 selon l'échantillon),
  à 10–11/14 caisses posées — la macro s'épuise dans un endgame déjà condamné.
- **Le corral ne couvre que ~5 %** des morts du 9 (contre 93–100 % sur 1/2/11 au §5). **Donc le
  corral (item B) n'est PAS le bon outil pour le 9.** Ses morts sont des poches de livraison /
  gel, sans région scellée.

**Test « LIVRAISON » prototypé** (`livraisonMorte()` dans `mesures/mort.cpp`) : un but VIDE dont
aucune caisse ne peut être poussée jusqu'à lui (BFS avant de poussées, joueur qui marche jusqu'à
l'appui ; caisses-sur-but = obstacles) rend l'état insoluble. **Relaxation optimiste ⇒ preuve,
donc censé être sans faux positif.** Mesuré (capture parmi les morts / faux positifs parmi les
solubles) :

| niveau | capture | faux positifs |
|---|---|---|
| 7 | **96 %** | 0 |
| 3 | 70 % | 0 |
| 6 | 50 % | 0 |
| 17 | 45 % | 0 |
| **9 / 11** | **0 %** | 0 |
| 0/1/2/5 | (peu de morts) | 0 (dont 109 solubles confirmés sur le 1) |

- ~~**ZÉRO faux positif partout** → le test est **sûr pour `checkDefaite`**.~~ ❌ **FAUX, corrigé le
  2026-07-21** (session ci-dessous) : mesuré par rejeu d'une solution gagnante (`mesures/fp`), le
  même test fait **106 faux positifs sur le 17**. L'oracle par sous-solve borné de `mort` mentait —
  **un échantillonnage ne prouve pas l'absence de faux positif ; un chemin gagnant, si.**
- **MAIS 0 % sur 9 et 11** : leurs morts ne sont **pas** de type « but orphelin » (un but
  qu'aucune caisse n'atteint). Ce sont des deadlocks de **CAPACITÉ** (les caisses atteignent
  chacune un but, mais pas assez de buts **distincts** — condition de Hall) ou de **gel**.
**Test « COUPLAGE » (Hall) essayé pour le 9/11** (`livraisonMorteCouplage()`) : appariement biparti
caisses restantes → buts vides livrables (atteignabilité mono-caisse depuis l'état) ; pas de
couplage saturant ⇒ mort. **Attrape le 9 à 100 %** (ses morts SONT de la capacité) — MAIS :

| | 9 | 17 / 2 | 3 / 6 / 7 | **1** | 11 |
|---|---|---|---|---|---|
| capture | **100 %** | 100 % | 86–91 % | 25 % | **0 %** |
| **faux positifs** | — | 0 | 0 | **52 % (76/147 !)** | — |

- **❌ LE COUPLAGE EST INUTILISABLE : 52 % de faux positifs sur le 1.** L'argument de solidité
  est FAUX — la livraison réelle est **séquentielle** (une caisse atteint son but *après* qu'une
  autre s'est écartée). L'atteignabilité depuis l'état **figé** rate ces arêtes → « pas de
  couplage » sur un état pourtant soluble. **Le couplage parfait sur l'atteignabilité instantanée
  n'est PAS une relaxation valide.** À ne pas ressortir comme élagage. (Comme borne `h`
  admissible, en revanche, le min-cost matching reste valide — c'est déjà `getHeuristique`.)
- **Le 11 reste à 0 %** même en couplage : ni orphelin ni capacité → **interactions simultanées
  caisse-caisse** (démêlage), le mur PSPACE du §4. Ininélageable sainement à bas coût.

**CONCLUSION deadlock — RÉVISÉE le 2026-07-21 :**
1. ~~Test par-but (orphelin) = le seul gain deadlock SÛR~~ → **RÉFUTÉ**, cf. session du 2026-07-21
   ci-dessous. Le test invente des morts (BFS non joueur-aware + obstacles-caisses injustifiés).
2. **Le 9 (capacité) et le 11 (interaction) ne se prunent PAS sainement** à ce coût. Pour eux, le
   levier n'est pas l'élagage mais le **GUIDAGE** (§6.4a : dé-prioriser, jamais couper).
3. **Nouveau** : plus aucun élagage deadlock n'est disponible. Le prochain levier reste le
   **guidage par portes** (ci-dessous), qui ne coupe rien et ne peut donc pas mentir.

#### ❌ Session du 2026-07-21 — le test par-but CÂBLÉ, MESURÉ, RÉFUTÉ (et le §6.1 de la veille avec)

**Ce qui a été fait.** `Game::butNonLivrable()` (game.cpp) : le test « but orphelin » du prototype,
en version chemin chaud (tampons réutilisés, flood-fill du joueur évité quand aucune direction
n'ouvre sur du neuf). Câblé dans `checkDefaite`, puis dans le solveur. Interrupteur `LIVRAISON`
(0 = coupé, **défaut**) pour comparer les régimes **sur le même binaire**.

**Le juge qui a tout tranché — `mesures/fp.cpp` (NEUF, à garder).** On résout le niveau SANS le
test, puis on rejoue la solution coup par coup en interrogeant le test sur **chaque état
traversé**. Ces états sont solubles **par construction** — une solution y passe. Donc :

> **toute détection sur un chemin gagnant est un faux positif PROUVÉ.**

C'est ce que l'échantillonnage de `mort` ne pouvait pas voir : lui classait des états quelconques
par sous-solve borné, avec un oracle faillible ; ici la solubilité est certaine.

| variante du test | 1 | 2 | 3 | 5 | 6 | 7 | **17** |
|---|---|---|---|---|---|---|---|
| caisses posées = obstacles (le prototype) | 0 | 1 | 5 | 6 | 1 | 13 | **106** |
| idem, restreint aux caisses **gelées** | 0 | **1** | 0 | 0 | 0 | 0 | **106** |
| aucun obstacle-caisse (diagnostic) | 0 | 0 | 0 | 0 | 0 | 0 | **86** |
| lecture de `distanceParBut` (§ ci-dessous) | 0 | 0 | 0 | 0 | 0 | 0 | **0** |

**DEUX défauts indépendants, tous deux mesurés :**
1. **Le BFS de livraison n'est PAS joueur-aware.** Il ne retient qu'**UNE** position de joueur par
   case atteinte (`joueurApres[a] = c`), alors que la même case atteinte « par l'autre côté »
   ouvre d'autres poussées. D'où 86 FP sur le 17 **même sans aucun obstacle-caisse**. C'est
   exactement l'erreur que `distanceParBut` corrige depuis le §2.2 (indexation par RÉGION) — et
   c'est la faille du prototype `mesures/mort.cpp`, donc **des chiffres « 0 FP » de la veille**.
2. **Tenir les caisses posées pour des obstacles fixes est faux**, même restreint aux caisses
   **GELÉES** (1 FP sur le 2). L'argument « le gel est permanent, donc c'est une preuve »
   paraissait solide ; la mesure dit non. **Troisième fois que ce terrain piège le projet** (§6.1
   avertissement) : gel naïf, `h` qui soustrait, caisses=murs — et maintenant gelées=murs.

**La seule version SÛRE ne rapporte rien.** Sans obstacle-caisse, « telle caisse atteint-elle tel
but ? » se **lit** dans `distanceParBut` (joueur-aware, déjà calculée) : O(buts × caisses), zéro
FP — c'est le symétrique exact de `staticDeadlock` (celui-ci coupe quand une CAISSE n'atteint plus
aucun but, celui-là quand un BUT n'est atteint par aucune caisse). Mesuré : **strictement neutre**
sur 0-7 (mêmes états à l'unité). Logique — `staticDeadlock` couvre déjà cette information.

**Ce que le test coupait quand il trichait** (ordre de grandeur du gain à espérer d'une version
correcte) : A\* optimal, niveau 17 **1 082 674 → 717 214** états (−34 %) à 213 poussées ; niveau 6
−13 % ; en macro, niveau 7 **210 849 → 133 056** (−37 %). **Il y a donc bien de la matière** — mais
elle est dans l'obstacle-caisse, précisément la part qu'on ne sait pas justifier.

**Piège d'architecture, à retenir** : `checkDefaite` est le **mauvais** point d'appel pour un test
cher ou faillible. Marquer `perdu` sur un état **intermédiaire de goal macro** fait avorter la
macro entière (`move()` refuse de jouer sur un état perdu) et le solveur retombe sur les poussées
simples : niveaux **3 et 5 perdus** (résolus → timeout 70 s) alors que le test seul ne coûtait que
+10 %. Le bon point est le moment d'**enfiler** un enfant (fait, `solveurastar.cpp`).

**État du code** : `butNonLivrable()` reste dans `game.cpp`, **coupé par défaut** et documenté
(game.h) ; le solveur ne l'appelle que sous `LIVRAISON=5`. À supprimer si on ne reprend pas la
piste — ou à reprendre par le BFS **(case, région joueur)**, seul moyen de garder l'obstacle-caisse
sans inventer de morts. Le canari est intact avec le défaut (4/97/131/134/110/213, états inchangés).

#### ❌ Session du 2026-07-21 (suite) — GUIDAGE PAR PORTES codé, mesuré, RÉFUTÉ, reverté

Méthode humaine de démêlage, codée comme **tie-break de poussée** (pas comme borne — donc zéro
faux positif par construction). Ordre de préférence : 1. **ouvre une porte** ; 2. **fait de la
place** sans ouvrir ni fermer ; 3. **ouvre ET ferme** ; 4. **ne fait que fermer** (scelle un
corral), en dernier. Une « porte » = de la connectivité, lue dans `getZoneJoueur()`.

**Deux points d'implémentation qui, eux, étaient justes** (à réutiliser si la piste ressort) :
- **Le coût est nul.** `enfiler` appelait déjà `e.getEtat(arene.reserve())`, qui fait DE TOUTE
  FAÇON le flood-fill de la zone de l'enfant (il canonise la position du joueur dans la clé) et
  la jetait. Il suffit de la matérialiser et de passer la surcharge `getEtat(cle, zone)`.
- **⚠️ La NORMALISATION est le piège.** Une seule caisse bouge (y compris sur toute une chaîne de
  goal macro), de la case A vers la case B. **A est TOUJOURS gagnée** (la caisse la libère, le
  joueur s'y tient) et **B souvent perdue**. Sans exclure A et B du comptage, une poussée
  parfaitement neutre affiche +1/−1 et les 4 tiers ne veulent plus rien dire. En les excluant,
  « neutre » redevient exactement 0 gagnée / 0 perdue.

**Mesuré** (interrupteur `PORTES` sur le même binaire : 0 coupé, 1 = tier en bits de poids FORT
devant le score de rangement, 2 = en bits de poids FAIBLE). Neutralité de `PORTES=0` vérifiée
**binaire contre binaire** (worktree sur `HEAD`) : 14/14 identiques.

| | 1 | 2 | 6 | 17 |
|---|---|---|---|---|
| A\* optimal, `PORTES=0` | 5 369 | 590 066 | 542 032 | 1 082 674 |
| A\* optimal, `PORTES=1` | **1 716** (÷3,1) | 590 060 | 518 445 (−4,3 %) | 1 082 009 (−0,06 %) |
| part de `f = C*` (§3) | **100 %** | 0,3 % | — | 6,7 % |

> **LE GAIN SUIT LA MASSE `f = C*`, LIGNE POUR LIGNE.** Ce n'est pas une déception, c'est le §3
> qui se vérifie : A\* doit développer TOUT état de `f < C*`, quel que soit l'ordre. **Un
> tie-break ne peut structurellement rien gagner là.** Le niveau 1 gagne parce qu'il est
> intégralement à `f = C*` ; les gros niveaux ne gagnent rien parce qu'ils n'y sont pas.

**Sur les cibles, ZÉRO** (macro, budget 120 s égal) : le 11 plafonne à **4/14 dans les trois
régimes** (~1,69 M dépilements à ±1,5 %), le 8 à **10/18 dans les trois** (~1,68 M). L'hypothèse
qui justifiait la piste — « en macro la recherche est tronquée, donc l'ordre décide *quand* la
solution tombe » — est **fausse** : la file MONTE (+458 à +1084 par millier d'états), le solveur
n'est nulle part près d'un but, et réordonner des ex æquo ne l'en rapproche pas. **Le démêlage
n'est pas un problème d'ordre de visite.**

**Et la variante forte COÛTE un niveau résolu** : macro 190 **2 748 386 états → TIMEOUT 400 s**,
191 **27 → 81**. Un signal grossier à 2 bits placé DEVANT le score de rangement écrase le
goal-ordering — c'est-à-dire exactement ce qui avait résolu ces deux niveaux. Ailleurs le gain
macro est marginal (2 : 433→397 ; 17 : 202 053→199 724 ; 7 : −0,2 %).

**La variante faible est quasi INERTE** : identique à `PORTES=0` sur 13 niveaux sur 14 (seule
exception : 190, 2 748 386 → 2 841 092). Le score lexicographique existant ordonne déjà si
finement qu'un départage de rang inférieur n'a presque jamais d'ex æquo à trancher.

- **Canari intact partout, dans les trois régimes** (4/97/131/110/213) — la promesse « pur ordre
  de visite » a tenu. C'est la seule chose que ce chantier a démontrée.
- **Confondant vérifié avant de conclure** : packer 2 bits de tier oblige à réserver `61/n` bits
  au lieu de `63/n` pour le score lexicographique. La largeur ne change réellement que pour
  `n = 21` ou `n = 31` buts — **aucun niveau mesuré**, donc l'effet observé vient bien des portes
  et de rien d'autre. (Piège à connaître si la piste ressort : ce fork rend l'encodage par défaut
  dépendant d'une variable d'environnement.)
- **Reverté** (`game.cpp`/`game.h`/`solveurastar.cpp` rendus à `HEAD`). Rien dans le diff n'avait
  de valeur autonome, et un deuxième interrupteur mort dans le chemin chaud après
  `butNonLivrable()` n'en valait pas le prix. Les 6 lignes utiles au corral (la condition
  « cette poussée ferme une porte » = `perdues > 0`) sont re-dérivables d'ici en dix minutes.

**CONCLUSION, et elle oriente toute la suite : le stock de tie-breaks est ÉPUISÉ.** Guidage
lexicographique, goal-ordering, portes — les trois sont faits, et le §3 dit pourquoi le troisième
ne pouvait pas payer là où on l'espérait. **Il ne reste que l'élagage prouvé** (item 4, corral)
pour attaquer la masse `f < C*` des gros niveaux.

#### ✅ Session du 2026-07-24 (suite 2) — CORRAL UNITAIRE : le PREMIER élagage deadlock sûr du projet

**D'où c'est parti — un outil, puis un œil.** Le rejeu pas à pas neuf (§6.3) permet de rejouer le
chemin du **meilleur état** d'un run qui n'aboutit pas. L'utilisateur l'a passé sur le niveau 4 et a
vu, à l'œil, **deux deadlocks créés aux coups 40 et 657 d'un chemin de 692**. Preuve qu'aucun n'est
détecté : `Game::move()` refuse tout coup sur un état `perdu` (game.cpp:465), donc si `checkDefaite`
les avait vus, le rejeu se serait arrêté au coup 40. Il va au bout. **C'est le §5 (72–90 % de morts
non détectées) rendu visible sur un cas concret, et c'est ce qui a rouvert l'item 4.**

**Le motif, donné par l'utilisateur comme « le plus commun » :**

```
   #          A ne peut que monter — appui en S
  A #         B ne peut qu'aller à gauche — appui en S
 B S #        S = coin mort, scellé par A et B
#####
```

**La règle (`Game::corralUnitaireMort`, game.cpp) :**

> Soit **S** une case libre dont les 4 voisins sont murs ou caisses, et **F** les caisses qui la
> bordent. Si pour chaque caisse de F, **toute** direction est exclue parce que la destination est
> un MUR, l'appui est un MUR, l'appui est **dans S**, ou la destination est une **case morte** —
> alors aucune caisse de F ne bougera jamais. L'état est mort dès qu'une d'elles n'est pas sur un but.

**Pourquoi c'est une PREUVE, et pas le énième faux positif.** Ce n'est **pas** « S est inaccessible
au joueur » — ce serait exactement l'erreur qui a tué le couplage de Hall (52 % de FP) :
l'atteignabilité instantanée n'est pas une relaxation valide. C'est la **circularité** : libérer une
caisse de F exige un appui en S, et S ne s'ouvre que si une caisse de F bouge. Rien à échantillonner.

**Validé par `mesures/fp` AVANT tout câblage** (nouvelle variante `-1` = corral) :
**0 faux positif sur les 11 niveaux résolus**, 4 et 8 compris. Rappel de l'enjeu : c'est ce même
juge qui avait trouvé 106 FP au test « but orphelin » que l'échantillonnage de `mort` déclarait sûr.

**Gain mesuré** (interrupteur `CORRAL`, même binaire, appelé à l'**enfilage** et non dans
`checkDefaite` — cf. le piège d'architecture ci-dessus) :

| niveau | sans | avec | |
|---|---|---|---|
| **4** (macro) | 4 413 543 | **665 967** | **×6,6** |
| **7** (macro) | 210 925 | **31 166** | **×6,8** |
| 9 (macro) | 1 364 579 | 1 215 113 | ×1,12 |
| 5 (macro) | 38 594 | 34 711 | ×1,11 |
| 0 / 1 / 2 / 3 / 6 / 17 (macro) | — | **identiques à l'unité** | = |
| 1 / 2 / 6 (**A\* pur**) | — | **identiques à l'unité** | = |

- **Poussées inchangées partout** (4/97/131/134/143/110/90/213/237) : canari intact.
- ⚠️ **Le gain est très LOCALISÉ.** Le motif est absent de 1, 2, 6, 17 et dominant sur le 7.
  « Le plus commun » vaut pour les niveaux où il a été rencontré, pas globalement.
- ⚠️ **ZÉRO gain en A\* pur sur 1/2/6** — le régime où le §5 mesure pourtant 72–90 % de morts.
  **Ce test n'attrape donc PAS la famille qui domine ces niveaux** (gel non détecté, capacité,
  démêlage). Le corral unitaire est un **coin** du problème deadlock, pas sa solution.

**Deux bugs du premier jet, tous deux instructifs** (il produisait des FP sur 8 niveaux sur 9) :
1. **`tcNone` est le SOL LIBRE ici**, pas du vide (`isLibre() = tcNone || tcGoal`). Le tenir pour un
   mur rendait *toutes* les cases scellées. Le commentaire de `calculDistancePoussee` parle de
   `tcNone` comme « remplissage hors contour » — c'est le même code pour les deux usages, et il
   n'est distinguable que par l'accessibilité, jamais par le type.
2. **Une caisse sur la destination ou l'appui était traitée comme un obstacle permanent** — le piège
   « caisses = murs », **quatrième fois** que ce terrain piège le projet. Une caisse peut s'écarter :
   seuls un mur, la circularité via S, ou une case morte prouvent quoi que ce soit.

#### ✅ Session du 2026-07-27 — CORRAL UNITAIRE terminé, MESURÉ en USok, PROMU en défaut

Les quatre points « reste à faire » de la session ci-dessus sont réglés ; le corral unitaire est
désormais **inconditionnel** dans `solveurastar.cpp` (interrupteur `CORRAL` retiré, comme
`ORDRE_TB`/`ORACLE_HUMAIN`/`BACKTRACK_MACRO` en leur temps).

- [x] **O(1) au lieu du balayage complet — et c'est une ÉQUIVALENCE PROUVÉE, pas une heuristique.**
  `corralUnitaireMort(int caisseArrivee)` (game.cpp) ne teste que les ~4 voisines de la case de
  repos de la caisse qui vient de bouger. Preuve (game.h) : une transition ne déplace qu'UNE caisse
  vers une seule case B, donc seules les voisines de B peuvent passer de non-scellée à scellée ; et
  la fatalité d'un S **déjà** scellé ne dépend que de la géométrie statique (murs, cases mortes,
  circularité via S), jamais de la position des autres caisses. **Vérifié binaire à binaire**
  (`CORRAL=1` incr vs `CORRAL=2` plein) : **états identiques à l'unité partout**, y compris le 9
  (1 215 113 des deux côtés, là où il élague le plus). Le cœur par-case est factorisé dans
  `corralSMort`, partagé par les deux formes ; le balayage complet reste pour le juge `mesures/fp`.
- [x] **Le 4 en macro mesuré : ×6,6** (4 413 543 → 665 967, poussées 355 intactes). C'est le niveau
  qui a lancé la piste (deadlocks vus à l'œil aux coups 40 et 657) — le corral les élague.
- [x] **Coût en TEMPS mesuré, en USok** (unité neuve, cf. §1). 1 USok = `bench 2 astar` (A\* pur,
  invariant du canari). Le corral incrémental coûte **~0 sur le chemin macro** (17 macro : 0,581
  USok avec **et** sans, états bon marché noyés) et **+6 % en A\* pur seulement** (2 astar : 1,001 →
  1,061 USok — là où chaque état est bon marché, le test pèse relativement plus). Là où il élague :
  **×8 en temps sur le 7** (0,42 → 0,05 USok), **×5,2 sur le 4** (15,3 → 3,0 USok). Coût négligeable,
  gain massif : promotion inconditionnelle justifiée (choix retenu : tous modes, y compris A\* pur
  où il pourrait élaguer sur des niveaux non encore testés).
- [x] **Fréquence du motif quantifiée** (`mesures/mort <niv> corral`, mode neuf : oracle sous-solve
  vs `corralUnitaireMort`, collecte corral COUPÉ sinon les morts-à-motif sont élagués avant d'être
  développés). **La fréquence prédit le gain, exactement :**

  | niveau | morts oracle | capture = **fréquence du motif** | faux positifs | gain états |
  |---|---|---|---|---|
  | 7 | 150 | **100 %** | 0 | ×6,8 |
  | 4 | 88 | **100 %** | 0 | ×6,6 |
  | 5 | 110 | **2 %** | 0 | ×1,11 |
  | 9 | 150 | **2 %** | 0 | ×1,12 |

  Dominant (100 %) sur 4/7 → gros gain ; rare (2 %) sur 5/9 → petit gain. **FP = 0 partout**, ce qui
  **recoupe indépendamment le juge `fp`** (celui-là sur des solutions gagnantes, celui-ci sur des
  morts oracle). Le corral unitaire reste donc un **coin** du problème deadlock (nul sur les morts
  de gel/capacité/démêlage qui dominent 5/9 et les gros A\* purs), mais un coin **sûr et gratuit**.

**État du code après promotion** (`ef150d3`) : `game.cpp`/`game.h` (`corralSMort` + les deux formes
de `corralUnitaireMort` + `caseApres`), `solveurastar.cpp` (appel inconditionnel à l'enfilage, gate
retiré), `mesures/mort.cpp` (mode `corral`), `mesures/usok.sh` (neuf). Reste ouvert : la structure
« liste de prédicats LOCAUX » pour ajouter d'autres motifs sans toucher au point chaud (chacun
localisé à la caisse bougée = O(1), à valider au juge `fp` = 0 FP sur les 11 résolus avant câblage).

#### ✅ Session du 2026-07-27 (suite) — MOTIF 2, « la PINCE » : première extension par la liste de motifs

**Le motif (idée utilisateur).** Deux caisses scellent une case libre S, mais chacune PEUT bouger —
seulement **vers S** (descente interdite car l'appui serait le mur du plafond, sortie latérale car
l'appui serait DANS S) :

```
###        A n'a qu'un coup : entrer dans S
A S B      B n'a qu'un coup : entrer dans S
 #         S n'a qu'UNE place → l'autre gèle hors but → MORT
```

**Pourquoi le corral d'origine le ratait.** Il exigeait que TOUTES les caisses-frontière soient
*immobiles*. Ici chacune a un coup légal — entrer dans S (destination = S, libre) — donc
`toutesImmobiles = false`, pas de déclenchement. **Le motif neuf, c'est la poussée dont la
DESTINATION est S** : elle remplit S, et deux caisses la visent pour une seule place.

**La règle générale** (dans `corralSMort`, elle CONTIENT l'ancienne). On classe chaque
caisse-frontière : **LIBRE** (une poussée mène hors de S → on ne conclut rien, on abandonne S),
**CAPTIVE** (poussées possibles, mais toutes vers S), **IMMOBILE** (aucune). Si aucune n'est LIBRE,
les positions finales sont figées sauf UNE captive qui peut se garer dans S ; donc `capacite = 1`
ssi S est un but ET qu'une captive existe. **Mort ssi `nOffGoal > capacite`.** Preuve : au plus une
caisse quitte sa case (vers S, qui se re-scelle sitôt occupée), les `k−1` autres restent gelées à
vie → `nOffGoal − capacite` caisses restent hors but pour toujours. Quand tout est immobile,
`capacite = 0` → la règle **coïncide à l'octet** avec le motif 1 : elle n'ajoute que le cas captif.

**Validé au juge `fp` AVANT toute conclusion** (solve corral coupé via la trappe `CORRAL=0`, sinon
le juge est aveugle aux FP qu'il élaguerait lui-même) : **0 faux positif sur les 11 résolus**
(0–9, 17). Gain, canari poussées intact :

| niveau | corral seul | **+ pince** | |
|---|---|---|---|
| **8** | 11 721 759 | **5 905 757** | **×1,98** — le corral seul ne le touchait PAS |
| 17 | 202 053 | **190 635** | −5,6 % — **le corral seul était NUL sur le 17** |
| 6 | 821 | 698 | −15 % |
| 5 | 34 711 | 30 510 | −12 % |
| 7 | 31 166 | 29 725 | −4,6 % |
| 0 / 1 / 2 / 3 / 4 / 9 | — | identiques | motif absent |

**Coût USok négligeable** (2 astar 1,024 USok, dans le bruit — la classification par caisse ne pèse
pas) et **net positif** là où elle élague (17 macro 0,550 vs 0,581).

- ⚠️ **La pince touche une famille de morts DIFFÉRENTE.** Elle attrape le **8 (×1,98)** et le **17**,
  que le motif 1 ratait entièrement — alors qu'elle est neutre sur le 4, où le motif 1 dominait. Les
  deux motifs sont **complémentaires**, pas emboîtés. « Ajouter des motifs locaux » n'est donc pas un
  raffinement marginal : chaque motif ouvre un coin neuf du problème deadlock.
- **La trappe `CORRAL=0` est RE-INTRODUITE** (défaut ON), après avoir été retirée à `ef150d3`. Motif :
  `fp` et `mort` doivent solver/collecter SANS le corral pour le juger. Le retrait complet rendait
  ces outils inutilisables. La prod ne touche jamais la variable.
- **Structure « liste de motifs » — amorcée, pas généralisée.** Motifs 1 et 2 partagent le même
  substrat (S scellée + classification de la frontière), donc ils vivent dans `corralSMort` plutôt
  que dans des prédicats indépendants qui re-balaieraient. C'est le bon niveau tant que les motifs
  sont S-centrés ; on ne fabrique pas d'abstraction générale à partir de deux cas (piège §11.4).

- [ ] **B — Corral rigoureux comme ÉLAGAGE** (le vrai levier des gros niveaux, régime `f<C*`).
  Prune seulement si la région est **scellée** ET **non-rouvrable** (aucune caisse-frontière
  poussable vers l'extérieur — induction PI-corral) ET **sous-dotée en buts atteignables**.
  Prouvablement mort, zéro faux positif. C'est le seul qui attaque la masse `f<C*` que le
  guidage ne peut pas toucher.
- [x] ~~**A — Guidage corral-aware**~~ — **❌ FAIT ET RÉFUTÉ le 2026-07-21** (c'est le guidage par
  portes ci-dessus : ranger les poussées par corral-créé croissant). La réserve écrite ici,
  « n'aide que le régime `f=C*` », était **exacte** — et c'est précisément ce qui l'a tué.
  Ne pas le reprendre sous un autre nom.
- [ ] **Morts peu profondes** : ~30 % des deadlocks sont prouvables par un sous-solve de
  < 500–2000 états (mesuré sur le 11). Une **mini-recherche bornée** qui ne déclare mort que si
  elle **épuise** l'espace sous un petit budget est une **preuve** (pas une heuristique) → sûre.
  Reste à voir le coût par nœud (ne pas la lancer sur chaque état).

### 6.2 Ordre de remplissage — multi-salles

`ordreButs` (rebours) sait vider **une** salle (« fond → entrée »). Sur plusieurs salles —
**10** (28+4), **18** (7+2+2), **24** (20+2), **25** (17+2), **26** (12+1) — il produit un ordre
**mélangé** (un but d'ici, deux de là) ; la macro rebondit entre salles et se mure.

**Mesuré sur le 10 (solution main, 2026-07-17).** La séquence des arrivées-sur-but est
`GGGGGGG DDD…D` : la satellite (4 buts) est remplie **d'un bloc**, puis la grosse (28). Le
re-couvrage tardif de la case d'entrée (2,10) est la danse de la case-porte (comme (4,11) au
niv 11), **pas** un entrelacement. **L'ordre entre salles est LIBRE** (dixit l'utilisateur : « on
pourrait faire G d'un coup n'importe quand ») — ce qui compte, c'est **ne pas entrelacer**.

- [ ] **Correctif minimal** dans `calculDistancePoussee()` (où `ordreButs` est construit) :
  grouper les buts par **composante connexe** (= salle), rebours **dans** chaque composante,
  émettre **salle par salle** (jamais mélangé). `butActif()` finira alors une salle avant
  l'autre, automatiquement.
- [ ] **Ne PAS inventer de règle inter-salles** à partir du seul niveau 10 (piège §11.4) : un
  ordre par défaut stable, point. La mesure sur les 33 tranche (canari intact = pas de
  régression sur les résolus, une seule composante = ordre inchangé).
- [ ] **Référence à conserver — ordre humain du 10** (partie gagnante 32/32, 2026-07-17), en
  (x,y) et par salle (G = satellite, D = grosse). Satellite d'abord (3 buts), puis la grosse du
  **coin bas-droit vers le haut, rangée par rangée (colonnes 17→16→15)** — un rebours propre.
  ⚠️ La case-porte de la satellite **(2,10) est posée tardivement** (rang 20, en plein
  remplissage de la grosse) : la satellite n'est **pas** scellée d'un bloc, sa porte suit la
  danse d'entrée (comme (4,11) au 11). Ordre :
  `(3,11)G (2,11)G (3,10)G (17,14)D (17,13)D (17,12)D (17,11)D (17,10)D (17,9)D (16,9)D (17,8)D
  (16,8)D (15,8)D (17,7)D (16,7)D (15,7)D (17,6)D (16,6)D (15,6)D (2,10)G (17,5)D (16,5)D (15,5)D
  (17,4)D (16,4)D (15,4)D (17,3)D (16,3)D (15,3)D (17,2)D (16,2)D (15,2)D`
- [ ] ⚠️ Réserve : sur 18/24/25/26 l'ordre inter-salles pourrait être **imposé** (une salle
  coupe l'accès à l'autre). Si l'un résiste, c'est le signal → repli anytime (§6.3), pas une
  règle gravée.

**Deuxième facette — un MUR INTERNE à la salle (niveau 11, salle unique).** Le 11 n'est PAS un
multi-salle : une seule salle de buts, mais avec un **mur en (4,12)** en plein milieu. Ce mur
impose l'ordre de remplissage, et le rebours le rate — parce qu'il vérifie qu'une caisse peut
quitter son but **d'un pas** puis l'oublie, sans jamais **simuler son trajet** (il pose (5,11) au
rang 10 alors que c'est la seule porte vers l'appui (5,10) du couloir de la ligne 10 → ordre
INFAISABLE, blocage constaté au 11ᵉ rangement). L'ordre humain, lui, respecte le mur et **doit
être le bon** (dixit l'utilisateur, à cause du mur du milieu).

- [x] ~~**Référence à conserver** (partie humaine gagnante du 11, 14/14) — `(5,13) (5,12) (4,13)
  (1,10) (1,13) (2,13) (3,13) (1,12) (1,11) (2,12) (3,12) (2,11) (5,11) (4,11)`.~~
  ⚠️ **PÉRIMÉ le 2026-07-20** : cet ordre coûte **460 000 états** sur 191 là où celui de la
  session du 2026-07-20 en coûte **28**. Conservé pour mémoire seulement — la référence est
  désormais celle du 2026-07-20. Les deux sont des parties gagnantes : **« humain et gagnant »
  ne veut pas dire « bon pour la macro »**, il faut mesurer.
- [ ] Conséquence pour le calcul : le rebours doit **simuler le trajet de tirage** (pas juste le
  premier pas) pour voir qu'un mur interne rend certains ordres infaisables. C'est la vraie
  limite de `calculDistancePoussee()` sur le 11, indépendante du multi-salle.

#### ❌ Session du 2026-07-19 — tout reverté, aucune approche ne reproduit l'ordre humain du 11

**Banc d'essai neuf : niveau 191** (ajouté par l'utilisateur). L'endgame du 11 isolé à l'os —
une file verticale de 14 caisses dans un couloir d'alimentation 1-case + la salle de buts du 11
(même mur interne, décalée en bas). Tout le bruit du niveau (démêlage, trajets) est supprimé : il
ne reste QUE l'ordre. **Juge validé** : l'ordre humain EXACT **résout 191 en 460 k états** (macro) ;
mes ordres calculés plafonnent tous à **max 13/14**. Itération rapide (`bench 191 macro`).

**Chaîne d'essais, tous réfutés (191 reste à 13/14), du plus simple au plus lourd :**
1. **Trajet de tirage complet** (BFS `distanceSortie` au lieu du test à un pas) : ne retarde
   PAS (5,11) — sa distance de sortie (2 pas, via (4,11)→(4,10)) le classe comme l'ancien score.
   L'ordre ne bouge quasiment pas là où il fallait.
2. **Remplissage AVANT + atteignabilité JOUEUR, greedy** (un but posable si une caisse peut y
   être poussée ET le joueur atteint l'appui, buts remplis = obstacles) : le réglage de priorité
   (profondeur vs fragilité vs « n'emmure personne ») **ne converge pas** — corriger (1,10) décale
   (5,11) et inversement. « Plus menacé d'abord » a même fait s'effondrer la macro (`AUCUNE`, 3 états :
   il comblait (4,13) en 1er à cause du mur, se murant aussitôt).
3. **Profondeur d'ENCLAVEMENT** (BFS sur le graphe des buts depuis les entrées) : mesurée, elle
   sort **(1,13) le plus profond (3), pas (5,13) (2)** → ne matche pas l'humain, qui remplit (5,13)
   en premier par **dépendance du bouchon** (réduit (5,12)/(5,13) derrière (5,11)), pas par profondeur.
4. **Garde anti-emmurement à 1 pas** (ne pas poser un but s'il rend un autre non-posable) : sans effet.
5. **Backtracking** (chercher une séquence de remplissage COMPLÈTE faisable, au lieu de la deviner) :
   **rend le MÊME ordre que le greedy** → le modèle juge déjà cet ordre complet (14/14), donc la
   recherche ne recule jamais. **Le problème n'est pas la recherche, c'est la FIDÉLITÉ du modèle.**
6. **Atteignabilité CAISSE** (BFS de poussée à une caisse depuis la réserve, le joueur devant
   atteindre l'appui à chaque pas — la vraie livraison de la caisse) : **ne change pas l'ordre non plus.**
   Permutations manuelles testées en plus ((3,12) plus tôt, réduit corké en tête) : **aucune ne résout 191.**

**Le constat, net** : le modèle bon marché déclare « posable » des ordres que la **macro** ne sait
pas jouer. Combler ce dernier écart = capturer tout le manœuvrage réel = **résoudre l'endgame**
(que la macro met déjà 460 k états à faire *avec* le bon ordre). C'est le **mur PSPACE** du §4.

~~**Soupçon à garder** : 191 a un couloir d'alimentation 1-case de large → banc peut-être
**trompeur**.~~ ❌ **SOUPÇON LEVÉ le 2026-07-20** : 191 est un **excellent** banc — le bon ordre
y passe en **28 états**. Ce n'était pas le juge qui mentait, c'était l'ordre qui était mauvais.
**Quand toutes les approches échouent sur un banc, soupçonner les approches avant le banc.**

**Garde-fous confirmés (LOUD, comme prévu §6.0)** : toutes ces variantes = pur guidage macro →
**canari intact** (l'`astar` optimal ne lit jamais `ordreButs`), **aucune régression** des résolus
en macro (0-7/9/17 tous OK ; le 7 même **amélioré**, 42 k → 7,5 k états). Un mauvais ordre fait
juste rater/ralentir la macro, jamais une fausse solution.

**Décision : tout reverté** (retour à l'état committé de `game.cpp`), on réfléchit avant de reprendre.
Seule modif UI conservée (aide au test à la main) : **scrollbar** (déclaré dans `mainwindow.ui`) +
**numéros de lignes/colonnes sur les axes** (`wgame.cpp`, marge autour du plateau, notation (x,y)).

#### ✅ Session du 2026-07-20 — l'ordre humain est TROUVÉ, mesuré, et il paie

**Ce qui a changé** : l'utilisateur a rejoué la salle **en énonçant le critère à chaque pose**,
et non plus seulement la séquence. Joué sur le **192** (copie du 11 sans les murs du haut, pour
faciliter les déplacements à la main). **La salle de buts est identique octet pour octet sur
11 / 190 / 191 / 192** (vérifié) — mur central (4,12) et porte unique en colonne 4 compris —
donc l'ordre relevé sur l'un vaut pour les quatre.

**L'ordre, en (x,y)** (remplace la référence de 2026-07-17, qui était moins bonne) :

```
(5,12) (5,13) (4,13) (3,13) (2,13) (1,13) (1,12) (1,11) (1,10) (3,12) (2,12) (2,11) (5,11) (4,11)
   └── poche droite ──┘└──── rangée du bas, D→G ────┘└─ colonne G, B→H ─┘└─ intérieur ─┘└─ porte ─┘
```

C'est un **épluchage** : la poche la plus enclavée, puis le pourtour, puis l'intérieur, et les
cases de circulation en dernier. Rien d'entrelacé.

**Mesuré (`bench <niv> macro`, oracle vs ordre calculé) :**

| niveau | ordre calculé | **ordre humain** |
|---|---|---|
| **191** (endgame isolé) | 13/14, échec à 1 305 000 états | ✅ **OK — 28 états**, 250 poussées |
| **190** (11 démêlé) | — | ✅ **OK — 2 748 389 états**, 220 poussées |
| **192** (11 sans murs hauts) | **2/14** | 11/14 à 1 477 000 états |
| **11** (le vrai) | jamais résolu | 8/14 à **12 429 000** états (900 s, file qui stagne) |

- **28 états sur 191.** Avec le bon ordre, la macro ne cherche plus, elle **exécute**. L'ancien
  ordre humain (§ ci-dessus, 2026-07-17) mettait **460 000 états** au même endroit : facteur
  **~16 000**. Le juge de 191 était bon ; c'est l'ordre qui était mauvais.
- **190 résolu** — première résolution produite par le chantier goal-ordering.
- ⚠️ **Le 192 résiste alors qu'il est plus FACILE à jouer à la main.** Retirer des murs
  a agrandi l'espace libre → plus d'états. 190, plus contraint, passe ; 192, plus ouvert,
  plafonne. **Un niveau plus simple pour l'humain peut être plus dur pour la recherche.**
- Le blocage du 192 et du 11 n'est donc **pas dans la salle** (l'ordre y est bon, cf. 191) mais
  dans l'**acheminement** des 14 caisses. Sur le vrai 11, c'est le **démêlage** qui domine :
  12,4 M états pour 8/14, avec `rangees 0` en régime courant et une file qui **stagne** — le
  solveur passe son temps dans le haut du niveau et n'engage la salle que par à-coups.
  **Le goal-ordering a fait son travail ; ce qui reste sur le 11 est un autre problème.**

**LE RÉSULTAT EXPLOITABLE — la précédence est un THÉORÈME, pas un score.**

Les « sinon ça bloque » de l'utilisateur se démontrent sur la seule géométrie. Pour poser une
caisse sur un but, il faut la pousser depuis une case voisine, le joueur étant deux cases
derrière. Sur **(5,11)** : nord → joueur en (5,9) = **mur** ; est → **mur** ; sud → (5,12), déjà
rempli (rang 1) ; **ouest (4,11) = seule route restante**. Donc **(5,11) strictement avant
(4,11)**, prouvé sans jouer. De même : **(5,12) avant (5,11)** (sud = mur), **(4,13) avant
(3,13)** (nord = mur central, est = mur).

> **Règle générale.** Pour chaque but, énumérer les approches (case de la caisse + case d'appui
> du joueur). Si toutes les approches viables passent par un autre but, ce but doit être rempli
> **après** lui. → un **ordre partiel**, dont l'ordre humain est une linéarisation.

**Pourquoi ce n'est pas la tentative n°1 du 2026-07-19.** Celle-là calculait une *distance de
sortie* et en faisait un **score** — d'où son échec (« ne retarde pas (5,11) »). Ici on ne score
pas (5,11), on **interdit** (5,11) après (4,11) : une **arête de précédence**. Objet différent.
C'est une **preuve**, donc sans faux positif — et le « dans un coin / ne gêne pas le perso » reste
une **préférence** (tie-break) qui départage les ordres que la précédence autorise.

- [x] **CALCULER cet ordre** — ✅ FAIT (2026-07-20 suite 2). La règle **précédence + contiguité
  de run** fait **27 états sur 191** (bat l'oracle). Détail dans la session « suite 2 » ci-dessous.
- [x] ~~**Piste pour le tie-break** : point d'articulation (Tarjan)~~ — **non retenue** ; le vrai
  critère est la **contiguité de run** (prolonger un segment posé / cul-de-sac mural), pas
  l'articulation ni le couloir degré-2 devine.
- [x] ⚠️ **Oracle temporaire** (`ORACLE_HUMAIN`) — ✅ **RETIRÉ** (2026-07-20 suite 2), avec tous
  les env de debug (`ORDRE_TB`, `ORDRE_RETRAIT`, `ORDRE_DUMP`, `ORDRE_TRACE`).
- [ ] Le 192 et le 11 demandent autre chose que l'ordre (acheminement / démêlage) → c'est là que
  le repli anytime (§6.3) et l'élagage deadlock (§6.1) reprennent la main. **Confirmé** : tb=5
  résout 190 mais pas 192/11 (démêlage), le goal-ordering a fini son travail.

#### ⏸️ Session du 2026-07-20 (suite) — la règle codée, testée, PAS ENCORE bonne. À reprendre.

**Code écrit, dans `game.cpp` (non commité, `git` géré par l'utilisateur) :**
- `Game::distanceLivraison(bloque)` — BFS de poussées avant, depuis les caisses de départ.
  **Corrigée en cours de session** : la 1ʳᵉ version pointait juste que la case d'appui était
  libre ; elle a été remplacée par une vraie vérification que **le joueur MARCHE** jusqu'à
  l'appui (BFS de marche imbriquée, sans traverser la caisse en cours de déplacement). Un vrai
  gain de rigueur, à garder quoi qu'il arrive.
- `Game::ordreParPrecedence()` — glouton avant + garde anti-échouage (n'accepte un but que si
  le poser laisse tous les autres encore livrables). Tie-break sélectionnable via env
  `ORDRE_TB` (0=fragile+proche, 1=fragile+profond, 2=profond, 3=proche, 4=coin/degré).
- Câblé dans `calculDistancePoussee()` sous env `ORDRE_RETRAIT` (absent = nouvelle règle actif).
- Debug : `ORDRE_DUMP` (dump l'ordre final), `ORDRE_TRACE` (dump candidats/surs/scores à
  chaque pas). **Tout ce debug est à retirer une fois la règle bonne.**
- Dans `mainwindow.cpp` : `joue()` émet un `qDebug` par coup (`[mouv] joueur (x,y)->(x,y)
  [POUSSE caisse ->(x,y)] [POSE]`) — **gardé** (fonctionnalité utile en soi, redemandée après
  avoir été perdue sur un checkout ; pas de bouton d'export, juste la console).

**Résultat mesuré : AUCUN tie-break ne reproduit les 28 états de 191.**

| ORDRE_TB | résultat sur `bench 191 macro` (60 s) |
|---|---|
| 0 (fragile, puis proche) | 11/14 (5/14 avant la BFS joueur-fidèle — a empiré !) |
| 1 (fragile, puis profond) | **AUCUNE en 3 états** — suspect, cf. piste à vérifier plus bas |
| 2 (profond) | 13/14, ordre de départ FAUX ((1,32) au lieu de (5,31)) |
| 3 (proche) | **AUCUNE en 51 721 états** |
| 4 (coin/degré) | 12/14 — le meilleur de la série, mais loin de 28 |

**LA DÉCOUVERTE DE LA SESSION — analysée sur une trace RÉELLE (192, ré-humain, `[mouv]`
complet, 522 coups, 188 poussées, 43 arrivées sur but) :**

- **`(4,11)` reçoit 14 arrivées — une par caisse, sans exception.** Chiffre exact, confirmé par
  l'utilisateur : « c'est l'entrée de l'entonnoir, chaque caisse y passe forcément ». Mon modèle
  gère déjà ÇA correctement (la garde refuse de le bloquer tant qu'il reste un autre livrable) —
  ce n'est PAS la source du problème.
- **L'ordre de pose DÉFINITIVE extrait de la trace (dernière arrivée sur chaque but, en ignorant
  les arrivées de transit) est IDENTIQUE, 14/14, à l'ordre humain donné avant la trace** —
  `(5,12)(5,13)(4,13)(3,13)(2,13)(1,13)(1,12)(1,11)(1,10)(3,12)(2,12)(2,11)(5,11)(4,11)`.
  Confirme que la référence est stable, pas un coup de chance.
- **Le vrai mécanisme, vu dans le détail des poussées (colonne x=1)** : la séquence `(1,10)->
  (1,11) POUSSE ->(1,12)` puis `(1,11)->(1,12) POUSSE ->(1,13)` n'est PAS deux décisions — c'est
  **UNE caisse poussée d'un trait** qui s'arrête au mur ou à la caisse déjà posée. La colonne se
  remplit fond→entrée par la **physique du jeu** (pousser aussi loin que possible), pas par un
  choix de but à chaque étape.
- **Conséquence pour l'algo — deux niveaux, pas un seul score :**
  1. **Entre couloirs / aux points de passage** (l'entonnoir (4,11)/(5,11)) → la précédence par
     approches, déjà codée, déjà correcte.
  2. **À l'intérieur d'un couloir droit de buts alignés** (ex. colonne x=1, buts (1,10)(1,11)
     (1,12)(1,13)) → PAS un score à comparer entre buts, un ordre **local** : fond du couloir
     → entrée, par construction. C'est pour ça que « profondeur » (tb=2), calculée comme
     distance BFS **globale** depuis le départ de la caisse, se trompe : elle mélange la
     longueur du trajet total et la position locale dans le couloir.
- [x] **Détecter les couloirs / ordonner fond→entrée** — ✅ résolu autrement que prévu. Pas
  besoin de détecter une structure « couloir degré-2 » : une simple **contiguité de run** dans
  le tie-break (prolonger un segment déjà posé) recolle l'ordre local, sans classer les cases en
  couloir/bloc. Cf. session « suite 2 » : les deux divergences de 191 étaient purement locales.
- [x] Piste suspecte `ORDRE_TB=1` (« aucune solution en 3 états ») — **sans objet** : l'env
  `ORDRE_TB` est retiré, et le tie-break retenu ne produit jamais de fausse « aucune solution »
  (garde anti-échouage + fallback candidats). Non re-creusé, non reproduit avec la règle finale.

**État du code en fin de session (non commité)** : `game.cpp`/`game.h` contiennent la règle,
l'oracle (§ ci-dessus) et le debug de trace. `mainwindow.cpp`/`.h` contiennent le qDebug de
mouvements (gardé) et un correctif `#include <cmath>` / `std::ceil` nécessaire à la compilation
sur Linux (absent avant, faute préexistante à `HEAD`, sans rapport avec le solveur).
**Reprendre ici** : détection de couloirs, avec 191 comme juge (28 états = objectif).

#### ✅ Session du 2026-07-20 (suite 2) — ordre CODÉ, mesuré, PROMU en défaut. Chantier bouclé.

**Le tie-break retenu : CONTIGUITÉ DE RUN** (dans `ordreParPrecedence`, `game.cpp`). Diagnostic
décisif : l'ordre calculé (ancien tb=0) ne divergeait de l'oracle (28 états) qu'à **DEUX endroits**,
tous deux des mélanges **locaux** dans une file droite de buts alignés (rangée y=13 : (2,13)/(3,13)
permutés ; colonne x=1 : (1,12) rejeté en fin de colonne). Tout le squelette inter-groupes était
déjà bon. Le principe qui recolle les deux — **sans** détecter de structure couloir/bloc :

> Dans un run droit de buts, **remplir en continu** : parmi les buts sûrs, préférer celui qui
> **prolonge un segment déjà posé** (voisin en ligne = but posé), puis celui qui **part d'un
> cul-de-sac mural**, puis le plus encoigné (degré), puis le plus proche.

**Chemin des essais (juge : `bench 191 macro`, cible 28 états ; canari intact — l'astar optimal
ne lit jamais `ordreButs`) :**

| variante | 191 | 2 | 3 | 7 | 190 |
|---|---|---|---|---|---|
| ancien tb=0 (fragile+proche) | échec 1,3 M | 9 159 | 276 177 | 11 007 | — |
| **contiguité (prolonge + wall-ext)** ✅ RETENU | **27** | **433** | **509** | 210 849 (7,5 s) | **2 748 386** |
| prolonge seul (sans wall-ext) | 28 | 2 114 | 276 177 | 113 855 | — |
| prolonge → profond | échec | 454 | 509 | 33 660 | **>300 s** |

- **27 états sur 191** — bat l'oracle humain (28). **190 résolu** (2,75 M, = oracle à 3 états près).
  1→14, 2 : 9 159→**433**, 3 : 276 177→**509**, 17 : 240 460→**202 053**. Poussées canari intactes.
- **Le wall-ext (« partir d'un cul-de-sac ») est indispensable sur 2/3** mais **toxique sur les
  BLOCS pleins** (niv 7 : coin de bloc pris comme seed → 210 k ; niv 9 pareil). Le remplacer par
  « profondeur » sauve les blocs mais **casse 190 ET 191** (la poche du 191 doit passer tôt parce
  qu'elle est **derrière un goulot**, pas parce qu'elle est profonde). **Tension irréductible en un
  seul scalaire local.** Choix tranché : garder le wall-ext — il ne coûte **aucun** niveau réel
  (niv 7 reste à 7,5 s ≪ 60 s ; niv 9 est deadlock-bound, jamais résolu en macro de toute façon).
- **Le vrai manque restant est de la CONNECTIVITÉ** : « poche-derrière-goulot en premier » n'est
  pas un critère géométrique local → c'est le **guidage connectivité §6.1** (item 3 de la feuille
  de route) qui l'apportera si on veut un jour sauver les blocs sans perdre les poches.

**Promotion (état commité par l'utilisateur ensuite)** : contiguité passée en **défaut** dans
`ordreParPrecedence` ; **retirés** — oracle `ORACLE_HUMAIN`, env `ORDRE_TB`/`ORDRE_RETRAIT`/
`ORDRE_DUMP`/`ORDRE_TRACE`, lambda `nbApproches`. Le rebours reste comme fallback si la précédence
ne rend pas une permutation complète. `game.cpp` compile pour le bench ET l'app GUI.

**Ce qui reste pour le vrai 11 / 192 / les multi-salles** : plus l'ordre (fini), mais l'**acheminement
et le démêlage** — item 3 (connectivité) puis 4 (corral) / 5 (repli anytime) de la feuille de route.

### 6.3 Robustesse / temps

- [ ] **Repli anytime pour la macro** : passe 1 avec macro plafonnée en états, passe 2 sans
  macro si le budget est épuisé. Le repli doit se déclencher sur le **budget**, pas sur
  l'échec (un cas lent n'émet jamais « aucune solution »). Borne surtout le temps des cas
  lents (8, 9).

#### ✅ Session du 2026-07-21 (suite 2) — le COÛT PAR ÉTAT de la goal macro (outil `macro`)

**L'hypothèse de départ, RÉFUTÉE — ne pas la reprendre.** `macroVersBut` descend le champ
`distanceParBut` en prenant à chaque pas la **première** direction décroissante dans l'ordre de
l'énumération, et n'y revient jamais. On pouvait croire qu'elle échouait souvent en s'étant peinte
dans un coin, alors qu'une autre descente de **même coût** passait. Mesuré : les échecs survenus
après un tel choix arbitraire sont **1,8 % (niv 11), 2,5 % (8), 5,2 % (7), 3,8 % (17)** — c'est la
borne HAUTE de ce qu'un backtracking récupérerait. Et seuls **1 à 4 % des pas** offrent ≥ 2
descentes. **La descente n'a presque jamais le choix** : changer de sens de poussée oblige le
joueur à faire le tour, et le champ joueur-aware élimine d'office la plupart des variantes
géométriques. Backtracker sur les forks ne rapporterait rien.

**Ce que la mesure a révélé à la place — la macro est une machine à échouer :**

| | 11 | 8 | 7 | 17 |
|---|---|---|---|---|
| tentatives de macro **par état développé** | **5,3** | 5,2 | 4,4 | 3,2 |
| taux de **succès** | **0,12 %** | 0,15 % | 4,2 % | 0,07 % |
| échouent au **pas 0** (la caisse ne bouge même pas) | **48,5 %** | 12,2 % | 46,1 % | 22,1 % |

Chaque tentative coûtait une **copie complète de `Game`** (chez l'appelant) **et** un
`getZoneJoueur()` — flood-fill de tout le plateau + allocation. Soit ~8,8 M flood-fills en 60 s sur
le 11, ~10,6 par état développé.

**Les deux correctifs, mesurés contre un binaire `HEAD` reconstruit (états identiques 9/9) :**
1. **La zone du 1ᵉʳ pas vient de l'appelant** (`macroVersBut(..., const QVector<bool>* zoneInitiale)`).
   Au pas 0 le plateau n'a pas bougé : le flood-fill refait **à l'identique** celui que le solveur
   vient de faire pour `getCaissesDeplacable`, ×5 caisses candidates. **50 % des flood-fills
   supprimés** (36 % sur le 8, dont les chaînes sont plus longues). ⚠️ Invalider la zone dès
   qu'une caisse bouge (`zoneCourante = nullptr`) — seul endroit où une erreur donnerait une zone
   périmée, donc silencieusement fausse.
2. **`macroPeutDemarrer()` : écarter AVANT de copier.** Le 1ᵉʳ pas, sans rien modifier ni copier.
   ⚠️ **Le risque n'est pas la perf, c'est le filtre trop zélé** — écarter une caisse que la macro
   savait avancer supprimerait des enfants en silence. Neutralisé en promouvant la condition de
   descente en méthode UNIQUE (`avanceVersBut`), partagée par la boucle et le pré-test : elles ne
   *peuvent* pas diverger. Plus un garde pour « caisse déjà sur le but » (macro triviale à `true`,
   qu'un pré-test naïf refuserait).

3. **Tampons de flood-fill réutilisés** — surcharge `getZoneJoueur(QVector<bool>&)` : à taille déjà
   bonne, `fill()` est un memset sans allocation. Et la file du parcours passe de `QList<short>`
   (un malloc par appel) à un `QVarLengthArray<short, 512>` **sur la pile**. Tampons hissés hors
   des boucles chez les trois appelants chauds : la boucle de `macroVersBut`, la boucle d'états du
   solveur, et **l'enfilage des enfants** (`getEtat(cle)` refaisait le flood-fill dans un QVector
   neuf, un par enfant). ⚠️ Le tampon doit être détenu en propre — une copie qui traîne fait
   détacher le `fill()`, ce qui annule le bénéfice (sans nuire à la correction).

| gain cumulé | 2 | 5 | 7 | 17 | **11** | 8 |
|---|---|---|---|---|---|---|
| point 1 seul | ×1,11 | ×1,12 | ×1,11 | ×1,07 | ×1,10 | ×1,07 |
| points 1 + 2 | ×1,13 | ×1,17 | ×1,17 | ×1,09 | ×1,29 | ×1,06 |
| **points 1 + 2 + 3** | **×1,43** | **×1,44** | **×1,50** | **×1,44** | **×1,53** | **×1,20** |

- **Le point 3 est le plus gros des trois, et c'était une SURPRISE** — il avait été annoncé comme
  le moins prometteur (« les allocateurs modernes sont bons sur ce profil »). Il vaut à lui seul
  ~×1,25 à ×1,35. **Le coût d'allocation d'un conteneur Qt dans une boucle à ~10 appels par état
  n'est pas négligeable, il est DOMINANT.** À se rappeler pour tout futur chemin chaud — et à
  mesurer plutôt qu'à pronostiquer.
- **Le point 2 est CONDITIONNEL à la forme du niveau** : ×1,29 sur le 11 (48,5 % d'échecs au pas 0)
  et **rien** sur le 8 (12,2 % seulement, et le pré-test refait le balayage pour les 88 % qui
  passent — les deux effets s'annulent). Un gain moyen sur l'ensemble aurait masqué les deux faits.
- ⚠️ **PIÈGE DE MESURE, à ne pas refaire** : un **seul** tirage de 60 s donnait ×1,02 sur le 8, et
  j'ai failli conclure « le 8 ne profite pas ». En triple, c'est ×1,07 avec trois valeurs serrées.
  **Un tirage unique de 60 s ne suffit pas** — meilleur de 3, toujours (le min approche le cas non
  perturbé mieux que la moyenne).
- Piste non retenue : faire rendre à `macroPeutDemarrer` la direction trouvée, pour que
  `macroVersBut` ne rebalaye pas le 1ᵉʳ pas. Alourdit l'API pour un gain visible seulement sur les
  niveaux à chaînes longues — à ne faire que sur mesure préalable.

#### ⏸️ Session du 2026-07-23 — pourquoi la macro échoue si souvent : `echecBloque`, pas les forks

**Point de départ — outil UI neuf (non commité) pour VOIR le champ que suit la macro.**
`Game::champDistanceButActif()` et `Game::cheminMacro(idxCaisse)` (`game.h`/`game.cpp`) exposent le
champ `distanceParBut` du but actif tel que `avanceVersBut` le lit réellement (pas une lecture
indépendante par case — cf. piège ci-dessous). Câblés dans `WGame`/`MainWindow` : case cochable
« Champ distance but actif », et un **clic sur une caisse** rejoue son `macroVersBut` complet sur
une COPIE et affiche le trajet réel jusqu'au blocage. ⚠️ Première version fausse, corrigée en
cours de session : lire `regions[joueurRéel][cell]` pour CHAQUE case indépendamment n'est pas la
même chose que ce que fait `avanceVersBut` (`regions[c][devant]`, la caisse comme référence, pas
le joueur figé) — un voisin pouvait afficher une distance qu'aucune poussée légale n'atteint. Corrigé
en ne s'appuyant plus que sur `avanceVersBut` lui-même (jamais de calcul dupliqué). État du code :
`game.cpp`/`game.h`/`wgame.cpp`/`wgame.h`/`mainwindow.cpp`/`mainwindow.h`/`mainwindow.ui`, non
commité.

**Cas d'école, niveau 11** (`plateau_niveau11.xsb`, export de l'état après 5 buts posés) : la
caisse en (10,3) descend `19→18→17→16→15` jusqu'à (7,4) et s'y bloque. Vérifié à la main (tests
`/tmp/.../testmacro*.cpp`, jetables) : à (7,3), Bas ET Gauche faisaient tous deux baisser la
distance (un vrai fork) ; Bas est testé en premier dans l'énum (`Haut, Droite, Bas, Gauche`) et
gagne, mais mène à un cul-de-sac (appui pris par une autre caisse réelle en (8,4)). **Ce n'est pas
l'ordre des buts** (théorème déjà validé, §6.2, chantier fermé) — reproduit par l'utilisateur à la
main en le respectant scrupuleusement, même résultat.

⚠️ **CORRIGÉ ensuite dans la même session — la première conclusion (« il aurait fallu un vrai
détour non-monotone ») était FAUSSE.** En forçant Gauche à (7,3), la macro avance encore 12
poussées... puis se rebloque à (3,12), et un premier test (avec un bug de méthode : `pousse()` brut
ne vérifie pas l'appui, contrairement à `avanceVersBut`) a fait croire à un second cul-de-sac réel
(mur + caisse immobile). **Faux** : il y avait un DEUXIÈME fork non exploré, à (3,11) (Bas ET
Gauche baissent tous deux la distance), masqué par le même biais d'énumération. Revérifié
proprement (zone réelle + appui, comme `avanceVersBut`) : forcer Gauche à (3,11) puis laisser
`macroVersBut` reprendre seul réussit **intégralement, en 19 poussées — l'optimum exact**
(`(3,11)→(2,11)→(2,12)→(1,12)→(1,13)`). **Aucun détour n'était nécessaire : deux forks en cascade,
tous les deux récupérables.** Piège à retenir : corriger UN fork trouvé ne suffit pas à conclure —
il peut y en avoir un autre plus loin sur le même chemin.

**Généralisé avec l'outil existant `mesures/macro`** (créé le 2026-07-21, walk `INSTRUM_MACRO` déjà
en place) sur 18 niveaux — 9 résolus + 9 cibles (mesuré à `c54d7d7`, avant les ajouts de cette
session — `macroVersBut` intact à ce point), ⚠️ **un seul tirage de 15 s par niveau, PAS le
« meilleur de 3 » que ce document impose pourtant (§6.3 ci-dessus) — à retraiter avant de trancher
quoi que ce soit dessus.**

| niveau | tentatives | échecs | dont `echecBloque` | dont **fork avant blocage** | reste moyen au blocage |
|---|---|---|---|---|---|
| 1 | 28 | 67,9 % | ~tous | 0,0 % | 16,2 |
| 2 | 1 324 | 87,2 % | ~tous | 3,5 % | 12,5 |
| 3 | 1 125 | 86,8 % | ~tous | 5,8 % | 12,0 |
| 5 | 228 648 | 83,0 % | ~tous | 14,9 % | 10,8 |
| 6 | 2 847 | 94,7 % | ~tous | 10,6 % | 9,6 |
| 7 | 519 482 | 92,5 % | ~tous | 9,7 % | 4,8 |
| **9** | 623 271 | 94,8 % | ~tous | **50,7 %** | 10,1 |
| 17 | 500 729 | 99,9 % | ~tous | 4,8 % | 30,7 |
| 8 | 723 626 | 100,0 % | ~tous | 2,0 % | 12,5 |
| 10 | 223 853 | 100,0 % | ~tous | 12,1 % | 20,5 |
| 11 | 526 273 | 99,9 % | ~tous | 3,1 % | 14,9 |
| 12 | 703 860 | 96,8 % | ~tous | 20,4 % | 10,1 |
| 13 | 645 264 | 73,6 % | ~tous | 5,6 % | 9,0 |
| 14 | 576 121 | 96,8 % | ~tous | 5,2 % | 12,5 |
| 15 | 556 700 | 91,3 % | ~tous | 5,4 % | 6,1 |
| 16 | 673 076 | 99,8 % | ~tous | 8,8 % | 15,7 |
| 18 | 675 348 | 90,5 % | ~tous | 1,2 % | 8,4 |

(niveau 0 omis, échantillon trop petit — 7 tentatives)

**Deux constats :**
1. **`echecBloque` (aucune direction ne fait baisser la distance) N'EST PAS le cas rare qu'on
   pensait — c'est LE mode d'échec, sur les 17 niveaux mesurés sans exception** (`echecPousse`,
   `echecDistance`, `echecRegion` restent négligeables partout). Et `reste moyen au blocage` (5 à
   31 poussées) dit que ce ne sont pas des quasi-réussites : la macro meurt souvent en plein
   milieu du trajet, pas à 1 coup du but.
2. **`echecAvecFork`** (la part déjà mesurée le 2026-07-21, « ce qu'un backtracking récupérerait »)
   reste dans la fourchette basse déjà documentée (1-20 %) sur la plupart des niveaux — confirme
   que backtracker sur les forks ne paierait toujours pas en général. **Sauf le niveau 9, à 50,7 %,
   un vrai outlier** — à isoler, piste distincte (réordonner le test statique des 4 directions,
   coût nul, pas du backtracking).

⚠️ **Piste « zone d'embut » : nuancée après coup, pas codée.** L'idée de départ : détecter
automatiquement la salle de buts (composantes biconnexes / points d'articulation, cf. §6.2 —
« Tarjan… non retenue » pour le tie-break d'ordre, mais candidat naturel ici) pour scoper un
secours de recherche. **D'abord écartée sur un « à partir du niveau 12 il n'y a plus de salle »
trop catégorique, puis corrigée par l'utilisateur** : la plupart des niveaux EN ONT une, seuls
quelques-uns n'en ont pas. Reste non codée — sans objet une fois la piste ci-dessous choisie, qui
ne dépend pas de la géométrie.

**🎯 PISTE RETENUE — mémoriser les forks, backtracker au lieu d'abandonner.** Proposée par
l'utilisateur après le cas d'école ci-dessus (deux forks en cascade, tous deux récupérables) :
à chaque pas où `avanceVersBut` trouve **plus d'une** direction qui avance, la descente actuelle
en retient une (la première de l'énum) et **oublie les autres pour toujours**. Au lieu de ça :
les empiler, et si la chaîne meurt (`echecBloque`), dépiler jusqu'au dernier fork et reprendre
avec la direction suivante.
- **Bon marché par construction** : une seule caisse bouge pendant tout `macroVersBut` — un fork
  n'a besoin de mémoriser qu'une copie de `Game` à cet instant (COW sur les tables statiques,
  coût comparable à ce que le solveur paie déjà par candidate) + la direction non essayée, pas un
  arbre de recherche. Et les forks sont rares (1-20 % des pas, sauf le 9) : peu de branches à
  rouvrir en pratique.
- **Couvre exactement `echecAvecFork`, mais VRAIMENT** (pas la borne haute) : le chiffre déjà
  mesuré ne dit que « un fork a été croisé quelque part », sans vérifier que l'autre branche mène
  au but — potentiellement après PLUSIEURS forks en cascade, ce qu'un simple retry-une-fois ne
  capture pas (c'est exactement l'erreur faite dans le cas d'école ci-dessus).
- **Ne couvre toujours pas** un vrai détour non-monotone (aucune direction ne baisse jamais la
  distance nulle part sur le chemin) — mais aucun cas confirmé de ce genre n'a encore été trouvé ;
  celui qu'on croyait tel s'est révélé être un second fork non exploré.

**✅ IMPLÉMENTÉ, MESURÉ, PROMU EN DÉFAUT le 2026-07-23 —
`Game::macroVersButBacktrack`** : isolée (ne touche pas `macroVersBut`, toujours utilisée telle
quelle par les outils de diagnostic UI — `cheminMacro`/`champDistanceButActif`/`arbreMacro`, qui
ont justement besoin de la descente gloutonne SANS retour en arrière pour montrer le problème).
Câblée sans condition dans `solveurastar.cpp` (interrupteur `BACKTRACK_MACRO` retiré après
verdict, comme `ORDRE_TB`/`ORACLE_HUMAIN` en leur temps). **Un bug de premier jet corrigé
avant toute mesure valable** : la première version travaillait sur une copie locale et ne
recopiait jamais le résultat dans `*this` — l'appelant récupérait un état inchangé, silencieusement
dupliqué du parent, rejeté par la dédup (cassait même le niveau 1, canari en échec immédiat).
Corrigé (`*this = std::move(etat)` avant chaque `return`).

**Canari intact** une fois corrigé, avant ET après la promotion (chiffres : [scores.md](scores.md)).

Le taux de succès de la macro par tentative (mesuré niveau par niveau, 20 s, tirage unique — pas
un score, un diagnostic interne à `mesures/macro`) grimpe fort sur les niveaux à beaucoup de
forks, mais reste plat sur les cibles 8/11/12. Ça n'a pas suffi à conclure : le proxy
« taux de succès par tentative » ne prédit pas bien la performance globale (cf. verdict
ci-dessous) — seul un solve complet, comparé binaire à binaire, tranche. Les états/poussées de
chaque solve complet sont dans [scores.md](scores.md), pas ici.

**✅ VERDICT RÉVISÉ — un vrai gain sur certains résolus, toujours plat sur les cibles.** Le premier
verdict (« ne paie nulle part ») reposait sur une comparaison tronquée : les deux régimes avaient
été arrêtés à budget de temps égal (20 s) sans qu'aucun des deux n'ait fini. Une fois les solves
complets obtenus (chiffres : [scores.md](scores.md)) :
- **Niveau 5 : gain net** (÷1,85 en états, canari intact).
- **Niveau 9 : bascule qualitative, pas juste un gain.** En défaut, aucun binaire testé ne le
  termine dans un budget raisonnable (25 M+ états et ça continue, en direct dans l'app). Avec
  `BACKTRACK_MACRO=1`, il se résout en ~150 s / 1 364 579 états. Faute d'avoir laissé le défaut
  tourner jusqu'au bout, pas de ratio exact — mais l'écart qualitatif (termine / ne termine pas
  dans un temps comparable) est le signal le plus net de toute cette session.
- **Niveaux 4 et 7 : neutres.** Niveaux 8/11/12 (cibles alors non résolues) : toujours plats **au
  budget testé** — aucun solve complet obtenu dans un sens ou l'autre, le signal par-tentative
  reste le seul disponible et il ne bouge pas. ⚠️ **Périmé pour le 8** : laissé tourner sans
  budget, il se résout (cf. suite ci-dessous). Restent **11 et 12 parmi les cibles travaillées**
  — pas parmi les 33 : 22 niveaux restent non résolus (§0).

**Décision : promu en défaut.** Gain réel et gratuit (canari intact, coût nul quand pas de fork)
sur des niveaux déjà résolus, jamais négatif au-delà du bruit de mesure (7 : +0,04 %) — suffisant
pour l'activer sans attendre un effet sur les cibles, qui restent à zéro de toute façon.
**Reste ouvert** : `macroVersButBacktrack` ne réutilise pas encore la zone du 1ᵉʳ pas fournie par
l'appelant (contrairement à l'ancien `macroVersBut`, cf. §6.3 « coût par état » plus haut) — perte
de perf connue, non corrigée, sans doute quelques % à regagner.

#### ✅ Session du 2026-07-23 (suite) — LE 8 TOMBE, sans aucune modif de code

**Sans plus de modification** (code de `d7eeef5` identique à `f5ceb0e` — seul `scores.md`
diffère), le 8 **se résout** laissé tourner **sans budget** : **11 721 760 états, 238 poussées**
([scores.md](scores.md)). La file montait encore (+519 par millier) quand un but a été touché ;
la jauge affichait `rangees 0 (max 12)/18` juste avant — le nœud gagnant a complété 12→18 entre
deux impressions (`checkVictoire` fiable, 238 poussées = 18 caisses posées).

- **Même leçon que le 9 (§6.3), et c'est la TROISIÈME fois** (4, 9, maintenant 8) : « ne termine
  pas dans le budget » ne veut pas dire « mort », il veut dire **lent**. Le verdict de la suite
  ci-dessus (« toujours plat sur 8 ») était un artefact de budget (arrêté à 20 s). **Un solve
  incomplet ne prouve rien sur la solubilité — seul un solve mené au bout tranche.**
- **Des trois cibles travaillées (8/11/12), il ne reste que 11 et 12.** ⚠️ **Ne pas lire ça comme
  « il ne reste que deux niveaux »** : la carte compte **11 résolus sur 33** (§0), les 22 autres
  n'ont simplement jamais été attaqués — 10, 13 à 16 et 18 à 32 sont non résolus, sans qu'aucun
  solve complet ait été tenté sur la plupart d'entre eux.
- **Canari intact par construction** : même binaire que la table backtrack, aucune modif.
- **238 poussées est sans référence** (premier solve du 8) — c'est la solution du macro, pas un
  optimum prouvé. Pour borner l'écart : `passages 8` donne les trajets solos (§3,
  `C* = Σ trajets solos + congestion`).

#### 🎯 Session du 2026-07-24 — la macro PROMEUT-elle ses enfants ? (outil `deltaf`, à `6b0a024`)

**La question.** À `f` égal, le comparateur préfère le `g` le plus GRAND (`solveurastar.cpp`).
Une goal macro de `N` poussées enfile un enfant à `g+N` : si `h` a baissé de `N`, `f` est inchangé
et cet enfant **double tous les états du palier** — « ranger une caisse » revient à passer en tête.
C'est le mécanisme qui explique le niveau 1. Mais rien ne le garantit :

```
Δf = N + poids·Δh      Δf = 0 → PROMU sur le palier      Δf > 0 → RELÉGUÉ sur le palier suivant
```

Une seule poussée non productive au sens du couplage suffit à faire `Δf > 0` — et l'enfant part
alors **derrière toute la masse restante**, qui se compte en millions sur un niveau dominé par le
mou. **Outil neuf : `mesures/deltaf`**, qui compte Δf au moment du `push_heap` (donc sur les
enfants réellement enfilés). Canari revérifié après l'ajout du paramètre : intact.

| niveau | enfants de macro | **Δf = 0 (promus)** | **Δf > 0 (relégués)** | N moyen |
|---|---|---|---|---|
| 1 / 2 / 3 / 5 / 6 / 9 / 11 / 17 | 9 à 385 106 | **100 %** | 0 % | 6 à 17 |
| 10 | 1 (la macro ne s'engage jamais) | 100 % | 0 % | 9,0 |
| **7** | 11 991 | 81,3 % | **18,7 %** | 3,2 |
| **8** | 13 653 | 62,2 % | **37,8 %** | 9,1 |
| **12** | 151 697 | **0 %** | **100 %** | **2,00** |

- **Le mécanisme est réel et il domine** : sur 9 niveaux sur 12, **tous** les enfants de macro
  restent sur leur palier. Par comparaison, les **poussées simples** n'y restent qu'à **20-44 %**
  (le reste part à `+2`). La macro n'élague pas seulement, elle **classe** — et c'est un effet
  distinct de l'élagage, jamais mesuré jusqu'ici.
- **`Δf < 0` n'existe nulle part** (0 sur ~9 M enfants) : `h` se comporte comme cohérente en
  pratique, aucun enfant ne remonte de palier.
- **LE 12 EST À L'ENVERS, à 100 %.** Ses 151 697 enfants de macro ont **tous** `N = 2` et
  **tous** `Δf = +2`. La macro pose une caisse sur le but actif, paie 2 de `g`… et `h` ne baisse
  **pas du tout**. Chaque fois qu'elle range une caisse, l'état produit est envoyé **derrière tout
  le palier courant**. C'est l'inversion exacte de ce qui fait marcher le niveau 1.

**POURQUOI — décomposition de Δh (mesurée sur les relégués de 7, 8 et 12).** `h` est joueur-aware,
donc deux causes possibles : le joueur laissé du mauvais côté, ou le couplage qui se réarrange. On
recalcule `h` de l'enfant **avec la position du joueur du parent** pour les séparer :

| | 7 | 8 | 12 |
|---|---|---|---|
| `Δh(caisses)` moyen (vaut `−N` si la macro a fait son travail) | −2,57 (N=4,6) | −7,29 (N=9,3) | **0,00 (N=2)** |
| `Δh(joueur)` — non nul dans… | **0 %** | **0 %** | **0 %** |
| cause : **le COUPLAGE se réarrange** | **100 %** | **100 %** | **100 %** |

- **Le joueur n'y est pour RIEN — 0 cas sur ~154 000.** Contre-intuitif, et l'explication est dans
  `calculDistancePoussee()` : `regions[joueur][caisse]` est calculée sur le plateau **statique**
  avec **UNE SEULE** caisse comme obstacle. Elle ne distingue deux positions de joueur que si cette
  caisse est un **point de coupure** — ce qui n'arrive jamais entre un parent et son enfant de
  macro. ⚠️ **Vérifié par self-test avant de conclure** (le `0 %` était trop propre pour être cru,
  cf. mesure.md) : rejouer `h` depuis chaque case donne 5 à 12 valeurs distinctes, donc le
  paramètre agit bel et bien. **Le joueur-aware compte pour la VALEUR de `h`, pas pour sa
  VARIATION.**
- **La cause unique est le COUPLAGE HONGROIS.** La caisse que la macro pose sur le but actif n'est
  pas celle que le couplage y destinait : elle lui **vole** son but, la victime doit viser plus
  loin, et le total ne bouge pas. Sur le 12 la compensation est **exacte** (0,00 sur 146 522
  enfants) — sa salle de buts est un bloc 3×5 où toutes les caisses sont interchangeables, donc le
  couplage a toujours une victime à recaser au même prix.

**Ce que ça ne dit PAS.** Le 12 n'est pas résolu, et **rien ici ne prouve que la relégation est LA
cause de son échec** — c'est un signal fort, pas une causalité établie. Rien n'est cassé non plus :
A\* reste optimal (`Δf` n'est qu'un ordre de visite), et le canari est intact. Le lien avec le §3
est direct : ce classement ne peut jouer **que** dans le régime `f = C*`, donc même réparé sur le
12, il ne toucherait pas la masse `f < C*`.

#### ⏸️ Session du 2026-07-24 (suite) — le régime « BUT DU COUPLAGE » codé, mesuré, EN ATTENTE du 12

**Ce qui est codé** (non commité — git géré par l'utilisateur). Nouveau type de solveur
`Solveur::AstarMacroCouplage`, libellé **« A\* macro — but du couplage (essai) »**, qui apparaît
tout seul dans le menu de l'app (`types()`/`creer()`, le pattern prévu). `AstarMacro` est
**inchangé** à côté : les deux régimes se comparent sur le même binaire, sans variable
d'environnement. Aussi accessible en CLI : `bench <niv> couplage`.

**La règle.** Dans le régime d'engagement, la macro tente **d'abord la seule caisse que le couplage
hongrois destine au but actif** (`Game::caisseAssignee`, qui rend l'appariement déjà calculé par
`getHeuristique` au lieu de le jeter). Si elle produit un enfant, on s'y engage et on s'arrête là ;
sinon on rejoue la passe complète comme avant. **Le régime ne retire donc aucune branche, il en
PRÉFÈRE une** — et celle qu'il préfère est exactement celle qui garantit `Δh = −N`, donc `Δf = 0`.

**Mesuré (`bench <niv> couplage` contre `bench <niv> macro`, même binaire, à `6b0a024` + ce diff) :**

| niveau | `macro` (défaut) | **`couplage`** | |
|---|---|---|---|
| **190** | 2 748 386 | **268 579** | **×10,2** |
| **191** | 27 | **15** | ×1,8 |
| 9 | 1 364 579 | 1 296 392 | ×1,05 |
| 5 | 38 594 | 37 172 | ×1,04 |
| 6 | 821 | 799 | ×1,03 |
| 4 | 4 413 543 | 4 413 156 | ≈ |
| 7 | 210 925 | 210 824 | ≈ |
| 0 / 1 / 2 / 3 / 17 | — | **identiques à l'unité** | = |

- **Poussées inchangées partout** (4/97/131/134/143/110/90/213/220/250) : **canari intact**, et
  190/191 gardent leurs 220/250 poussées. Le risque annoncé — « le couplage ignore le mur interne
  du 11 et fait perdre 190/191 » — **ne s'est pas réalisé** : ce sont au contraire les deux plus
  gros gains. Le couplage et `ordreButs` ne se contredisent pas là où on le craignait.
- **Aucune régression sur aucun niveau résolu.** C'est cohérent avec la mesure `deltaf` : ces
  niveaux étaient déjà à 100 % de `Δf = 0`, il n'y avait rien à casser ni rien à gagner — sauf
  190/191, dont personne n'avait mesuré le Δf (bancs, hors des 33).
- **Le 8 : NEUTRE, et c'était prévu.** Mené au bout sur une autre machine (chiffres :
  [scores.md](scores.md), `706a801`) : **11 719 844 états / 238 poussées**, contre 11 721 760 / 238
  en régime `macro` — **−0,016 %**. Or `deltaf` y voyait **37,8 % d'enfants de macro relégués**,
  le pire taux après le 12. **La prédiction faite à partir du flux était la bonne** : sur le 8 les
  enfants de macro ne pèsent que **0,4 %** des enfants enfilés (13 653 contre ~3,4 M), donc corriger
  leur classement ne pouvait rien donner. **Le taux de relégation ne dit rien à lui seul — il faut
  le pondérer par la part de macro dans le flux.** C'est la lecture à faire pour tout futur niveau.

**🎯 LE 11 ATTEINT 11/14 — record, et il invalide une prédiction de `deltaf`.** Run utilisateur en
régime `couplage`, **arrêté à la main** vers 57,7 M dépilements (la file montait toujours, mémoire
proche des 5 Go). ⚠️ **Aucun verdict, dans aucun sens** — un arrêt manuel n'est pas un échec du
régime, exactement comme le « TIMEOUT » du 8 le même jour n'en était pas un (le 8 est tombé ensuite,
sans modif). ⚠️ **Et le 11/14 lui-même est à relativiser** : rien ne dit que cet état est SOLUBLE.
Le niveau 4 a montré le même jour qu'un chemin de record peut passer par des deadlocks non détectés
(§6.1) — **un record de caisses rangées peut donc être atteint dans une branche morte**.
`mesures/mort` saurait trancher. Dernière jauge :

```
w1 | 57673000 depiles | file 30837887 (+277 MONTE) | vus 85790194 | f 233 h(reste) 207 | rangees 0 (max 11)/14
```

- **11/14 contre 8/14**, le meilleur jamais atteint sur le vrai 11 (§6.2, 12,4 M états / 900 s).
- ⚠️ **`deltaf` avait prédit un effet NUL sur le 11** — ses 4 643 enfants de macro étaient déjà à
  **100 % de `Δf = 0`**, donc « rien à récupérer ». **La prédiction est fausse.** Le régime apporte
  sur le 11 quelque chose que la distribution de Δf ne capture pas : à retenir comme limite de cet
  outil — **Δf mesure le classement des enfants de macro, pas ce que la macro rend possible plus
  loin** (une macro qui pose la bonne caisse change l'état atteignable, pas seulement son rang).
- À nuancer : `f 233` pour `h(reste) 207` donne `g ≈ 26` — le solveur est encore très en amont, et
  `rangees 0` en régime courant dit que le 11/14 est un **pic isolé**, pas le régime établi.
- 🔴 **LE MUR MÉMOIRE EST DE RETOUR, et le §6.5 est PÉRIMÉ.** 85,8 M états vus / 30,8 M en file →
  ordre de **5 Go** (arène ~2,6 Go à 15 shorts/clé, `meilleurG` ~1,4 Go, file ~0,7 Go à 24 o par
  `SElement`, `noeuds` ≥ 0,7 Go), contre les **599 Mo** sur lesquels le §6.5 déclarait le mur
  disparu — **×10**. Les chantiers classés « sans objet » (hachage 128 bits, blocs pour
  `noeuds`/file ouverte) **redeviennent d'actualité** dès qu'un niveau va aussi loin. Estimation
  non mesurée : à confirmer par `/usr/bin/time -v` ou la RSS du run.
- [ ] Question ouverte, non mesurée : sur le 12, la macro s'engage 151 697 fois pour 3,95 M
  poussées simples. Est-ce qu'elle s'engage **trop peu**, ou **trop souvent et à mauvais escient** ?
- [ ] Coût non mesuré : ce régime paie **un hongrois de plus par état développé**
  (`caisseAssignee`). Invisible sur les gains ci-dessus, mais à regarder si un niveau ralentit sans
  que ses états baissent.

### 6.4 🧠 Le RÉSEAU DE NEURONES — comme GUIDE, JAMAIS comme coupeur

**Le fantasme, à garder tel quel.** Un RN pour orienter la recherche. Le risque fatal est le
**faux positif** : un état soluble mal noté et **élagué** rend le niveau insoluble en silence
(§5 : c'est exactement pourquoi « corral > 0 » ne se prune pas). Donc **deux formes sûres, et
deux seulement** :

- **(a) Comme GUIDE dans la file** : dé-prioriser un état suspect (le repousser dans le tas),
  **jamais l'élaguer**. Au pire on perd un peu de temps, jamais la solution. En mode optimal
  ça n'aide que le régime `f=C*` ; en mode approché ça peut faire plonger.
- **(b) Comme MINEUR de motifs hors-ligne** : on n'en retient que des **règles validées et
  prouvées sûres** (p. ex. des corrals rigoureux découverts automatiquement), jamais une
  décision de coupe apprise et opaque.

**Jamais en élagage direct appris.** La sûreté doit être *prouvée*, pas *entraînée*.

### 6.5 ⚠️ ROUVERT le 2026-07-24 — le mur mémoire est de retour sur le 11

~~Hachage 128 bits, blocs pour `noeuds`/file ouverte, beam pour borner la mémoire : **abandonnés**
— pic 599 Mo sur tout le tour.~~ **« À ne rouvrir que si un niveau futur repousse ce mur » : c'est
fait.** Le 11 en régime `couplage` passe **85,8 M états vus / 30,8 M en file**, soit de l'ordre de
**5 Go** — ×10 les 599 Mo qui avaient clos le sujet (détail du calcul en §6.3, session du
2026-07-24 suite). Le pic de 599 Mo n'était pas une propriété du solveur, c'était une propriété des
niveaux **qu'on savait finir**. Dès qu'un run va au bout de ses forces, la mémoire redevient le
facteur limitant — et sur une machine à 8 Go, c'est le swap qui arrête le solveur, pas le temps.

**Décomposition, calée sur les compteurs RÉELS du 8** (`706a801`, solve complet : `arene = 17 739 915
cles, meilleurG = 17 739 915, noeuds = 24 128 131, file = 9 759 745, capacite file = 16 777 216`) :

| poste | 8 (18 caisses, 17,7 M vus) | 11 extrapolé (14 caisses, 85,8 M vus) |
|---|---|---|
| arène (`(N+1)` shorts/clé) | 674 Mo | **2,6 Go** |
| `meilleurG` (8 o/cellule + charge) | ~270 Mo | ~1,1 Go |
| `noeuds` (8 o, **1,36 par état vu**) | 193 Mo | ~930 Mo |
| file (24 o × capacité) | 402 Mo | ~800 Mo |
| **total** | **~1,5 Go** | **~5,4 Go** |

- **L'arène domine**, et son coût par clé croît avec le nombre de caisses. C'est le premier poste à
  attaquer si le mur redevient bloquant.
- ⚠️ `noeuds` fait **1,36 entrée par état vu**, pas 1 : la goal macro pose un nœud **par poussée**
  de la chaîne (pour que `reconstruire()` la rejoue). Un régime qui allongerait les chaînes le
  ferait grossir d'autant.
- [ ] Ces chiffres restent **calculés, pas mesurés** : confirmer par `/usr/bin/time -v` (RSS réelle)
  avant de dimensionner quoi que ce soit.

---

## 7. Pièges d'implémentation à ne pas refaire

- **`getEtat()->QByteArray` = big-endian, `appliqueEtat` = natif** → `decodeCle` obligatoire
  dans tout harnais (cf. §5, le bug qui a faussé `mou`).
- **`idxCaisse` est un index de CASE** (jusqu'à 320 sur les niveaux 20×16), **pas** un rang de
  caisse → `quint16`, jamais `quint8` (débordement silencieux, canari aveugle).
- **`slots` est un mot-clé Qt** (`#define slots`) → ne jamais nommer un membre `slots`.
- **`QVector::operator[]` non-const appelle `detach()`** (copie profonde COW) → utiliser
  `.at()` dans le chemin chaud (`checkDefaite` n'est pas const : coûtait ×1,85).
- **Move ctor doit être `noexcept`** sinon `std::vector` recopie à chaque doublement.
- **Copier `gagne`/`perdu`/`nbDep…`** dans les ctors de copie/déplacement — SAUF le flag
  `traceMouvements`, délibérément absent (il décrit le Game interactif, ne doit pas se propager
  aux clones du solveur).
- **`noeuds` et `meilleurG` doivent être réinitialisés** à chaque `run()` (sinon la racine
  n'est pas à l'indice 0 → `reconstruire()` boucle).

---

## 8. Carte du code

- **`game.cpp`** — le POD de jeu. `getEtat`/`appliqueEtat` (clé arène), `pousse` (poussée
  téléportée), `checkDefaite` (`casesMortes` + gel + `dynamicDeadlock`), `calculDistancePoussee`
  (`distanceParBut` joueur-aware, `distancePoussee` ; `ordreButs` via `ordreParPrecedence` =
  précédence de livraison + **contiguité de run**, `distanceLivraison` en support ; rebours en
  fallback), `getHeuristique` (couplage hongrois + score de guidage), `macroVersBut` / `butActif`
  / `macroPeutDemarrer` (pré-test) / `avanceVersBut` (la condition de descente, exemplaire unique).
  ⚠️ **Tout est calculé au CHARGEMENT** (`calculDistancePoussee` + `calculCaseMorte` dans le ctor
  `Game(Level)`) et jamais recalculé : le solveur part d'une copie de `depart` pour hériter des
  tables en COW. Ce qui n'est PAS précalculé, c'est la macro elle-même — son *trajet* se lit dans
  `distanceParBut`, mais sa *faisabilité* dépend de l'état, donc elle est rejouée à chaque état.
- **`solveurastar.cpp`** — A\* (`poids`, `macro`). `SElement` (clé seule), `TableG`/`Arene`,
  régime d'engagement de la macro, re-développement en optimal / fermeture en pondéré.
- **`cle.h`** — `Arene` (blocs), `Cle` (offset 4 o), `TableG` (adressage ouvert).
- **`solveur.*`** — socle `QThread`, fabrique (`types()`/`creer()`), `reconstruire()`.
- **`mesures/`** — harnais externes ; `mort.cpp` (neuf) et `mou.cpp` (corrigé) pour le taux de
  deadlock.
