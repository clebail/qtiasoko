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
  (solve mené au bout, états/poussées/commit relevés). À ce jour : **15 résolus sur les 33**
  (0-11, 17, 21 et 32 — le 11 le 2026-07-28, le **10, le 21 et le 32 le 2026-07-29**, cf. §6.0).
  **Les 18 autres — 12, 13 à 16, 18 à 20, 22 à 31 — ne sont PAS résolus**, y compris
  ceux dont ce document parle beaucoup (18, 24, 25, 26 au §6.2 ; 13-16 dans les tableaux de
  diagnostic du §6.3). Apparaître dans un tableau de mesure ne veut PAS dire résolu : `mort`,
  `macro` et la jauge `rangees` tournent justement sur des niveaux qu'on ne sait pas finir.
  Hors carte : 190 et 191 sont des **bancs d'essai** (endgame du 11 isolé), résolus mais ils ne
  comptent pas dans les 33.
- ⚠️ **CORRIGÉ le 2026-07-29 — « les non-résolus n'ont, pour la plupart, jamais été attaqués »
  était FAUX.** Cette phrase figurait ici et au §6.6, et elle a orienté à tort une reprise de
  travail (« il suffirait de les lancer »). L'utilisateur les relance **régulièrement** ; aucun ne
  passe dans un temps raisonnable. Ce n'est donc pas un trou de mesure, c'est un mur. Ce qui reste
  vrai : **la frontière bouge quand les leviers changent** — le 10 et le 21 sont tombés le
  2026-07-29 **sans une ligne de code neuve**, simplement parce qu'ils n'avaient pas été relancés
  depuis que le corral-N et le plongeon existent. Relancer après chaque promotion, donc.
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
| `fp <niv> [variante]` | **(neuf, 2026-07-21) LE JUGE D'UN ÉLAGAGE** : rejoue une solution GAGNANTE et interroge le test sur chacun de ses états — tous solubles par construction, donc **toute détection est un faux positif prouvé**. À passer AVANT de câbler quoi que ce soit dans `checkDefaite`. Variantes : `-1` corral unitaire+pince, `-2` gate corral-N, **`-3` précédence par paires** (2026-07-30 — a réfuté celle-ci en une heure, 9 niveaux sur 10 en faute) |
| `macro <niv> [s]` | **(neuf, 2026-07-21)** POURQUOI la goal macro échoue : tentatives/succès, cause de l'échec, **à quel pas** il survient, et la part d'échecs survenus après un choix arbitraire de descente. Tourne à budget de temps → marche sur les niveaux jamais résolus |
| `deltaf <niv> [s]` | **(neuf, 2026-07-24)** la macro **PROMEUT-elle** ses enfants dans la file ? Distribution de `Δf = N + poids·Δh` sur les enfants enfilés, macro contre poussée simple ; `Δf = 0` = promu par le tie-break `g`, `Δf > 0` = relégué d'un palier. Décompose Δh en part **caisses** et part **joueur** |
| `usok <niv> [mode]` | **(neuf, 2026-07-27)** coût en **TEMPS** normalisé par machine (§ règle ci-dessous). Chronomètre l'étalon `bench 2 astar` ET la cible sur le même binaire, rend la cible en **USok**. `CORRAL=1 usok.sh …` = coût d'une feature sur la cible. Script `mesures/usok.sh`, pas un binaire |
| `moureel <niv> [astar]` | **(neuf, 2026-07-28) OÙ NAÎT LE MOU DE `h`** : rejoue une solution **optimale** et décompose le mou poussée par poussée. Sur un chemin optimal `C*(état) = C* − g`, donc `mou = (C* − g) − h` **sans aucun sous-solve**. Repère les poussées de RECUL (`Δh = +1`), suit le devenir de la case libérée (la même caisse revient ? une autre passe ? le joueur ?) et compte les conflits de trajets. Auto-vérifié par `Σ(1+Δh) = mou` |
| `bench <niv> <mode> record` | **(neuf, 2026-07-28)** écrit en `.xsb` **chaque état qui bat le record de caisses posées**, daté en dépilements (stderr, entrelacé avec la jauge). Vérifie que le chemin reconstruit mène bien à l'état exporté. C'est ce qui a chiffré le plongeon AVANT de le coder. `bench` accepte aussi un **chemin `.xsb`** au lieu d'un numéro |
| `ordre <niv>` | **(neuf, 2026-07-29) POURQUOI LA MACRO SE MURE** : imprime `ordreButs` (carte des rangs en base 36 + déroulé), et vérifie **deux** précédences — la **locale** du §6.2 (approches du dernier pas) et une **globale** neuve (trajet de tirage complet : *G doit précéder B si, B traité comme occupé, plus aucune caisse n'atteint G*). Statique, O(buts²×plateau), aucune recherche. À sa création : **0 violation sur les 14 résolus + 190/191, 1 à 29 sur 11 non-résolus** — depuis que la précédence globale est CODÉE (2026-07-30, §6.2), **0 violation partout**, l'outil ne sert donc plus qu'à surveiller les régressions et le murage LOCAL |
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
- ⚠️ **Chronométrer le CPU, PAS le mural** (neuf, 2026-07-31). Sur une machine qu'on utilise en même
  temps, le mural est du bruit pur : le même run (21 défaut) a rendu **254 s puis 1391 s** de mural
  pour **254 s puis 251 s de CPU**. Le CPU rejoue à moins de 1 %, le mural varie d'un facteur 5,5 —
  et le meilleur-de-3 n'y peut rien, ce n'est pas du bruit gaussien mais de la contention.
  `/usr/bin/time -l` et sa ligne `user` suffisent. ⚠️ **`usok.sh` chronomètre le mural** (builtin
  `time`, `%R`) : **à corriger**, sinon l'USok n'est pas un mètre.
- ⚠️ **Le nombre d'ÉTATS n'est pas portable entre plateformes** (neuf, 2026-07-31). À commit égal,
  le niveau 10 rend **2 160 492 états sur macOS et 2 175 724 sur Linux** (+0,70 %) : `std::sort` et
  `push_heap` n'ont pas la même implémentation entre libc++ et libstdc++, donc les ex æquo ne sont
  pas départagés pareil. **Ce qui dépend de la géométrie est portable à 0,01 %** (enclos, sous-solves,
  fraction de morts) ; ce qui dépend de la trajectoire dérive. Le piège « jamais à un chiffre écrit »
  vaut donc aussi **entre machines**, pas seulement dans le temps — et en régime plongeon la dérive
  est amplifiée (×17 sur le coût du plongeon gagnant du 10).
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

### ✅ 2026-07-28 — LE MOU OBSERVÉ SUR PIÈCES (outil `moureel`)

Le §3 avait **déduit** la congestion de quatre niveaux et d'un raisonnement. Elle est maintenant
**mesurée poussée par poussée**, exactement, et sans le moindre sous-solve : sur un chemin OPTIMAL,
`C*(état) = C* − g` par définition, donc `mou(état) = (C* − g) − h`.

**LA LOI, et elle est structurelle.** En dérivant le long du chemin, `Δh` ne vaut **jamais que −1 ou
+1** — jamais 0, jamais ±2. Une poussée est donc soit PRODUCTIVE (elle consomme une unité de trajet
solo), soit un **RECUL** qui coûte 2 (elle s'éloigne, et il faudra revenir). D'où :

> **`mou = 2 × (nombre de poussées de recul)`** — et par conséquent **le mou est TOUJOURS PAIR**.
> Les quatre valeurs du §3 (2, 2, 6, 12) le sont, ce que personne n'avait relevé.

| niveau | C\* | h(départ) | mou | reculs | mou/reculs |
|---|---|---|---|---|---|
| 1 | 97 | 95 | 2 | 1 | 2 |
| 2 | 131 | 129 | 2 | 1 | 2 |
| 3 | 134 | 128 | 6 | 3 | 2 |
| 17 | 213 | 201 | 12 | 6 | 2 |

- **Les événements de congestion sont RARISSIMES** : 0,8 % à 2,8 % des poussées. Tout le reste du
  chemin optimal est du trajet solo pur — le §3 avait raison sur les artères.
- **Et concentrés** : 5 des 6 reculs du 17 surviennent dans les **9 premières poussées**, tous dans
  la même zone (x 3-8, y 8-11). « Tout l'écart est concentré là où les caisses se démêlent » : vérifié.
- **La caisse qui recule REVIENT sur la case libérée dans 9 cas sur 10.** La déduction du §3
  (« elle s'éloigne + devra revenir ») est confirmée par observation directe.
- ⚠️ **`autour = 0` sur les 10 reculs** : aucune caisse adjacente au moment du recul. **La congestion
  n'est PAS de la densité locale**, contrairement à ce que supposait le §4.

**LES DEUX CAUSES DU MOU, enfin départagées** — le §3 les énonçait toutes deux (« assise sur le
trajet d'une autre, **ou** qui bloque le joueur ») sans jamais les séparer. Elles ne coexistent pas :
elles se répartissent PAR NIVEAU.

| niveau | reculs | **aller-retour PUR (joueur)** | **écart pour laisser passer** |
|---|---|---|---|
| 1 | 1 | **1** | 0 |
| 2 | 1 | **1** | 0 |
| 3 | 3 | 0 | **2** |
| 17 | 6 | 0 | **6** |

Sur le 1 et le 2, la caisse sort et revient **à la poussée suivante**, sans qu'aucune autre ne bouge :
la configuration des caisses est **rigoureusement identique** avant et après. Deux poussées qui ne
changent rien au plateau — **sauf la position du joueur**. Sur le 3 et le 17, aucun cas de ce type :
tous les reculs laissent passer d'autres caisses.

⚠️ **Piège §11.4, tombé dedans en direct** : après avoir vu 1 et 2, j'ai conclu « le mou est du coût
de mobilité du JOUEUR » — ce que corroborait joliment le `Δh(joueur) = 0` de `deltaf` (§6.3). Les
niveaux 3 et 17 l'ont démenti aussitôt, et ils le démentent **là où ça compte** (le 17 porte 93 % de
sa masse en `f < C*`). Deux niveaux ne font pas une loi, même quand une autre mesure semble les
appuyer.

**DEUX CANDIDATS `h` RÉFUTÉS, avec les chiffres :**

| candidat | 1 | 2 | 3 | 17 | verdict |
|---|---|---|---|---|---|
| mou réel | 2 | 2 | 6 | 12 | — |
| 2 × (caisses sur le trajet d'une autre) | 6 | 16 | 16 | — | **SURESTIME ×3 à ×8 → inadmissible** |
| 2 × (conflits CROISÉS entre paires) | 0 | 0 | 0 | 0 | **VIDE → admissible mais inutile** |

- Le comptage géométrique surestime parce qu'il **ignore le TEMPS** : une caisse ne gêne que si elle
  est encore là quand l'autre passe. Sur le 2, huit caisses se gênent géométriquement et **une seule**
  doit s'écarter — les sept autres conflits sont résolus **gratuitement par l'ORDRE de passage**.
- **D'où la vraie nature du mou : `mou` n'est pas le nombre de conflits, c'est le nombre de conflits
  qu'AUCUN ORDRE ne peut éviter.** Ça explique d'un coup pourquoi l'oracle du mou, les PDB par paires
  et les caisses-murs ont tous échoué : ils cherchaient une propriété **géométrique** là où le mou est
  le résidu d'un problème d'**ORDONNANCEMENT** — c'est-à-dire précisément ce que Sokoban a de
  PSPACE-complet (§4).
- Le conflit croisé (l'analogue du *linear conflict* du taquin : A sur le trajet obligatoire de B
  **et** B sur celui de A) est une **preuve** — mais il ne se déclenche jamais. Ça confirme et
  généralise le « 0/15 paires sur le 17 » du §4 : **le mou n'est jamais dû à un conflit symétrique
  entre DEUX caisses**, il vient d'interactions à **3+ caisses**.

**Conséquence pour la feuille de route : la « méga astuce » n'est pas une `h` plus serrée obtenue par
comptage.** Toute borne qui capturerait le mou devrait résoudre un ordonnancement optimal. Ce qui
reste : l'élagage prouvé (corral, §6.1) et le contournement anytime (plongeon, §6.0) — les deux
leviers qui ont effectivement fait tomber le 8 et le 11 aujourd'hui.

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

> 🎯 **PROCHAIN CHANTIER (2026-07-28) — LE PLONGEON-SUR-RECORD** (idée utilisateur).
> Le corral-N (§6.1 suite 3) a fait sa part : il élague le **bois mort**. Ce qui bloque
> le 11 (et vraisemblablement les 22 non-résolus) est le **DÉMÊLAGE** pur — le mur PSPACE
> (§4/§6.2). Diagnostic prouvé : l'endgame bloquant du 11 est **solvable en 9 états macro**,
> mais A\* optimal refuse d'y foncer (il doit d'abord vider toute la masse `f < C*` vivante).
>
> **L'idée** : dès qu'un état bat le **max de caisses posées** (`nouveauMaxCaisses`), le
> **prioriser** — un **plongeon greedy borné** (best-first sur `h` seul, budget d'états) qui
> tente de le compléter. Solution → gagné (SOUS-OPTIMAL). Budget épuisé → on remonte dans
> l'A\* normal. C'est la famille **anytime / diving**, la forme concrète du « repli anytime »
> du §6.3.
> - ~~**Pourquoi maintenant et pas avant** : un record pouvait être une **branche morte** (le
>   11/14 mirage). Le corral-N élague les morts → **les records sont enfin fiables** → plonger
>   devient sûr.~~ ❌ **RÉFUTÉ le 2026-07-28, mesuré (voir ci-dessous) : les records ne sont PAS
>   fiables**, même corral-N promu. Sur le niveau 4, les records **4/20 et 7/20 sont MORTS** —
>   « AUCUNE » rendu par l'**A\* pur**, qui est complet, donc espace épuisé et pas un artefact du
>   régime d'engagement ; et identique avec `CORRAL=0`, donc pas un faux positif du corral non
>   plus. Ce sont de vraies branches condamnées que le corral ne voit pas. Le bornage du plongeon
>   n'est donc PAS une précaution de confort, c'est la pièce maîtresse — et il faut prévoir de ne
>   pas replonger dans une lignée déjà condamnée (les records 1 à 7 du niveau 4 sont tous sur la
>   même branche morte : on plongerait sept fois pour rien).
> - Ce qui sauve l'affaire : **le plongeon échoue VITE sur les morts** (« AUCUNE » en < 1 s sur ces
>   fixtures en macro). Quelques milliers d'états de budget suffisent à les rejeter sans douleur.
> - **Assumé** : ça renonce à l'optimalité (record = `h` bas / `g` haut → prioriser = greedy).
>   Donc **régime d'essai SÉPARÉ** (comme couplage/pondéré), jamais le défaut : le canari des
>   résolus reste sur l'optimal. Pour les **non-résolus**, une solution sous-optimale = **première
>   résolution**, un vrai gain.
> - **À border** : un record vivant mais loin du but (le §6.3 mesure reste 5-31 au blocage) — le
>   plongeon doit être **borné** pour ne pas y perdre, d'où le repli A\*.
> - Distinct du pondéré (écarté) : chirurgical (plongeon sur record, A\* optimal entre) vs
>   gonflement global de `h`.
> - 🎯 **LE BANC, c'est le 8 — pas le 11** (idée utilisateur, 2026-07-28) : « à 1 million d'états
>   dépilés, on arrive sur un motif solvable ». Le 8 est le SEUL niveau qui mesure les **deux**
>   côtés du plongeon, parce qu'il est **résolu** (238 poussées, 4 376 070 états) : on lit le temps
>   gagné ET les poussées perdues. Le 11, lui, n'a aucune solution de référence — n'importe quel
>   résultat y serait un gain, il ne peut donc pas dire ce que le plongeon COÛTE.
> - ⚠️ **Nuance sur « ça renonce à l'optimalité » : sur le 8 il n'y a rien à renoncer.** Le régime
>   d'engagement de la macro n'est **déjà pas** optimal par construction (il ne génère que les
>   macros vers le but actif et abandonne le reste) ; 238 est la solution du macro, pas un C\*
>   prouvé (§6.3, suite du 2026-07-23). L'argument ne pèse vraiment que sur les canaris (1, 2, 17),
>   où les poussées macro coïncident avec C\*.
> - **Protocole AVANT de coder** (celui qui a prouvé le diagnostic du 11) : `nouveauMaxCaisses`
>   porte le chemin → exporter l'état de record vers ~1 M dépilements en `.xsb` (comme
>   `level0194`) et le faire résoudre seul. S'il tombe en quelques états macro, le plongeon est
>   prouvé rentable sur le 8 **sans avoir écrit une ligne de solveur**.

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

#### ✅ Session du 2026-07-27 (suite 3) — CORRAL-N par STRIP + A\* BORNÉ : le premier levier `f<C*`

**C'est l'item B, et il MARCHE.** Premier levier de tout le projet qui attaque la masse `f<C*` des
gros niveaux — là où tie-breaks, motifs locaux et goal-ordering étaient tous nuls (§3).

**L'architecture, en trois étages (chacun a un rôle distinct) :**
1. **DÉTECTION** (`detecteEnclosArrivee`, incrémental) — trouve les régions libres que le joueur
   n'atteint pas, au contact de la caisse arrivée. Coût +3-4 % USok sur le chemin macro (§ USok).
2. **GATE** (filtre, pas preuve) — ne garde que les enclos scellés avec ≥1 caisse-frontière hors
   but, **sous-dotés en buts** (Hall) et **non-rouvrables en un pas**. Ramène 40 % → 10-21 % des
   enfilages. ⚠️ **Le gate seul est FP** (juge `fp`, variante -2 : FPs dès le coup 0 sur 1/2/3/6/7 —
   le non-rouvrable un-pas rate les corrals rouvrables en plusieurs coups). Donc il **ne prune
   jamais** : il ne fait que décider **quoi soumettre à la preuve**. Un gate heuristique ne peut pas
   faire de FP puisque c'est l'A\* qui tranche.
3. **PREUVE** (`sousSolveEnclos`, mini-BFS borné) — **strip** (on retire toutes les caisses non
   frontière → relaxation valide : moins d'obstacles = joueur plus libre) puis BFS de poussées borné.
   **Mort ssi l'espace est ÉPUISÉ sous budget** = preuve (le strip relaxe, l'exhaustion prouve).
   Mémoïsé par **frontière triée** (le verdict ne dépend que d'elle, tout le reste est statique) :
   amortissement **×37 (niv 4), ×223 (niv 8)** — le même corral revient des centaines de fois, jugé
   une seule. C'est ce qui rend le coût soutenable malgré 10-21 % de déclenchement.

**Sound par construction** (pas d'échantillonnage, pas de gate faillible dans la décision) : on ne
prune que sur un `sousSolveEnclos == MORT`. **Canari intact sur les 11 résolus** (poussées inchangées).

**Budget — les morts sont PEU PROFONDES** (§6.1 confirmé). Balayage : le gain d'états **sature dès
budget ~150**, le coût de sous-solve **explose ×6 au-delà** (inconnus qui ne prouvent rien). Réglage
retenu : **`CORRAL_BUDGET=150`**.

**⚠️ UN BUG CORRIGÉ, trouvé par le niveau 11** (à retenir — il faisait rater des corrals morts). Le
test non-rouvrable identifiait les « voisins d'enclos » d'une caisse-frontière par `libre && hors
zone` — ce qui inclut les cases d'AUTRES régions scellées. Une caisse bordant DEUX enclos était
déclarée rouvrable (le joueur « entrerait » dans l'autre enclos) alors que CELUI-CI reste scellé.
Corrigé : appartenance à la **région courante** (`dansRegion` / test sur `file`). Gain immédiat :
niv 5 ×2,74 → **×3,34**, et surtout **le corral qui bloquait le 11 au coup 38 est enfin vu**.

**Gains mesurés (`CORRAL_DETECT=3 CORRAL_BUDGET=150`, états, poussées TOUTES intactes) :**

| niv | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 17 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ratio états | = | = | ×1,05 | ×1,02 | **×9,9** | **×3,3** | ×1,22 | ×1,22 | ×1,34 | **×3,4** | **×7,7** |

**USok** (le coût des sous-solves paie-t-il ?) : **net GAIN à forte incidence** (17 : **×5,6 plus
rapide** ; 4/9 idem), **neutre** au point d'équilibre (5 : ×0,96), **légère perte** à faible
incidence (7 : ×1,37, 8 : perte — déjà bien nettoyés par la pince, les sous-solves des durs
non-morts sont du coût pur). Les niveaux qui perdent sont **déjà rapides en absolu** ; ceux qui
gagnent sont les gros. La fraction de durs prouvés morts prédit tout : **40,6 % sur le 4, 24,5 % sur
le 9, ~10 % sur 7/17**.

**LE NIVEAU 11 — grande avancée, mais pas encore résolu, et on sait POURQUOI.**
- Le corral-N a rendu le 11 **6× moins cher** pour atteindre 11/14 (**14 M vus** contre 85 M avant),
  mémoire **÷2** (~2,5 Go contre 5 Go). Il élague massivement les branches mortes (dont le coup-38).
- Mais il **ne finit pas** : le blocage restant n'est PAS du deadlock, c'est le **DÉMÊLAGE**
  (§4/§6.2, le mur PSPACE). **Prouvé** : l'état bloquant exporté (endgame 11/14) est **solvable** —
  mode3 le résout en **9 états macro**. Le solveur ne « fonce » pas dedans parce qu'A\* est optimal
  et doit d'abord développer toute la masse `f < C*` vivante (le démêlage). Ce n'est ni un FP ni un
  bug : c'est l'optimalité face à un espace vivant énorme.
- Piste laissée pour demain (idée utilisateur) : un **démêlage plus savant**. Le pondéré (§2.1)
  foncerait dans le vivant mais renonce à l'optimalité — écarté par l'utilisateur.

**Outillage (branche de chantier, `CORRAL_DETECT` = interrupteur de mesure, PAS promu en défaut) :**
- `game.cpp` : `sousSolveEnclos` (mini-BFS strippé), `detecteEnclosArrivee` (détection+gate+preuve
  mémoïsée), `gateEnclosMort` (gate full-scan pour `fp`), `detecteRegionsScellees` (coût détection).
- `solveurastar.cpp` : `CORRAL_DETECT` (1=coût full-scan, 2=coût incr, 3=strip+A\* mémoïsé+prune),
  `CORRAL_BUDGET`, stats sur stderr, cache `QHash<frontière,verdict>` dans `run()`.
- `mesures/bench.cpp` : modes test `enclos` (mini-solve sur toutes les caisses) et `gate`
  (`gateEnclosMort` sur un plateau) ; `CORRAL_DBG` dumpe régions + verdicts.
- `mesures/fp.cpp` : variante `-2` (juge le gate-comme-prune → réfuté).
- Fixtures : `level0194` (endgame bloquant du 11, solvable), `level0195/0196` (parent vivant /
  corral-4 mort).

**Reste ouvert / à faire :**
- [x] **Décider la PROMOTION** — ✅ **FAIT le 2026-07-28** (session ci-dessous). Gate GARDÉ, budget
  figé à 150, `CORRAL_DETECT` retiré.
- [x] **Nettoyer** : `CORRAL_DBG`, macro `RESET_REGION`, modes test — ✅ fait avec la promotion.
- [x] **La clé du cache d'enclos ignore la position du JOUEUR** — ✅ **MESURÉ le 2026-07-29**
  (étages 0 et 1, session dédiée plus bas) : **0 faux positif prouvé** sur 3 050 transferts rejugés,
  mais **492 prunes MANQUÉS** prouvés, et 163 preuves qui **ne se transportent pas**. Correction
  elle-même toujours non faite (étage 2).
- [ ] **Le 11/12/les 22 non-résolus** : le corral-N a fait sa part (bois mort), reste le **démêlage
  savant** (§6.2) — c'est le prochain chantier convenu.

#### ✅ Session du 2026-07-28 — CORRAL-N PROMU EN DÉFAUT (et l'écart bench/UI expliqué)

**Ce qui change.** `detecteEnclosArrivee` + preuve `sousSolveEnclos` sont appelées **sans
condition** à l'enfilage, juste après le corral unitaire et sur la zone joueur que l'enfilage
calcule de toute façon. `CORRAL_DETECT` **retiré** (ses modes 1 et 2 étaient de l'instrumentation à
résultat jeté), `CORRAL_BUDGET` **figé à 150** en constante nommée (le balayage avait montré la
saturation du gain à ~150 et l'explosion ×6 du coût au-delà, en « inconnus » payés plein tarif).

**Les trois choix qui restaient ouverts, et leur raison :**
- **Le GATE est GARDÉ.** Il ne peut pas changer un verdict (c'est l'A\* borné qui tranche, cf. la
  réfutation du gate-comme-prune au juge `fp` variante -2) : il ne décide que **quoi soumettre à la
  preuve**, et ramène 40 % → 10-21 % des enfilages. Le retirer ne rendrait pas un état de plus, il
  multiplierait les sous-solves.
- **La trappe `CORRAL=0` couvre désormais les DEUX étages** (unitaire + N). Non négociable : `fp` et
  `mort` doivent solver/collecter sans aucun corral, sinon le juge est aveugle aux faux positifs
  qu'il est censé chercher — c'est déjà la raison qui avait fait ré-introduire la trappe à `66db7ca`.
- **Les stats `[CORRAL-N]` sur stderr sont GARDÉES** (elles n'étaient pas du debug de chantier) : la
  **fraction de durs prouvés morts** est ce qui PRÉDIT le gain sur un niveau neuf — 40,6 % sur le 4,
  24,5 % sur le 9, ~10 % sur 7/17. Coût nul devant le flood-fill de l'enfilage.

**Vérification — binaire contre binaire, la seule qui vaille.** Le défaut post-promotion reproduit
**à l'unité** le régime d'essai `CORRAL_DETECT=3 CORRAL_BUDGET=150` mesuré sur le binaire d'avant :
4 / 14 / 412 / 499 / 67 224 / 9 123 / 570 / 24 376 / 354 622 / 24 786, **poussées inchangées**
(4/97/131/134/355/143/110/90/237/213) — canari intact. Chiffres et commit : [scores.md](scores.md).

**⚠️ L'ÉCART BENCH / UI ÉTAIT CELUI-LÀ** (constaté par l'utilisateur, d'abord attribué à un écart
Mac/PC). L'app ne recevait **jamais** `CORRAL_DETECT=3` : lancée depuis un launcher, elle n'hérite
pas de l'environnement du shell où l'on tape les `bench`. Elle tournait donc en régime « corral
pince » pendant que les mesures tournaient en corral-N — 4 : 665 967 contre 67 224, 17 : 190 635
contre 24 786, 8 : 5,9 M contre 4,4 M. **La promotion efface l'écart par construction** : plus
aucune variable d'environnement ne gouverne l'élagage. Leçon générale au §7.

**⚠️ RÉSERVE CONNUE — la clé du cache n'inclut pas le JOUEUR. CORRECTION REPORTÉE** (décidé le
2026-07-28 : on promeut d'abord, on regardera pour corriger plus tard). `sousSolveEnclos` lit `playerPoint` (game.cpp) : le verdict dépend d'où
est le joueur, puisque c'est de là que part le flood qui décide des poussées faisables. Or le cache
est indexé par la **seule frontière triée**, sur la foi du commentaire « le verdict ne dépend que
d'elle, tout le reste est statique ». Deux conséquences, de gravité très inégale :
- **Dépendance à l'ordre d'arrivée** des états (le premier calculé fixe le verdict de tous les
  suivants de même frontière). Sans importance : « sur les gros niveaux on n'est pas à 1000 ou 2000
  états près », et le binaire reste **déterministe** (vérifié : 3 runs du 9 identiques à l'unité,
  plus un run à `QT_HASH_SEED=0` — les deux conteneurs hachés du solveur ne sont jamais itérés).
- **Risque de FAUX POSITIF**, lui non mesuré : un MORT est une preuve *pour la position de joueur
  qui a servi au calcul* ; le transférer ailleurs n'est pas justifié par l'argument du strip. Le
  risque est faible en pratique (après strip, l'extérieur de l'enclos est presque toujours d'un seul
  tenant, donc même zone ⇒ même verdict) mais **« presque toujours » n'est pas la barre du projet
  pour un élagage** — c'est la forme du piège qui a tué le couplage de Hall. Le canari n'en dit
  rien : un FP qui ne coupe pas le chemin optimal reste invisible.
- Si on veut trancher un jour : ajouter la **zone canonique du joueur** (son `getMinIdx` dans le
  board strippé) à la clé du cache et re-mesurer. Compteurs inchangés ⇒ le problème était théorique ;
  compteurs qui bougent ⇒ on a trouvé des prunes injustifiés. Coût : le taux de cache, qui est ce
  qui rend le corral-N soutenable (amortissement ×228 sur le 8).

**Nettoyage fait** : `CORRAL_DETECT`/`CORRAL_BUDGET`/`corralDetect` (`solveurastar.cpp`),
`CORRAL_DBG` et la macro `RESET_REGION` (remplacée par une lambda, `gateEnclosMort`),
`detecteRegionsScellees` **supprimée** (c'était le mode 1 — plus aucun appelant, et son
`volatile sink` n'avait de sens que pour chronométrer un résultat jeté). `gateEnclosMort` **reste**,
seul usage : le juge `fp -2`, qui documente pourquoi le gate ne prune pas. Le mode test
`bench <niv> enclos` reste aussi : `sousSolveEnclos` est désormais du code de PROD, il faut pouvoir
le rejouer sur les fixtures (`level0195` vivant, `level0196` mort).

- [x] ~~**B — Corral rigoureux comme ÉLAGAGE**~~ — **✅ FAIT ci-dessus (2026-07-27 suite 3)**, mais
  réalisé par **strip + A\* borné** (preuve par exhaustion) plutôt que par le test structurel
  « non-rouvrable + Hall » pur, qui s'est révélé **FP** (le non-rouvrable un-pas n'est pas une
  preuve). Le structurel sert de **gate** (filtre), l'A\* borné de **juge**. Zéro FP, canari intact.
- [x] ~~**A — Guidage corral-aware**~~ — **❌ FAIT ET RÉFUTÉ le 2026-07-21** (c'est le guidage par
  portes ci-dessus : ranger les poussées par corral-créé croissant). La réserve écrite ici,
  « n'aide que le régime `f=C*` », était **exacte** — et c'est précisément ce qui l'a tué.
  Ne pas le reprendre sous un autre nom.
- [ ] **Morts peu profondes** : ~30 % des deadlocks sont prouvables par un sous-solve de
  < 500–2000 états (mesuré sur le 11). Une **mini-recherche bornée** qui ne déclare mort que si
  elle **épuise** l'espace sous un petit budget est une **preuve** (pas une heuristique) → sûre.
  Reste à voir le coût par nœud (ne pas la lancer sur chaque état).

#### ✅ Session du 2026-07-29 — LA CLÉ DU CACHE SANS LE JOUEUR : mesurée en deux étages

**La réserve** (posée le 2026-07-28, correction reportée) : `sousSolveEnclos` part de `playerPoint`,
donc son verdict dépend de la position du joueur — mais le cache est indexé par la **seule frontière
triée**. Le commentaire du code affirmait « le verdict ne dépend que d'elle, tout le reste est
statique ». **C'est faux dans un solve réel**, et le transfert joue dans **DEUX sens** — le plan n'en
voyait qu'un :

| verdict caché | vrai verdict ici | effet | corriger… |
|---|---|---|---|
| MORT | pas mort | prune injustifié (le FP redouté) | fait **perdre** des états |
| vivant / **inconnu** | MORT | **prune MANQUÉ** | fait **GAGNER** des états |

La seconde ligne était l'angle mort : `game.cpp` mémoïse les **trois** verdicts sans les distinguer,
or un `inconnu` (budget 150 épuisé) est un verdict **vide**, figé ensuite pour toutes les positions
de joueur suivantes.

⚠️ **Aucun juge existant ne peut voir ce bug.** `fp` juge `corralUnitaireMort` (variante −1) et
`gateEnclosMort` (−2) — il n'exerce **jamais** `detecteEnclosArrivee` avec son cache, et ne le
pourrait pas : le bug n'existe que dans un solve où l'historique a peuplé le cache. Le canari est
aveugle aussi (un FP qui épargne le chemin optimal ne se voit pas). D'où une mesure dédiée.

**ÉTAGE 0 — compter les collisions** (`CACHE_JOUEUR=1`). À chaque cache-hit, recalculer la zone
canonique du joueur sur le board strippé et la comparer à celle du calcul d'origine. **Le verdict
rendu ne change pas** → états identiques à l'unité dans les deux régimes (vérifié sur 10 niveaux),
seul le temps bouge. Exemplaire UNIQUE du calcul (`Game::zoneCanoniqueStrip`), partagé avec
`sousSolveEnclos` : les deux **ne peuvent pas** diverger.

| niveau | hits testés | configs distinctes | amortissement | zone diff | **MORT** | inconnu |
|---|---|---|---|---|---|---|
| **9** | 170 069 | **60 498** | **×3,8** | 2 358 (1,39 %) | **183** | 2 175 |
| 4 | 25 164 | 1 122 | ×23 | 683 (2,71 %) | 0 | 683 |
| 17 | 22 982 | 383 | ×61 | 9 (0,04 %) | 0 | 9 |
| 8 | 6 100 321 | 26 832 | ×228 | 62 (0,00 %) | 0 | 62 |
| 0-3, 5, 6, 7 | ≤ 11 945 | — | — | **0** | 0 | 0 |

- **Un seul niveau sur dix transfère des verdicts MORT** : le 9. Ailleurs, le risque de faux positif
  n'a littéralement jamais l'occasion de se produire.
- ⚠️ **Prédiction RÉFUTÉE** : « plus un verdict est réutilisé, plus il a d'occasions d'être
  transféré » ⇒ le 8 (×228) devait être le cas décisif. **C'est l'inverse** — le niveau au plus fort
  amortissement collisionne le moins. C'est la **variété des frontières** qui crée les collisions
  (60 498 configs sur le 9 pour 20× moins de hits), pas la fréquence de réutilisation.
- **Coût du recalcul : NUL** — 17 macro 0,096 USok des deux côtés, 9 macro 9,608 → 9,621 (+0,13 %).
  Conséquence directe : **si on corrige la clé, le flood n'est pas le problème**, seul le taux de
  cache l'est. La piste d'optimisation « mono-composante » envisagée est donc **sans objet**.

**ÉTAGE 1 — rejuger les collisions** (`CACHE_JOUEUR=2`) : relancer `sousSolveEnclos` pour la position
RÉELLE, croiser avec le verdict caché. Le verdict **rendu** reste celui du cache — sinon on
mesurerait un autre solveur.

| niveau | collisions | `MORT→vivant` | `MORT→inconnu` | **`inconnu→MORT`** |
|---|---|---|---|---|
| 9 | 2 358 | **0** | 163 | **311** |
| 4 | 683 | **0** | 0 | **181** |
| 17 | 9 | **0** | 0 | 0 |

> ⚠️ **PIÈGE DE LECTURE, tombé dedans en écrivant l'outil : « inconnu » n'est PAS « vivant ».** Le
> premier rapport agrégeait `MORT→vivant` et `MORT→inconnu` sous l'étiquette « FAUX POSITIFS » et
> annonçait 163 sur le 9. Un `inconnu` est « budget épuisé sans conclure » — il ne prouve **rien**.
> Seul `MORT→vivant` est un faux positif, et il vaut **0**. Libellé corrigé dans `solveurastar.cpp`.

**Conclusions, asymétriques :**
1. **Aucun faux positif prouvé**, à trois budgets (150, 10 000, 100 000) sur 3 050 transferts.
2. **Mais 163 preuves ne se transportent pas.** Rejugés à budget large, les 163 `MORT→inconnu` du 9
   sont **163/163 « toujours inconnu »**, avec un coût **exactement** égal au budget (163×10 000 puis
   163×100 000) : **aucun n'a épuisé son espace**. Or ces mêmes enclos avaient été prouvés morts en
   **< 150 états** depuis l'autre position de joueur — l'espace atteignable est **> 666× plus grand**
   selon où se tient le joueur. **L'argument du plan (« après strip, l'extérieur est presque toujours
   d'un seul tenant, donc même zone ⇒ même verdict ») est RÉFUTÉ sur ces cas.**
3. **Conclure « vivant » est hors de portée de cet outil**, et c'est structurel : `sousSolveEnclos`
   est un BFS aveugle fait pour **prouver la mort par exhaustion**. Monter le budget coûte
   linéairement sans améliorer ses chances. Ne pas insister par là.
4. **Le gain, lui, est PROUVÉ : 492 prunes manqués** (311 + 181), l'exhaustion sous budget étant une
   preuve. C'est le résultat solide de la mesure — et il **renverse** la présentation d'origine, qui
   ne voyait qu'un risque.

- [ ] **ÉTAGE 2, non fait** : mettre la zone canonique du joueur dans la clé, solve complet, binaire
  contre binaire. Il supprime les 163 transferts douteux **et** récupère les 492 prunes manqués d'un
  coup. Seule inconnue : le **taux de cache**, qui est ce qui rend le corral-N soutenable.
- **État du code** : instrumentation `CACHE_JOUEUR` (0 = coupée, défaut ; 1 = étage 0 ; 2 = étage 1)
  dans `game.cpp`/`game.h`/`solveurastar.*`, plus `CACHE_JOUEUR_BUDGET` (défaut 10 000) pour
  l'arbitrage. **Elle ne fait que COMPTER** — elle n'ajoute ni ne coupe aucun comportement, donc elle
  ne peut pas faire diverger l'app du bench (§7). À retirer une fois l'étage 2 tranché.

#### ⚠️➡️❌ Session du 2026-07-29 — LE CORRAL-N COÛTE ×7 à ×25 SUR LES GROS NIVEAUX

> ❌ **LA LIGNE DU 10 EST FAUSSE, corrigée le 2026-07-31.** Le bloc `[CORRAL-N]` d'où viennent ses
> 55 001 399 états de sous-solve et ses 23,6 % de durs morts **n'appartient pas au run du 10** :
> re-mesuré sur les DEUX plateformes au même commit, le 10 rend **4,18 M états de sous-solve pour
> 0,4 % de durs morts, soit ×1,94** — et les deux machines s'accordent **à 0,01 %** là-dessus (cf.
> session du 2026-07-31). Le ×7,3 du 21, lui, se reproduit exactement et reste valide.
> **Ce chiffre a orienté deux jours de travail ; il n'a jamais existé.**

Mesuré sur les deux niveaux tombés ce jour-là, **et personne ne l'avait vu** :

| niveau | états de recherche | états de **sous-solve** | ratio | durs prouvés morts | inconnus |
|---|---|---|---|---|---|
| **21** | 2 923 006 | **21 442 865** | **×7,3** | 8,2 % | **71 %** |
| ~~**10**~~ ❌ | ~~2 175 724~~ | ~~**55 001 399**~~ | ~~**×25**~~ | ~~23,6 %~~ | ~~**68,7 %**~~ |
| **10** (vrai, 31/07) | 2 175 724 | **4 182 892** | **×1,94** | **0,4 %** | 99,5 % |

- ⚠️ **Le prédicteur du §6.1 (« la fraction de durs morts prédit le gain ») est PRIS EN DÉFAUT** : à
  8,2 % il annonçait une perte en états sur le 21 ; mesuré `CORRAL=0`, le corral y gagne **×1,66**
  (4 861 308 → 2 923 006). Un prune supprime une descendance entière — 328 757 prunes pèsent lourd
  même à faible taux.
- **Mais le COÛT, lui, empire avec la taille**, indépendamment de l'efficacité : le 10 a une fraction
  saine (23,6 %, comparable au 9) et le pire ratio. ⚠️ **Le corral-N a été promu en défaut sur un
  échantillon de niveaux à moins de 400 000 états** (4, 5, 7, 9, 17) — **aucun** de ceux où on
  l'utilise aujourd'hui. La promotion n'est pas invalidée, elle est **hors domaine de mesure**.
- **Le suspect est le taux d'INCONNUS** (68-71 %) : chacun paie le budget plein (~145 états) et ne
  prouve rien. C'est le **même chiffre** que l'étage 1 éclaire du côté du gain — un seul phénomène,
  deux symptômes : ce qui coûte est aussi ce qu'on rate.
- ⚠️ **À LIRE AVEC LA SESSION DU 2026-07-30 ci-dessous** : ces ratios disent ce que le corral
  **DÉPENSE**, pas ce qu'il **RAPPORTE**. Chronométré sur solve complet, il rend **×2,45** (niv 9) et
  **×5,7** (niv 17). Un ratio sous-solve/recherche n'est pas un verdict.
- [x] **À faire** : `CORRAL=0` chronométré (USok) sur 10 et 21. Si le corral-N y fait perdre du
  temps, il faut le **conditionner** — ce qui est exactement le §6.6 (aucun levier n'est universel).
  ✅ **FAIT le 2026-07-31** (session dédiée en fin de §6.1) : le 21 **perd ×2,13**, le 10 **gagne
  ×1,54** — et une fois les deux étages séparés, **l'étage N perd sur les deux**.

#### ✅ Session du 2026-07-30 (fin) — LE CORRAL-N PAIE, et le run BORNÉ le sous-estime STRUCTURELLEMENT

**⚠️ Chantier ouvert sur une prémisse FAUSSE, à dire d'emblée.** Il partait du ×5,1
sous-solve/recherche mesuré le matin même sur la fixture r08 du 13 (§6.2). Deux biais, tous deux
identifiables après coup :
1. **C'était de l'A\* PUR**, où les états sont bon marché et où le corral pèse donc relativement
   beaucoup plus. Le §6.1 le disait déjà : « ~0 sur le chemin macro, +6 % en A\* pur seulement ».
   Le régime de production est `coupl-plongeon`, pas l'A\* pur.
2. **Un RATIO DE COÛT n'est pas un EFFET NET.** 148 M états de sous-solve pour 29 M états de
   recherche dit ce que le corral dépense, pas ce qu'il rapporte.

**PHASE 1 — non résolus, borné 120 s, `coupl-plongeon`, défaut contre `CORRAL=0` :**

| niveau | dépilés défaut | dépilés `CORRAL=0` | surcoût/état | vus économisés | progression |
|---|---|---|---|---|---|
| 13 | 1 222 000 | 1 289 000 | +5 % | −24 % | 8 contre **10** |
| 28 | 2 673 000 | 3 111 000 | +16 % | −18 % | **13** contre 9 |
| 14 | 1 639 000 | 2 390 000 | +46 % | −36 % | égale (12) |
| 31 | 1 275 000 | 2 551 000 | **×2,0** | −56 % | égale (10) |
| 27 | 1 240 000 | 3 105 000 | **×2,5** | −62 % | égale (13) |

**Trié, et la corrélation est nette : plus le corral élague, plus il coûte, dans le même rapport.**
Logique — ce qui coûte (les sous-solves) est ce qui produit les prunes, et les 65-70 % d'« inconnus »
se paient au même tarif sans rien rendre. Lu ainsi, le corral-N ressemble à un **échange à parité** :
il divise l'espace par deux en multipliant le temps par deux. **Cette lecture est FAUSSE.**

**PHASE 2 — témoins résolus, solve COMPLET, en USok** (étalon stable sur les 4 appels : 6,73 à
6,80 s, 1 % d'amplitude) :

| niveau | USok défaut | USok `CORRAL=0` | **gain temps** | états défaut | états `CORRAL=0` | gain états | poussées |
|---|---|---|---|---|---|---|---|
| **9** | **2,673** | 6,556 | **×2,45** | 83 029 | 1 296 470 | ×15,6 | 237 = 237 |
| **17** | **0,094** | 0,535 | **×5,7** | 24 813 | 202 180 | ×8,1 | 213 = 213 |

> **LA LEÇON, et elle vaut plus que le résultat : UN RUN BORNÉ SOUS-ESTIME STRUCTURELLEMENT TOUT
> ÉLAGAGE.** Couper une branche supprime toute sa **descendance** — or cette descendance ne se
> développe que dans des états qu'une fenêtre de 120 s n'atteint jamais. D'où la contradiction
> apparente : parité sur les cinq bornés, ×2,45 et ×5,7 sur les deux complets. C'est le §6.3 pour la
> quatrième fois (« un solve incomplet ne prouve rien — seul un solve mené au bout tranche »),
> appliqué cette fois non pas à la solubilité mais à l'**évaluation d'une feature**.

- **Canari intact** : poussées identiques dans les deux régimes (237 et 213), et l'étalon rend ses
  590 066 états / 131 poussées à chaque appel.
- ⚠️ **L'ITEM N'EST PAS CLOS, et il ne faut pas lire ce tableau comme s'il l'était.** 9 et 17
  appartiennent à l'**échantillon d'origine de la promotion** (niveaux à moins de 400 000 états). Ils
  confirment que le corral paie **là où il payait déjà** ; ils ne disent rien du domaine hors mesure,
  qui est exactement ce que la session du 2026-07-29 mettait en cause.
- [x] **Ce qui ferme l'item** : solve **complet** du **10** et du **21**, défaut contre `CORRAL=0`.
  ✅ **FAIT le 2026-07-31**, session ci-dessous — et l'item est allé plus loin que sa formulation :
  `CORRAL=0` coupant les DEUX étages, il a fallu les séparer pour imputer quoi que ce soit.
- **Outillage** : `mesures/` inchangé (le protocole n'utilise que `bench` et `usok.sh`). Script de
  campagne jetable, non versionné.

#### ✅ Session du 2026-07-31 — L'ITEM FERMÉ, et l'ÉTAGE N SÉPARÉ DE L'UNITAIRE+PINCE

**L'item demandait « 10 et 21, solve complet, défaut contre `CORRAL=0` ». Fait — et la réponse
n'était pas exploitable telle quelle**, parce que `CORRAL=0` coupe les **deux** étages : elle mesure
leur somme et n'impute rien. D'où un interrupteur de chantier, **`CORRAL_N=0`**, qui coupe le seul
étage N (strip + A\* borné) en laissant le corral unitaire + pince actif. Il **coupe, il n'ajoute
pas** (§7) : le défaut reste les deux étages, l'app ne peut pas diverger du bench. Neutralité
vérifiée binaire contre binaire — états, poussées et stats `[CORRAL-N]` **identiques à l'unité** sur
le 10 et le 21.

**Le tableau, tous régimes en `coupl-plongeon`, USok sur temps CPU** (1 USok = 6,44 s ici) :

| niv | % durs morts | défaut | `CORRAL_N=0` | `CORRAL=0` | **étage N** | **unitaire+pince** |
|---|---|---|---|---|---|---|
| 4 | 22,9 % | 0,256 | 5,306 | 19,936 | **×20,7** | ×3,76 |
| 17 | 71,4 % | 0,096 | 0,526 | 0,550 | **×5,5** | ×1,05 |
| 8 | 7,9 % | 0,370 | 1,163 | 2,744 | **×3,14** | ×2,36 |
| 9 | 21,6 % | 2,812 | 6,306 | 6,828 | **×2,24** | ×1,08 |
| 5 | 41,6 % | 0,090 | 0,104 | 0,130 | ×1,16 | ×1,25 |
| **7** | 15,2 % | 0,090 | 0,067 | 0,486 | **÷1,34** | ×7,3 |
| **10** | 0,4 % | 45,41 | 37,25 | 70,41 | **÷1,22** | ×1,89 |
| **21** | 8,2 % | 38,99 | 18,71 | 18,35 | **÷2,08** | ×0,98 |

- **L'étage N perd sur les trois niveaux hors échantillon de promotion** (7, 10, 21) et gagne
  massivement sur celui-ci (4, 9, 17) plus le 8. La promotion du 2026-07-28 n'était pas fautive,
  elle était **hors domaine** — exactement la réserve du 2026-07-29, cette fois chiffrée.
- **L'unitaire+pince n'est pas le levier marginal qu'on croyait** : ×7,3 sur le 7, ×3,76 sur le 4,
  ×2,36 sur le 8, ×1,89 sur le 10 — où il fait TOUT le travail. Il ne coûte jamais rien (−2 % au
  pire, sur le 21, où son motif est absent). Le gain que le §6.1 attribuait au « corral » sur le 10
  est le sien, pas celui de l'étage N.
- **`CORRAL=0` rend une solution égale ou meilleure** sur les deux gros : 147 poussées contre 165
  (21), 542 contre 544 (10). C'est l'instabilité du plongeon, pas une propriété du corral — mais
  elle pèse dans la décision.
- Poussées inchangées partout ailleurs : canari intact dans les trois régimes.

**❌ LE PRÉDICTEUR « FRACTION DE DURS MORTS » EST RÉFUTÉ, définitivement.** Sur les sept premiers
niveaux il séparait gain et perte **sans une inversion** (tout ce qui est ≥ 21,6 % gagne, tout ce qui
est ≤ 15,2 % perd) — et **le 8 casse la loi** : 7,9 % de durs morts, sous le 21 (8,2 %) qui perd,
et il **gagne ×3,14**. Le ratio sous-solve/recherche ne prédit pas davantage (le 9 est à ×28,9 et
gagne, le 10 à ×1,94 et perd) : il mesure la dépense, pas le rendement.

> ⚠️ **Leçon de méthode, et elle a été payée en direct dans la session** : la loi « monotone sur
> sept niveaux » a été annoncée, puis cassée par le **huitième**, ajouté à la demande de
> l'utilisateur (« mesurer d'abord sur plus de niveaux »). C'est le §11.4 en une heure — dont quatre
> des sept points étaient des niveaux triviaux (0-3, 6 se résolvent en 0,01 à 0,04 s, soit 0,002 à
> 0,006 USok : **sous la résolution du chronomètre**, ils ne peuvent rien départager).

**LE CANDIDAT QUI SURVIT, et il n'est pas ajusté — il est arithmétique :**

> **états de recherche ÉPARGNÉS ÷ états de sous-solve DÉPENSÉS**

| niv | épargnés / dépensés | verdict |
|---|---|---|
| 4 | 967 336 / 100 827 = **9,6** | gain ×20,7 |
| 17 | 165 949 / 50 368 = **3,3** | gain ×5,5 |
| 8 | 178 163 / 187 096 = **0,95** | gain ×3,14 |
| 9 | 1 076 102 / 2 395 652 = **0,45** | gain ×2,24 |
| 5 | 20 640 / 47 272 = **0,44** | gain ×1,16 |
| 21 | 1 928 614 / 21 442 865 = **0,090** | perte ×2,08 |
| 7 | 4 932 / 64 431 = **0,077** | perte ×1,34 |
| 10 | 87 976 / 4 183 492 = **0,021** | perte ×1,22 |

**Monotone sur les huit**, et le seuil n'est pas un réglage : c'est le **rapport de coût entre un
état de sous-solve et un état de recherche**. Estimé depuis les temps mesurés — 10 : 14,8 µs contre
106,7 µs ; 21 : 8,3 contre 24,8 ; 8 : 8,1 contre 37,2 — il vaut **0,14 à 0,34**, et il tombe dans le
trou observé entre 0,090 (perte) et 0,44 (gain). La bascule est **expliquée**, pas constatée.

- ⚠️ **Mais ce n'est PAS un gate** : « états épargnés » n'est connaissable qu'en faisant tourner les
  deux régimes — en vol, le solveur connaît ses prunes, pas la descendance qu'ils ont supprimée.
  Outil d'ANALYSE. Conditionner l'étage N demanderait d'abord un **estimateur de descendance
  épargnée**, chantier distinct, à discuter avant de coder.

**⚠️ DEUX CORRECTIONS DE MÉTHODE, toutes deux mesurées :**

1. **Le temps MURAL est inutilisable sur une machine partagée, `usok.sh` le chronomètre pourtant.**
   Le même travail (21 défaut) a rendu **254,41 s puis 1390,80 s** de mural pour **253,80 s puis
   251,09 s de CPU**. Le CPU rejoue à moins de 1 % sur les quatre runs, le mural varie d'un facteur
   5,5. Toutes les mesures ci-dessus sont donc en **temps CPU** (`/usr/bin/time -l`, `user`), écart
   assumé au §1. `usok.sh` utilise le builtin `time` (`%R` = mural) : **à corriger** si on veut que
   l'USok reste un mètre.
2. **Deux tirages, pas trois** — justifié par la mesure et non par le confort : l'écart CPU entre
   tirages est de 0,5 à 1,1 %, très en dessous du bruit de 3 % que la règle des 3 tirages combat.
   L'étalon, lui, est bien en meilleur de 3 (6,476 / 6,498 / 6,490 s).

**⚠️ ET UN PIÈGE NEUF : le nombre d'états N'EST PAS PORTABLE** (constaté par l'utilisateur, qui a
rejoué le 10 sur sa machine Linux et dans l'app). À commit égal, sur le même niveau :

| | macOS | Linux | écart |
|---|---|---|---|
| états explorés | 2 160 492 | 2 175 724 | **+0,70 %** |
| durs jugés | 3 456 570 | 3 474 427 | +0,52 % |
| configs distinctes / états de sous-solve | 27 947 / 4 183 492 | 27 943 / 4 182 892 | **−0,01 %** |
| **plongeon gagnant** | **259 états** | **4 339 états** | **×17** |

- **Ce qui dépend de la GÉOMÉTRIE est portable à 0,01 %** (configurations d'enclos, états de
  sous-solve, fraction de durs morts) ; ce qui dépend de la **TRAJECTOIRE** dérive de ~0,6 %.
  L'ordre de dépilement départage autrement les ex æquo — `std::sort`/`push_heap` n'ont pas la même
  implémentation entre libc++ et libstdc++.
- **Le plongeon amplifie violemment cette dérive** : même record 13/32, atteint 15 000 états plus
  tard, complété en 4 339 états au lieu de 259. Nouvelle illustration de l'instabilité du §6.3.
- **Le run de l'app a reproduit le bench au chiffre près** (états, enfilages, durs, morts,
  sous-solve, cache, et les douze lignes `[plongeon]` budgets compris) : l'écart app/bench du §7 a
  bien disparu avec la promotion, c'est vérifié de l'extérieur.

**État du code** : ⚠️ **RIEN.** L'interrupteur `CORRAL_N` qui a produit ces chiffres était un
outil de chantier (`solveurastar.cpp`, +16/−5 : une constante `corralNActif` et deux gardes) ; il a
été **retiré avant le merge**, le corral redevenant inconditionnel comme avant. Les mesures, elles,
restent valides — elles ont été prises binaire contre binaire, défaut vérifié identique à l'unité.
**Le re-dériver est un travail de dix minutes** : les deux étages vivent déjà dans deux blocs
`if (corralActif …)` distincts (corral unitaire puis corral-N à l'enfilage, plus le même couple dans
`plonge()`), il suffit d'ajouter une seconde constante au second de chaque paire. Ne pas le
reproposer comme feature : il ne COUPE qu'un élagage, il n'ajoute rien, et le défaut ne doit pas
dépendre d'une variable d'environnement (§7).

**Reste ouvert :**
- [ ] **Conditionner l'étage N**, bloqué faute d'observable en vol (cf. ci-dessus). Tant que ce n'est
  pas tranché, le défaut reste les deux étages — il gagne sur 4/5/8/9/17 et perd sur 7/10/21.
- [ ] **Corriger `usok.sh`** pour chronométrer le CPU et non le mural.
- [ ] **Les 18 non-résolus ne sont pas plaçables sur cet axe** : les stats `[CORRAL-N]` ne
  s'impriment qu'en **fin** de `run()`, donc un run tué n'en rend aucune — même piège que le
  profilage du §6.6, qui ne peut relever que ce qui part en continu. Les faire partir
  périodiquement avec la jauge est un ajout de trois lignes, sans effet sur aucun verdict.
- [ ] **11 et 32 laissés de côté** (décision utilisateur) : leurs runs `CORRAL=0` sont d'ordre de
  grandeur inconnu et un run interrompu ne rendrait aucun verdict.

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

#### 🎯 Session du 2026-07-29 — LE GOAL-ORDERING N'AVAIT PAS FINI SON TRAVAIL (outil `ordre`)

**La phrase à retirer.** La session du 2026-07-20 (suite 2) se clôt sur « chantier bouclé » et
« le goal-ordering a fini son travail ; ce qui reste est l'acheminement et le démêlage ». **C'est
faux, et ça a détourné neuf jours de travail vers le corral et le plongeon.** Mesuré aujourd'hui :
**11 des 19 niveaux non résolus ont un ordre de remplissage DÉFECTUEUX**, dont trois par un bug de
la règle déjà codée.

**Ce qui a déclenché la mesure** : l'utilisateur exporte l'état d'un run du **27** arrêté à
**17/20 caisses posées** (85 %, « de bon espoir »). Rejoué seul, il rend **`AUCUNE 0`** — zéro état
développé, l'état est mort à la racine. Vérifié `CORRAL=0` : **ce n'est pas le corral**. La cause se
lit à l'œil sur le plateau : les caisses posées sur les deux premières rangées **ferment l'accès aux
trois buts restants**, plus aucun retournement n'est possible. Le solveur n'a pas échoué à chercher,
**il a exécuté un ordre qui condamnait la partie**.

**L'outil (`mesures/ordre`, cf. §1).** Statique, aucune recherche. Il imprime l'ordre calculé (carte
des rangs en base 36 sur le plateau — le format qui rend le défaut visible d'un coup d'œil) et teste
DEUX précédences :
1. **LOCALE** — celle du §6.2, déjà codée : à son tour, ce but a-t-il encore une approche (caisse en
   `G−d`, joueur en `G−2d`) dont aucune case ne porte un but déjà rempli ?
2. **GLOBALE, neuve** — le **trajet de tirage complet**, exactement ce que le §6.2 réclamait sans
   jamais le faire (« le rebours doit simuler le trajet de tirage, pas juste le premier pas ») :

   > **G doit précéder B si, en traitant B comme OCCUPÉ, plus aucune caisse du départ n'atteint G.**

   BFS de tirage à rebours depuis chaque but, une fois par autre but marqué obstacle.
   `O(buts² × plateau)` ≈ 100 000 opérations sur le 27 — négligeable, et **statique**, donc
   calculable au chargement comme `casesMortes`. **C'est une ARÊTE de précédence, pas un score** —
   la forme qui a marché le 2026-07-20, pas celle qui a échoué le 19.

⚠️ **Relaxation OPTIMISTE** (joueur supposé capable d'atteindre n'importe quel appui, aucune autre
caisse) : **une violation est une PREUVE, un « 0 violation » ne promet rien.** Le 12 l'illustre —
ordre parfaitement sain, et il échoue quand même (son problème est ailleurs, cf. §6.3).

**LE DISCRIMINANT — c'est le résultat de la session :**

| | violations |
|---|---|
| les **14 résolus** + 190 + 191 | **0 partout** |
| **11 non résolus sur 19** | 1 à **29** |

Le test n'est pas creux : il produit **1 à 99 arêtes** de précédence selon les niveaux (28 sur le 11,
32 sur le 21, 99 sur le 10). Il est simplement **silencieux là où l'ordre marche**. Seize niveaux qui
passent contre onze qui ne passent pas, sans un seul faux positif.

**DEUX FAMILLES, et elles ne se soignent pas pareil :**

| famille | niveaux | ce que c'est |
|---|---|---|
| **A — murage LOCAL** | **22, 29, 32** | la règle **déjà codée** est violée par sa **propre sortie** → **BUG** de `ordreParPrecedence`, pas une limite théorique. Le 22 se mure au rang 25 sur le but (12,9) |
| **B — murage GLOBAL** | 13, 15, 18, 20, **23**, **25**, 27, 28 | précédence locale OK, trajet de tirage ignoré → la limite connue. Massif sur le **23 (26 arêtes violées)** et le **25 (29)** |

**Le cas du 27, en clair** : ses trois buts du haut sont classés aux rangs **17, 18, 19** (les
derniers), alors que **(6,2), rempli au rang 16, est leur passage obligé**. L'ordre remplit de bas en
haut ; il fallait l'inverse.

**Reste à faire :**
- [x] **Famille A** — ✅ **CORRIGÉE le 2026-07-29** (session ci-dessous). Cause : le glouton se
  peignait dans un coin puis FORÇAIT. Corrigé par backtracking borné ; **le 32 redevient sain, aucun
  autre ordre ne bouge**. ⚠️ Le 22 résiste, et deux modes d'échec neufs sont apparus (voir plus bas).
- [x] **Famille B** — ✅ **FAIT le 2026-07-30** (session dédiée ci-dessous) : `precedenceGlobale()` +
  tri topologique stable. 0 violation sur les 35 niveaux, 30 ordres inchangés à l'octet, canari
  intact. ⚠️ Neutre au profilage borné — corrigé n'est pas débloqué.
- [ ] ⚠️ **Ne pas conclure que l'ordre EST la cause** des 11 échecs — c'est une corrélation sur un
  test optimiste. Le vrai juge sera : corriger, relancer, compter les niveaux qui tombent.
- [x] ⚠️ **Le correctif MULTI-SALLES perd son cas d'école** : le **10 est tombé le 2026-07-29 sans
  lui** (2 175 724 états, 544 poussées), alors que tout le §6.2 le présentait comme le niveau qui
  l'exigeait. Son ordre calculé est d'ailleurs **sain** (99 arêtes, 0 violée). Le correctif reste
  ouvert pour 18/24/25/26, mais **déclassé en priorité**.

#### ✅ Session du 2026-07-29 (suite) — FAMILLE A CORRIGÉE : le glouton ne force plus, il recule

**La cause, isolée par une trace jetable.** Quand plus aucun but n'est « sûr » (poser n'importe
lequel condamne un autre), l'ancien code faisait `surs = candidats` et posait **quand même**. Mesure
du nombre de relâchements :

| relâchements | niveaux |
|---|---|
| **> 0** | 27 (7), 22 (6), 32 (6), 25 (2) |
| **0** | 1, 3, 7, 10, 21, 23, 29, 190, 191 |

**Aucun niveau résolu ne relâche jamais.** Le relâchement est donc le symptôme exact du murage, pas
un incident bénin — et il n'y avait aucune raison de croire qu'il fût rare.

**La correction** : empiler les candidats sûrs **triés par le tie-break** et **RECULER** au lieu de
forcer (`ordreParPrecedence`, game.cpp). C'est la même correction que `macroVersButBacktrack`
(§6.3) : mémoriser les forks au lieu de les oublier. Sur un niveau qui ne relâche jamais, la pile ne
recule jamais et le premier candidat est toujours retenu → **ordre identique, canari intact PAR
CONSTRUCTION**.

⚠️ **Ce n'est pas la tentative n°5 du 2026-07-19** (« backtracking : rend le MÊME ordre que le
greedy »). Celle-là portait sur le 191, où la garde ne se relâche jamais : la recherche n'avait rien
à explorer. Ici on ne recule que sur un échec avéré du modèle.

**⚠️ DEUX BUGS DE PREMIER JET, tous deux instructifs :**
1. **Le repli était PIRE que l'existant.** En cas d'échec, je reprenais le plus long préfixe atteint
   (`meilleurOrdre`) — un chemin d'**exploration**, pas un ordre réfléchi. Résultat mesuré : le **27
   passait de sain à muré au rang 18**, le 25 de muré au rang 10 à **muré au rang 1**. Corrigé en
   refaisant *exactement* l'ancien glouton relâché : **le backtracking est un BONUS, il ne peut
   qu'améliorer.** Règle générale : un ajout dont le cas d'échec n'est pas *identique* à l'existant
   n'est pas un ajout, c'est un remplacement.
2. **Le budget coûtait ×10 au CHARGEMENT.** `ordreParPrecedence` tourne dans le ctor `Game(Level)` —
   donc à chaque ouverture de niveau dans l'app. À `200×nbButs`, le 22 passait de **0,51 s à 5,11 s**.
   Balayé (méthode `CORRAL_BUDGET`) :

   | budget | 32 | temps de chargement du 22 |
   |---|---|---|
   | 50 | muré | 0,11 s |
   | **200** ✅ | **SAIN** | **0,23 s** (moins que les 0,51 s d'origine) |
   | 1000 | sain | 0,91 s |
   | 5600 | sain | 4,67 s |

   **Figé à 200** : au-delà, on paie sans rien gagner.

**Vérification — ordre par ordre, binaire contre binaire** (les 33 niveaux + 190 + 191, cartes de
rangs comparées) : **UN SEUL ordre change, celui du 32.** Canari revérifié au solveur
(4/97/131/134/143/110/90/213, états inchangés).

**Résultat, et il est maigre — 1 niveau sur 4 :**

| niveau | avant | après |
|---|---|---|
| **32** | muré rang 14 | **SAIN** ✅ |
| 22 | muré rang 25 | muré rang 25 |
| 25 / 23 / 13 / 15 / 18 / 20 | murés | murés |

**⚠️ LE VRAI DIAGNOSTIC EST AILLEURS, et c'est le résultat important de la session.** Pourquoi le
backtracking n'aboutit-il pas ? Deux modes, mesurés :

| mode | niveaux | symptôme |
|---|---|---|
| **A — budget épuisé** | 22, 13, 15, 27 | atteint 11 à 19 buts, budget à 0 |
| **B — échec au RANG 0** | **25, 23, 18, 20** | **0 but posé**, budget quasi intact |

Le mode B est un aveu du modèle : **dès le premier rang, la garde estime que TOUT choix condamne un
but** — sur des niveaux pourtant solubles. `distanceLivraison` est donc **trop PESSIMISTE**, et la
cause est connue et déjà écrite au §6.1 : elle ne retient qu'**UNE** position de joueur par case
atteinte (`joueurApres[a] = c`), exactement le défaut qui avait produit 86 faux positifs au test
« but orphelin ». Elle rate des routes, déclare des buts non livrables, la garde refuse tout, et le
glouton relâche.

- [x] **La correction de fond : rendre `distanceLivraison` joueur-aware**, indexé par
  **(case, région joueur)** comme `distanceParBut` l'est depuis le §2.2. C'est la troisième fois que
  ce même défaut est identifié dans ce projet (§6.1 pour `butNonLivrable`, ici pour la garde).
  ✅ **FAIT et committé** (`816412d`, constaté le 2026-07-30 — le code avait pris de l'avance sur ce
  document). Effet mesuré : le **mode B est guéri**, 20/23/25 ne violent plus aucune arête et ne
  sont plus murés. ⚠️ **Coût non documenté** : le chargement du 22 passe de **0,23 s à 1,78 s** (×8).
  C'est le ctor `Game(Level)`, donc payé à chaque ouverture de niveau dans l'app.

#### ✅ Session du 2026-07-30 — FAMILLE B PORTÉE DANS LE SOLVEUR (précédence globale)

**⚠️ La ligne de base avait bougé, et le tableau du 2026-07-29 était périmé** (il précédait le
correctif joueur-aware ci-dessus). Re-mesuré avant de coder quoi que ce soit :

| | plan (29/07) | **mesuré le 30/07** |
|---|---|---|
| famille B (arêtes violées) | 8 niveaux, jusqu'à 29 arêtes | **5 niveaux : 13, 15, 18, 27, 28 — 2 à 3 arêtes** |
| 20 / 23 / 25 | violés (25 → 29 arêtes) | **0 violation** |
| famille A (murage local) | 22, 29, 32 | **22 (rang 25), 13 (14), 15 (14), 18 (10)** |

**Le code, deux pièces, aucune variable d'environnement (§7) :**
1. **`Game::precedenceGlobale()`** (game.cpp) — la règle du §6.2 enfin codée : BFS de tirage à
   rebours depuis chaque but, une fois par autre but marqué occupé. Rend `requis[B]`. Statique, donc
   calculé une fois au chargement comme `casesMortes`. **Coût mesuré : nul** (identique à la
   centiseconde sur 10/22/24/25/27/31/13/32 — noyé dans les `distanceLivraison` du glouton).
2. **Clé de tête du tie-break** + **TRI TOPOLOGIQUE STABLE en post-passe**.

**Le diagnostic qui a décidé du design, et il n'était pas prévu.** Le tie-break seul ne corrigeait ni
le 27 ni le 28. Cause trouvée par trace jetable : sur le 27, les buts (2,1)/(3,1)/(4,1) ne sont
**JAMAIS livrables** selon `distanceLivraison`, le glouton ne les choisit donc jamais, et la boucle
de complétion (« les buts jamais livrables ferment la liste ») les colle en **fin** de liste — aux
rangs 17-19, alors que (6,2), leur passage obligé, est posé au rang 16. **Leurs rangs n'étaient pas
un choix, c'était un résidu.** D'où la post-passe, qui les remonte à leur place.

> **Le tri est STABLE au sens fort** : on émet toujours le premier but dont tous les prédécesseurs
> sont déjà émis. Donc un ordre qui respecte déjà ses arêtes ressort **inchangé** — l'identité, et
> donc le canari, sont préservés **par construction**, pas par réglage.

**Mesuré (binaire contre binaire, worktree sur `HEAD`) :**

| juge | résultat |
|---|---|
| arêtes violées, 35 niveaux | **0 partout** |
| cartes de rangs | **30 identiques à l'octet**, 5 changent (13, 15, 18, 27, 28) |
| canari solveur (0-9, 17, 190, 191) macro | **états ET poussées identiques à l'unité** |
| murage local du 15 | **14 → sain** |

- ⚠️ **Variante FILTRE DUR codée puis RETIRÉE** (garde `attente == 0` sur les sûrs + couches de
  raffinement dans le repli) : **cartes de rangs identiques sur les 35 niveaux**, donc strictement
  **inerte**. Consignée dans le code pour ne pas être reproposée sans un cas qui la distingue.
- ⚠️ **Le gain est NEUTRE au profilage borné** (120 s, `coupl-plongeon`, ref contre new) : 13 (9/16),
  15 (10/15), 27 (13/20), 28 (13/20) **inchangés** ; seul le **18 passe de 8/11 à 9/11**. La crainte
  inverse ne s'est pas réalisée non plus — remonter des buts jamais livrables n'empêche pas la macro
  de s'engager (le 27 atteint toujours 13/20). **Correction gratuite et prouvée, pas un déblocage.**
- [ ] **Le vrai juge reste à passer** : relancer 13/15/18/27/28 **sans budget**, compter ceux qui
  tombent. C'est la règle que le §6.2 s'était donnée (« ne pas conclure que l'ordre EST la cause »),
  et le §6.3 l'a vérifiée trois fois : « ne termine pas dans le budget » veut dire **lent**, pas mort.
  ⚠️ **Entamé sur le 13** (session ci-dessous) : run `coupl-plongeon` arrêté par un plantage de
  terminal à 7,19 M dépilés / 17,8 M vus, `max 9/16`, file **+2051 qui MONTE**. Aucun verdict.

#### ❌ Session du 2026-07-30 (suite) — LE 13 JUGÉ : ordre SAIN, niveau muré quand même

**Le point de départ** : passer le « vrai juge » ci-dessus sur le 13, premier des cinq niveaux dont
l'ordre a changé. Il n'est pas tombé, mais il a livré trois faits et une réfutation.

**FAIT 1 — l'ordre du 13 est SAIN, et ça ne suffit pas.** `ordre 13` : **0 violation locale, 0
violation globale** (4 arêtes de précédence). Le correctif du jour n'avait rien à corriger ici, et le
niveau se mure quand même. **Le goal-ordering n'est pas le verrou du 13.**

**FAIT 2 — deux records du 13 sont MORTS, prouvés par EXHAUSTION** (A\* pur, donc complet, relancé
depuis les fixtures de `bench <niv> <mode> record`) :

| record | posées | verdict | états | prunes corral |
|---|---|---|---|---|
| **r09** | 9/16 | `AUCUNE`, espace épuisé | **5 191 833** | 3 181 508 sur 28,9 M enfilages (67,5 % des durs) |
| **r08** | 8/16 | `AUCUNE`, espace épuisé | **29 032 799** | 653 093 sur 162,8 M (5,2 % des durs) |
| r07 | 7/16 | **arrêté à la main, AUCUN VERDICT** | 43,9 M dépilés / 64,2 M vus, 4,1 Go | — |

- ⚠️ **r07 : arrêt manuel ⇒ aucun verdict, dans aucun sens** — même règle que le 8 et le 31. Sa file
  MONTAIT encore (+864 par millier) quand il a été coupé, à l'inverse de r08 dont la file plafonnait
  à 1,9 M avant de drainer. Il ressemblait à un espace vivant **ou** simplement énorme ; on ne sait pas.
- **Le verdict de r08 ne repose quasiment pas sur le corral** (653 k prunes pour 29,0 M états
  explorés, 2,2 %), là où celui de r09 en dépendait lourdement. Corroboration indépendante bienvenue.
- **Coût du corral-N sur r08, et il alimente l'item ouvert du §6.1** — ⚠️ cité alors comme « ×7 à
  ×25 sur les gros niveaux » : **le ×25 du 10 est FAUX** (corrigé le 2026-07-31, vrai ratio ×1,94),
  seul le ×7,3 du 21 tient : **148,1 M états de sous-solve pour 29,0 M états de recherche, ×5,1**, avec **64,6 %
  d'inconnus** et 5,2 % de durs prouvés morts. Plein tarif pour presque rien.

**FAIT 3 — LES RECORDS NE SUIVENT PAS `ordreButs`, et c'est là qu'est le trou.** Rangs réellement
posés (l'ordre calculé va de 0 à 15) :

| record | r02 | r06 | r07 | r08 | r09 |
|---|---|---|---|---|---|
| rangs posés | `0,1` | `0,1,6,9,14,15` | `0,1,4,6,9,14,15` | `0,1,3,4,6,7,11,15` | `0,1,3,4,5,6,7,11,12` |
| préfixe de l'ordre ? | **oui** | non | non | non | non |

> **`ordreButs` ne pilote que le BUT ACTIF DE LA MACRO. Les poussées simples posent une caisse sur
> n'importe quel but, à n'importe quel moment.** Un ordre prouvé correct n'est donc jamais *appliqué* :
> le solveur pose les rangs 3, 4, 6, 7, 11 et 12 en laissant le **rang 2 vide**, et c'est ça qui scelle.

Le motif, lisible sur le plateau : **(14,6) est enclavé entre les murs (13,6) et (15,6)**, donc ses
seules approches sont verticales — appui joueur en (14,4) par le haut, en (14,8) par le bas. Il faut
occuper les **deux** pour le condamner ; c'est le cas dès r08. Même motif ailleurs dans la salle :
(15,9) entre (14,9) et (16,9), (15,3) entre (14,3) et (16,3).

**❌ LA PRÉCÉDENCE PAR PAIRES — proposée, codée en diagnostic, RÉFUTÉE LE JOUR MÊME.**

`precedenceGlobale()` ne teste qu'**un** bloqueur : elle est structurellement aveugle à ce motif (aucun
but SEUL ne ferme les deux routes de (14,6) — elle rend d'ailleurs 0 arête vers lui). La
généralisation évidente : *{B1,B2} affame G si, B1 ET B2 traités comme occupés, plus aucune caisse
n'atteint G*. Elle **retrouve exactement** `{(14,4),(14,8)} → (14,6)` sur r08 et r09, en quelques
millisecondes de calcul statique, là où A\* a mis 34 M d'états.

**Et elle est fausse.** Juge `fp -3` (rejeu de solutions GAGNANTES ⇒ toute détection est un faux
positif PROUVÉ), macro, niveaux résolus :

| niveau | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 9 | 17 |
|---|---|---|---|---|---|---|---|---|---|---|
| poussées | 4 | 97 | 131 | 134 | 355 | 143 | 110 | 90 | 237 | 213 |
| **faux positifs** | **0** | 1 | 2 | 4 | **10** | 6 | 4 | **14** | 6 | 1 |

**9 sur 10 en faute** ; seul le 0 est propre, et il fait 4 poussées. Le mécanisme était écrit d'avance
et il se réalise : l'arête suppose « B1 et B2 RESTENT occupés », or les solutions **ressortent des
caisses des buts** (§4 : le parking temporaire et le ressortir-d'un-but sont indispensables). Les
records du 13 le font eux-mêmes — (14,9) est posée dans r04–r08 et **plus du tout** dans r09.

- **La version renforcée « exiger B1 et B2 IMMOBILES » ne sauve rien** : le plus grand point fixe de
  gel (sain par induction sur *le premier instant où une caisse du bloc bouge*) **s'effondre à vide**
  sur r09 — la caisse (16,5) peut sortir vers (15,5), ce qui décoince (16,4), puis (16,3), puis toute
  la cascade. Vérifié à la main avant de coder.
- **Le repli « dé-prioriser au lieu de couper » (§6.4a) ne mord pas non plus** : le §3 dit qu'un
  guidage ne touche pas la masse `f < C*`, qui est exactement ce qui bloque le 13.
- ⚠️ **Ça invalide rétroactivement les lectures de r04 à r07** faites avec ce prédicat (il criait dès
  4 caisses posées). **Ne survivent que r08 et r09, prouvés par A\*.**

**DEUX LEÇONS DE MÉTHODE :**
1. **La précédence est un objet d'ORDONNANCEMENT, pas de SOLUBILITÉ.** « Si tu remplis B avant G, tu
   ne pourras plus remplir G » est vrai comme conseil d'ordre et faux comme test de mort, parce que
   rien n'oblige B à rester rempli. Les confondre, c'est le piège « caisses = murs » sous un cinquième
   déguisement (gel naïf, `h` qui soustrait, caisses=murs, gelées=murs — et maintenant
   buts-remplis=murs). **Le juge `fp` l'a tranché en une heure, sans une ligne dans le solveur.**
2. **Un ordre PROUVÉ CORRECT ne vaut que s'il est APPLIQUÉ.** Tout le §6.2 depuis le 2026-07-19 porte
   sur la qualité de `ordreButs` ; personne n'avait vérifié que le solveur le suivait. Il ne le suit
   pas, et il ne peut pas — les poussées simples ne le lisent jamais.

**État du code** : `mesures/precedencepaires.h` (NEUF — BFS en exemplaire unique, partagé
`ordre`/`fp`, entête abondamment marqué RÉFUTÉ pour qu'il ne ressorte pas), `mesures/ordre.cpp`
(section paires + délégation à l'entête ; le verdict affiché est un **indice**, avec les chiffres de
FP en clair), `mesures/fp.cpp` (variante **-3**). **Rien dans le solveur, aucune variable
d'environnement** (§7). Fixtures : `mesures/build/bench/record_niv13_r01..r09*.xsb`.

**Reste ouvert :**
- [ ] **Le 13 n'est toujours pas jugé** : aucun run mené au bout. r07 à relancer si on veut savoir à
  partir de combien de caisses posées la partie est condamnée — prévoir des heures et surveiller la
  RSS (4,1 Go à 30 min, +2 Go par quart d'heure, machine à 18 Go).
- [ ] **La vraie question soulevée par le FAIT 3, et elle est neuve** : peut-on faire RESPECTER
  l'ordre aux poussées simples ? ⚠️ La forme naïve est **déjà réfutée** (§4 : « interdire de remplir
  dans le désordre » rend le niveau 1 **insoluble**) — donc ne pas la reprendre telle quelle. À
  discuter avant de coder quoi que ce soit. ➡️ **Repris le 2026-07-31, cf. ci-dessous.**

#### ⏸️ Session du 2026-07-31 — LE TEST EN DUR RETIRÉ, l'escalade RÉFUTÉE, la piste déplacée

**Ce qui est parti.** `game.cpp` portait `int budget = (numNiveau == 13) ? 100000 : 200;` dans
`ordreParPrecedence` — un **numéro de niveau en dur dans le solveur**, daté du 2026-07-30 et marqué
« À RETIRER ». Retiré. ⚠️ **Il portait la prémisse du §6.2** : le « FAIT 1 — l'ordre du 13 est SAIN »
n'était vrai que grâce à lui. Sans lui, le 13 est **muré au rang 14 sur le but (16,5)**, et le
chargement retombe de **63,94 s à 0,84 s**.

**L'escalade de budget, essayée et réfutée le jour même** (relancer une fois à 100 000 quand la pile
se vide alors que le budget est à 0 — donc *tronqué* et non *épuisé*, une distinction que le code
sait déjà faire) :

| niveau | résultat | chargement |
|---|---|---|
| **13** | ✅ devient **sain** | **65,02 s** |
| **18** | ❌ muré au rang 10 — **il n'escalade même pas** | 0,16 s |
| **22** | ⏸️ **> 9 min, arrêté sans finir** | — |

- ⚠️ **Le murage du 18 n'est PAS un coût de recherche** : son espace est réellement épuisé à
  budget 200. **Aucun ordre sain complet n'existe dans ce modèle**, aucun budget n'y changera rien.
  C'est une information neuve, et elle sépare deux causes qu'on confondait.
- **L'escalade échange un numéro en dur contre un temps de chargement NON BORNÉ**, payé dans le ctor
  `Game(Level)` donc à chaque ouverture de niveau dans l'app. Retirée. Ne pas la reproposer sans
  traiter d'abord le coût de `distanceLivraison`, rappelée pour chaque candidat de chaque rang.
- **Seuls trois niveaux sont murés** (balayage des 33 + 190/191) : **13** (rang 14), **18** (rang 10),
  **22** (rang 25).

**🎯 LA PISTE RETENUE (idée utilisateur) — RÉVÉLER LES BUTS AU FIL DU RUN.** Le murage n'existe que
parce qu'on exige une **permutation complète, décidée à l'aveugle, avant le premier coup**. Si le but
suivant est choisi **depuis l'état courant**, la question « existe-t-il un ordre sain complet ? » ne
se pose plus : elle devient « quel but ensuite, depuis cet état-ci ? », locale et toujours
répondable. Le 18 cesse d'être un problème par disparition de la question.

- **C'est R1 gratuitement** : un but non révélé est du sol ordinaire, donc une poussée simple ne peut
  pas y poser définitivement une caisse. Aucun veto à écrire, aucune exemption de transit à régler —
  et la **case-porte du 11 se résout seule** (elle n'est pas encore un but quand les caisses la
  traversent, donc rien ne les empêche de repartir). ⚠️ Rappel utilisateur : **une case-porte n'est
  une destination finale qu'en fin de run**, jamais pendant l'acheminement.
- **Trois garde-fous décidés avant de coder** : `casesMortes` reste calculée sur **TOUS** les buts
  (la masquer inventerait des cases mortes — le piège du projet en cinquième déguisement) ; `h`
  aussi (le couplage est le seul levier universel, §2.2) ; et le choix se fait **au JALON** — quand
  un but vient d'être rempli, au plus `nbButs` fois par chemin, jamais par état.
- **Première version à écrire, la moins chère** : `butActif()` rend non plus le premier but non
  rempli de `ordreButs`, mais **le premier but non rempli encore LIVRABLE depuis l'état courant** —
  une seule passe `distanceLivraison`, et seulement quand l'actif vient d'être rempli ou est devenu
  inatteignable. L'ordre statique reste la **préférence** ; il cesse d'être une camisole. Aucun
  refactor des lambdas de tie-break de `ordreParPrecedence`.
- [ ] **Juge le moins cher, à passer avant de toucher au solveur** : vérifier hors ligne, sur les
  fixtures **r08/r09 du 13** (déjà produites), qu'un ordre révélé dynamiquement existe bien depuis
  ces états réels. S'il se mure aussi en dynamique, la piste tombe sans une ligne de solveur.

### 6.3 Robustesse / temps

- [ ] **Repli anytime pour la macro** : passe 1 avec macro plafonnée en états, passe 2 sans
  macro si le budget est épuisé. Le repli doit se déclencher sur le **budget**, pas sur
  l'échec (un cas lent n'émet jamais « aucune solution »). Borne surtout le temps des cas
  lents (8, 9).

#### 🎯 Session du 2026-07-28 — MESURE PRÉALABLE du PLONGEON (avant toute ligne de solveur)

**L'outil : `bench <niv> <mode> record`** (neuf). Écrit en `.xsb` **chaque état qui bat le max de
caisses posées**, au fil du solve, et le DATE en dépilements (la ligne `[record]` part sur stderr,
donc elle s'entrelace avec la jauge — pas besoin de toucher à la signature du signal). Le chemin
reconstruit est rejoué sur une copie du départ pour compter les poussées **et vérifier que ce
chemin mène bien à cet état** (une fixture fausse ne se verrait pas autrement — c'est le bug
big-endian de `mou`, §5). `bench` accepte désormais un **chemin `.xsb`** à la place d'un numéro, ce
qui rend les fixtures produites directement re-solvables.

**La question du chantier, et sa réponse chiffrée** : à quel moment apparaît un état complétable à
bas coût, et que coûte-t-il de le finir ? Mesuré sur les 10 résolus (hors 8), `macro` depuis chaque
record, budget 30 s :

| niveau | solve complet | 1ᵉʳ record **vivant** | dépilements | états pour finir | poussées totales |
|---|---|---|---|---|---|
| 2 | 412 | 6/10 | < 1 000 | 38 | 133 (+2) |
| 3 | 499 | **1/11** | < 1 000 | 442 | **134 = optimum** |
| 4 | 67 224 | 8/20 | ~2 000 | 120 | 357 (+2) |
| 6 | 570 | 7/10 | < 1 000 | 4 | **110 = optimum** |
| 9 | 354 622 | 12/14 | ~85 000 | 8 | **237 = optimum** |
| 5 | 9 123 | 9/12 | ~9 000 | 15 | 151 (+8) |
| 7 | 24 376 | 6/11 | ~22 000 | 1 509 | 112 (+22) |
| 17 | 24 786 | 5/6 | ~24 000 | 22 | **213 = optimum** |

⚠️ **Ce tableau ne dit PAS le gain — il ignore ce que coûtent les plongeons RATÉS**, et c'est
l'erreur qu'on a failli garder. Mesuré (états explorés avant « aucune solution » depuis chaque
record mort) : de **1 à 59 771 états**, avec une queue lourde — 59 770 et 59 771 sur le 9, ~5 000
sur le 7, ~4 280 sur le 5, ~810 sur le 2. **Un plongeon raté peut coûter plus cher que tout ce
qu'on espérait gagner.** Bilan NET (dépilements + tous les ratés + le succès) :

| niveau | défaut | **budget 100** | **budget 2000** |
|---|---|---|---|
| 4 | 67 224 | **2 670 → ×25** | 2 171 → ×31 |
| 9 | 354 622 | **85 557 → ×4,1** | 95 057 → ×3,7 |
| 5 | 9 123 | 9 398 → −3 % | 15 098 → **perte ×1,65** |
| 7 | 24 376 | 24 315 → neutre | 27 519 → −13 % |
| 2 | 412 | 768 → −87 % | 3 310 → **perte ×8** |
| 3 | 499 | 723 → −45 % | 642 → −29 % |

> **Un budget SERRÉ bat un budget large** — contre-intuitif, et la raison est dans les données :
> **quand un record est cher à finir, le suivant est presque toujours bon marché.** Sur le 4, le
> record 8 coûte 120 états, le 14 en coûte 19 et le 16 en coûte 13 — tous atteints au même moment
> (~2 000 dépilements). S'acharner sur un record ne sert à rien : le suivant fait le travail pour
> dix fois moins cher.

**🎯 LE SEUIL DE REMPLISSAGE BAT LE BUDGET (idée utilisateur) — et c'est le réglage retenu.** Le
budget décide *ce qu'on perd quand on se trompe* ; un **seuil en % de buts remplis** décide *quand
on tente*. Comme les records morts sont **concentrés dans le bas du tableau**, le seuil les évite au
lieu de les payer :

| niveau | 4 | 7 | 2 | 3 | 6 | 5 | 17 | **9** |
|---|---|---|---|---|---|---|---|---|
| morts jusqu'à | 35 % | 45 % | 50 % | 55 % | 60 % | 67 % | 67 % | **79 %** |
| 1ᵉʳ vivant | 40 % | 55 % | 60 % | 64 % | 70 % | 75 % | 83 % | 86 % |

À **seuil 80 %** : **zéro plongeon raté sur les huit niveaux**, aucune perte nulle part, et
**7 niveaux sur 8 rendent l'OPTIMUM EXACT** (le 4 est à +2) :

| niveau | 4 | 9 | 5 | 7 | 17 | 2 | 3 | 6 |
|---|---|---|---|---|---|---|---|---|
| défaut | 67 224 | 354 622 | 9 123 | 24 376 | 24 786 | 412 | 499 | 570 |
| seuil 80 % | **~2 013** | **85 008** | 9 003 | ~24 003 | ~24 022 | ~314 | ~303 | ~303 |
| | **×33** | **×4,2** | = | = | = | = | = | = |

Deux raisons, toutes deux lisibles dans les données : plus il y a de caisses posées, **moins il
reste à faire** (les plongeons gagnants coûtent 3 à 22 états), **et** les branches condamnées se
révèlent surtout tôt.

- ⚠️ **Le seuil ne GARANTIT rien** : sur le 9, un record est encore mort à **11/14 = 79 %**, juste
  sous la barre. Rien ne dit qu'un autre niveau n'aura pas un mort à 85 ou 90 % — c'est même
  l'attendu sur les gros non résolus, où le démêlage se joue tard. **Résonance directe avec le 11**,
  dont le record est justement **11/14 = 79 %** (§6.3, 2026-07-24) : la mesure du 9 prouve qu'un
  11/14 peut parfaitement être MORT.
- **D'où le réglage retenu : seuil pour décider QUAND plonger, budget pour borner ce qu'on perd
  quand le seuil s'est laissé avoir.** Le seuil fait le gros du travail, le budget est le garde-fou.
- ⚠️ Réserve §11.4 : ces 80 % sont calés sur **8 niveaux**, dont plusieurs triviaux. Défaut
  raisonnable, pas une loi — à revérifier dès qu'un non-résolu tombe.

#### ✅ Session du 2026-07-28 (suite) — PLONGEON CODÉ et MESURÉ : la prédiction tombe juste

**Le code** : `Solveur::AstarMacroPlongeon` (« A\* macro — plongeon sur record (essai) »), régime
**SÉPARÉ** comme le couplage — `AstarMacro` reste le défaut, aucune variable d'environnement (§7).
`SolveurAStar::plonge()` : best-first sur **h SEUL** (à h égal, le plus profond d'abord), goal macro
et les deux corrals actifs, cache d'enclos **partagé avec la recherche principale** (un enclos déjà
jugé ne se rejuge pas). Déclenché au point exact où le record est battu, si `rangees ≥ 80 %` des
buts. Un échec rend `noeuds` à sa taille d'avant : **un plongeon raté ne laisse aucune trace**. Les
états du plongeon sont **comptés dans le compteur** — sinon les chiffres ne seraient pas comparables
au défaut. CLI : `bench <niv> plongeon`.

| niveau | macro | **plongeon** | | poussées |
|---|---|---|---|---|
| **4** | 67 224 | **2 061** | **×32,6** | 359 (+4) |
| **9** | 354 622 | **85 729** | **×4,1** | **237 = optimum** |
| 0 | 4 | 5 | −1 état | 4 = |
| 1 | 14 | 15 | −1 état | 97 = |
| 2 | 412 | 417 | −5 états | 131 = |
| 3 | 499 | 500 | −1 état | 134 = |
| 5 | 9 123 | 9 124 | −1 état | 143 = |
| 6 | 570 | 571 | −1 état | 110 = |
| 7 | 24 376 | 24 377 | −1 état | 90 = |
| 17 | 24 786 | 24 788 | −2 états | 213 = |

**Deux gains massifs, et une NEUTRALITÉ parfaite ailleurs** (+1 à +5 états, poussées identiques au
canari sur 9 niveaux sur 10). Le seuil tient sa promesse : **un seul plongeon tenté par niveau, et
il réussit du premier coup** — jamais un raté à payer.

**La mesure préalable avait prédit le comportement à l'état près** — c'est la validation de la
méthode « mesurer avant de coder », pas seulement du chantier :

| niveau | déclenché à | états de plongeon | prédit par les fixtures |
|---|---|---|---|
| 3 | 9/11 = 82 % | 3 | r9, 3 états ✓ |
| 5 | 10/12 = 83 % | 3 | r10, 3 états ✓ |
| 17 | 5/6 = 83 % | 23 | r5, 22 états ✓ |
| 9 | 12/14 = 86 % | 9 | r12, 8 états ✓ |
| 4 | 16/20 = 80 % | — | ×33 prédit, ×32,6 obtenu ✓ |

- **Seul écart** : le 4 rend **359 poussées, pas les 357** annoncés. Normal et attendu — la mesure
  préalable lançait un A\* **macro** depuis le record, le solveur lance un **greedy** ; les deux ne
  prennent pas le même chemin. L'ordre de grandeur du gain, lui, est exact.
- **Le canari n'est pas concerné** : régime séparé, `AstarMacro` inchangé. Les poussées du plongeon
  sont d'ailleurs identiques au canari partout sauf sur le 4.

#### ❌➡️✅ Session du 2026-07-28 (fin) — LE SEUIL EN % RÉFUTÉ, remplacé par un BUDGET RELATIF

**Le 8 a tout fait basculer.** Il ne gagnait RIEN à seuil 80 % (4 376 071 états, +1). Cause :
son record plafonne à **12/18 = 67 %**, sous la barre — le plongeon ne se déclenchait qu'une fois la
partie déjà jouée. À seuil 66 % il passe à **1 171 492 (×3,7)**, à 238 poussées, le plongeon
réussissant depuis 12/18 en **23 états**. (C'est l'observation utilisateur « à 1 million d'états
dépilés, on arrive sur un motif solvable » — vérifiée, à 1,17 M.)

**Mais baisser le seuil ne marche pas non plus** : à 66 %, le 3 gagne (×2,51) alors que le **2 perd
2 poussées et le 5 en perd 8** — on plonge depuis un chemin qui a déjà dévié. **Aucune valeur ne
convient : 80 % est bon pour 4/9, 66 % est bon pour 8/3 et mauvais pour 2/5.**

**La carte des records du 8 dit pourquoi, et donne la bonne variable.** Les records n'arrivent pas
régulièrement, ils tombent **par PAQUETS séparés de longs plateaux de stagnation** :

| records | atteints à | stagnation qui suit |
|---|---|---|
| 1-2 | ~0 | 22 000 |
| 3 | 22 000 | 136 000 |
| **4-10** | **158 000** | **1 013 000** (à 10/18) |
| **11-12** | **1 171 000** | **3 205 000** (à 12/18) |
| 13-18 | 4 376 000 | fin |

Et surtout : le record **5/18 — 28 % du plateau** — est **complétable en 1 133 états**. Au même
pourcentage, le **9** a des records **MORTS qui coûtent 59 771 états** à réfuter. **Le pourcentage de
remplissage ne distingue pas les deux cas : ce n'est structurellement pas la bonne variable.**

> **CE QUI LES DISTINGUE, c'est le TRAVAIL DÉJÀ CONSENTI.** Le 8 atteint son 5/18 après 158 000
> dépilements ; le 9 atteint ses records morts après ~1 000. D'où la règle retenue, qui remplace À
> LA FOIS le seuil et le budget fixe :
>
> **budget du plongeon = (états déjà développés) / 100**, et on plonge à CHAQUE record.
>
> *Plus on a ramé, plus il est rationnel de parier.* Le 2 (412 états au total) n'accorde jamais assez
> de budget pour qu'un plongeon aboutisse → il ne plonge jamais et garde son optimum **par
> construction, pas par réglage**. Les plongeons ruineux du 9 sont étouffés (budget ~10 à ce stade).
> Le 8 à 158 000 dépilements dispose de 1 591.

**Mesuré (`bench <niv> plongeon`, contre `macro`) :**

| niveau | macro | **plongeon** | | poussées |
|---|---|---|---|---|
| **4** | 67 224 | **2 238** | **×30,0** | 359 (+4) |
| **8** | 4 376 070 | **159 484** | **×27,4** | 240 (+2) |
| **9** | 354 622 | **85 797** | **×4,13** | **237 = optimum** |
| 2 | 412 | 431 | −5 % | **131 = optimum** |
| 3 | 499 | 502 | = | **134 = optimum** |
| 5 | 9 123 | 9 120 | = | **151 (+8)** ⚠️ |
| 0 / 1 / 6 / 7 / 17 | — | ±2 % | = | **identiques** |

**Les trois plus gros solves gagnent ×4 à ×30 pour 0 à 4 poussées de plus.** Sur le 8, le plongeon
gagnant part de **4/18 = 22 %** en 340 états — le greedy fait mieux que l'A\* macro de la mesure
préalable (1 133 états depuis 5/18), il fonce vraiment.

- ⚠️ **Le 5 est le cas défavorable** : +8 poussées pour un gain d'états nul. Son record à 75 % est
  complétable en 15 états alors que le budget en autorise déjà 90 — le plongeon part, alors
  qu'attendre le record suivant donnait l'optimum. **Aucun budget ne corrige ça** : l'information
  manquante est « combien de temps me reste-t-il ? », que le solveur ne connaît pas. C'est la
  frontière habituelle du projet — on sait mesurer ce qui s'est passé, pas prédire ce qui reste.
  Et c'est un argument de plus pour le vrai levier : une `h` plus serrée, elle, SAIT ce qui reste.
- ⚠️ **Ce qui est gagné et ce qui ne l'est pas.** Le paramètre ne porte plus sur une **propriété du
  plateau** (« à partir de quel remplissage un état devient sûr » — faux en général : complétable à
  28 % sur le 8, mort à 79 % sur le 9) mais sur le **comportement observé du solveur**. Il a donc une
  chance de tenir sur un niveau jamais vu. Mais **1/100 reste un nombre choisi** : à 1/1000 le 8
  raterait ses 1 133 états, à 1/10 le 5 se dégraderait davantage. On a déplacé l'arbitraire, pas
  supprimé.
**✅ LE DIVISEUR BALAYÉ puis FIGÉ à 1/50 (fin de session).** Le 1/100 initial venait de projections,
jamais d'un balayage — exactement le reproche fait aux constantes empiriques. Mesuré sur les 12
résolus :

| diviseur | ce qui casse |
|---|---|
| 1/10 | **le 2 dérive à 139 poussées** (+8) : on plonge trop tôt, depuis un chemin déjà dévié |
| 1/15 | le 2 dérive encore (133) |
| **1/20 à 1/100** | **plage SÛRE** — poussées correctes partout |
| 1/500 | **le 4 REPERD tout** (67 159 états au lieu de 2 115), le 8 retombe à 1 174 706 |

**Les deux bords ont un mécanisme identifié** : en haut un budget trop généreux fait plonger depuis
un record trop précoce (qualité perdue) ; en bas un budget trop maigre ne paie plus le plongeon
gagnant — le 4 a besoin de 35 états à ~2 100 dépilements (donc diviseur ≤ 60), le 8 de 340 états à
158 000 (donc ≤ 464). **1/50 est au centre** : ×2,5 de marge en haut, ×10 en bas.

**Le 1/100 coûtait ×6,9 sur le 8** (159 484 contre 22 991) sans qu'on le sache. Chiffres complets :
[scores.md](scores.md). ⚠️ **La fenêtre est ÉTROITE (facteur 5)** et le 4 la ferme de justesse : son
plongeon gagnant réussit **en 35 états pour un budget de 41**, soit 17 % de marge. Un niveau dont le
premier record complétable demanderait 50 états au même stade échapperait au plongeon. C'est la
fragilité connue du réglage.

- **Le log par TENTATIVE** (`[plongeon n] record r/N a X depiles | budget B -> REUSSI/echec en E
  etats | cumul ...`) est ajouté après coup : sur le 11, 14 minutes se sont écoulées sans qu'on
  puisse savoir si le solveur avait seulement tenté quelque chose, et un run arrêté à la main
  n'imprime jamais son bilan de fin. Il montre le mécanisme en clair — sur le 4, les 7 records morts
  sont réfutés pour **51 états au total**, puis le 8ᵉ aboutit ; cumul 4 % du travail.
- **Propriété qui n'était pas dans l'intention de départ, et qui rend le régime SÛR : le surcoût est
  borné a priori.** Chaque record coûte au plus 1 % du travail fait à cet instant, et il y a au plus
  un record par but — le plongeon ne peut donc jamais faire dérailler un solve, au pire l'alourdir de
  quelques pour cent. Observé : +4,6 % (2), +1,4 % (7), +0,1 % (17). Aucun budget FIXE ne pouvait
  offrir cette garantie.

**✅ DÉCISION : le plongeon reste un SOLVEUR À PART ENTIÈRE, PAS le défaut** (2026-07-28). Ce n'est
pas de la prudence de façade, c'est le canari qui l'impose :
- Le canari est **le juge de toute modif** du projet (§0) — une `h` qui surestime ou un élagage faux
  positif « fait manquer l'optimum **sans aucun signal** ». Avec le plongeon en défaut, les poussées
  de référence deviendraient 359 / 151 / 240 / 243 au lieu de 355 / 143 / 238 / 241, et surtout
  elles deviendraient **INSTABLES** : toute modif décalant le compteur d'états décale le moment du
  plongeon, donc le record d'où il part, donc le nombre de poussées. **On perdrait l'invariant qui
  détecte les régressions silencieuses.**
- La promotion n'apporterait rien de fonctionnel : le régime est déjà dans le menu de l'app et en
  CLI (`bench <niv> plongeon` / `coupl-plongeon`). C'est **le régime à utiliser sur un niveau NON
  RÉSOLU** — c'est ainsi que le 11 est tombé.
- Argument renforcé par le balayage : un réglage qui gouverne la QUALITÉ de la solution et dont la
  fenêtre utile fait un facteur 5 n'a pas sa place dans le défaut.

#### 🎉🎉 2026-07-28 — **LE NIVEAU 11 EST RÉSOLU**, deux fois le même jour

**La cible historique du projet tombe.** Jamais finie jusque-là (meilleur résultat antérieur :
11/14 caisses posées, arrêt manuel à 57,7 M dépilements le 2026-07-24 ; et avant cela 8/14 à
12,4 M). **12 niveaux résolus sur 33** — la carte du §0 est à jour.

| régime | états | poussées | commit |
|---|---|---|---|
| `couplage` + corral-N (run utilisateur, mené au bout sans budget) | **87 085 967** | **241** | `cb4780c` |
| **`couplage` + corral-N + PLONGEON** | **13 918 468** | 243 (+2) | (ce diff) |

- **×6,3 pour +2 poussées.** Le plongeon gagnant part de **10/14 caisses posées** et coûte
  **11 états**, sur un budget de 139 184 dont il n'utilise donc que 0,008 %. **10 plongeons tentés**
  sur tout le run. C'est le résultat le plus net du régime : il n'accélère pas seulement des niveaux
  déjà résolus, **il rend abordable le plus dur**.
- **Ce qui a rendu ça possible, dans l'ordre** : le corral-N (élague le bois mort — 24 088 361
  enfilages prunés sur 349 M, amortissement de cache ×208 pour 670 738 configurations distinctes
  jugées), le régime `couplage` (seul à amener le 11 à 11/14), et le plongeon (fonce dans le vivant).
  **Aucun des trois seul n'y arrivait.**
- ⚠️ **Le mur mémoire du §6.5 est confirmé et dépassé** : 123,98 M clés en arène, file à 41,5 M,
  `noeuds` à 142,5 M → de l'ordre de **8 Go** sur le run brut. Le plongeon, en divisant les états par
  6,3, divise aussi la mémoire d'autant — **c'est le premier levier qui repousse le mur mémoire**,
  alors qu'il n'a pas été conçu pour ça.
- **241 poussées est sans référence** (premier solve du 11, régime macro donc pas un optimum prouvé).
  Pour borner l'écart : `passages 11` donne les trajets solos (§3).

**Quatre conclusions, toutes mesurées :**
1. **Le levier est RÉEL mais INÉGAL.** Gros gains sur 2/3/4/6/9, **zéro** sur 5/7/17 — là, le
   premier record vivant n'apparaît qu'à ~97 % du solve, il n'y a plus rien à gagner. Comme toute
   technique de ce projet, il mord sur une famille, pas partout (§2.2).
2. **La dégradation est minime, et souvent NULLE.** Sur 3/6/9/17, plonger rend **exactement
   l'optimum du niveau**. Ailleurs +2 poussées (2, 4) ; seuls le 5 (+8) et le 7 (+22) paient
   vraiment. C'était l'inconnue principale du chantier — elle est levée dans le bon sens.
3. **Les records MORTS dominent la phase initiale, partout** : 11 d'affilée sur le 9, 8 sur le 5,
   6 sur le 3 et le 6. Un plongeon sur chaque record plongerait donc des dizaines de fois dans le
   vide avant son premier succès → **budget serré obligatoire** (les échecs, eux, tombent en < 1 s).
4. ⚠️ **« Meilleur record » n'est PAS « meilleur état » — le niveau 3 le prouve.** Son record
   **1/11 est vivant et se complète à l'optimum**, puis les records 2 à 6 sont **morts**, puis ça
   redevient vivant. Le solveur bat donc son record en s'enfonçant dans des branches condamnées
   alors qu'il avait déjà eu un état parfaitement complétable en main. Un plongeon qui ne se
   déclenche que sur un record **strictement supérieur** rate ce cas.

⚠️ **Réserve de méthode** : « MORT » signifie ici « A\* **macro** épuise l'espace depuis cet état »,
ce qui n'est **pas** une preuve d'insolubilité (le régime d'engagement est incomplet — il ne génère
que les macros vers le but actif). Vérification faite en **A\* pur** (complet) sur deux fixtures du
niveau 4 : r04 et r07 sont **réellement morts**, et identiques avec `CORRAL=0` (donc pas un faux
positif du corral). Pour le design, c'est de toute façon la bonne mesure : si le plongeon utilise la
macro, il échouera exactement là où ces solves échouent.

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

#### ✅ Session du 2026-07-29 — LE PLONGEON À L'ÉPREUVE DE DEUX NIVEAUX NEUFS (10, 21)

**Deux niveaux tombent le même jour, sans une ligne de code** : le **21** (2 923 006 états, 165
poussées) et le **10** (2 175 724, 544). Détails et réserves : [scores.md](scores.md).

**Le seuil en % est réfuté une quatrième et cinquième fois.** Le plongeon gagnant part de :

| niveau | 8 | **10** | **21** | 11 | 9 (records morts jusqu'à) |
|---|---|---|---|---|---|
| remplissage au déclenchement | 22 % | **40,6 %** | **54 %** | 71 % | **79 %** |

Aucune valeur de seuil n'attrape cet ensemble. Le **budget relatif** (dépilés/50) les prend tous —
c'est sa justification définitive.

⚠️ **MAIS le plongeon n'est plus gratuit sur les gros niveaux.** Son coût cumulé :

| niveau | 21 | 4 | **10** |
|---|---|---|---|
| cumul des plongeons | **0,065 %** | ~4 % | **9,72 %** |
| états du plongeon gagnant | 53 | 35 | **4 339** |

Le 10 a **32 buts, donc jusqu'à 32 records, donc 32 plongeons à payer**. La borne a priori (au plus
1/50 du travail par record) tient, mais elle croît avec le nombre de buts — à surveiller sur les
niveaux à beaucoup de caisses (22 en a 27, 24 en a 22).

**⚠️ L'INSTABILITÉ DES POUSSÉES EST MESURÉE — c'était l'argument qui a refusé la promotion du
plongeon en défaut (§6.3), il est maintenant chiffré.** Sur le 21, le simple fait de couper le
corral change la solution :

| régime | états | **poussées** |
|---|---|---|
| corral-N ON | 2 923 006 | **165** |
| `CORRAL=0` | 4 861 308 | **147** |

**+18 poussées (+12 %) pour une modif qui ne touche pas la qualité de la recherche.** Mécanisme :
décaler le compteur d'états décale le moment du plongeon, donc le record d'où il part. **En régime
plongeon, les poussées ne sont donc PAS un canari, même approximatif** — et le meilleur résultat
connu sur le 21 est 147, pas 165.

#### ⚠️➡️❌ Session du 2026-07-29 — le log des plongeons N'EST PAS un détecteur de branche morte

> **RÉFUTÉ le jour même, par le profilage à témoins (§6.6).** Ce qui suit reste vrai *techniquement*
> — `E ≪ B` signifie bien « espace épuisé » — mais la conclusion qu'on en tirait (« le 12 s'enfonce
> dans des culs-de-sac, c'est son problème ») est **fausse** : avoir des records morts est **BANAL**,
> y compris sur les niveaux qui tombent. Mesuré sur les témoins résolus :
>
> | niveau | statut | records morts |
> |---|---|---|
> | **4** | ✅ résolu en 40 408 états | **7** |
> | **9** | ✅ résolu | **6** |
> | 6 / 17 | ✅ résolus | 2 |
> | 12 | non résolu | **1** |
>
> Le plan le disait déjà (« les records MORTS dominent la phase initiale, **partout** ») ; la section
> ci-dessous l'a oublié faute de témoin. **Leçon de méthode : un indicateur mesuré sur les seuls
> niveaux qui ÉCHOUENT ne prouve rien — il faut le passer sur ceux qui réussissent.** C'est le même
> piège que le §11.4, appliqué à un diagnostic au lieu d'une loi.
>
> Ce qui distingue réellement les résolus n'est pas l'absence de records morts, c'est que **le
> plongeon finit par réussir** — un constat *a posteriori*, donc pas un prédicteur.

#### 🎯 Session du 2026-07-29 — le log des plongeons, ce qu'il dit vraiment

**La lecture, et elle est gratuite** (le log existe depuis le 2026-07-28) :

> Sur `[plongeon n] … budget B -> echec en E etats` :
> **`E = B`** ⇒ **budget épuisé**, on ne sait rien.
> **`E ≪ B`** ⇒ **espace épuisé** ⇒ le record est **MORT** (dans le régime macro, cf. réserve §6.0).

**Appliqué au 12** (run arrêté à 35,1 M dépilements, `rangees 9 (max 11)/15`) :

| plongeon | record | budget | états | verdict |
|---|---|---|---|---|
| 1 | 1/15 | 122 | 6 | **espace épuisé** |
| 2 / 3 | 2-3/15 | ~47 000 | **= budget** | budget épuisé |
| 4 → 11 | 4/15 → **11/15** | 48 k → **686 282** | 4 811 → **1** | **espace épuisé** |

**Neuf des onze records du 12 sont MORTS**, dont le 11/15 réfuté en **un seul état** sur un budget de
686 282. Le 12 n'approche pas la solution : il accumule des records dans des culs-de-sac.
**Conséquence pour le §6.3 (deltaf)** : la relégation `Δf = +2` à 100 % n'est **pas** la cause de son
échec — le régime `couplage`, conçu pour elle, tourne ici et n'y change rien. Le plan le soupçonnait
déjà (« rien ici ne prouve que la relégation est LA cause »), c'est confirmé.

**Même diagnostic sur le 27** par une autre voie : son record **17/20 (85 %)** rejoué seul rend
`AUCUNE 0` — mort à la racine, et **pas à cause du corral** (identique en `CORRAL=0`). Cause trouvée
au §6.2 : l'ordre de remplissage a condamné la partie au rang 16.

- ⚠️ **« De bon espoir » est un piège.** Un `max 17/20` affiché par la jauge peut être un cul-de-sac
  intégral. Le §6.3 avait prévu le cas (« rien ne dit qu'un autre niveau n'aura pas un mort à 85 ou
  90 % ») — **le 27 le réalise**.
- ⚠️ **PAS un mur mémoire — correction du même jour.** J'avais écrit ici « MUR MÉMOIRE confirmé »
  parce que le 12 atteignait **6,2 Go de RSS** en 2 h 46. **Faux** : la machine a **18 Go** et
  `memory_pressure` rendait **82 % de mémoire libre**. Deux erreurs cumulées — avoir lu
  `vm_stat: Pages free` comme « mémoire disponible » (les pages inactives et purgeables sont
  réutilisables, et le §1 avertit déjà que **`ps rss` ment sur macOS**), et avoir extrapolé le
  « machine à 8 Go » du §6.5, qui décrit une AUTRE machine. **Ne jamais déduire une saturation d'une
  RSS sans lire la RAM totale ni la pression réelle.**
  Ce qui reste vrai du diagnostic : la file contenait **95 % des états vus** et montait de **+4 305
  par millier** — c'est ÇA qui disait l'absence de convergence. Le 12 n'était pas au mur, il était
  **lent**. Le plafond réel sur cette machine est vers 15-16 Go, soit de l'ordre de **150 M états
  vus** (extrapolation du §6.5).

#### ⏸️ Session du 2026-07-29 (fin) — LE 31 : 188 M états vus, arrêté sans verdict

Premier des deux candidats désignés par le profilage (§6.6) — retenu pour ses **0 record mort** et
une dizaine de plongeons échouant *uniquement* par budget. Lancé sans budget, **arrêté à 45 min sans
conclusion**. À retenir avant de le reprendre :

| à 2 min (profilage) | à 30 min | à 45 min (arrêt) |
|---|---|---|
| max 10/20, file +724 | max 15/20, file **+97 (stagne)** | **max 16/20**, file +273, **188 M vus** |

- **188 M états vus, c'est plus que tout ce que ce projet a résolu** (87 M pour le 11 en `couplage`
  seul, qui était le record). Le 31 n'a donc pas été « essayé sérieusement » — il a été *entamé*.
- ⚠️ **La pente de la file est AMBIGUË, et ce niveau le montre.** Le profilage la donnait comme le
  seul signal cohérent (résolus entre +278 et +685). Mais une pente basse a **deux** causes opposées :
  une recherche qui se referme, **ou** un espace où presque aucune poussée n'est légale. Sur le 31 —
  **densité 18,2 %, ZÉRO point d'articulation**, et un « gros démêlage d'entrée » identifié à l'œil
  par l'utilisateur — c'est la seconde lecture qui est plausible. Elle n'est d'ailleurs pas stable :
  +724 → +1579 → +97 → +273 au fil du même run.
- **Le front reste très en amont** : `f 248 h(reste) 220` ⇒ `g ≈ 28` après 107 M dépilements. Le
  `max 16/20` est un **pic isolé** d'une branche lointaine, pas le régime courant (`rangees 0`) —
  exactement le tableau du 11 en juillet (`g ≈ 26`, « pic isolé »).
- **Ses plongeons épuisent leur budget au symbole près** (`384 787 -> echec en 384 787`,
  `392 483 -> 392 483`). Ses branches ne sont pas condamnées ; elles sont trop vastes pour être
  réfutées. **Le « 0 record mort » qui l'avait fait choisir signifie donc « on ne sait rien », pas
  « c'est prometteur ».** Troisième indicateur du jour à se dégonfler.
- [ ] **Le 14 n'a jamais démarré** (runs séquentiels) : c'est le candidat le plus frais — 12/18 en
  deux minutes, 0 record mort, 10 plongeons par budget. À lancer en premier à la reprise.
- [ ] Si le 31 est repris : prévoir **des heures**, pas des minutes, et surveiller la file plutôt que
  la RSS.

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

### 6.6 🧭 CLASSER LE PLATEAU pour choisir les leviers (idée utilisateur, 2026-07-28)

> « Les gains apportés par telle ou telle astuce dépendent grandement de la nature du plateau. Si on
> arrive à déterminer à l'avance à quelle famille appartient un plateau, on gagne sur tous les points. »

**Le constat est déjà chiffré, chantier par chantier — on ne s'en est simplement jamais servi comme
d'un système.** Presque chaque levier a produit, en même temps que son gain, l'indicateur qui
PRÉDIT ce gain :

| levier | gain max | famille où il mord | **prédicteur, déjà mesuré** |
|---|---|---|---|
| couplage hongrois joueur-aware | ×59 | **universel** | aucun — à garder partout |
| goal macro + goal-ordering | ×1000 à ×14000 | salle de buts unique | s'effondre en **multi-salles** (10, 18, 24-26) |
| tie-breaks (guidage, portes) | ÷2,8 | — | **part de `f = C*`** (§3) : 100 % → gain, 0,3 % → zéro |
| corral unitaire, motif 1 | ×6,8 | coins scellés | **fréquence du motif** : 100 % sur 4/7, 2 % sur 5/9 |
| pince, motif 2 | ×1,98 | **autre** famille | nulle sur le 4, décisive sur 8/17 |
| corral-N | ×9,9 | enclos sous-dotés | **% de durs prouvés morts** : 40,6 % (4), 24,5 % (9), 10 % (7/17) |
| backtrack macro | qualitatif | descentes à forks | **taux de forks** : 50,7 % (9) → bascule ; 1-20 % → rien |
| but du couplage | ×10,2 | — | **% relégués × part de macro dans le flux** |
| plongeon sur record | ×33 | record vivant précoce | **date du 1ᵉʳ record ≥ 80 %** |
| pondéré | ×34 | petits niveaux | **PIRE** sur les gros |

**Aucun levier n'est universel sauf le couplage.** Le §6.1 l'écrit déjà noir sur blanc pour le
corral (« la fréquence prédit le gain, exactement ») ; ce tableau ne fait que constater que c'est
vrai partout.

**Ce qui manque : ces prédicteurs sont tous A POSTERIORI** — ils exigent un solve ou un
échantillonnage par sous-solves. Pour choisir le régime AVANT de lancer, deux voies :

- **(a) Indicateurs STATIQUES au chargement**, à la manière de `casesMortes`/`distanceParBut`.
  Candidats calculables sans rien explorer : **composantes connexes de buts** (= le multi-salle, qui
  décide du goal-ordering), **densité caisses / espace libre** (= la congestion, donc le démêlage),
  **comptage statique des motifs corral** (le motif 1 se voit sur le plateau nu), **degré moyen des
  cases libres** (couloirs contre salles ouvertes).
- **(b) Passe de PROFILAGE bornée** : 10 000 états, on relève les prédicteurs qu'on sait déjà
  produire (`INSTRUM_F` pour `f<C*`, `macro` pour les forks, les stats `[CORRAL-N]` pour les durs
  morts, `deltaf` pour la part de macro dans le flux), puis on choisit le régime. **Une seconde pour
  orienter des heures de solve.**

> **(b) d'abord.** Elle réutilise des mesures **déjà validées**, là où (a) demanderait de prouver que
> chaque indicateur statique corrèle vraiment avec le gain — sur 11 niveaux résolus, c'est le piège
> du §11.4 en plein. Les indicateurs statiques s'ajouteront à (b) à mesure qu'ils font leurs preuves.

**Ce que ça débloquerait concrètement** : un profilage dirait lesquels des 19 non résolus sont
multi-salles (donc bloqués sur l'ordering), lesquels sont riches en corrals (donc déjà bien servis),
lesquels sont du pur démêlage — au lieu de lancer un solve de plusieurs heures au hasard.

⚠️ **CORRIGÉ le 2026-07-29** : ce paragraphe disait « les 22 niveaux non résolus n'ont, pour la
plupart, jamais été attaqués ». **C'est faux** — ils sont relancés régulièrement, aucun ne passe
(cf. §0). Le profilage ne sert donc pas à *découvrir* des niveaux faciles, il sert à *choisir le
régime* sur des niveaux qui résistent.

#### ✅ 2026-07-29 — LA CARTE DES 33, PAR PROFILAGE BORNÉ (la voie (b), enfin faite)

**Protocole** : `bench <niv> coupl-plongeon` pendant **120 s**, puis `kill`. On ne relève que ce qui
part sur `stderr` **en continu** (jauge, lignes `[plongeon]`) — donc lisible même sur un run tué,
contrairement aux stats de fin. **Les 15 résolus sont inclus comme TÉMOINS** : sans eux on ne sait
pas lire les chiffres des autres, et c'est justement ce qui a réfuté le « détecteur de branche
morte » (§6.3).

⚠️ **Calibration du budget** : 0-9 et 17 finissent tous dans les 120 s. **Mais 10, 11, 21 et 32 —
résolus — n'y arrivent PAS** (2,2 M à 13,9 M états). À deux minutes ils sont indiscernables d'un
non-résolu : le 10 affiche `max 4/32`, soit 12 %. **La progression à budget borné ne prédit donc
RIEN** à elle seule.

**GROUPE A — ne démarre pas (`max ≤ 1`)** : **23** (0/18), **20** (1/18), **25** (1/19), **22**
(1/27), **12** (1/15).
> ⚠️ **Quatre des cinq sont exactement les niveaux dont l'ordre était MURÉ** (§6.2). La correction du
> jour a levé le murage **sans débloquer l'acheminement** : ils ne posent toujours aucune caisse. Le
> goal-ordering était nécessaire, il n'était pas leur verrou principal. Le 22 reste muré en plus.

**GROUPE B — plafonne à mi-chemin** (progression / pente de la file / plongeons morts-budget) :

| niv | progression | pente file | plongeons |
|---|---|---|---|
| 28 | 13/20 (65 %) | +1013 | 8M / 5b |
| 27 | 13/20 (65 %) | +1632 | 2M / 11b |
| **14** | 12/18 (67 %) | +2759 | **0 mort** / 10b |
| 19 | 10/15 (67 %) | +1946 | 8M / 2b |
| 15 | 10/15 (67 %) | +1190 | 2M / 5b |
| **31** | 10/20 (50 %) | **+724** | **0 mort** / 10b |
| 29 | 10/16 (62 %) | — | 2M / 2b |
| 16 | 7/15 (47 %) | **+256** | 2M / 3b |
| 30 | 4/18 (22 %) | **+83** | 2M / 2b |
| 13 | 9/16 (56 %) | +1512 | 0M / 8b |
| 18 | 8/11 (73 %) | +3263 | 1M / 5b |
| 26 | 7/13 (54 %) | +1080 | 3M / 4b |
| 24 | 5/22 (23 %) | — | 5M / 0b |

**Témoins résolus, à comparer** : 32 → 14/15 et **+685** ; 21 → 6/13 et **+595** ; 11 → 6/14 et
**+278** ; 10 → 4/32 et +2356.

**Le signal le moins mauvais est la PENTE DE LA FILE** : les résolus tiennent entre +278 et +685 (le
10 excepté). Les non-résolus à faible pente — **30 (+83), 16 (+256), 31 (+724)** — sont ceux dont
l'espace n'explose pas.
⚠️ **Mais elle est AMBIGUË et INSTABLE**, démontré sur le 31 le soir même (§6.3) : une pente basse
peut venir d'une recherche qui converge **ou** d'un espace où presque aucune poussée n'est légale ; et
elle a fait +724 → +1579 → +97 → +273 au cours d'un seul run. **Indice, pas prédicteur.**

> **BILAN DU PROFILAGE, sans complaisance** : il a surtout servi à ÉLIMINER trois prédicteurs qu'on
> croyait tenir — les **records morts** (banals, 7 sur le 4 qui tombe en 40 000 états), la
> **progression à budget borné** (le 10 est à 12 % et il tombe), la **pente de la file** (ambiguë et
> instable). Ce qui SURVIT : la partition **Groupe A / Groupe B**, qui recoupe le murage d'ordre.
> Éliminer de faux signaux est un résultat — c'est moins que ce qu'on espérait.

- [ ] **Candidats prioritaires** : **31** et **14** (zéro record mort, une dizaine de plongeons qui
  n'échouent QUE par budget → branches non condamnées), puis **30** (pente la plus basse du corpus).
- [ ] Ne pas relancer à l'aveugle 12/20/22/23/25 (Groupe A) : leur problème est l'acheminement, pas
  le temps.

**✅ PREMIÈRE BRIQUE POSÉE le 2026-07-29 — l'outil `ordre` (§6.2) est un prédicteur STATIQUE qui
marche.** Il classe sans rien explorer, il sépare 16 niveaux qui passent de 11 qui échouent, et il
dit **quelle famille** de défaut (locale = bug, globale = trajet de tirage). C'est exactement la voie
(a) « indicateurs statiques au chargement », que le §6.6 avait rangée après la voie (b) faute de
preuve qu'un indicateur statique corrèle avec le gain. **Celui-ci corrèle.** Les autres candidats
statiques (composantes de buts, densité, degré moyen) restent à valider — mesurés le 2026-07-29, ils
ne prédisent PAS le coût : le 4 a 20 caisses et 55 560 états (le corral le sert), le 8 a 1 seul point
d'articulation et 4,4 M états. **La difficulté n'est pas une propriété du plateau, c'est
plateau × leviers disponibles.**

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
- ⚠️ **`bench` teste `endsWith(".xsb")` — un fichier `.xsb.txt` retombe SILENCIEUSEMENT sur le
  niveau 0** (2026-07-29). `arg1.toInt()` rend 0, aucune erreur n'est levée, et le harnais résout un
  AUTRE niveau en affichant un résultat parfaitement plausible (4 poussées = le canari du 0). Deux
  minutes perdues à interpréter le résultat du mauvais plateau. Renommer ou copier avant de mesurer.
- ⚠️ **Charger une position de MILIEU DE PARTIE comme un niveau recalcule tout le statique**
  (2026-07-30). `ordre <fixture.xsb>` sur un record du 13 affiche « ORDRE MURÉ au rang 14 » — mais
  c'est l'ordre calculé **pour ce plateau-là**, dont les caisses de départ sont celles du milieu de
  partie, pas celles du niveau. `ordreParPrecedence`, `casesMortes` et `distanceParBut` tournent dans
  le ctor `Game(Level)` et ne connaissent que le plateau qu'on leur donne. **Ne rien conclure sur le
  niveau d'origine à partir de l'ordre affiché sur une de ses fixtures** ; seules les mesures qui ne
  dépendent que de la géométrie (murs) se transportent.
- ⚠️ **Un bloc de statistiques recopié à côté d'un résultat n'en vient pas forcément** (2026-07-31).
  La ligne du niveau 10 de [scores.md](scores.md) portait un bloc `[CORRAL-N]` — 22,3 M durs, 23,6 %
  de morts, 55 M états de sous-solve — **étranger au run** : les deux plateformes rendent 3,47 M
  durs et 0,4 % de morts pour ce même solve. Personne ne l'a vu pendant deux jours, et ce chiffre a
  servi de « signal » pour ouvrir un chantier. **Vérifier la cohérence interne d'un relevé avant de
  raisonner dessus** : ici, 22 M de durs pour 2,17 M d'états faisait 10 durs par état contre 1,6
  mesurés, sur un plateau dont la géométrie est fixe — l'incohérence était lisible sans rien relancer.
- ⚠️ **Un interrupteur d'ENVIRONNEMENT dans le solveur fait diverger l'APP du bench, en silence**
  (2026-07-28). L'app lancée depuis un launcher (Finder, .desktop, Qt Creator) n'hérite pas de
  l'environnement du shell où l'on tape les `bench` : toute feature gardée par un `qgetenv` tourne
  donc en mesure et **pas** en jeu. Symptôme trompeur : un écart d'états qu'on attribue à la
  MACHINE (« Mac contre PC ») alors qu'il vient du binaire d'à côté — c'est arrivé avec
  `CORRAL_DETECT`. Un interrupteur ne doit vivre que le temps d'un chantier ; à la promotion, il
  part. Ceux qui restent (`CORRAL=0`) ne servent qu'aux outils de mesure et **coupent**, jamais
  n'ajoutent : un défaut coupé se voit tout de suite, un défaut manquant ne se voit jamais.

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
  deadlock ; **`ordre.cpp`** (neuf, 2026-07-29) pour la précédence de remplissage — il lit
  `Game::getOrdreButs()`, accesseur const ajouté exprès plutôt que de remettre un `qgetenv` de debug
  dans le chemin chaud (§7) ; **`precedencepaires.h`** (neuf, 2026-07-30) — BFS de tirage à rebours à
  deux bloqueurs, en **exemplaire unique** partagé par `ordre` et `fp -3`. ⚠️ **Objet RÉFUTÉ comme
  élagage** (§6.2) : conservé pour la LECTURE d'un état précis, jamais comme test.
