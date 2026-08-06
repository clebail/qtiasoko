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
| **mode HYBRIDE** (dans l'app) | **(neuf, 2026-08-01) LE SOLVEUR ANNOTE UNE PARTIE HUMAINE.** Case à cocher : l'ordre de remplissage s'affiche en chiffres sur les buts, et à CHAQUE coup joué à la main l'UI rejoue le **régime d'engagement du solveur** (`solveurastar.cpp:331-343` — `getCaissesDeplacable` → `macroPeutDemarrer` → `macroVersButBacktrack` + `!isPerdu`) et surligne les macros jouables. Clic sur une caisse cerclée = la macro se joue ; clic sur une case libre = le perso y marche ; **clic DROIT = « il aurait dû y avoir une macro ici »**, qui consigne la CAUSE (poussable dans aucune direction / échec au pas 0 / descente bloquée en (x,y) avec N restants / aboutit mais `perdu`) + le plateau. Tout part dans `hybride_niveau_XXXX.txt` (un par niveau, en AJOUT, flush par ligne). C'est le seul outil qui répond à « l'ordre est-il BIEN JOUÉ ? », là où `ordre` ne répond qu'à « est-il FAISABLE ? »<br>**(2026-08-01, suite) LE RANG DU COUP HUMAIN** — à chaque poussée vraiment choisie (hors macro, hors rejeu), l'UI rejoue l'**enfilage** du solveur sur l'état d'avant (mêmes enfants, mêmes élagages dans le même ordre, même clé de tri que le comparateur) et journalise `[rang] (x,y) Dir \| rang R/N \| h .. f .. \| meilleur (x,y) Dir \| df ±k`. Trois variantes : `HORS REGIME MACRO` (le solveur ne générerait aucune poussée simple — c'est la mesure du désaccord), `⚠ ECARTE par le solveur` (**faux positif d'élagage PROUVÉ** si la partie est gagnée : c'est le juge `fp` étendu aux niveaux NON RÉSOLUS), `⚠ INTROUVABLE` (le miroir a divergé du solveur). ⚠️ Rang **parmi les frères**, pas dans la file globale : ce qui s'y transporte, c'est `df` |
| `pas0 <niv>` | **(neuf, 2026-08-01) POURQUOI AUCUNE MACRO N'EST DISPONIBLE**, sur le plateau de DÉPART. Pour chaque couple (caisse, but), rejoue le contrat EXACT de l'UI — `macroPeutDemarrer`, descente `macroVersButBacktrack` menée au bout, `!isPerdu` — et classe les échecs : *amorce puis bloque en (x,y)*, *détour non-monotone requis*, *joueur du mauvais côté*. Répond en une seconde à « le premier but choisi change-t-il quelque chose au démarrage ? » (sur le 12 : non, aucun des 15 n'est atteignable). ⚠️ **Le premier jet ne testait que `macroPeutDemarrer` et annonçait l'inverse** — amorcer n'est PAS aboutir. Outil de chantier |
| **injection d'ordre par FICHIER** | **(neuf, 2026-08-01)** `ordre_niveau_XXXX.txt` dans le répertoire courant écrase l'ordre calculé de ce niveau. Complète `ORDRE_HUMAIN`, qui est une variable d'environnement et **n'atteint donc pas l'app** lancée par un launcher (§7) : c'est le seul moyen de JOUER un ordre à la main en mode hybride et de voir où il coince. Même parseur, exemplaire unique. **Bruyant des deux côtés** (`[ORDRE_FICHIER]` sur stderr, et le journal hybride écrit `ordre de remplissage ⚠ INJECTE depuis …` au lieu de `calcule`) — un fichier oublié changerait sinon le comportement en silence, le pire cas du §7. Absent = rien ne change |
| **rejeu de journal + INTENTIONS** (dans l'app) | **(neuf, 2026-08-01) CAPTURER LE PLAN, PAS LE COUP.** Touche `L` : relit `hybride_niveau_XXXX.txt`, en extrait la **dernière partie GAGNÉE** (les `[undo]` retirent le dernier coup) et l'installe dans le rejeu pas à pas existant — aucune mécanique de navigation en double. `N` saute à la prochaine **poussée choisie** (macros et marche franchies d'un coup). Six touches d'intention en vocabulaire **FERMÉ** : `E` écarter du chemin d'une autre caisse · `O` ouvrir un passage joueur · `G` garer pour plus tard · `A` préparer un appui · `T` **sortir pour reprendre dans l'autre sens** (= le RECUL du §3) · `R` rapprocher · `?` je ne sais pas. **Une frappe par PLAN**, valable jusqu'à la suivante — c'est l'objet même : le rang d'un coup isolé ne peut pas voir un plan sur plusieurs coups. Sortie : `hybride_niveau_XXXX_intentions.txt`, avec le **numéro de coup** (sans lui les annotations seraient orphelines). ⚠️ Flèches et Retour arrière **neutralisés** pendant une session : ils modifient le plateau sans toucher à `posPas`, et le numéro de coup écrit devient faux |
| `diverge`, `paires`, `trace`, `passages`, `congestion` | mou de `h`, interactions de paires, solution pas à pas, cartes de trajets |
| **historique des RECORDS + critique du solveur `C`** (dans l'app) | **(neuf, 2026-08-03) LE MIROIR DE L'ANNOTATION D'INTENTIONS, mais sur ce que le SOLVEUR fait.** Le solveur a DEUX points d'enfilage (recherche principale + `plonge()`) et `nouveauMaxCaisses` écrasait le chemin visionné à CHAQUE record — un sélecteur conserve tous les chemins d'un run, voir le record 7 ET le record 8 ne demande plus qu'un seul run. Touche `C` : boîte de texte LIBRE (pas de vocabulaire fermé — celui des intentions a mis deux sessions à se stabiliser, on ne le refait pas sans savoir ce qu'on y met), journal `solveur_niveau_XXXX_critique.txt`, plateau `.xsb` joint à chaque entrée pour que `mort`/A\* puisse juger l'état après coup. `C` inerte pendant une session d'intentions (deux journaux distincts, ne pas mélanger) |
| `bench <fichier.xsb> record` → `.chemin` | **(neuf, 2026-08-03)** à côté de chaque `.xsb` exporté, une lettre par coup (H/D/B/G, ordre de `EDirection`) : permet de rejouer le chemin d'un record HORS de l'app, pour le passer à `mort`/`fp` |
| `paquet <niv> [depiles] [budget] [mode]` | **(neuf, 2026-08-03)** fréquence + coût du motif « paquet de caisses hors but non livrable » (cf. §6.1). `paquet <fichier.xsb> [budget]` = mode JUGE, verdict MORT/vivant/inconnu sur UN plateau — sert de `fp` pour ce motif |
| `gabarit.py <niv>` | **(neuf, 2026-08-03, scratchpad)** un plateau ASCII par but ACTIF (buts déjà remplis affichés comme posés, rien pré-rempli) — support pour DESSINER une règle de cases mortes à la main sans que l'instrument ne suggère le vocabulaire (cf. §6.2) |
| `juge_loi.py` | **(neuf, 2026-08-03, scratchpad)** juge une loi de cases mortes contre TOUTES les parties humaines gagnantes d'un coup (murs seuls, ordre injectable) : toute caisse sur une case déclarée morte est un faux positif PROUVÉ. A validé la loi du §6.2 sur 21/24 parties, et localisé les 3 exceptions à des ordres faux. ⚠️ **PERDU avec le scratchpad de sa session** — les scratchpads sont éphémères, tout outil qui doit resservir se rapatrie dans `mesures/` le jour même |
| `loi <niv> [gabarit.txt]` | **(neuf, 2026-08-04)** LE JUGE DE LA LOI DE L'ORDRE (§6.2). Sans argument : la table des cases mortes but par but. Avec un gabarit : compare la table CALCULÉE au dessin FAIT À LA MAIN, case par case, et sort non nul au moindre écart — la loi n'étant dérivée d'aucun théorème, ce dessin est sa seule vérité de référence, et le canari ne verrait jamais un écart (un élagage trop mordant ne casse que des niveaux qu'on ne finit pas). Compare le **SURPLUS** (loi moins table ordinaire). Accepte aussi un `.xsb` : verdict `geleHorsTour` + cases mortes sur un plateau isolé |
| `porte <niv>` | **(neuf, 2026-08-04) LA PRÉCÉDENCE CAISSE → BUT** — d'espèce neuve, toutes les autres sont but → but. *Si remplir G prive le joueur de TOUS les appuis d'une caisse C, alors C doit avoir bougé avant G.* Statique, O(caisses × buts × plateau), relaxation optimiste (une contrainte est une preuve, un silence ne promet rien). ⚠️ Une poussée dont la destination est une case MORTE ne compte pas comme une issue — sans ce test l'outil est muet. Rend **0 sur les 15 résolus**, et 2 sur 18 non résolus : le 16 (avant le rang 0) et le 30 |

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

> ⚠️ **`f = C*` est NÉCESSAIRE, pas SUFFISANT — mesuré le 2026-08-06** (§6.2). Ce paragraphe et le
> tableau du guidage par portes (2026-07-21) se lisent comme un **prédicteur** (« le gain suit la
> masse `f = C*`, ligne pour ligne »). C'est faux dans ce sens-là : en réordonnant les chiffres du
> score lexicographique, **trois runs à ≥ 99,6 % de `f = C*` rendent ÷1,90 (1 astar), une PERTE
> (6 astar) et zéro exactement (4 macro)**. Il faut en plus que le comparateur ait des **ex æquo à
> départager** — ce que le régime d'engagement de la macro supprime presque (une poignée d'enfants
> par état). Ce qui reste vrai, et c'est l'essentiel : un tie-break ne peut rien gagner **là où
> `f < C*` domine**.

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

#### ✅⚠️ Session du 2026-08-03 — LE PAQUET NON LIVRABLE : un motif d'élagage SOUND, trouvé par la CRITIQUE du chemin du solveur

**L'origine, et c'est une inversion de la campagne hybride** (idée utilisateur) : au lieu d'annoter
ce que l'humain fait, annoter ce que le **solveur** fait — *« pourquoi c'est de la merde »*. Le
raisonnement qui l'a fait retenir : une intention est une hypothèse de **guidage**, et le guidage
est fermé depuis le 2026-07-21 ; « pourquoi cet état est fichu » est une hypothèse d'**élagage**,
le seul levier que le §6.1 laisse ouvert. **Et cette fois il y a un ORACLE** — `mort` / A\* pur
peut contredire le jugement humain, ce que la campagne d'intentions n'a jamais pu faire (sept
observables essayés le 2026-08-02, six réfutés, un silencieux).

**LE MOTIF, énoncé par l'utilisateur puis formalisé :**

> Un groupe de caisses **8-connexe, hors but**, dont on retire TOUTES les autres caisses du plateau
> en gardant TOUS les buts, ne peut pas être entièrement posé ⇒ **l'état réel est MORT**.

**Sound par construction** : retirer des caisses ne fait qu'augmenter la liberté du joueur et des
caisses restantes ; garder tous les buts leur laisse toutes leurs destinations. Point clé vérifié
dans le code : **`checkVictoire` teste « aucune caisse hors but », pas « tous les buts remplis »** —
c'est ce qui rend l'instance relâchée valide et la réduction rigoureuse plutôt qu'argumentée.

**LA MÉTHODE DE RÉDUCTION est de l'utilisateur, et c'est elle qui a tout débloqué** : *« pour le
16, ne garde que les caisses en haut à gauche, tu verras 2 poussées max ⇒ deadlock »*. Mesuré :
`CORRAL=0 bench fixture.xsb astar` → **`AUCUNE` en 3 états explorés**, soit l'état de départ plus
exactement deux poussées. La prédiction était juste au chiffre près. Là où l'A\* pleine taille
n'avait rendu aucun verdict après 48 M et 60 M d'états, la fixture conclut en une seconde.

**SIX DIAGNOSTICS HUMAINS, SIX CONFIRMATIONS, ZÉRO RÉFUTATION** (états annotés en mode critique
sur les chemins de record des niveaux 14, 15, 16) :

| état | prédiction utilisateur | verdict oracle |
|---|---|---|
| 16 #1 (poussée 8) | mort, sûr | ✅ MORT — par réduction, 3 états |
| 16 #4 (poussée 21) | mort, sûr | ✅ MORT — prouvé avec ET sans corral |
| 15 #1 (poussée 60) | mort assurée | ✅ MORT — prouvé deux fois |
| 14 #5 (poussée 37) | deadlock à coup sûr | ✅ MORT — paquet de 9 caisses |
| 14 #6 (poussée 39) | deadlock à coup sûr | ✅ MORT — même paquet |
| 16, coup 18 du record 7/15 | « deadlock futur, (9,4) et (10,5) » | ✅ MORT — **et le bon paquet**, {(8,4),(9,4),(10,5)} |

Les « incertain » de l'utilisateur sont restés inconnus des deux côtés — aucune de ses hésitations
n'a été comptée comme une affirmation, ce qui rend le score lisible.

**FRÉQUENCE MESURÉE** (outil neuf `mesures/paquet`, sur les états RÉELLEMENT dépilés, donc déjà
passés à travers `checkDefaite`, le corral unitaire, la pince et le corral-N) :

| niveau | morts non détectés par l'élagage actuel | coût / état jugé | amortissement cache |
|---|---|---|---|
| **16** | **71,4 %** (28 574 / 40 000) | 7,8 états de sous-solve | ×645 |
| 4 | 24,5 % | 22,4 | ×316 |
| 17 | 16,5 % | 28,2 | ×223 |
| 14 | 7,6 % | 5,8 | ×1040 |
| 15 | 1,0 % | 10,3 | ×991 |

**Et la profondeur est bonne là où la fréquence est haute** — c'est le chiffre qui décide (§6.1,
`corral.cpp`) : sur le 16, **52 % des états à UNE caisse rangée sont déjà morts**, 71 % à deux,
84,6 % à trois ; sur le 4, les 24,5 % sont **tous à 0 caisse rangée**. Cas « continent », pas
« brindille ». Le 14 est l'inverse (rien avant 3/18, pic à 7/18) et le 15 est plat à 1 %.

**PAS DE FAUX POSITIF** : le juge `fp` étendu — rejeu des **213 états de la partie humaine gagnante
du 16**, tous solubles par construction — rend **0 MORT**, 94 vivants, 119 inconnus (budget, donc
sans danger). ⚠️ **Première fois que `fp` tourne sur un niveau NON RÉSOLU** : ce sont les parties à
la main qui le permettent.

**CÂBLÉ derrière `PAQUET=1`** (défaut inactif, cf. `solveurastar.h`). Canari **intact** sur onze
niveaux : 4 / 97 / 131 / 134 / 357 / 151 / 110 / 90 / 240 / 237 / 213 poussées, inchangées.

⚠️ **UN BUG DE CÂBLAGE, TROUVÉ PAR L'UTILISATEUR EN LANÇANT L'UI** (« je l'ai lancé avec PAQUET=1,
même erreur au coup 18 »). **Le solveur a DEUX points d'enfilage** — la recherche principale et
`plonge()`, qui a sa propre copie des élagages. Le test n'était câblé qu'au premier, donc le
plongeon explorait librement des états que la recherche refusait, et `nouveauMaxCaisses` enregistre
les records **d'où qu'ils viennent**. **Toutes les mesures prises avant le correctif sont fausses**,
y compris un balayage de budget complet et un « 8/15 sur le 16 » annoncé puis retiré puis
re-confirmé. Corrigé : `cachePaquet` est passé à `plonge()` comme l'est déjà `cacheEnclos`.

**BALAYAGE DU BUDGET, après correctif** (le 9 est le juge : à budget trop haut il cesse de se
résoudre) :

| budget | niv 4 | niv 17 | **niv 9** | niv 16 (300 s) |
|---|---|---|---|---|
| *défaut* | 40 408 | 24 813 | 83 029 | max 7/15 |
| 50 | 17 641 | 21 642 | ✅ 80 352 | max 7/15 |
| 500 | 140 670 | 20 774 | ✅ 78 516 | — |
| 1 000 | 21 694 | 20 774 | ✅ 78 010 | — |
| **2 000** | 9 302 | 20 733 | ❌ **non fini** | **max 8/15** (3 fois sur 3) |

- **La régression du 9 n'est pas le motif, c'est le budget** : il a **33 000 paquets distincts**
  (contre 423 sur le 16), donc le cache n'amortit rien et on paie le budget plein tarif à chaque
  fois — 28,8 M états de sous-solve à 2 000, **1,6 M à 50**.
- **Le 8/15 du 16 se reproduit trois fois sur trois à budget 2 000**, jamais à 50 ni au défaut.
  Le plafond de 7/15 tenait depuis le début du chantier.
- ⚠️ **Le compte d'états en `coupl-plongeon` est du bruit** : le 4 fait 17 641 → 188 105 → 140 670
  → 21 694 → 9 302 entre budgets voisins, sans tendance. Seuls « ça finit / ça ne finit pas » et
  les poussées y sont robustes.

**⚠️ RIEN N'EST PROMU.** Le motif est sound et bon marché, mais **le réglage n'est pas tranché** :
50 sauve le 9 et perd le 8/15 du 16, 2 000 fait l'inverse. Conditionner demanderait un critère, et
un seuil calé sur deux niveaux est exactement ce qui a fait promouvoir le corral-N trop tôt le
2026-07-28. **Observable prometteur, lisible en vol et gratuit : le taux d'amortissement du cache**
(×19 181 sur le 16 contre ×3,9 sur le 9, cinq ordres de grandeur) — à mesurer sur les quinze
résolus avant d'en faire quoi que ce soit.

**Reste ouvert :**
- [ ] Le réglage du budget, ou un gate fondé sur l'amortissement.
- [ ] **Le motif ne débloque PAS le 16** : 8/15 au lieu de 7/15, le niveau reste non résolu.
- [ ] `paquet <fichier.xsb> [budget]` juge un plateau isolé — c'est ce mode qui a servi de juge `fp`.

### 6.2 Ordre de remplissage — multi-salles

`ordreButs` (rebours) sait vider **une** salle (« fond → entrée »). Sur plusieurs salles —
**10** (28+4), **18** (7+2+2), **24** (20+2), **25** (17+2), **26** (12+1) — il produit un ordre
**mélangé** (un but d'ici, deux de là) ; la macro rebondit entre salles et se mure.

**Mesuré sur le 10 (solution main, 2026-07-17).** La séquence des arrivées-sur-but est
`GGGGGGG DDD…D` : la satellite (4 buts) est remplie **d'un bloc**, puis la grosse (28). Le
re-couvrage tardif de la case d'entrée (2,10) est la danse de la case-porte (comme (4,11) au
niv 11), **pas** un entrelacement. **L'ordre entre salles est LIBRE** (dixit l'utilisateur : « on
pourrait faire G d'un coup n'importe quand ») — ce qui compte, c'est **ne pas entrelacer**.

- [x] **Correctif minimal** dans `calculDistancePoussee()` (où `ordreButs` est construit) :
  grouper les buts par **composante connexe** (= salle), rebours **dans** chaque composante,
  émettre **salle par salle** (jamais mélangé). `butActif()` finira alors une salle avant
  l'autre, automatiquement.
  ✅ **FAIT le 2026-08-01** (session dédiée en fin de §6.2) : **×7,54 sur le 10** (2 160 492 → 286 428
  états, 544 poussées des deux côtés), binaire contre binaire. Réalisé non pas en post-passe mais
  **dans le tri topologique** — grouper après coup casserait les arêtes de précédence.
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

- [x] 🎉 **Référence à conserver — ordre humain du 12, et IL RÉSOUT LE NIVEAU** (donné par
  l'utilisateur le 2026-07-31 après lecture de l'ordre calculé, mesuré dans la foulée).
  Salle de 15 buts, trois colonnes x=13/14/15 sur y=5..9 :
  ```
  (15,9) (15,8) (15,7) (15,6) (15,5)   ← colonne 15, BAS→HAUT
  (13,5) (13,6) (13,7) (13,8) (13,9)   ← colonne 13, HAUT→BAS
  (14,9) (14,8) (14,7) (14,6) (14,5)   ← colonne 14 (centrale), BAS→HAUT, en dernier
  ```
  ⚠️ **Les deux colonnes latérales se remplissent en sens OPPOSÉS** — c'est le point que la règle
  calculée rate. Elle les fait toutes deux de haut en bas et les entrelace :
  `(13,5)(13,6)(13,7) (15,5)(15,6)(15,7)(15,8) (13,8)(15,9)(13,9)`, puis récupère correctement la
  colonne centrale bas→haut. ⚠️ Réserve du §6.2 : « humain et gagnant » ne veut pas dire « bon pour
  la macro » — l'ordre humain du 11 de juillet coûtait 460 000 états sur le 191 là où le calculé en
  coûte 27. **Mesuré ici, et la réserve ne s'applique pas : cet ordre-là est bon.**

  **Le résultat, même binaire, `coupl-plongeon`, 120 s de budget chacun :**

  | ordre | résultat |
  |---|---|
  | calculé (défaut) | 1 808 000 états, **`max 1/15`** — pas une deuxième caisse posée |
  | **humain (injecté)** | ✅ **RÉSOLU — 2 097 527 états, 212 poussées**, plongeon gagnant depuis 6/15 |

  ⚠️ **L'ORDRE ÉTAIT LE VERROU DU 12, et le plan affirmait le contraire.** Le §6.2 écrivait « le 12
  l'illustre — ordre parfaitement sain, et il échoue quand même (son problème est ailleurs) ». Son
  ordre était sain **au sens des tests** et mauvais en pratique ; le corriger suffit à le faire
  tomber. ⚠️ Obtenu avec `ORDRE_HUMAIN` (variable d'environnement, outil de chantier) : **non
  reproductible avec le binaire par défaut**, donc pas encore une ligne de [scores.md](scores.md).
  Les 212 poussées viennent du plongeon et ne valent pas comme canari.
- ⚠️ **LE 12 EST LE CONTRE-EXEMPLE QUI MANQUAIT AUX OUTILS** (2026-07-31). `ordre 12` rend **0
  violation locale, 0 violation globale, 0 arête-paire violée, aucun murage** — et l'ordre est
  quand même mauvais, de l'avis de l'utilisateur qui sait jouer le niveau. Les trois tests
  attrapent donc les ordres **INFAISABLES**, pas les ordres **MAL JOUÉS**. C'est la réserve que
  l'outil imprime lui-même (« ce test ignore l'ACHEMINEMENT ») enfin illustrée par un cas concret :
  jusqu'ici on savait que le 12 était « sain et échouait quand même », sans savoir qu'il était
  **sain à tort**.

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
  ➡️ ⚠️ **REQUALIFIÉ le 2026-08-01** (session « mode hybride », fin du §6.2) : « sain » et
  « déclassé » restent vrais au sens où le 10 tombe sans le correctif — mais son ordre est bel et
  bien **mauvais**, et on sait enfin en quoi. La satellite est éclatée aux rangs 0/14/29/31, donc
  **dès la première pose le but actif part dans l'autre salle** et la macro devient indisponible.
  Mesuré sur une partie humaine gagnante : **4 macros lancées sur 32**, 884 états bloqués sur le
  même but. Le correctif est **re-priorisé**, et son argument n'est plus esthétique.

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

#### ⏸️ Session du 2026-07-31 (soir) — ORDRE DYNAMIQUE, MASQUAGE, INJECTION : où on en est

**Ce qui est ACQUIS :**
1. 🎉 **Le 12 tombe avec l'ordre humain** (ci-dessus). Premier niveau dont on prouve que l'ordre
   était le verrou — et le plan disait l'inverse.
2. ❌ **Les trois tests de précédence n'attrapent pas les ordres MAL JOUÉS.** Sur le 12, l'ordre
   calculé et l'ordre humain passent tous deux à 0 violation locale / globale / par paires et
   « aucun murage » — l'un résout, l'autre n'atteint pas 2 caisses posées. Les tests séparent
   *infaisable* de *faisable*, pas *mauvais* de *bon*.
3. ⚠️ **`ordreButs` n'est appliqué QUE par la macro** (FAIT 3, reconfirmé au code) : les poussées
   simples ne sont générées que si `macrosOk == 0` (`solveurastar.cpp:812`) et **ne lisent jamais
   l'ordre**. Un ordre parfait ne sert donc à rien tant qu'il n'est pas *appliqué*.
4. ⚠️ **Le masquage « un seul but visible » n'a AUCUN effet là où on l'attendait** — vérifié dans le
   code : `getHeuristique` apparie déjà toutes les caisses à tous les buts, `butActif` rend déjà le
   premier non rempli, `checkVictoire` est identique en fin de partie. **Ses seules dents sont dans
   le chemin DEADLOCK** : `game.cpp:213` ne teste `staticDeadlock`/`dynamicDeadlock` que sur
   `tcCaisse`, jamais sur `tcGoalCaisse` — une caisse posée sur un but n'est jamais jugée. Idem pour
   `nOffGoal` du corral (`game.cpp:862`). Masquer = lever cette exemption. **Non fait, décision à
   prendre** (c'est de la détection de deadlock : coupes fausses vis-à-vis du vrai problème, donc
   régime incomplet et LOUD).
5. **Les ordres calculés, jugés par l'utilisateur** : **10 OK** (trois colonnes contiguës bas→haut,
   satellite éclatée aux rangs 0/14/29/31), **11 OK** (à deux transpositions près de la référence
   de juillet), **12 MAUVAIS** (colonnes latérales toutes deux haut→bas et entrelacées, au lieu de
   x=15 bas→haut puis x=13 haut→bas), **13 MURÉ** au rang 14.
6. **Le défaut du 13 est localisé** : la colonne x=16 (7 buts) est attaquée **par les deux bouts** —
   rangs 7,8,9 par le bas, puis rangs 12,13 par le haut — et (16,5)/(16,6) restent en sandwich. Le
   même motif est parcouru contigument sur le 10, donc la règle sait le faire. Les 4 variantes de
   `LIVR_DURE` murent toutes (0 → rang 14 ; 1/2/3 → rang 15, autre but).
7. **Géométrie du 13, utile pour la partie à la main** : deux buts n'ont qu'**une seule approche** —
   **(14,8)** (caisse en (14,7) poussée vers le bas, joueur en (14,6) ; (14,10) est un mur) et
   **(16,4)** (caisse en (16,5) poussée vers le haut, joueur en (16,6) ; (16,2) est un mur et x=18
   interdit l'approche depuis le couloir droit). Ce sont des **théorèmes** : (14,8) avant (14,7) et
   (14,6) ; (16,4) avant (16,5) et (16,6). L'ordre calculé respecte le premier et se piège sur le
   second. **(16,5) est le plus contraint malgré ses 3 approches** — ses trois appuis (16,3), (16,7)
   et (14,5) sont eux-mêmes des buts, et l'ordre calculé les remplit tous les trois avant lui.

**QUESTIONS EN SUSPENS, à reprendre :**
- [ ] **Pousser le masquage jusque dans le chemin deadlock ?** (point 4). Gain : les états où une
  caisse est garée/gelée sur un but hors ordre deviennent élagables. Risque : coupes **fausses**
  vis-à-vis du vrai problème — régime séparé obligatoire, canari des 15 résolus comme juge.
  ⚠️ Ne réglerait PAS le cas vu sur le 13 : la caisse coincée en (14,6) n'a aucune direction
  poussable, donc `dynamicDeadlock` (qui exige `nbPoussable > 0`) ne la voit pas, et
  `staticDeadlock` regarde `casesMortes` où (14,6) est un but. Il faudrait le test de **gel**.
- [ ] **Faire respecter l'ordre aux poussées simples (R1)** — toujours pas fait, et c'est ce qui
  manque pour que le 12 tombe **sans** injection. Deux variantes : sèche (interdire toute pose hors
  ordre dans le repli) ou avec exemption de transit. ~~⚠️ Le cas du 13 penche pour la sèche : la
  caisse fautive de (14,7) peut encore bouger latéralement, donc une exemption la laisserait
  passer.~~ ❌ **RÉFUTÉ le 2026-07-31 (nuit)** : la partie gagnante du 13 fait **84 départs de but**
  pour 16 poses définitives — la sèche interdirait la solution. Cf. la session dédiée ci-dessous.
- [ ] **Corriger la contiguïté de run** pour que le 12 et le 13 sortent le bon ordre tout seuls.
  Le 12 donne enfin une **cible mesurable** (son ordre humain résout), le 13 une cible localisée
  (colonne x=16). Diagnostic manquant : les candidats et leurs clés de tri au rang décisif — c'est
  la trace `ORDRE_TRACE` retirée le 2026-07-20, une dizaine de lignes à remettre.
  ⚠️ **Le 13 n'est plus une cible valable** (session ci-dessous) : la règle qui reproduirait son
  ordre humain reproduirait un ordre **mesuré perdant**. Reste le 12, et le murage de 18/22.
- [x] **Ordre à la main du 13** — ✅ **RELEVÉ ET MESURÉ le 2026-07-31 (nuit)**, session dédiée
  ci-dessous. La règle EST sortie (« remplir en peigne, du fond vers l'entrée ») ; l'ordre, lui,
  fait **perdre** le solveur.
- [ ] **Régime `ordre-dyn` : à garder ou à jeter ?** Il coûte plus cher que `coupl-plongeon` partout
  où il a été mesuré (6 : ×87, 7 : ×9, 5 : ×8,3 ; 17 : −11 %), résout les 8 niveaux testés, et n'a
  débloqué ni le 13 (8/16 contre 9/16 à 120 s) ni rien d'autre. Sa garde anti-échouage est
  **relâchée presque en permanence** (trace `[ordre]`), donc elle ne sert quasiment à rien.
  ➡️ **NE PAS LE JETER TOUT DE SUITE** (2026-08-01, fin du §6.2) : on sait maintenant **ce qu'il
  achète**, ce qui manquait pour trancher. Avec un ordre STATIQUE, la macro est indisponible pour
  tout but qu'on n'est pas en train de faire — sur le 10 joué à la main, **886 états sur 1 658**
  (53 %) sans aucune macro, le but actif figé sur un but d'une autre salle. Ça ne prouve pas qu'il
  paie ; ça donne enfin l'hypothèse à tester. ⚠️ Le tester **après** le correctif salle par salle,
  pas avant : celui-ci supprime la majeure partie du symptôme à coût nul.
- [ ] **Le 18 restera muré quoi qu'on fasse** au modèle statique : son espace de recherche est
  réellement épuisé à budget 200 (il n'escalade pas). Aucun ordre sain complet n'y existe.

**État du code, non commité** (branche `ordre-dynamique`) : `butActif` dynamique + garde
anti-échouage + trace `[ordre]` (`game.cpp`/`game.h`) ; régime d'essai `ordre-dyn` (`solveur.*`,
`mesures/bench.cpp`) ; **injection `ORDRE_HUMAIN`** (`game.cpp`, outil de chantier, jetable) ;
affichage UI du but actif en bleu et des autres en sable sur l'état-max (`goal.cpp`, `wgame.*`,
`mainwindow.cpp`).

#### ❌ Session du 2026-07-31 (nuit) — L'ORDRE HUMAIN DU 13 : relevé sur partie gagnante, et il fait PERDRE

**La partie.** L'utilisateur a joué le 13 à la main jusqu'au bout et livré la trace `[mouv]`
complète (le `qDebug` par coup de `mainwindow.cpp`, gardé depuis le 2026-07-20 — il vient de payer
une deuxième fois). **Rejouée avec contrôle de légalité** à chaque poussée — caisse présente à la
source, destination libre, appui atteignable par flood-fill depuis la position réelle du joueur —
elle rend **276 poussées, toutes légales, 16/16 : GAGNÉ**. Les quatre `[undo]` forment deux groupes
qui annulent exactement deux poussées ; c'est pris en compte.

⚠️ **Le rejeu n'est pas une précaution de confort.** C'est lui qui garantit qu'une transcription de
276 poussées ne porte pas d'erreur silencieuse : une poussée fausse casse la légalité d'une
suivante, et le seul rejeu qui va au bout est le bon. Même exigence que le « vérifier que le chemin
reconstruit mène bien à l'état exporté » de `bench … record` (§6.3) — et même raison, le bug
big-endian de `mou` (§5). **Les chiffres ci-dessous ne sont pas une lecture de la trace, ils sont un
rejeu.** (Transcription et script : jetables, non versionnés.)

**L'ORDRE DÉFINITIF DE POSE** (dernière arrivée sur chaque but, transits ignorés — la même
extraction que sur la trace du 192 le 2026-07-20) :

```
(14,8) (14,6) (15,3) (14,4) (16,3) (16,4) (16,6) (14,3)
(16,8) (16,5) (14,5) (16,7) (14,7) (16,9) (15,9) (14,9)
```

**Il passe les trois tests** (`ordre 13` sous `ORDRE_HUMAIN`) : 0 violation locale, 0 globale,
0 arête-paire, aucun murage. L'ordre calculé, lui, est **MURÉ au rang 14 sur (16,3)** et viole 2
arêtes globales. **Le 13 n'est donc PAS un second 12** : ici les outils voient bien le défaut.

**❌ ET POURTANT IL FAIT PERDRE.** Même binaire (`8ae4cc9`, arbre propre), même régime
`coupl-plongeon`, seule la variable d'environnement diffère :

| | ordre **calculé** (défaut) | ordre **humain** (injecté) |
|---|---|---|
| meilleur remplissage | **9/16**, atteint dès **544 445** dépilés | **1/16**, encore 1/16 à **5 802 000** dépilés |
| plongeons tentés | 8 | **0** — aucun record ne se produit |
| `rangees` en régime courant | 1 à 6 | **0** |

**Dix fois plus de travail pour neuf fois moins de caisses posées**, et la signature est celle du
**Groupe A** (§0) : la macro ne s'engage jamais, le solveur brûle des états bon marché en poussées
simples (5,8 M dépilés en 300 s contre ~1,8 M pour la référence, qui elle fait tourner macro et
corrals).

> ⚠️ **L'INVERSION EST COMPLÈTE : l'ordre que les trois tests VALIDENT est celui qui perd, celui
> qu'ils déclarent MURÉ est celui qui avance.** C'est le pendant exact du 12, où l'ordre humain
> débloquait. La réserve « humain et gagnant ne veut pas dire bon pour la macro » (§6.2, 2026-07-20)
> était connue **dans un seul sens** — « moins bon » (l'ordre du 11 de juillet : 460 000 états contre
> 27). Elle vaut aussi dans le sens **catastrophique**, et un ordre validé par les outils n'est pas
> une garantie de quoi que ce soit.

**LA GÉOMÉTRIE, et elle explique le prix.** La salle est un **peigne** : deux colonnes de 7 buts
(x=14 et x=16) séparées par les dents (15,4) (15,6) (15,8), et bordées à gauche par (13,4) (13,6)
(13,8). Conséquences, toutes statiques :

- **6 buts sur 16 n'ont AUCUNE approche latérale** — (14,4) (14,6) (14,8) et (16,4) (16,6) (16,8) :
  ils ne peuvent être servis qu'en circulant *le long* de leur colonne. Ce sont les « murs chiants »
  de l'utilisateur, et ils occupent les rangs **0, 1, 3, 5, 6, 8** de l'ordre humain — tous dans les
  neuf premiers.
- **La rangée 3 n'est alimentée QUE par le haut de la colonne 16.** Une caisse ne peut entrer en
  (15,3) que depuis (16,3) poussée à gauche (joueur en (17,3)) ou depuis (14,3) poussée à droite
  (joueur en (13,3)) — et **(13,3) n'est atteignable que depuis (14,3) poussée à gauche**, donc
  depuis l'intérieur. Il n'y a pas d'entrée externe en haut de la salle.
- **D'où l'ENROULAGE, qui est littéral** : la première caisse fait rangée 9 vers la droite →
  (16,9) → poussée jusqu'en haut de la colonne 16 → (16,3) → rangée 3 vers la gauche → (15,3) →
  (14,3) → garage en **(13,3)**, puis relancée à droite et descendue toute la colonne 14 jusqu'en
  **(14,8)**. Une trentaine de poussées pour une caisse. **17,2 poussées par caisse** sur l'ensemble
  de la partie (276 pour 16) — c'est le prix du tour.

**LA RÈGLE EST SORTIE, elle, et elle est formulable** (l'utilisateur annonçait l'inverse : « ça va
pas être simple d'en sortir une règle ») :

> Chaque ligne de buts est alimentée par une **extrémité**. On la remplit **du fond vers l'entrée** —
> mais **EN PEIGNE** : on saute les buts qui gardent une alimentation latérale indépendante, parce
> qu'ils servent de passage au joueur et d'entrée aux caisses tant qu'ils sont vides. Ils se
> comblent en dernier.

Sur le 13 elle rend exactement les rangs relevés : colonne 14 (alimentée par le haut) → **8, 6, 4**
puis (14,3) puis 5, 7, 9 ; colonne 16 (alimentée par le bas) → **3, 4, 6, 8** puis 5, 7, 9. C'est
précisément ce qui manque à la **contiguïté de run**, qui remplit *en continu* — et ça explique le
défaut noté le 2026-07-31 (soir) : « la colonne x=16 est attaquée par les deux bouts, (16,5)/(16,6)
restent en sandwich ».

- ⚠️ **Mais ne pas la coder en espérant débloquer le 13** : elle reproduirait à peu près l'ordre
  humain, donc le tableau ci-dessus. Elle ne se justifie que pour rendre l'ordre calculé **sain**
  (il est muré) et sur les deux autres murés, **18 et 22**. Bénéfice attendu : nul sur le 13.

**LE CHIFFRE EXPLOITABLE, et il tranche une question ouverte.** Les 16 buts reçoivent **100
arrivées** pour **84 départs** — (14,9) est traversé 10 fois, (16,7) et (16,9) 8 fois chacun :

> **Dans cette salle, les cases-BUTS SONT le couloir de livraison.** Il n'y a pas d'autre route.

- **❌ La variante SÈCHE de R1 est morte sur le 13** (interdire toute pose hors ordre aux poussées
  simples) : elle interdirait les 84 transits qui font la solution. Le plan penchait pour la sèche
  la veille au soir, sur l'argument « la caisse fautive de (14,7) peut encore bouger latéralement » ;
  la partie dit le contraire. **Sixième déguisement du piège « caisses = murs »** (gel naïf, `h` qui
  soustrait, caisses=murs, gelées=murs, buts-remplis=murs, et maintenant buts-dans-l'ordre=murs).
- **✅ Le MASQUAGE est le bon mécanisme, et pour une raison mesurée** : un but non révélé est du sol
  ordinaire, donc les 84 transits passent **gratuitement**, sans aucune exemption à écrire. C'est
  l'argument « R1 gratuitement » du 2026-07-31 (soir), enfin appuyé par un chiffre.

**Reste ouvert :**
- [ ] **Le verrou du 13 n'est pas l'ordre — c'est l'EXÉCUTION de l'enroulage.** La macro sait le
  faire **une fois** (elle atteint 1/16), jamais deux : chaque but suivant redemande un tour complet
  avec quinze caisses dans le passage. C'est `echecBloque` (§6.3) sur un trajet de 30 poussées.
  Piste à discuter avant de coder : rien dans le plan n'attaque ce cas.
  ➡️ ⚠️ **REQUALIFIÉ le 2026-08-01** (dernière session du §6.2) : ce n'est pas l'exécution, c'est la
  **GÉNÉRATION**. Mesuré sur une partie gagnée à la main : une macro vers l'entrée (14,9) est
  engageable dans 87 % des états, donc `solveurastar.cpp:792` supprime les poussées simples et **89 %
  des coups de la solution ne sont l'enfant de rien**. La macro n'échoue pas — elle est offerte en
  permanence, et elle est **perdante**.
- [ ] **Ne pas relancer le 13 sur l'ordre.** Trois voies y ont été fermées en deux jours : l'ordre
  calculé (muré), l'escalade de budget (chargement non borné), l'ordre humain (perdant).

#### 🎯 Session du 2026-08-01 — LE MODE HYBRIDE : cinq niveaux rejoués à la main, annotés par le solveur

**L'outil** (§1, idée utilisateur) : on joue à la main pendant que l'UI rejoue à chaque coup le
**régime d'engagement du solveur** et affiche l'ordre de remplissage sur les buts. Tout est
journalisé. Ça répond à la question que ni `ordre`, ni `fp -3`, ni la précédence par paires ne
savaient poser — **« l'ordre est-il BIEN JOUÉ ? »**, et non « est-il faisable ? » (c'est la réserve
du 12, §6.2 : les trois tests séparent l'infaisable du faisable, pas le mauvais du bon).

**Cinq niveaux joués et GAGNÉS à la main : 1, 2, 3, 4 et 10.** Aucun n'est un score
([scores.md](scores.md) ne bouge pas : ce sont des parties humaines, pas des solves).

**FAIT 1 — LA MACRO POSE 100 % DES CAISSES. Les poussées simples font 100 % du démêlage.**

| niveau | buts | **macros lancées** | coups | poussées (optimum) |
|---|---|---|---|---|
| 1 | 6 | **6** | 256 | 97 (= 97) |
| 2 | 10 | **10** | 517 | 157 (131) |
| 3 | 11 | **11** | 376 | 138 (134) |
| 4 | 20 | **20** | 1 156 | 418 (355) |

Pas une seule caisse posée à la main sur les quatre. **Ce sont deux machines disjointes**, et ça
déplace le diagnostic : le problème du solveur n'est pas que la macro soit mauvaise — elle ne rate
rien — c'est qu'il n'a **aucun guidage** pendant la phase où elle ne peut pas s'engager.

**FAIT 2 — l'ordre calculé de 1/2/3/4 est bon ET jouable : 0 inversion sur les quatre.** C'est la
première fois qu'on le sait. ⚠️ Réserve : l'ordre était AFFICHÉ, donc c'est de la conformité
volontaire — ça prouve qu'il est **jouable jusqu'au bout**, pas qu'il soit optimal.

**FAIT 3 — LES TRANSITS SONT MASSIFS, et cette fois sur cinq niveaux** (le plan ne les tenait que
du 13, 100 arrivées / 84 départs) :

| niveau | 2 | 3 | 4 | **10** |
|---|---|---|---|---|
| arrivées sur but | 15 | 26 | 59 | **209** |
| poses définitives | 10 | 11 | 20 | 32 |
| **transits** | 5 | **15** | **39** | **177** |

> **La variante SÈCHE de R1 est re-tuée, cinq fois.** Interdire les poses hors ordre aux poussées
> simples interdirait 177 transits sur le seul niveau 10. Le **masquage** reste le seul mécanisme
> qui les laisse passer gratuitement (§6.2, 2026-07-31 nuit).

**FAIT 4 — le démêlage d'entrée du 4, mesuré : 142 des 153 états qui précèdent la première pose
n'offrent AUCUNE macro** (93 %), en une seule plage continue. Contre 12 sur le 1, 19 sur le 2, 3 sur
le 3. Ce chiffre-là n'est pas contaminé par le FAIT 6 ci-dessous : rien n'était encore posé, il n'y
avait aucun ordre dont s'écarter. **Le trou de démêlage est un trou de GUIDAGE** — `ordreButs` n'y
est pas lu (les poussées simples ne le lisent jamais), les tie-breaks sont épuisés (§6.1), il ne
reste rien. C'est la seule phase du jeu où le solveur n'a aucune information directionnelle.

**FAIT 5 — LE 10, TROIS ORDRES INTER-SALLES, TOUS GAGNANTS.** Le 10 est le multi-salles d'école
(satellite 4 buts en (2-3, 10-11), grosse salle 28 buts en x=15-17). L'ordre calculé **entrelace** :
satellite aux rangs **0, 14, 29, 31**.

| ordre joué | satellite aux rangs joués | coups | poussées | **macros lancées** | états sans macro |
|---|---|---|---|---|---|
| l'ordre calculé, suivi | 0,1,20,31 | 1 607 | 519 | **30/32** | 133 (8 %) |
| satellite d'un bloc, en 1ᵉʳ | 0,1,2,3 | 1 610 | 521 | **29/32** | 132 (8 %) |
| **grosse salle d'abord** | 28,29,30,31 | 1 658 | 524 | **4/32** | **886 (53 %)** |

- **L'ordre ENTRE salles est libre au sens du coût** — les trois gagnent, à +3 % près. Ça confirme
  le « on pourrait faire G d'un coup n'importe quand » de juillet. Ce qui n'est pas libre, c'est
  **d'entrelacer**.
- **L'EXCEPTION « PORTE » EST MORTE.** La partie « satellite d'un bloc » met (2,10) en **4ᵉ
  position**, pas au rang 20 comme la partie de juillet. La porte n'a pas besoin d'attendre la
  grosse salle : sa précédence est **interne à la salle**, et `ordreParPrecedence` la produit déjà.
  Le correctif salle par salle n'a donc **aucune exception à écrire**.
- **Mais la 3ᵉ variante n'est pas exploitable par le solveur** : 4 macros sur 32, 28 caisses posées
  à la main. Cause lue dans le journal — `butActif()` rend le premier but non rempli de `ordreButs`,
  donc **(3,10), rang 0, reste le but actif pendant 884 états** pendant qu'on remplit l'autre salle.
  Aucune macro vers la grosse salle n'est jamais générée.

**FAIT 6 — ⚠️ LE PIÈGE DE LA SESSION, tombé dedans en direct.**

> **La métrique « états sans macro » ne mesure PAS la difficulté — elle mesure l'ÉCART À
> `ordreButs`.** Dès qu'on joue autre chose que le but actif, elle sature.

Le 113 contre 87 des deux premières variantes avait été lu comme « aller poser 4 caisses à l'autre
bout concentre le démêlage ». **Faux** : après (3,10) rang 0, le but actif saute à (15,8) rang 1 —
dans l'autre salle — pendant qu'on finit la satellite ; les 113 états sont bloqués sur (15,8), pas
sur une difficulté de plateau. C'est la 3ᵉ variante (884 états sur (3,10)) qui a rendu le mécanisme
lisible. **Un instrument d'observation peut mesurer l'observateur** — cf. §7.
Les chiffres de 1/2/3/4 ne sont PAS touchés : 0 inversion, donc aucun écart possible.

**CE QUE ÇA CHANGE POUR LE CORRECTIF MULTI-SALLES** (ouvert depuis le 2026-07-17, déclassé le
2026-07-29 au motif que « le 10 est tombé sans lui ») : il gagne une justification qu'il n'avait
pas. Ce n'est plus « l'humain préfère ne pas entrelacer », c'est **entrelacer rend la macro
indisponible** — dès la première pose, le but actif part dans l'autre salle.

- [x] **À CODER, décidé le 2026-08-01** — dans `calculDistancePoussee()` : composantes
  connexes de buts (= les salles), rebours + contiguïté **inchangés à l'intérieur** de chaque
  composante, **émission salle par salle**. Aucune exception « porte ». ~~Les niveaux à une seule
  salle sortent **inchangés à l'octet par construction** — donc zéro risque sur les 15 résolus.~~
  Juges dans l'ordre : `ordre` sur les 35 (0 violation partout), canari au solveur, puis `bench 10`.
  ✅ **FAIT le 2026-08-01**, les trois juges passés, **×7,54 sur le 10**. ⚠️ Le « zéro risque sur les
  15 résolus » était **faux** : le **0 et le 10 sont multi-salles et résolus** (mesuré avant de coder,
  cf. la session dédiée). Le 0 ressort tout de même identique — trois salles d'un but sont déjà
  groupées trivialement.
- [ ] **Prédiction à vérifier après le correctif** (c'est elle qui le valide ou l'enterre) : avec la
  satellite aux rangs 0-1-2-3 puis la grosse salle, la partie « satellite d'un bloc » doit passer de
  **29/32 macros et un trou de 113** à **~32/32 et pas de trou**. Rejouable en dix minutes.
- [ ] **Ordre inter-salles : ne rien graver.** Les trois variantes gagnent ; rien dans cette session
  ne dit quelle salle doit passer en premier. Défaut stable, point (§6.2, piège du 2026-07-17 :
  « ne PAS inventer de règle inter-salles à partir du seul niveau 10 »).

**L'ARGUMENT NEUF POUR `ordre-dyn`**, que le plan s'apprêtait à jeter (« coûte plus cher partout,
n'a rien débloqué ») : avec un ordre STATIQUE, la macro est indisponible pour **tout but qu'on n'est
pas en train de faire**. Sur un multi-salles, c'est la moitié de la partie (886 états sur 1 658).
Ce n'est pas une preuve qu'`ordre-dyn` paie — il reste plus cher partout où il a été mesuré — mais
c'est la première fois qu'on sait **ce qu'il achète**.

**Reste à faire :**
- [ ] **Rejouer les autres niveaux dans ce mode** (campagne en cours). Ce qu'il faut lire dans un
  journal : les inversions par rapport à `ordreButs`, les `-> POUSSEES SIMPLES` (et le but actif qui
  les bloque, pour ne pas retomber dans le FAIT 6), le rapport transits/poses, et les causes
  `[manque]`.
- [x] **Le 13 et le 12 dans ce mode** — ce sont les deux niveaux dont le plan dit le plus de choses
  contradictoires sur l'ordre. Le mode devrait trancher sans une ligne de solveur.
  ✅ **13 FAIT le 2026-08-01** (dernière session du §6.2) : il a tranché, et pas sur l'ordre — le 13
  est **gagné à la main** et 89 % de cette partie est hors de l'arbre du solveur. **Le 12 reste à
  faire**, et c'est maintenant le plus intéressant des deux : son ordre humain, lui, RÉSOUT.

**État du code** (non commité, branche `ordre-dynamique`) : `mainwindow.ui` (case `cbHybride`),
`mainwindow.*` (surcouches, journal, exécution de macro, marche au clic, rapport `[manque]`,
`plateauXsb` factorisé avec l'export), `wgame.*` (rangs des buts, macros jouables, cases signalées,
clic droit). `solveur.h` : `appuis` passée de `protected` à `public` — l'UI descend les poussées en
coups de marche par la même recette que `reconstruire()`, exemplaire unique. **Rien dans le
solveur**, aucune variable d'environnement (§7). Canari vérifié après coup : 4/97/131/134/213
poussées, 4/14/412/499/24 786 états.
➡️ ⚠️ **« Rien dans le solveur » n'est plus vrai depuis la session ci-dessous** (2026-08-01,
suite) : deux constantes (`CORRAL_BUDGET`, `corralActif`) sont passées de `solveurastar.cpp` à
`solveurastar.h` pour que le miroir de l'UI ne puisse pas dériver. Aucun changement de
comportement, vérifié binaire contre binaire.

- [ ] **Repli anytime pour la macro** : passe 1 avec macro plafonnée en états, passe 2 sans
  macro si le budget est épuisé. Le repli doit se déclencher sur le **budget**, pas sur
  l'échec (un cas lent n'émet jamais « aucune solution »). Borne surtout le temps des cas
  lents (8, 9).

#### ❌ Session du 2026-08-01 (suite) — LE RANG DU COUP HUMAIN : le démêlage n'est PAS un problème de CLASSEMENT

**La question, et pourquoi elle valait d'être posée.** Le FAIT 4 ci-dessus disait « le trou de
démêlage est un trou de GUIDAGE » — les poussées simples ne lisent jamais `ordreButs`, les
tie-breaks sont épuisés (§6.1), il ne reste rien. Mais « le solveur n'a aucun guidage » n'était pas
un nombre. D'où la mesure : **à quelle place le solveur classerait-il la poussée qu'on vient de
jouer à la main ?** Rang 1-3 en permanence ⇒ le démêlage n'est pas un problème de guidage et il faut
chercher ailleurs ; rang 80 ⇒ on connaît l'écart à combler, et le journal devient un corpus étiqueté
(état → bon coup) pour juger une `h` candidate **hors ligne**, sans écrire une ligne de solveur.

**L'outil** (`MainWindow::mesureRangCoup`, cf. §1) : à chaque poussée **vraiment choisie** (ni rejeu
de solution, ni exécution de macro), l'UI rejoue l'enfilage de `solveurastar.cpp` sur l'état d'avant
— mêmes enfants, mêmes élagages dans le même ordre (`isPerdu` → corral unitaire → corral-N au même
budget), même clé de tri que le comparateur — et journalise le rang du coup joué.

**RÉSULTAT — et c'est un résultat NÉGATIF, donc le plus utile de la session.** Les neuf journaux
existants rejoués hors ligne (harnais jetable, non versionné), **204 poussées mesurées** en régime
poussées simples :

| niv | mesurées | enfants (moy.) | rang moyen | **rang 1** | **`df` = 0** | rang max |
|---|---|---|---|---|---|---|
| 4 | 60 | 6,8 | 3,12 | 45 | 75 % | 18 |
| 7 | 37 | 4,1 | 2,43 | 9 | 62 % | 6 |
| 8 | 33 | 8,0 | 2,06 | 21 | 64 % | 6 |
| 10 | 22 | 9,6 | 2,41 | 15 | 68 % | 9 |
| 6 | 20 | 4,8 | 3,60 | 1 | 85 % | 5 |
| 2 | 18 | 5,3 | 3,33 | 1 | 39 % | 6 |
| 3 / 5 / 1 | 7 / 4 / 3 | ~4,4 | 2,6 / 4,0 / 2,0 | 3 | — | 5 |

- ⚠️ **LE FACTEUR DE BRANCHEMENT EST MINUSCULE : 3,3 à 9,6 enfants.** « Rang 80 » n'était pas au
  menu — il n'y a structurellement pas la place. **Le bon coup n'est jamais enterré** : 1ᵉʳ dans
  **47 % des cas** (95/204), jamais au-delà du 18ᵉ, et le pire maximum sur un niveau est 9.
- **67 % des coups humains sont sur le MÊME palier `f` que le meilleur enfant** (`df = 0`, 137/204).
  A\* les développera de toute façon (§3, régime `f < C*`), et un tie-break n'y mord pas. **Ce que
  l'humain sait ne se lit donc pas dans le classement d'un état isolé** — il porte sur un PLAN de
  plusieurs coups, que cette mesure ne peut pas voir par construction.
- **`df` ne vaut jamais que 0, +2 ou +8, jamais impair.** Le +2 (47 cas) est la signature du
  **RECUL** de `moureel` (§3) : l'humain joue délibérément une poussée de congestion là où une
  poussée productive existait. Le +8 (5 cas, tous sur le 4) est un réarrangement du couplage
  hongrois, cohérent avec `deltaf` (§6.3) — non creusé.

**CONSÉQUENCE POUR LA FEUILLE DE ROUTE.** La piste « guidage du démêlage par un meilleur classement
des poussées simples » est **fermée avant d'avoir coûté une ligne de solveur** — c'est exactement ce
que le guidage par portes avait coûté en 2026-07-21, et qui avait été payé plein tarif. Ce qui
reste : capturer l'INTENTION sur plusieurs coups (piste discutée, non codée : vocabulaire fermé de
cinq motifs sur le coup joué), ou l'affectation caisse→but voulue, comparée au hongrois.

⚠️ **DEUX RÉSERVES, et elles bornent la portée :**
1. **Tous ces niveaux sont RÉSOLUS.** Le démêlage qui bloque 13/18/22/27 peut être d'une autre
   nature. **La prochaine mesure utile est une partie à la main sur un NON-RÉSOLU** — c'est elle qui
   confirme ou casse la conclusion.
2. **Trois des neuf niveaux n'apportent que 3 à 7 points** (1, 3, 5) et ne pèsent rien (§11.4).
   L'essentiel vient du 4 (60), du 7 (37), du 8 (33).

**✅ EFFET DE BORD NON PRÉVU — le juge `fp` s'étend aux niveaux NON RÉSOLUS.** Sur une partie gagnée,
tout état traversé est soluble par construction : un élagage qui écarte le coup joué est donc un
faux positif **PROUVÉ**, le raisonnement exact de `mesures/fp`. Or `fp` exige une solution de
référence, donc il ne tourne que sur les résolus. Le mode hybride en fabrique une **à la main**.
Mesuré ici : **0 coup écarté sur 204**, sur 9 niveaux — corroboration indépendante du corral
(unitaire, pince et N confondus). ⚠️ La preuve ne vaut que si la partie est effectivement gagnée et
qu'aucun `[undo]` n'intervient après ; le journal porte les deux, le dépouillement tranche.

**LE DÉPOUILLEMENT DE 5, 6 ET 7** (journaux fournis le même jour, parties gagnées à la main) —
**il corrige DEUX faits de la session du matin :**

| | buts | **macros** | coups | poussées | arrivées/poses → transits | états sans macro | inversions |
|---|---|---|---|---|---|---|---|
| 5 | 12 | **12/12** | 421 | 143 | 30 / 12 → **18** | 26 (6 %) | **0** |
| 6 | 10 | **7/10** | 368 | 116 | 22 / 10 → **12** | 46 (12 %) | **0** |
| 7 | 11 | **10/11** | 369 | 120 | 32 / 11 → **21** | **143 (39 %)** | **0** |

- ❌ **LE FAIT 1 TOMBE : la macro ne pose PAS 100 % des caisses.** Quatre poses à la main — les rangs
  **3, 4, 5 du niveau 6** (la colonne (1,5)(1,4)(1,3), d'un bloc) et le **rang 0 du niveau 7**
  (10,6). Le rang 3 du 6 est précisément là où le clic droit a consigné un `[manque]` :
  `DESCENTE BLOQUEE en (3,4), reste 3`. Sur les quatre niveaux du matin elle ne ratait rien ; sur
  neuf, elle rate quatre fois.
- ⚠️ **LE FAIT 4 EST À NUANCER : le démêlage n'est pas qu'un trou d'ENTRÉE.** Sur le 4, les 142 états
  sans macro formaient **une plage continue avant la première pose**. Sur le 7, les 143 sont éclatés
  sur **quatre buts en plein milieu** (35 + 35 + 31 + 26) : après chaque pose il faut aller rechercher
  une caisse loin, et la macro est indisponible pendant tout le trajet. **Deux formes différentes du
  même trou**, et la seconde n'est pas couverte par « le démêlage se joue au début ».
- **Le compteur est propre ici** (pas le piège du FAIT 6, §7) : **0 inversion sur les trois**, les
  macros suivent `ordreButs` rang par rang. Les 143 états du 7 sont donc une vraie indisponibilité de
  macro, pas un écart à l'ordre.
- ⚠️ **PIÈGE DE DÉPOUILLEMENT, tombé dedans en direct** : `grep -c POUSSE` compte AUSSI les lignes
  `-> POUSSEES SIMPLES` du journal. Les poussées de 5/6/7 avaient été annoncées à 169/162/263 avant
  correction (vraies valeurs 143/116/120) — l'erreur vaut jusqu'à **+119 %** sur le 7, celui qui a
  le plus d'états sans macro. **Compter `POUSSE caisse`, jamais `POUSSE`.**

**Canari, binaire contre binaire** (worktree sur `HEAD`, `bench <niv> macro`) — les deux constantes
déplacées ne changent rien : **0/1/2/3/5/6/7/17 identiques à l'unité**, 4 / 14 / 412 / 499 / 9 123 /
570 / 24 376 / 24 786 états, 4/97/131/134/143/110/90/213 poussées.

**Reste ouvert :**
- [x] **Rejouer un NON-RÉSOLU en mode hybride** (13, 18, 22 ou 27) : c'est le seul juge de la
  conclusion ci-dessus. Tant qu'il n'est pas passé, « le démêlage n'est pas un problème de
  classement » ne vaut que pour les niveaux qu'on sait déjà finir.
  ✅ **FAIT le 2026-08-01 sur le 13** (session dédiée ci-dessous, niveau **gagné à la main**). La
  conclusion **tient** sur les coups classables (rang moyen 3,9 sur 43) — mais elle est **dépassée** :
  89 % des coups de cette partie gagnante ne sont **pas générés du tout** par le solveur.
- [ ] **Le `df = +8` du niveau 4** (5 cas) : réarrangement du couplage sur une poussée simple, jamais
  vu ailleurs. À regarder si on reprend `h`.
- [ ] **Piste suivante, discutée non codée** : l'INTENTION du coup, en vocabulaire FERMÉ (écarter du
  chemin de telle caisse / ouvrir un passage joueur / rapprocher du but / garer pour plus tard /
  préparer un appui). C'est la seule des pistes envisagées qui capture un plan sur plusieurs coups —
  précisément ce que le rang ne peut pas voir. ⚠️ Vocabulaire clos : du texte libre ne serait pas
  dépouillable.

**État du code** (non commité, branche `ordre-dynamique`) : `mainwindow.*`
(`mesureRangCoup` + appel dans `joue()` + `nomDirection` factorisé avec le journal de macro) ;
`solveurastar.h`/`.cpp` — `CORRAL_BUDGET` et `corralActif` **déplacés** de l'implémentation vers
l'en-tête, sans changement de comportement, pour que l'UI passe le MÊME budget et le même régime
d'élagage que le solveur (deux copies de `150` dériveraient sans que rien ne le signale, §7).
Harnais de rejeu des journaux : **jetable, non versionné** (scratchpad).

#### 🎯 Session du 2026-08-01 (fin) — LE 13 GAGNÉ EN HYBRIDE : 89 % de la solution N'EST PAS DANS L'ARBRE

**C'est l'item que la session ci-dessus réclamait comme seul juge** (« rejouer un NON-RÉSOLU en mode
hybride »). Fait sur le 13, et la réponse déborde la question posée.

**Le journal** (`hybride_niveau_0013.txt`, deux parties) : une abandonnée à 8/16, puis **une GAGNÉE —
914 coups, 276 poussées, 16/16**, 2 `[undo]`. Même total de poussées que la partie de juillet
(§6.2, 2026-07-31 nuit) mais **ordre de pose différent** — j'y reviens plus bas.

**LE RÉSULTAT : le coup humain n'est pas mal classé, il n'est pas GÉNÉRÉ.**

| | poussées | par macro | **choisies** | **jamais générées** |
|---|---|---|---|---|
| **13** (gagné) | 408 | **17 (4 %)** | 391 | **348 (89 %)** |
| 1 / 4 | 194 / 418 | 188 / 358 | 6 / 60 | **0 / 0 (0 %)** |
| 8 / 2 / 3 / 7 | 373 / 157 / 138 / 120 | 259 / 136 / 130 / 77 | 114 / 21 / 8 / 43 | 6 % / 14 % / 12 % / 14 % |
| 5 / 6 | 227 / 126 | 214 / 79 | 13 / 47 | 23 % / 55 % |
| résolus cumulés | 1 846 | 1 180 | 666 | 236 (36 %) |

`HORS REGIME MACRO` ne veut pas dire « mal classé » : dès qu'**une** macro s'engage,
`solveurastar.cpp:792` (`if (macrosOk == 0)`) ne génère **aucune** poussée simple. Sur le 13 une
macro est engageable dans 87 % des états, donc 89 % des coups de la seule solution connue **ne sont
l'enfant de rien**. Le solveur ne peut pas trouver cette partie, quel que soit le temps qu'on lui
donne. C'est l'incomplétude que le §6.0 énonce depuis toujours (« il ne génère que les macros vers le
but actif et abandonne le reste ») — **jamais chiffrée sur une solution réelle jusqu'ici**.

⚠️ **LE TÉMOIN QUI EMPÊCHE DE LIRE ÇA COMME UN ARTEFACT.** Le taux hors-régime mesure aussi l'écart
volontaire à `ordreButs` (c'est le piège du FAIT 6, §7). Le comparatif juste est donc la **3ᵉ partie
du 10**, où l'utilisateur s'écartait délibérément (grosse salle d'abord, 4 macros sur 32) :

| partie du 10 | poussées | macros lancées | choisies | hors régime |
|---|---|---|---|---|
| ordre calculé suivi | 519 | 30 | 31 | 29 % |
| satellite d'un bloc | 521 | 29 | 43 | 35 % |
| **grosse salle d'abord** (écart maximal) | 524 | **4** | **495** | **30 %** |

> **Même en jouant à contre-ordre de bout en bout sur un niveau RÉSOLU, on plafonne à 30 %.** Le 89 %
> du 13 n'est donc pas un effet du désaccord, c'est une **différence de nature** : sur le 10, s'écarter
> rend la macro indisponible (`macrosOk == 0`) et le solveur retombe sur les poussées simples — il
> *pourrait* jouer le coup ; sur le 13 la macro reste engageable **quoi qu'on fasse**, donc le solveur
> reste enfermé en régime macro.

**POURQUOI : le but actif reste figé sur la PORTE D'ENTRÉE.**

| but actif | états | % | dont macro dispo |
|---|---|---|---|
| **(14,9) rang 0** | **709 / 916** | **77 %** | 588 |
| (14,7) rang 3 | 197 | 22 % | 195 |

Or **(14,9) est le but le plus traversé du plateau — 10 arrivées sur 96** : c'est le couloir
d'alimentation de la salle, et l'ordre calculé le pose **au rang 0**. La partie gagnante le pose en
**12ᵉ**. Les 588 macros offertes bouchaient donc l'entrée — et l'utilisateur le confirme de
l'autre bout : *« je connais le niveau et je sais que l'ordre calculé et donc les macros vont me faire
perdre »*. **Le solveur ne génère quasiment que des coups perdants et supprime tous les autres.**

**CE QUE ÇA CONFIRME** (et qui ne bouge pas) :
- **0 coup ÉCARTÉ sur 391** → zéro faux positif d'élagage **prouvé sur un niveau NON RÉSOLU**,
  première fois (corral unitaire + pince + N confondus). ⚠️ 2 `[undo]` dans la partie gagnée, à
  déduire en toute rigueur ; sans effet, le total est nul.
- **La conclusion de la session précédente TIENT** sur les 43 poussées réellement classables :
  branchement 13,4, rang moyen 3,9, rang 1 dans 32 % des cas, `df = 0` à 64 %, max 19. Le démêlage
  n'est pas un problème de classement, et la réserve n° 1 (« tous ces niveaux sont résolus ») est
  levée. ⚠️ **Mais elle est dépassée** : le problème n'est ni en aval (le rang) ni en amont (l'ordre),
  il est à la **GÉNÉRATION**.
- **80 transits** (96 arrivées / 82 départs / 16 poses) — la variante sèche de R1 re-tuée une
  septième fois.

**UN SECOND ORDRE GAGNANT, DIFFÉRENT, ET LA MÊME RÈGLE.** L'ordre de pose relevé :

```
(14,8) (14,6) (14,4) (14,3) | (15,3) (16,3) (16,4) (16,6) (16,8) (16,9) | (15,9) (14,9) | (16,5) (14,5) (16,7) (14,7)
└─ colonne 14, en peigne ─┘   └──── colonne 16, en peigne ────────────┘   └─ l'entrée ─┘   └─ les buts sautés ─┘
```

Rangs calculés correspondants : `1 4 5 6 10 12 13 15 8 7 2 0 14 11 9 3`. **Deux parties gagnantes,
deux ordres distincts, une seule structure** — peigne, puis l'entrée, puis les buts à alimentation
latérale. La règle de juillet (« remplir en peigne, du fond vers l'entrée ») en sort nettement
renforcée : elle n'était appuyée que sur un ordre unique.

**❌ LA PISTE « TRAFIC » : un DÉTECTEUR, pas un correcteur.** L'hypothèse à tester était : le but à
fort trafic doit être posé tard, et ce trafic est prédictible **statiquement**.

*Méthode* — `mesures/passages` ne convenait pas (il **résout** le niveau, ou somme des sous-niveaux à
une caisse qu'il faudrait fabriquer). Miroir Python jetable du BFS de tirage de `precedenceGlobale`
(`game.cpp:1585`), **validé en reproduisant à l'unité les quatre comptes d'arêtes du plan** : 13→4,
11→28, 21→32, 10→99. ⚠️ Un miroir non validé n'aurait rien valu (§7).

| corrélation de rang (Spearman) | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 10 | **13** |
|---|---|---|---|---|---|---|---|---|---|
| trafic RÉEL ↔ rang calculé | +0,94 | +1,00 | +0,84 | +0,78 | +0,83 | +0,65 | +0,73 | +0,74 | **+0,21** |
| **gêne STATIQUE ↔ rang calculé** | 0,94 | 0,93 | 0,44 | 0,81 | 0,73 | 0,03 | 0,81 | 0,39 | **−0,14** |
| gêne statique ↔ trafic réel | 1,00 | 0,93 | 0,54 | 0,72 | 0,72 | 0,10 | 0,92 | 0,52 | **0,01** |

- **La version « peut passer par »** (une case-but est-elle dans le BFS de tirage d'un autre but ?)
  **SATURE** — 15/15 sur les seize buts du 13. Relaxation trop optimiste, elle ne discrimine rien.
  Écartée. La version retenue est la **gêne** : de combien les trajets des autres s'allongent si ce
  but-ci est occupé (∞ = coupé). **Stable à son paramètre** (pénalité de coupure 5 → 200 : les
  résolus ne bougent pas d'un centième, le 13 va de −0,28 à −0,14).
- ✅ **`ordreParPrecedence` respecte DÉJÀ la règle** là où il marche (+0,73 à +0,94 sur 1/2/4/5/7) —
  et c'est logique après coup, sa **garde anti-échouage** fait exactement ce travail. Il n'y a rien à
  ajouter de ce côté.
- ✅ **Le 13 est le seul du corpus à corrélation NÉGATIVE** : son ordre calculé pose les buts gênants
  trop tôt. C'est un **discriminant statique de plus**, dans la lignée de `ordre` (§6.6, voie (a)), à
  quelques millisecondes.
- ❌ **Mais la gêne ne prédit ni le trafic réel du 13 (0,01) ni l'ordre joué (+0,34).** On sait
  **détecter** que son ordre est mauvais, pas **construire** le bon. Et même si on savait : le plan a
  déjà mesuré que l'ordre humain du 13 injecté fait **PERDRE** (1/16 contre 9/16). **L'ordre n'est pas
  le levier du 13** — c'est la troisième voie fermée par là (§6.2, 2026-07-31 nuit : ordre calculé
  muré, escalade de budget, ordre humain perdant ; et maintenant le trafic).

⚠️ **PIÈGE DE DÉPOUILLEMENT, tombé dedans en direct** — le premier tableau annonçait **23 %** au lieu
de 89 % : le test de régime cherchait `macro jouable` et **ratait le PLURIEL** (`2 macros jouables`),
ne comptant que les états à exactement une macro. Détecté parce que le compte ne recoupait pas les
348 lignes `HORS REGIME` que l'UI écrit elle-même. **Deux compteurs indépendants du même fait dans le
même journal, c'est ce qui a sauvé la mesure** — même leçon que `POUSSE caisse` contre `POUSSE`.

**Reste ouvert :**
- [ ] **Le verrou du 13 est la GÉNÉRATION, et rien dans le plan ne l'attaque.** Ce qui l'adresserait :
  laisser le solveur **refuser de s'engager** — générer les poussées simples *en plus* des macros sous
  condition. C'est un élargissement combinatoire majeur (le régime d'engagement est ce qui rend la
  macro payante, §2.2), **à cadrer avant d'écrire une ligne**.
  ⚠️ Deux fausses pistes déjà écartées, au code : **`ordre-dyn` ne le règle pas** (il rend le premier
  but non rempli *encore livrable*, or (14,9) reste livrable — il resterait actif ; cohérent avec le
  8/16 mesuré) ; **le masquage non plus tel quel** ((14,9) étant rang 0, il serait révélé en premier).
- [x] **Généraliser à un second niveau à peigne** — le **20** est le candidat désigné par
  l'utilisateur, et il est **Groupe A** (ne démarre pas, 1/18). Prédiction à falsifier avant de jouer,
  ci-dessous.
  ❌ **FAIT le 2026-08-01, et la prédiction est RÉFUTÉE** (session suivante) : le 20 est **gagné deux
  fois à la main** avec **0 à 1 % hors régime**. Le 89 % du 13 est une **singularité**, pas une
  famille — deux niveaux à peigne, deux verrous différents.
- [x] Le trafic statique comme **détecteur** : le passer sur les 18 non-résolus coûte quelques
  millisecondes et dirait combien partagent le défaut du 13.
  ✅ **FAIT le 2026-08-01.** Corrélation gêne statique ↔ rang calculé :

  | | valeurs |
  |---|---|
  | **14 résolus (témoins)** | médiane **+0,81**, min +0,03 (le 6) — **aucun négatif** |
  | **non-résolus NÉGATIFS** | **29 (−0,21)**, **13 (−0,14)** — les deux seuls sur 18 |
  | non-résolus faibles | 14 (+0,06), 20 (+0,18), 19 (+0,27) |

  **Le défaut du 13 est partagé par exactement un autre niveau, le 29** — désigné par une mesure
  statique et non à l'intuition, c'est le prochain candidat hybride. ⚠️ Deux réserves : le **6 est à
  +0,03 et il est résolu**, donc « faible » ne discrimine rien (seul « négatif » le fait) ; et ça ne
  repose que sur **2 cas**. ⚠️ Le **12 est à +0,61** : le trafic ne voit pas son défaut, ce qui est
  cohérent — le sien est un sens de parcours, pas une porte posée trop tôt (session ci-dessous).

#### 🎉 Session du 2026-08-01 (suite) — LE 20 GAGNÉ À LA MAIN : le 13 est une SINGULARITÉ, et le détour non-monotone est CONFIRMÉ

**Le 20 est un second niveau à peigne** (constat utilisateur) : colonne x=17 pleine (y=4→13), colonne
x=16 en alternance buts/murs (les dents), et la rangée du bas (14,13)…(17,13) pour entrée. 18 buts.
Quatre parties jouées, **deux GAGNÉES** (17/18 à l'affichage, victoire au dernier coup). Ce n'est pas
un score ([scores.md](scores.md) ne bouge pas — parties humaines).

**❌ PRÉDICTION FAITE PUIS RÉFUTÉE, des deux côtés.** Avant la partie, j'avais prédit : *si (14,13)
ou (15,13) sortent dans les premiers rangs, le 20 aura la signature du 13*. **Les deux conditions
tombent :**

| | 13 | **20** |
|---|---|---|
| entrée de salle dans l'ordre calculé | rang **0** | rangs **10, 11, 12** (« entrée tard » respecté) |
| **hors régime** (coups choisis que le solveur ne génère pas) | **89 %** | **0 à 1 %** sur les 4 parties |
| macros lancées / buts | 1 / 16 | **18 à 21** / 18 |
| verdict de l'utilisateur sur l'ordre calculé | perdant | *« finalement l'ordre est bon »* |

> **LE 89 % DU 13 EST UNE SINGULARITÉ, pas une famille.** Sur le 20 — même géométrie de peigne, même
> classe de difficulté, non résolu lui aussi — le solveur et le joueur sont d'accord à 99 % : les
> poussées jouées à la main sont bien celles qu'il générerait. **Deux niveaux à peigne, deux verrous
> différents.** Et c'est la prédiction ratée qui l'établit : sans elle on aurait généralisé.

**✅ LA QUESTION DE L'UTILISATEUR, TRANCHÉE — et c'est sa seconde hypothèse qui est la bonne.**
Constat posé après la campagne : *« le perso gêne systématiquement l'enclenchement des macros, ou
alors — et à mon sens c'est tout à fait plausible — une macro ne peut pas pousser une caisse 2 fois
sur la même case »*.

1. **La seconde est vraie, et STRUCTURELLEMENT.** `avanceVersBut` (game.cpp) n'accepte un pas que si
   `dpb[devant] == dCur - 1` : la descente est **strictement décroissante**, donc sans détour ni
   retour. (Rigoureusement, la distance est indexée par *(case, région joueur)* — une même case
   pourrait rouvrir avec une région différente et une distance plus basse ; très contraint en
   pratique.) Le §6.3 l'écrivait comme une limite du backtracking sur forks : « ne couvre pas un vrai
   détour non-monotone — **aucun cas confirmé de ce genre n'a encore été trouvé** ». **Il l'est.**

   ➡️ ⚠️ **CORRIGÉ LE SOIR MÊME — j'avais fusionné DEUX énoncés distincts**, et la parenthèse
   « très contraint en pratique » servait à couvrir l'écart. Constat utilisateur : *« les macros
   permettent de passer une caisse 2× par le même chemin »*. Vérifié sur les **566 macros détaillées**
   des journaux : **5 repassent par la même case dans une seule invocation.** Exemple, niv 26, macro
   `(5,6) → but (10,4)` :

   ```
   (5,6)→bas→(5,7) →gauche→ (4,7) → (3,7) → (2,7)     la caisse part à GAUCHE
   (2,7) →haut→ (2,6) →droite→ (3,6) →bas→ (3,7)      elle contourne par le haut
   (3,7) →droite→ (4,7) → (5,7) → (6,7) → …           et REVIENT vers la droite
   ```

   | énoncé | verdict |
   |---|---|
   | la macro ne fait pas de détour **NON-MONOTONE** | **VRAI** — la distance décroît toujours |
   | la macro ne repasse pas 2× par la même **CASE** | **FAUX** — 5 cas sur 566 |

   Le mécanisme est le **joueur-aware** : après le contournement, le joueur a changé de région, donc
   la même case a une distance PLUS BASSE. L'invariant tient, il n'interdit simplement pas de
   repasser. **La macro sait donc faire des manœuvres de retournement** — elle est plus puissante
   qu'écrit ici, et « elle ne sait faire que des trajets monotones » est à bannir du vocabulaire.
2. **La première est FAUSSE**, et c'est la mesure qui le dit.

**L'INSTRUMENT QUI A TRANCHÉ** (`Game::diagnosticPas0`, game.h/game.cpp + rapport du clic droit).
Le journal confondait sous un seul libellé « ECHEC AU PAS 0 » les deux causes — 29 des 55 clics de la
campagne étaient donc **illisibles**. La distinction s'obtient en **relâchant la seule contrainte de
zone** : on rappelle `avanceVersBut` avec une zone totale, et une direction qui passe alors mais pas
avec la zone réelle isole exactement le placement du joueur. **Aucune logique dupliquée** — c'est le
même exemplaire unique de la condition de descente, celui que `game.cpp` interdit explicitement de
réécrire. Canari revérifié après coup : 4/14/412/499/9 123/570/24 376/24 786 états,
4/97/131/134/143/110/90/213 poussées.

| cause, après désambiguïsation | occurrences |
|---|---|
| **DÉTOUR NON-MONOTONE REQUIS** (aucune direction ne baisse la distance, joueur placé où l'on veut) | **14** |
| **LE JOUEUR EST DU MAUVAIS CÔTÉ** (l'appui est hors de sa zone) | **0** |
| DESCENTE BLOQUÉE en cours de route | 6 |

- ⚠️ **NE PAS LIRE « 14 CAS » : c'est UN cas, observé 14 fois.** Les quatorze portent sur **la même
  caisse, en (5,5)**, testée contre **treize buts actifs différents** — l'utilisateur a cliqué à
  chaque changement de but pour voir si la macro finirait par la vouloir. Jamais. C'est un cas
  d'école propre (*une caisse que la macro ne peut bouger vers AUCUN but*), pas un échantillon.
- ⚠️ **Et il ne bloque pas le 20.** Cette caisse est dans le labyrinthe d'acheminement de gauche, loin
  de la salle ; la manœuvrer aux poussées simples est le régime normal, et le solveur les génère bien
  ici (0-1 % hors régime). Le détour non-monotone est une **limite confirmée**, pas le verrou du 20.

**CE QUE ÇA LAISSE POUR LE 20, et c'est inconfortable.** Ordre bon (dixit l'utilisateur, et il gagne
avec), macro qui s'engage 18 à 21 fois sur 18 buts, aucun coup humain hors de l'arbre, aucun coup
écarté par un élagage — **tout ce qu'on sait mesurer dit « le solveur peut jouer cette partie »**, et
il reste à 1/18 en 120 s (Groupe A, §6.6). Le verrou du 20 n'est ni l'ordre, ni la génération, ni
l'élagage : c'est le **démêlage** au sens du §4, c'est-à-dire trouver la séquence. Rien dans le plan
ne l'attaque, et le §6.1 a fermé les tie-breaks.

**Reste ouvert :**
- [ ] **Le secours de recherche borné pour les détours non-monotones** (§6.0, item 6 — laissé ouvert
  depuis le 2026-07-23 faute de cas confirmé) : **le cas existe maintenant**, mais **un seul**, et
  non bloquant. Ne pas ouvrir le chantier sur cette base — la barre du projet est un cas qui COÛTE
  quelque chose de mesurable. À rouvrir si la campagne en produit d'autres.
- [ ] **Relever d'autres `[manque]` avec le libellé désambiguïsé** : 14 occurrences d'un seul motif ne
  disent rien de la fréquence relative des deux causes. C'est bon marché — un clic droit.
- [ ] **Pourquoi le 20 reste à 1/18** alors que rien ne s'y oppose dans ce qu'on mesure. C'est la
  question ouverte la plus nette de la campagne hybride, et elle est **négative** : elle dit ce que
  le verrou n'est pas.

#### 🎉 Session du 2026-08-01 (soir) — SEPT NON-RÉSOLUS GAGNÉS À LA MAIN, et trois généralisations mortes

**La campagne hybride a produit, en une journée, sept parties gagnantes sur des niveaux que le
solveur ne finit pas** : 12, 13, 18, 19, 20, 22, 23. ⚠️ **Aucun n'est un score** —
[scores.md](scores.md) ne bouge pas, ce sont des parties humaines, pas des solves. Ce qu'elles
valent, c'est ce qu'elles apprennent sur le solveur.

| niv | buts | poussées | **macros** | **hors régime** | inversions | transits | états sans macro |
|---|---|---|---|---|---|---|---|
| 20 | 18 | 603 | 21/18\* | **0 %** | 0 | — | — |
| 12 | 15 | 234 | 10/15 | **1 %** | 16 | 37 | 21 % |
| 23 | 18 | 549 | **18/18** | **1 %** | **0** | 51 | **45 %** |
| **26** | 13 | 232 | **13/13** | **2 %** | 1 | 41 | **67 %** |
| 19 | 15 | 321 | **15/15** | **3 %** | **0** | 31 | 15 % |
| **25** | 19 | 436 | 20/19\* | **6 %** | **0** | 106 | **66 %** |
| **24** | 22 | 587 | **22/22** | **10 %** | **0** | 92 | 17 % |
| **22** | 27 | 502 | 19/27 | **67 %** | 148 | **119** | 32 % |
| **13** | 16 | 276 | **1/16** | **89 %** | 52 | 80 | — |
| **18** | 11 | 247 | 4/11 | **90 %** | 26 | 34 | 6 % |

\* le 20 et le 25 relancent des macros après des transits, d'où un total supérieur au nombre de buts.

➡️ ⚠️ **LA COLONNE « ÉTATS SANS MACRO » EST SURESTIMÉE — corrigée le 2026-08-02.** Elle comptait les
lignes `[hybride]` du journal, **y compris celles des branches ABANDONNÉES** : un coup annulé par
`[undo]` laissait sa ligne d'état derrière lui. Le corpus portait **2 877 `[undo]`**, et le journal du
18 était **à 48 % fait de branches mortes**. Les journaux ont été récrits (undo appliqués, séquences
de coups vérifiées identiques à l'octet contre le commit d'origine) ; valeurs recalculées :

➡️ 🔴 **ET CETTE CORRECTION EST ELLE-MÊME SUPERSÉDÉE, dans l'heure — les deux versions mesuraient
dans le MAUVAIS ESPACE.** Le compteur prend une ligne `[hybride]` par **COUP**, marche comprise. Or
le solveur ne voit jamais un état de marche : `pousse()` **téléporte** le joueur (§2.1), son espace
d'états est en **POUSSÉES**. On comptait donc des états qui n'existent pas pour lui — et le résultat
dépendait de combien le joueur avait tourné en rond, ce qu'un constat utilisateur sur le 18 a fait
voir (« beaucoup de trajets du perso pour revenir au départ »).

| niv | 23 | 25 | 26 | 20 | 22 | 12 | 24 | 19 | 18 |
|---|---|---|---|---|---|---|---|---|---|
| publié (par coup) | 45 % | 66 % | 67 % | — | 32 % | 21 % | 17 % | 15 % | 6 % |
| undo appliqués | 39,5 % | 64,1 % | 56,3 % | 50,4 % | 28,6 % | 18,4 % | 17 % | 13,8 % | 1,6 % |
| **PAR POUSSÉE — la bonne** | **25,1 %** | **52,2 %** | **50,7 %** | **46,0 %** | **19,5 %** | **7,8 %** | **9,7 %** | **11,0 %** | **6,8 %** |

Hors tableau : **11** 12,4 %, **13** 10,2 %, **4** 13,7 %, **7** 30,8 %, **10** 2,5 %, **2** 11,5 %,
**3** 5,1 %.

- ⚠️ **Sur le 18, les deux biais se compensaient presque** : 6 % publié, 6,8 % juste. Un chiffre exact
  par accident, obtenu en cumulant deux erreurs de sens opposé. **C'est le pire cas de tous** — rien
  ne l'aurait signalé.
- ⚠️ **Deux niveaux MONTENT** en passant par poussée (6 : 12,5 → 17,2 % ; 18 : 1,6 → 6,8 %) : leur
  joueur marche beaucoup dans des états qui, eux, OFFRENT une macro. Le biais n'a donc pas de signe
  fixe, on ne pouvait pas le corriger « à la louche ».
- **Ce qui ne bouge toujours pas** : le classement (25 et 26 en tête, puis 20), et la colonne
  « hors régime », qui compte des poussées jouées.

- ⚠️ **Le 12 ne se recoupe pas** : 21 % publié, 24,3 % mesuré avant correction, **18,4 %** après. La
  valeur d'origine ne vient donc pas de la même partie ni du même dénominateur — non élucidé, à ne
  pas réconcilier de force.
- ⚠️ **La phrase « le 23 est le cas extrême, 45 %, le record du corpus » était fausse AVANT cette
  correction** : le tableau ci-dessus donne déjà 66-67 % au 25 et au 26. Erreur de lecture
  préexistante, indépendante du biais des undo.
- **Ce que ça ne change pas** : la colonne « hors régime », qui compte des poussées effectivement
  jouées, pas des états traversés. Le 89 % du 13 et le 90 % du 18 tiennent.
- 🔴 **La leçon, et elle vaut au-delà de ce tableau** : **un journal en AJOUT n'est pas une trace, c'est
  un brouillon.** Tout compteur qui balaie ses lignes compte aussi ce que le joueur a défait. Le
  parseur de rejeu, lui, appliquait les `[undo]` depuis toujours — donc les poussées étaient justes et
  les états faux, dans le même fichier, sans que rien ne le signale.

**DIX non-résolus gagnés à la main dans la journée** (12, 13, 18, 19, 20, 22, 23, 24, 25, 26). La
série hors régime devient **0, 1, 1, 2, 3, 6, 10, 67, 89, 90** — le continuum se confirme, avec un
trou entre 10 % et 67 % qui ne repose que sur l'absence de mesure intermédiaire, pas sur une
structure. **Ne pas y voir deux groupes.**

**LE CORRECTIF MULTI-SALLES VALIDÉ CÔTÉ HUMAIN.** Le **24** (salles 20+2) et le **25** (17+2) sont
joués **sans une inversion**, l'ordre groupé suivi tel quel, avec la macro qui pose toutes les
caisses (22/22, 20/19). Le **18** (7+2+2), lui, est joué dans l'ordre inter-salles **INVERSE** du
correctif (petite salle en premier) — et il gagne aussi. **Ça conforte le « ne rien graver » du
§6.2** : ce qui compte est de ne pas entrelacer, pas l'ordre entre salles. Deux points de plus
qu'hier, où l'affirmation ne reposait que sur le 10.

⚠️ **Le 25 et le 26 plafonnent à 66-67 % d'états sans macro** — le double du 22 (32 %) et le
quadruple du 24 (17 %). Sur le 25, **243 poussées choisies contre 193 par macro** : plus de la moitié
du travail est du démêlage pur, et le solveur générerait 94 % de ces coups. Ce sont les meilleurs
candidats pour le corpus d'intentions.
➡️ Chiffres corrigés le 2026-08-02 (biais des `[undo]`, cf. sous le tableau ci-dessus) : **25 à
64,1 %, 26 à 56,3 %, 22 à 28,6 %**, le 24 inchangé à 17 %. L'écart 25/26 contre 22 se réduit de
« le double » à ×2,2 et ×2,0 — **le classement ne bouge pas, les rapports si.**

**⏸️ LE 18 AU SOLVEUR — AUCUN VERDICT, et le MUR MÉMOIRE est de retour.** L'ordre humain du 18
(petite salle en premier) injecté, `coupl-plongeon`, laissé sans budget :

| | |
|---|---|
| dépilements | **104 768 000** |
| **états vus** | **217 013 303** |
| file à l'arrêt | 139 912 264 (+1021, en hausse) |
| max | 10/11 |
| fin | **tué par la pression mémoire** à 11 Go de footprint, swap saturé (2 Go / 3) |

- ⚠️ **Aucun verdict, dans aucun sens** — quatrième application de la règle (31, 13/r07, 11 de
  juillet, et maintenant le 18). Ce qu'on peut écrire : *le 18 ne se résout ni avec l'ordre calculé
  en 40,6 M dépilements, ni avec l'ordre humain en 104,8 M*. Rien de plus.
- **217 M états vus est le RECORD du projet**, devant les 188 M du 31 et loin devant les 87 M qui ont
  suffi au 11. Le 18 n'a pas été « essayé sérieusement », il a été entamé.
- 🔴 **Le §6.5 se re-confirme, plus fort** : il rouvrait le mur mémoire sur les 5 Go du 11 ; ici on
  atteint **11 Go** et c'est le système qui tranche, pas le temps. Les chantiers classés « sans
  objet » (arène — poste dominant —, hachage 128 bits, blocs pour `noeuds`/file) redeviennent
  d'actualité dès qu'un run va au bout de ses forces.
- ⚠️ **`ps rss` a menti dans le mauvais sens, en direct** : la RSS s'est **effondrée de 8,7 à 2,6 Go**
  au moment précis où `phys_footprint` atteignait 11 Go — le compresseur macOS sortait les pages de
  la RSS. S'y fier aurait fait conclure que tout allait mieux. Le §1 l'avertissait ; c'est la
  première fois qu'on le voit se produire pendant un run.

**⚠️ TROIS GÉNÉRALISATIONS PROPOSÉES ET TOMBÉES, LE MÊME JOUR, SUR LE MÊME SUJET.** Elles sont
consignées parce que la mécanique de l'erreur est plus instructive que le résultat :

| affirmation | tombée sur | après combien de points |
|---|---|---|
| « le 89 % du 13 est une **famille** » | le 20 (1 %) | 1 |
| « le 89 % du 13 est une **singularité** » | le 18 (90 %) | 3 |
| « c'est une **partition** sans valeur intermédiaire » | le 22 (**67 %**) | 5 |

La série complète est **0, 1, 1, 3, 29, 30, 35, 67, 89, 90** (les trois valeurs médianes viennent des
variantes du 10) : **c'est un CONTINUUM, pas une structure.** C'est le §11.4 exécuté trois fois en une
journée, à chaque fois avec deux à cinq points. **Ne plus annoncer de structure sur ce compteur.**

**Ce qui est mesuré, et rien de plus :**
- La part des coups humains que le solveur **ne génère pas** varie continûment de 0 à 90 %.
- Elle dépend d'**au moins deux facteurs qui se cumulent** : l'écart à `ordreButs`, et le fait qu'une
  macro **reste engageable malgré cet écart**. On ne sait pas les séparer.
- ⚠️ **Le nombre d'inversions ne la prédit pas**, et c'est ce qui empêche de la réduire à un artefact
  d'observation (le FAIT 6) : le 18 a **26 inversions et 90 %**, le 10 en a **191 et 30 %**, le 12 en
  a 16 et 1 %. Le témoin décisif est la 3ᵉ partie du 10 — l'écart maximal du corpus, et le compteur
  reste bas parce que s'écarter y **coupe** la macro (`macrosOk == 0`), ce qui rend la main au régime
  des poussées simples.

**LE GROUPE QUI INTERROGE LE PLUS : 19, 20, 23.** Ordre calculé suivi **sans une inversion**, macro
qui pose **toutes** les caisses (15/15, 18/18), 0-3 % hors régime — **tout ce qu'on sait mesurer dit
que le solveur peut jouer ces parties**, et il ne les trouve pas (10/15 et 0/18 au profilage à 120 s).
Le 23 est le cas extrême : ~~**45 % de ses états n'offrent aucune macro**, le record du corpus~~
(⚠️ **39,5 % après correction du biais des `[undo]`, et ce n'est PAS le record** — le 25 et le 26
sont au-dessus, dans le tableau de cette même session ; double erreur relevée le 2026-08-02), et ses
162 poussées choisies sont générables à 99 %. **Leur verrou n'est ni l'ordre, ni la génération, ni
l'élagage : c'est le démêlage du §4.** Rien dans le plan ne l'attaque, et le §6.1 a fermé les
tie-breaks.

**Deux acquis ponctuels :**
- **La cause « LE JOUEUR EST DU MAUVAIS CÔTÉ » existe** — 0 occurrence sur les 14 du niveau 20, mais
  une sur le 12 (au départ, caisse (6,4)) et une sur le 22 (appui (16,11) hors zone). L'hypothèse
  utilisateur du 2026-08-01 est donc **vraie mais rare**, là où le détour non-monotone domine.
- **Les transits explosent avec la taille** : 119 sur le 22 (27 buts) pour 146 arrivées et 27 poses.
  La variante sèche de R1 est re-tuée une huitième fois.

#### ✅ Session du 2026-08-01 (suite 3) — LE CORRECTIF MULTI-SALLES CODÉ ET PROMU : ×7,5 sur le 10

**Ouvert depuis le 2026-07-17, déclassé le 2026-07-29, re-priorisé le 2026-08-01, fait.**

**Le code, deux pièces, aucune variable d'environnement (§7) :**
1. **`Game::sallesDeButs()`** (game.cpp) — composantes connexes des cases-buts en **4-connexité**.
   ⚠️ Ce n'est pas « les pièces du plateau » (le plateau est connexe pour le joueur), c'est
   l'adjacence des BUTS. Statique, O(nbButs²) avec nbButs ≤ 32, calculé au chargement.
2. **Une préférence de salle DANS le tri topologique stable**, pas en post-passe. ⚠️ Ce choix est une
   question de correction, pas de style : remonter les buts d'une salle en bloc **après** le tri
   casserait les arêtes de précédence que ce tri vient d'établir. En préférant, parmi les buts
   **PRÊTS**, celui de la salle en cours, on obtient le groupement maximal **compatible** avec les
   précédences — jamais au prix d'une violation. Sur un niveau à salle unique la préférence ne
   discrimine rien : l'ordre ressort **inchangé, par construction**.

**⚠️ UNE AFFIRMATION DU PLAN CORRIGÉE AVANT DE CODER.** Le §6.2 promettait « les niveaux à une seule
salle sortent inchangés à l'octet — donc **zéro risque sur les 15 résolus** ». C'est faux, et la
vérification préalable l'a montré :

| | niveaux |
|---|---|
| **une seule salle** | **30 sur 35** |
| **multi-salles** | **0** (trois salles d'UN but !), **10** (28+4), 18 (7+2+2), 24 (20+2), 25 (17+2), 26 (12+1) |

**Le 0 et le 10 sont multi-salles ET résolus** — et le 0 est le canari le plus simple du projet. Le
risque n'était pas nul, il était circonscrit à deux niveaux. (Le 0 ressort finalement identique :
trois salles d'un but sont déjà « groupées » trivialement.)

**LES TROIS JUGES, dans l'ordre annoncé :**

| juge | résultat |
|---|---|
| `ordre` sur les 35 | **0 arête violée**, et exactement les 3 murages locaux préexistants (13, 18, 22) |
| cartes de rangs, avant/après | **identiques partout sauf 10 et 18** — vérifié contre les ordres enregistrés dans les journaux hybrides, qui sont des **traces du binaire d'avant** |
| canari solveur (12 niveaux) | **poussées identiques** : 4/97/131/134/355/143/110/90/237/213/220/250 |

**LE GAIN, binaire contre binaire — même binaire, seul l'ordre change** (l'ancien réinjecté par
`ORDRE_HUMAIN`, `coupl-plongeon`) :

| ordre du 10 | états | poussées |
|---|---|---|
| ancien (satellite éclatée aux rangs 0/14/29/31) | **2 160 492** | 544 |
| **nouveau (satellite groupée 0-1-2-3)** | **286 428** | 544 |
| | **×7,54** | **identiques** |

- **L'ancien ordre réinjecté reproduit exactement le 2 160 492 macOS** du 2026-07-31 : l'injection
  rejoue fidèlement le comportement d'avant, le ×7,54 n'est donc imputable qu'au groupement.
- **544 poussées des deux côtés**, alors qu'on est en régime plongeon où les poussées ne sont pas un
  canari (§6.3) — la solution est la même, elle est juste trouvée sept fois plus vite.
- **Vérifié aussi à la main** : en mode hybride, le 10 rejoué avec le nouvel ordre passe à **32 macros
  lancées sur 32 buts** (contre 30 et 29) et **0 coup hors régime** (contre 9 et 15). La macro pose
  désormais toutes les caisses, et le but actif reste dans la salle qu'on est en train de faire.

⚠️ **LA PRÉDICTION DU PLAN N'EST TENUE QU'À MOITIÉ, et l'autre moitié est réfutée.** Elle annonçait
« ~32/32 macros **et pas de trou** » (le trou de 113 états sans macro). Les macros y sont ; le trou
**GRANDIT** — 132 → **187**. Et le compteur n'est pas contaminé cette fois (0 inversion, 0 hors
régime), donc c'est une vraie indisponibilité : faire la satellite d'un bloc oblige à y amener quatre
caisses de loin, sans macro pendant tout le trajet. C'est cohérent avec le **FAIT 4** (« le trou de
démêlage est un trou de GUIDAGE ») : un correctif d'ordre ne pouvait structurellement pas le combler.
**Deux choses distinctes avaient été mises dans la même prédiction.**

- [ ] **18, 24, 25, 26 non mesurés** — les quatre autres multi-salles. Le 18 voit son ordre changer
  (regroupé en 7+2+2) ; les trois autres restent à vérifier. C'est là que le correctif peut encore
  rapporter, ou ne rien donner.

#### ❌ Session du 2026-08-01 (suite 2) — LE 12 : c'est le SENS de parcours qui décide, pas le groupement

**Le 12 gagné à la main en hybride** — 234 poussées, 15/15, **10 macros** lancées, **1 % hors
régime**, 37 transits. Troisième non-résolu joué, troisième fois que le solveur générerait les coups
joués : avec le 20 (0-1 %) et le 12 (1 %), ~~**le 89 % du 13 est définitivement une singularité**~~.
➡️ ❌ **RÉFUTÉ LE JOUR MÊME par le 18** (gagné à la main quelques heures plus tard, **90 % hors
régime**). Ce n'est pas une singularité, c'est une **PARTITION** — et sur quatre non-résolus joués
elle est nette, sans valeur intermédiaire :

| niveau | hors régime | macros lancées |
|---|---|---|
| **13** | **89 %** | 1 / 16 |
| **18** | **90 %** | 4 / 11 |
| 12 | 1 % | 10 / 15 |
| 20 | 0-1 % | 12 à 21 / 18 |

⚠️ **Deux fois de suite j'ai généralisé sur trop peu de points** — d'abord « le 13 est une famille »
(réfuté par le 20), puis « le 13 est une singularité » (réfuté par le 18). Le §11.4 en une journée,
dans les deux sens. **Le bon énoncé est descriptif** : deux niveaux sur quatre ont l'essentiel de
leur solution hors de l'arbre du solveur, deux ne l'ont pas ; on ne sait pas encore ce qui les sépare.

**L'ordre de pose relevé, comparé à celui de juillet :**

```
aujourd'hui : (15,5)(15,6)(15,7)(15,8)(15,9) | (13,5)…(13,9) | (14,9)(14,8)(14,7)(14,6)(14,5)
juillet     : (15,9)(15,8)(15,7)(15,6)(15,5) | (13,5)…(13,9) | (14,9)(14,8)(14,7)(14,6)(14,5)
```

**Une seule variable diffère : le SENS de la colonne 15.** Même joueur, même niveau, deux parties
gagnées, colonnes 13 et 14 identiques, aucun entrelacement des deux côtés. C'est l'expérience la plus
propre du projet sur l'ordre de remplissage — et elle a été obtenue sans rien coder, en relevant deux
parties.

**Mesuré (même binaire, `coupl-plongeon`, `ORDRE_HUMAIN`, budget 900 s) :**

| ordre | groupé ? | dépilés | résultat |
|---|---|---|---|
| **juillet — colonne 15 BAS→HAUT** | oui | **2 097 527** | ✅ **RÉSOLU, 212 poussées** |
| calculé (témoin) | **non**, entrelacé | 10 183 000 | tué au budget, `max 10/15` |
| **aujourd'hui — colonne 15 HAUT→BAS** | oui | 10 587 000 | tué au budget, `max 9/15` |

> **CE QUI EST ÉTABLI, ET RIEN DE PLUS** : l'ordre de juillet résout en 2,1 M états ; celui
> d'aujourd'hui **n'a pas résolu en 10,6 M**, soit au moins **×5 plus cher — si tant est qu'il
> résolve**. Le groupement ne suffit donc pas à lui seul : deux ordres également groupés, ne différant
> que par le sens d'une colonne, se comportent très différemment face à la macro.

⚠️ **CE QUI N'EST PAS ÉTABLI, et que ce paragraphe affirmait à tort dans sa première rédaction :**
1. **Que l'ordre d'aujourd'hui ne résout PAS.** Il a été **tué à 900 s**, et le §6.3 le répète —
   « ne termine pas dans le budget » veut dire **lent**, pas mort (le 4, le 9 et le 8 sont tous
   tombés laissés sans budget). Seul un run mené au bout trancherait.
2. **Qu'il est « pire que l'ordre entrelacé »**, sur la foi de `max 9/15` contre `max 10/15`. Le §6.6
   a **réfuté la progression à budget borné comme prédicteur** — le 10 y affiche 12 % et il tombe.
   Comparer deux runs tués par leur remplissage est exactement l'erreur que ce document interdit.

**Corrigé après remarque de l'utilisateur** (« je ne serais pas aussi catégorique, mon run manuel
passe sans souci ») : sa partie GAGNE, et rien ici ne dit le contraire. La phrase du §6.2
(2026-07-31) sur les colonnes en sens opposés reste **plausible et non contredite**, pas « confirmée
et chiffrée ».

- ⚠️ **« Humain et gagnant » ≠ « bon pour la macro », TROISIÈME fois** — et pour la première fois sans
  échappatoire : ni joueur différent, ni niveau différent, ni ordre approximatif. **Deux parties
  gagnantes du même joueur sur le même niveau, dont une seule est jouable par le solveur** *dans le
  budget mesuré*. Le §6.2 énonçait la règle depuis le 2026-07-20 ; ici elle est isolée à une variable.
  ⚠️ Ce que ça ne dit PAS : que l'ordre d'aujourd'hui soit mauvais. Une partie humaine gagnante reste
  une partie gagnante — l'écart mesuré porte sur le **coût pour la macro**, pas sur la validité de
  l'ordre.
- **B reproduit 2 097 527 / 212 à l'unité** (chiffre du 2026-07-31, autre binaire) : canari gratuit
  qui confirme au SOLVEUR que le correctif multi-salles du jour est bien inerte sur le 12 (mono-salle),
  et pas seulement d'après la carte des rangs.
- ⚠️ **PIÈGE DE LECTURE, tombé dedans en direct** : j'ai d'abord écrit « A n'est pas nul pour autant,
  `max 9/15` contre `max 6/15` pour le témoin » — en comparant un run **fini** à un run **encore en
  cours**. Le témoin a continué jusqu'à `max 10/15`, et l'ordre A est en réalité le pire. **Un run
  borné ne se lit qu'à budget égal**, y compris quand les deux tournent sous ses yeux.

**➡️ SUITE DU MÊME JOUR — L'ORDRE INJECTÉ DANS L'APP, ET L'ÉCART LOCALISÉ.**

L'utilisateur a rejoué le 12 en hybride **avec son ordre injecté** (mécanisme neuf, ci-dessous) et l'a
**gagné**. Ce que ça apprend, et qui n'était pas prévisible :

| partie du 12 | macros lancées | avant la 1ʳᵉ pose | après |
|---|---|---|---|
| ordre calculé | 10/15 | 34 états, **100 % sans macro** | 27 % sans macro |
| **son ordre (injecté)** | **15/15** | 33 états, **100 % sans macro** | 21 % sans macro |

- **Son ordre fait poser les 15 caisses par la macro** (contre 10/15). Une fois le démarrage passé, il
  est **meilleur** que l'ordre calculé — alors que c'est lui qui coûte ≥×5 au solveur. Les deux faits
  cohabitent, et c'est ce qui rendait le diagnostic difficile.
- **Constat utilisateur : « le seul problème vient du fait que la macro ne se déclenche pas pour la
  première caisse ».** Vérifié, et il est **plus large que ça** — outil `pas0` (§1) sur le plateau de
  départ : **AUCUN des 15 buts n'est atteignable par macro**, pas seulement le premier. Cause
  identique partout : la caisse (8,4) s'amorce puis **bloque en (9,4)** (reste 11), et la caisse (6,4)
  est bloquée **au pas 0 par la position du joueur** (`Haut baisserait la distance, appui (6,5) hors
  zone`) — **première occurrence de cette cause dans tout le corpus**, 0 sur les 14 du niveau 20.
- **Conséquence : le choix du premier but ne change RIEN au démarrage**, il est identique pour les
  trois ordres. Le trou de 33 états est le **FAIT 4** (trou de GUIDAGE : les poussées simples ne lisent
  jamais `ordreButs`), le même que les 142/153 du niveau 4. **Il n'explique donc pas l'écart A/B.**

**OÙ NAÎT L'ÉCART, alors — lu dans les jauges des deux runs, sans rien relancer :**

| record | **B — juillet (résout)** | **A — aujourd'hui** |
|---|---|---|
| 1/15 | 6 208 dépilés | 6 109 dépilés |
| **2/15** | **7 888** | **2 345 974** — **×297** |
| 3, 4, 5 | 8 046 → 8 238 | ~2 350 000 |
| 6/15 | 2 097 527 → **plongeon RÉUSSI en 15 états** | 5 150 555 → échec au budget plein (100 991) |

> **Ce n'est pas la première caisse, c'est la DEUXIÈME.** Les deux ordres posent la première au même
> prix (~6 100 états, à 1,6 % près). Ensuite B enchaîne les quatre suivantes en 2 000 états ; A met
> **2,34 millions** pour la seule seconde. Tout l'écart est là, et le reste en découle.

**Hypothèse, NON vérifiée** : c'est la contiguïté de run du §6.2 (2026-07-20) — « une caisse poussée
d'un trait s'arrête au mur ou à la caisse déjà posée », la colonne se remplit fond→entrée **par la
physique du jeu**. Remplir depuis l'entrée obligerait à approcher chaque but par le côté déjà occupé.
⚠️ **Pas confirmé sur la géométrie du 12** : l'accès à la salle se fait par le bas en (13,10) et les
murs en x=12 interdisent les appuis à gauche — la vérifier demanderait `pas0` sur l'état d'APRÈS la
première pose, qu'on n'a pas exporté. À faire avant d'en tirer quoi que ce soit.

**Conséquence pour la cible « corriger la contiguïté de run ».** Elle se complique : il ne suffit pas
de ne pas entrelacer, il faut le **bon sens de parcours**, et **aucune règle du projet ne le donne**.
C'est exactement là que le §6.2 a échoué en juillet — le wall-ext (« partir d'un cul-de-sac ») est
indispensable sur 2/3 et toxique sur les blocs pleins, tension jugée alors « irréductible en un seul
scalaire local ». Le 12 fournit désormais une **cible mesurable** pour la trancher (son ordre juillet
résout, son ordre d'aujourd'hui non), ce qui manquait.

**🎯 LE CORPUS D'INTENTIONS — amorcé le soir même, et il tient.** La piste « capturer l'INTENTION en
vocabulaire fermé » était *discutée, non codée* depuis le matin (fin de la session « rang du coup
humain »). Elle est codée, et les quatre premiers niveaux annotés donnent :

| niv | étiquettes | répartition |
|---|---|---|
| 1 | 3 | OUVRIR 3 |
| 2 | 5 | OUVRIR 5 |
| 3 | 3 | OUVRIR 1, GARER 2 |
| **4** | **11** | **OUVRIR 29 · ECARTER 19 · GARER 12** (poussées couvertes) |

**Le 4 est le premier corpus exploitable, et il recoupe DEUX résultats du plan par une voie neuve :**
- **60 poussées choisies, toutes couvertes**, 0 avant la première étiquette. Le format tient.
  ➡️ ⚠️ **57, pas 60** (recompté le 2026-08-02, session ci-dessous) : le journal du 4 porte 418
  lignes `POUSSE caisse` mais **5 `[undo]`**, et le compte annoncé ici était le compte BRUT. Le
  chiffre undo-aware est **415 poussées / 57 choisies**. Sans effet sur les conclusions.
- **`OUVRIR` et `ECARTER` sont EXCLUSIVEMENT dans la phase d'entrée** (coups 9 à 131, tous à
  `posees 0/20`), `GARER` exclusivement après (coups 488, 1040, 1103, à 10/18/19 caisses posées).
  Deux vocabulaires pour deux phases, sans que ce soit décidé d'avance.
  ➡️ ❌ **« `GARER` exclusivement après » est RÉFUTÉ au-delà du 4** (2026-08-02, session ci-dessous) :
  sur les huit niveaux annotés, **5 des 16 `GARER` sont à `posees 0`** (6, 7 et 8). `GARER` est la
  seule catégorie SANS phase — et celle qui couvre le plus de poussées. Le reste du découpage tient
  et se renforce.
- **47 des 60 poussées choisies sont dans le trou de démêlage d'entrée** — le **FAIT 4** mesurait
  « 142 des 153 états sans macro » sur ce même niveau et le qualifiait de *trou de guidage* ; on sait
  maintenant **ce qu'il y a dedans**.
- ⚠️ **Et ce sont les DEUX CAUSES DU MOU du §3** — « écarter une caisse assise sur le trajet d'une
  autre **ou** qui bloque le joueur » — que `moureel` avait séparées par niveau (1 et 2 : joueur ;
  3 et 17 : laisser passer). Retrouvées à la main, sur un cinquième niveau, par une voie
  indépendante. C'est la première corroboration croisée de ce découpage.
- **La correction fonctionne** : le coup 33 porte `OUVRIR` puis `ECARTER` ; le dépouillement garde la
  seconde. Deux annotations au même numéro = une correction.
  ➡️ ❌ **SÉMANTIQUE CHANGÉE le 2026-08-02** (session ci-dessous) : la même gestuelle sert aussi à
  énoncer **deux** intentions, et « la dernière gagne » perdait la première en silence. Le coup 33 se
  lit désormais `OUVRIR` **ET** `ECARTER`. Règle complète et les 4 cas du corpus : session du
  2026-08-02.

⚠️ **`T` (RETOURNER) ajouté après coup**, sur constat utilisateur : ses trois `GARER` du 4 n'en sont
pas, ce sont des *« je sors la caisse de la zone pour la reprendre dans le bon sens »*. C'est le
**RECUL du §3**, la seule catégorie du vocabulaire qui corresponde à une grandeur déjà chiffrée
ailleurs (`mou = 2 × reculs`, la caisse revenant sur la case libérée 9 fois sur 10). **Un vocabulaire
fermé doit être ajusté par celui qui joue, pas deviné** — les trois `GARER` du 4 sont à relire comme
des `T`.

**État du code de la journée du 2026-08-01** (non commité, branche `ordre-dynamique`) :
- `game.cpp`/`game.h` — **`sallesDeButs()`** + préférence de salle dans le tri topologique (le
  correctif multi-salles, **PROMU**, aucun interrupteur) ; **`diagnosticPas0()`** (outil de
  diagnostic UI) ; **`cheminOrdreInjecte()`** + lecture du fichier d'ordre (chantier).
- `mainwindow.cpp`/`.h` — rapport du clic droit éclaté en causes distinctes ; le journal hybride
  annonce la SOURCE de l'ordre (`calcule` / `⚠ INJECTE depuis …`) ; **`rejoueJournal()`,
  `prochainCoupChoisi()`, `noteIntention()`** + les touches `L`/`N`/`E`/`O`/`G`/`A`/`T`/`R`/`?` ;
  **légende des touches** dans la barre d'état ; flèches et Retour arrière neutralisés en annotation.
- `mesures/pas0.cpp` + `pas0.pro` — **neuf**, chantier.
- ⚠️ **À RETIRER avec la campagne** : l'injection par fichier, `pas0`, le rejeu de journal + les
  intentions + la légende, et `diagnosticPas0` si le clic droit ne sert plus. **À GARDER** :
  `sallesDeButs()` et le groupement, qui sont de la production.
- **Validation du parseur de journal, à refaire si on y touche** : les chemins extraits des 14
  journaux ont été **rejoués sur un plateau** — tous légaux, tous gagnants, 0 coup illégal, malgré
  2 602 `[undo]` dans le corpus (dont 608 sur le seul 18). C'est le seul test qui prouve que la
  gestion des undo est correcte ; un parseur faux dérape dès le premier et n'arrive jamais au bout.
- ⚠️ **`mesures/bench` (binaire versionné) a été écrasé** par un rebuild — comportement identique
  (canari vérifié), mais c'est un fichier suivi par git qui n'avait pas à changer.

#### ✅ Session du 2026-08-02 — LE CORPUS D'INTENTIONS SUR 8 NIVEAUX : le vocabulaire fermé TIENT

**Ce qui a été fait** : l'utilisateur a rejoué et annoté les **huit premiers niveaux** (1 à 8), là où
le corpus de la veille n'en portait que quatre. Le 8 est en plus une **partie NEUVE** (2026-08-02) —
**244 poussées** contre les 373 consignées au tableau du §6.2, pour 24 poussées choisies contre 114 ;
elle passe tout près des 238 du solveur. Les sept autres annotent les parties de la veille.
⚠️ **Ces 244 ne sont PAS un score** : c'est une partie humaine, [scores.md](scores.md) ne bouge pas —
la ligne du 8 y reste celle du solve (238 poussées).

⚠️ **Dépouillement fait par un miroir Python jetable du parseur `rejoueJournal()`** (dernière partie
gagnée, `[undo]` qui dépile, `[macro] LANCEE`/`TERMINEE` qui marque l'appartenance). **Validé avant
toute lecture** : il reproduit **à l'unité** les `N coups (M par macro)` que l'app écrit elle-même en
tête de chaque session d'annotation, sur les huit. Un miroir non validé n'aurait rien valu (§7,
même exigence que le miroir du BFS de tirage).

**LE RÉSULTAT — le vocabulaire fermé à 7 entrées n'a jamais manqué :**

| | étiquettes | poussées couvertes |
|---|---|---|
| OUVRIR | 19 | 64 |
| GARER | 18 | 117 |
| ECARTER | 5 | 21 |
| RETOURNER | 4 | 17 |
| **`?` (INCONNU)** | **0** | — |
| **total** | **46** *(sur 43 coups annotés)* | 206 poussées choisies |

⚠️ **Les « poussées couvertes » se CHEVAUCHENT** depuis la règle de conjonction ci-dessous : un coup
à deux étiquettes compte ses poussées dans les deux lignes, la colonne ne se somme donc pas. Le
corpus fait **206 poussées choisies**, pas 219.

- **Un seul `?` dans tout le corpus** (niveau 7, coup 148, sur 50 frappes en comptant les sessions
  écrasées) — et il est **corrigé deux fois derrière** (`GARER` puis `RETOURNER`). Il n'en survit
  **aucun** au dépouillement.
- **Couverture totale : 0 poussée choisie avant la première étiquette, sur les huit niveaux.**
- ⚠️ **Ce qui compte n'est pas le score, c'est que l'échappatoire ait été MISE À L'ÉPREUVE et
  abandonnée.** Un vocabulaire fermé dont la sortie de secours n'est jamais prise ne prouve rien (on
  ne saurait pas si elle est inutile ou si l'annotateur s'en interdit l'usage) ; ici elle est prise
  une fois, puis remplacée par une catégorie existante. **Aucune catégorie ne manque.**
- ⚠️ **LA MAILLE, et elle borne tout ce qui précède** : 43 étiquettes pour 206 poussées choisies.
  Une seule frappe `GARER` du niveau 6 en couvre **44** à elle seule. « Tous les coups sont utiles »
  est établi à la maille du **PLAN**, jamais du coup — c'est précisément l'objet de l'outil (le rang
  d'un coup isolé ne peut pas voir un plan sur plusieurs coups), mais ça ne se lit pas comme une
  couverture coup par coup.

**LES CATÉGORIES SE RÉPARTISSENT PAR PHASE, et c'est net sur huit niveaux là où le 4 seul le
suggérait :**

| catégorie | à `posees 0` | remplissage aux autres étiquettes |
|---|---|---|
| **ECARTER** | **5/5** | — |
| **OUVRIR** | **15/18** | 33 %, 36 %, 83 % |
| **GARER** | 5/16 | 11 à 95 %, sans regroupement |
| **RETOURNER** | **0/4** | **36 %, 45 %, 54 %, 66 %** |

- **`ECARTER` et `OUVRIR` sont le vocabulaire de l'ENTRÉE** (20 étiquettes sur 23 à `posees 0`) —
  c'est le **FAIT 4** vu de l'intérieur : le trou de démêlage d'entrée est fait de ces deux gestes.
- **`RETOURNER` est le vocabulaire du MILIEU** : jamais à l'entrée, toujours entre 36 % et 66 %.
- **`GARER` n'a pas de phase** — et c'est ce qui réfute la lecture du 4 (corrigée ci-dessus). C'est
  la fourre-tout du vocabulaire : plus d'étiquettes qu'`ECARTER` et `RETOURNER` réunis, et **plus de
  la moitié des poussées couvertes du corpus**. Si une catégorie doit être scindée un jour, c'est
  celle-là.

⚠️ **UNE TENSION AVEC LE §3, à ne PAS trancher sur quatre points.** `RETOURNER` est le **RECUL** de
`moureel` (`mou = 2 × reculs`). Or `moureel` mesure les reculs comme **concentrés au tout début** —
5 des 6 du niveau 17 dans les **neuf premières poussées**. Ici aucun `RETOURNER` n'est à `posees 0`.
Les deux mesures ne portent ni sur les mêmes niveaux (1/2/3/17 contre 5/7) ni sur la même abscisse
(`posees` n'est pas le numéro de poussée : les neuf premières poussées du 17 sont toutes à
`posees 0`). **C'est une question ouverte, pas un résultat** — 4 étiquettes sur 2 niveaux, soit
exactement le format d'erreur du §11.4 commis trois fois le 2026-08-01.

**⚠️ RÈGLE DE CONJONCTION — adoptée en cours de campagne, sur constat utilisateur** (*« il y a des
fois où je voudrais répondre O et G »*). Le plan posait « deux annotations au même numéro = une
correction, la dernière gagne » ; **cette sémantique perd de l'information en silence**, puisque la
même gestuelle sert à se raturer et à énoncer deux intentions.

- **L'UI n'est PAS en cause et ne bouge pas** : `noteIntention()` écrit une ligne par frappe, le
  journal est fidèle. Tout se joue au **dépouillement**.
- **L'asymétrie décide** : lire une conjonction comme une correction **perd** une étiquette sans
  signal ; lire une correction comme une conjonction en **ajoute** une, et une étiquette de trop se
  repère. Donc **on ACCUMULE**, avec deux exceptions mécaniques : **`?` suivi de quoi que ce soit est
  superseded** (c'est la définition de la touche — « je ne sais pas *encore* »), et **la même
  étiquette deux fois n'en fait qu'une**.
- **Quatre coups concernés sur 43** : 4/33 `OUVRIR+ECARTER`, 8/220 `GARER+OUVRIR`, 1/8
  `OUVRIR+OUVRIR` (une seule), 7/148 `? → GARER+RETOURNER`. ⚠️ **Le 7/148 est le seul où la règle
  peut se tromper** — `GARER` puis `RETOURNER` peut être une rature. Non tranché, à confirmer par
  celui qui a tapé.
- **Aucune touche d'effacement ajoutée**, délibérément : changer la sémantique de l'outil au milieu
  d'un corpus rendrait le 25 incomparable aux huit premiers. À faire seulement si le besoin de
  rature explicite se manifeste vraiment (~5 lignes, plus une ligne `EFFACE` au journal).

**Reste ouvert :**
- [ ] **Le corpus ne porte que des niveaux RÉSOLUS** — c'est la réserve n° 1 posée le 2026-08-01 et
  elle n'est pas levée : 206 poussées choisies sur 8 niveaux qu'on sait finir. Les candidats sont
  déjà désignés par la campagne : le **25** a à lui seul **243 poussées choisies** (66 % d'états sans
  macro) et le **26** 67 %. **Un seul des deux doublerait le corpus, sur du non-résolu.**
- [ ] **Ce que le corpus ne dit toujours pas** : à quoi il sert. Il décrit des plans ; rien n'indique
  encore comment un plan se transforme en signal exploitable par le solveur — et le §6.1 rappelle
  que le guidage par classement est fermé. **Ne pas coder sur cette base sans avoir d'abord énoncé
  ce qu'on en ferait.**
- [ ] `GARER` à scinder ? À décider **par celui qui joue**, pas déduit (c'est la leçon du `T`).

#### 🎯 Session du 2026-08-02 (suite) — ONZE NIVEAUX RÉ-ANNOTÉS : l'instrument décidait du vocabulaire

**Le corpus du matin (8 niveaux, 46 étiquettes) a été intégralement REFAIT**, plus les niveaux 9, 10
et 11 joués et gagnés à la main dans la foulée. État final : **11 niveaux, 96 coups annotés.**

| | matin (8 niv) | **soir (11 niv)** |
|---|---|---|
| OUVRIR | 22 | **46** |
| RAPPROCHER | 14 | 21 |
| ECARTER | 6 | 19 |
| RETOURNER | 1 | 5 |
| **GARER** | **10** | **1** |
| `?` INCONNU | 0 | **4** (tous sur le 10) |

> 🎯 **LE RÉSULTAT DE LA SESSION : `GARER` s'est effondré de 10 à 1**, et pas parce qu'on a changé sa
> définition — parce qu'on a donné à l'annotateur **la vue des coups suivants**. `GARER` était la
> fourre-tout (le §6.2 du matin la décrivait comme « la seule catégorie SANS phase, celle qui couvre
> le plus de poussées »). Dès qu'on voit où la caisse VA, on sait si elle finit sur un but (`R`), si
> elle laisse passer (`E`), si elle ouvre (`O`), si elle ressort (`T`).
>
> **Une catégorie vague n'était pas un défaut du vocabulaire, c'était un défaut de l'INSTRUMENT.**

**L'OUTILLAGE, et chaque pièce vient d'un constat utilisateur en cours d'annotation :**

| ajout / retrait | ce qui l'a déclenché |
|---|---|
| **`N` RETIRÉE** (saut à la poussée choisie suivante) | *« un raccourci trop facile, qui fait rater des étapes »* — elle imposait la maille de la POUSSÉE |
| **`A` retirée** | 0 usage sur 46 étiquettes |
| **`?` reformulé** (« je ne sais pas ENCORE le dire ») | « réflexe » était faux : personne ne joue au hasard |
| **APERÇU des 12 prochaines arrivées** (chiffres sur le plateau, orange = choisie, gris = macro) | *« je ne vois qu'un coup à la fois, alors que pour choisir entre E et R j'ai besoin de voir plusieurs coups »* |
| **Cadre orange/gris sur la caisse du coup** + `Coup : A TOI` au panneau | *« si une poussée n'est pas la mienne, il faut que je le voie en interface »* |
| **ANNULATION (Retour arrière)** | *« un retour en arrière devrait annuler la dernière touche, sinon je vais y passer ma vie »* |
| **Zone du joueur en violet + compte `Zj`**, armée par `L`, sans bascule | *« O permet au perso de couvrir plus de cases ou d'autres cases »* |
| **compteur « reste N à toi »** | *« est-ce que tu sais quand plus aucun coup n'est à moi ? »* |
| `[manque]` **ancré au numéro de coup**, écrit dans le journal d'INTENTIONS | il partait orphelin dans le journal de jeu, qui est une donnée brute qu'on relit |

⚠️ **Un bug corrigé au passage, de la famille habituelle** : `journalIntentions` n'était fermé que
dans `rejoueJournal()`, donc il **survivait à un changement de niveau**. Une frappe avant d'avoir
pressé `L` écrivait dans le fichier du niveau précédent, avec les numéros de coup du niveau courant.
Silencieux. Corpus vérifié indemne (aucune ligne `coup 0/0`, et le miroir recoupe les en-têtes).
C'est le troisième état de la journée qui survit à ce qui le justifiait — avec la zone armée hors
rejeu et la marque de poussée affichée hors annotation. **Même forme à chaque fois, et c'est celle
qui ne se voit jamais à l'écran.**

**LE TEST-RETEST, et il bascule dans la journée :**

| niveau | deux annotations de la MÊME partie | accord |
|---|---|---|
| **7** (matin, avant outillage) | 14:32 contre 17:19 | **0 / 7** |
| **7** (soir, après) | 17:19 contre 20:0x | **7 / 8** |
| 3 | deux passes à 20 min | 5 / 5 |
| 4 | veille contre soir | 2 / 8 — dont **3 désaccords qui sont la correction `GARER`→`T` que le plan avait écrite** |

Le 0/7 du matin s'expliquait par la **maille** : la passe au `N` annotait à la poussée
(`RAPPROCHER` 12 fois), l'autre au plan. Retirer `N` et montrer la suite a suffi.

**LES OBSERVABLES A POSTERIORI — six essayés, UN survit.** Objectif : juger `E` sans jugement humain
(*« hyper difficile à juger sur le coup, il faut qu'on trouve un moyen de juger a posteriori »*).

| observable | verdict |
|---|---|
| devenir de la case libérée | bruit — sur 300 coups, toute case finit traversée |
| idem, borné à W poussées | tout bascule en « RIEN », aucune séparation |
| distance à la case finale | `GARER` 9/9, pile ou face |
| écart avant que la caisse rebouge | contaminé par la rafale |
| mobilité des caisses (poussées légales) | ne sépare pas les `O` inertes des `E` |
| **zone du joueur gagnée ≥ 2 ⇒ `O`, jamais `E`** | ✅ **SURVIT — 0 contre-exemple** sur tous les `E` du corpus |

⚠️ **Et un septième, annoncé comme fonctionnel puis RÉFUTÉ dans la même heure** : « la manœuvre
place-t-elle la caisse » donnait `T` 4/4 — artefact d'une **rafale non bornée**, qui suivait la caisse
à travers la macro et mesurait la livraison du solveur, pas la manœuvre humaine. Bornée aux poussées
CHOISIES, `T` tombe à 0/3. La « signature du `T` » (0 poussée neuve, mobilité −2) tombe avec.

> **LA LEÇON, payée quatre fois dans la journée : c'est toujours l'UNITÉ DE MESURE.** Poussée au lieu
> de manœuvre, manœuvre au lieu de manœuvre-choisie. Un observable juste sur la mauvaise unité rend
> un résultat propre et faux — et il se présente comme un résultat.

⚠️ **LA LIMITE DU SEUL OBSERVABLE QUI RESTE** : il est **sûr mais silencieux**. Sur les niveaux à
démêlage dense, **12 des 19 `OUVRIR` laissent la zone du joueur RIGOUREUSEMENT inchangée** (4 : 3/5
inertes ; 9 : 2/5 ; 11 : **7/9**). Là où le démêlage est dur, « ouvrir » ne veut donc pas dire « le
perso atteint plus de cases » — et l'hypothèse de rechange (ouvrir une route de CAISSE) est réfutée
par la mobilité. **On n'a aucun observable pour ce que l'utilisateur voit là.**

**LES `[manque]` — 25 signalements avec cause, et la distribution existe enfin :**

| cause | occurrences |
|---|---|
| **DESCENTE BLOQUÉE** en cours de route | **18** |
| détour non-monotone requis | 4 |
| joueur du mauvais côté | 3 |

- **`echecBloque` écrase tout (72 %)** — le §6.3 l'écrivait déjà (« c'est LE mode d'échec, sur les 17
  niveaux mesurés sans exception »), mais par échantillonnage. **C'est maintenant mesuré sur des
  chemins de solution réels.** Deux régimes distincts : la macro meurt **à 1-3 poussées du but**
  (6, 7) ou **avec 10 à 26 de distance restante** (2, 9).
- ⚠️ **Une case tue la macro trois fois** : `(3,7)` sur le niveau 7, malgré jusqu'à 3 branches de
  backtracking. Premier goulot identifié comme tel.
- 🎯 **L'ITEM 6 A SON CAS.** Le secours borné pour les détours non-monotones est fermé depuis le
  2026-07-23 faute d'un cas qui coûte ; le 2026-08-01 en avait trouvé **un** (niveau 20), non
  bloquant. La campagne en produit **quatre de plus** (2 ×2, 3, 5), tous sur des chemins gagnants.
  ⚠️ **Ce qui n'est toujours PAS établi : leur coût pour la RECHERCHE.** Sur le niveau 2 ces macros
  absentes coûtent 16 poussées de convoyage **à l'humain** ; le solveur, lui, résout le 2 à
  l'optimum en poussées simples. La barre de l'item 6 est à moitié franchie.

**TROIS FAITS PONCTUELS, tous neufs :**
- **Le 11 : après le coup 393 (4/14 posées), les 177 poussées restantes sont TOUTES de macro**, et ses
  9 `OUVRIR` sont tous à `posees 0/14`. Le §6.2 déduisait que *« le blocage du 11 n'est pas dans la
  salle mais dans l'acheminement »* ; on le lit ici en deux nombres, sur une partie gagnante.
- **Le 9 joué à la main en 237 poussées — le compte exact du solveur.** Partie humaine, pas un score.
- **Le niveau 2 a une RAMPE DE LANCEMENT** : 5 de ses 10 macros partent de la même case (11,6), et
  **16 poussées choisies sur 16 après le coup 19 ne font qu'y convoyer des caisses**. Au coup 205 la
  macro est **à une poussée près** : 0 macro jouable en (11,7), 19 poussées offertes depuis (11,6).

**LE PREMIER `?` DU CORPUS — quatre, tous sur le niveau 10** (coups 141-158, `posees 3/32`). Le matin,
le plan notait que tant qu'aucun `?` ne survivrait, le corpus n'aurait jamais produit **la donnée qui
montre qu'une catégorie manque**. Elle est là, et le geste est identifiable : la caisse A **quitte**
(10,3) pendant que la caisse B **vient l'y remplacer**. Une substitution sur la même case — ni
« écarter », ni « ouvrir », mais **un échange de destinations**. ⚠️ Rapprochement à ne pas graver mais
à retenir : `deltaf` (§6.3) mesure exactement cet objet dans le solveur — *« la caisse que la macro
pose n'est pas celle que le couplage y destinait : elle lui VOLE son but »* — et les cinq `df = +8`
du niveau 4 sont un réarrangement du couplage, jamais creusé. **Non tranché : l'utilisateur n'a pas
encore dit ce qu'il faisait.**

**Reste ouvert :**
- [ ] 🎯 **RÈGLES PLUS FIABLES — constat utilisateur en fin de session** : *« vraiment pas facile de
  choisir entre tous ces trucs »*. Ses données lui donnent raison : `R` est stable, `G` s'est
  effondré, **`O` contre `E` est instable** (toute la phase d'entrée du 4 a changé de camp entre deux
  passes) — et aucun observable ne les sépare. **Le vocabulaire demande de CLASSER là où les classes
  se recouvrent.** Piste proposée, non codée, à discuter : **DÉSIGNER au lieu de classer** — cliquer
  le *bénéficiaire* (l'autre caisse, ou le perso) plutôt que choisir une étiquette ; `E` et `O` s'en
  déduisent, et on gagne **laquelle**, que le plan tient pour la seule information non déductible.
  Second volet : **ne plus annoter `R` du tout** (la partie étant finie, l'avenir de chaque caisse est
  connu) et ne garder que les poussées qui n'avancent pas la caisse — c'est-à-dire le mou du §3.
  ⚠️ Ce second volet suppose un observable qui a déjà échoué deux fois : **à vérifier avant de coder**.
  ⚠️ **RÉSERVE DE L'UTILISATEUR, posée avant tout codage** : *« même le bénéficiaire, ce n'est pas
  évident »*. À prendre au sérieux — c'est lui qui annote, et c'est exactement le genre d'avertissement
  que la journée a validé deux fois (le `T` ajouté sur son constat, le `N` retiré sur le sien). Si
  désigner s'avère aussi flou que classer, la conclusion ne sera pas « il faut une troisième forme
  d'étiquette » mais **que l'intention n'est pas décidable coup par coup dans cette phase**, ce qui
  est un résultat en soi et rejoint le §3 (le mou est un résidu d'ORDONNANCEMENT, pas une propriété
  locale). **Ne pas coder la désignation avant un essai à la main sur un ou deux plans.**
- [ ] **Les 4 `?` du niveau 10** : demander à l'utilisateur ce qu'ils faisaient. Une entrée manque
  peut-être au vocabulaire, et elle aurait un correspondant déjà mesuré dans le solveur.
- [ ] **Douze journaux restent annotables** : 12, 13, 18, 19, 20, 22, 23, 24, 25, 26, 27.

**État du code** (non commité, branche `ordre-dynamique`) : `mainwindow.cpp`/`.h` — retrait de `N` et
de `prochainCoupChoisi()`, retrait de `A`, reformulation de `?`, `journalSignalement()`,
`pousseAnnulation()` + les deux piles, aperçu des poussées à venir, fermeture du journal au
changement de niveau, `lbPas` descendu dans la barre d'état (il élargissait la fenêtre) ;
`wgame.cpp`/`.h` — `showZoneJoueur()`, `setPousseeCourante()`, `setApercuSuite()`. **Rien dans le
solveur, aucune variable d'environnement** (§7). Tout ceci part avec la campagne.

#### 🎯 Session du 2026-08-03 — QUATRE NIVEAUX GAGNÉS À LA MAIN (14, 15, 16, 17), et LA LOI DE L'ORDRE ENFIN TROUVÉE

**Quatre parties humaines gagnantes de plus** : 14, 15, 16 (non résolus) et **17** (résolu, 213
poussées — l'optimum exact). Journaux compressés (`compresse_journaux.py`, filtre de niveaux
ajouté : `python3 compresse_journaux.py 14 15 16 17 --ecrire`, sans quoi les 24 journaux sont
tous réécrits). Trois nouveaux outils dans l'UI : sélecteur d'HISTORIQUE DES RECORDS (le solveur a
DEUX points d'enfilage — recherche principale et plongeon — un record écrase l'ancien chemin
visionné, donc voir le record 7 ET le record 8 d'un même run demandait de les conserver tous),
touche **`C`** = critique du chemin du SOLVEUR en texte libre (miroir de l'annotation d'intentions,
mais sur ce que le solveur fait, journal `solveur_niveau_XXXX_critique.txt`, plateau joint).

**LE GRADIENT « HORS RÉGIME MACRO »** — part des poussées CHOISIES que le générateur du solveur ne
produit PAS du tout (ligne `[rang]`) :

| niveau | résolu ? | hors régime | `df = 0` (même palier que le meilleur frère) | poussées de macro |
|---|---|---|---|---|
| 14 | non | **100 %** | — | 17 % |
| 15 | non | 78 % | 51 % | 12 % |
| 16 | non | 35 % | 76 % | 36 % |
| **17** | **oui** | **4 %** | **94 %** | **46 %** |

Monotone sur les quatre. Sur le niveau que le solveur sait finir, 96 % des coups humains sont dans
son générateur et 94 % au même `f` que le meilleur frère ; sur celui qu'il ne finit pas du tout,
aucun. ⚠️ Quatre points, un seul résolu — piège §11.4, à ne pas graver, mais premier candidat qui
ordonne des niveaux SANS les résoudre.

**LE 16 EST UN CONTRÔLE QUI FERME LA QUESTION DE L'ORDRE** : ordre humain = ordre calculé, **15/15**
buts au même rang. Et le solveur plafonne quand même à 7/15 sur 9,7 M dépilements. **L'ordre n'est
pas la variable** sur ce niveau — c'est l'acheminement. (Sur le 14 et le 15, l'ordre humain diffère
du calculé, injecté et mesuré : +3 caisses au plafond sur le 14, mais −2 sur le 15 — pas de loi.)

**ZÉRO FAUX POSITIF D'ÉLAGAGE**, vérifié en rejouant les `[undo]` des quatre parties (1280 + 987 +
1050 + 553 coups) : aucune ligne `⚠ ECARTE par le solveur` ne survit dans le chemin gagnant.
`checkDefaite` et le corral-N sans faute sur trois niveaux NON résolus — première fois que `fp`
peut être étendu à du non-résolu.

---

**🎯 LA LOI DE L'ORDRE — trouvée en fin de session, par itération avec l'utilisateur sur le 16.**

**Point de départ : « les poussées simples ne respectent PAS l'ordre de remplissage »**, constaté
par l'utilisateur sur un plateau exporté du 16 (record 7/15, coup 18) : `(8,11)` posé au rang 11
alors que les rangs 0 à 3 sont vides. `ordreButs` PILOTE la macro mais rien n'interdit une poussée
SIMPLE de déposer une caisse sur n'importe quel but.

**QUATRE FORMULATIONS RÉFUTÉES avant la bonne, chacune par la mesure ou par le juge `fp` :**

1. **Ordre strict sur TOUS les buts** — repose sur « interdire de remplir dans le désordre », déjà
   réfuté au §4 (rend le niveau 1 insoluble : atteindre (17,6) exige de faire ÉTAPE sur (16,6), qui
   est un but). Confirmé à nouveau : réfuté par la propre partie de l'utilisateur sur le 16 (35 et
   36 états où (8,9) et (9,9), rangs 13-14, sont remplis bien avant leur tour).
2. **Buts EN COIN seulement** (aucune poussée sortante, testable sur les murs seuls) — codé en
   régime séparé `AstarMacroCouplagePlongeonCoins` (`PAQUET`-like, `qgetenv`). Canari : **9/11
   niveaux identiques, mais le NIVEAU 6 PASSE DE RÉSOLU À `AUCUNE`**. Cause : l'ordre CALCULÉ du 6
   remplit la colonne 2 avant la colonne 1, alors que (1,5) ne s'atteint qu'EN TRAVERSANT la
   colonne 2 — même piège que le niveau 1, sur un vrai plateau cette fois.
3. **`casesMortes` recalculée avec UN SEUL but** (« vu du solveur, seul le but actif existe, le
   reste est du sol ») — mesuré : tue **cinq buts futurs sur la ligne 11** du 16, dont trois cases
   de TRANSIT ((9,11)(10,11)(11,11)) — casse l'acheminement.
4. **`casesMortes` avec TOUS les buts restants (rang ≥ actif)** — sûr, mais **une case-but n'est
   JAMAIS morte** dans cette vue (elle est sa propre graine du BFS à rebours) : ne coupe RIEN de ce
   qui posait problème. Confirmé par l'utilisateur : *« une case but n'est jamais morte »*.

**LA LOI QUI SURVIT, obtenue par un GABARIT DE DESSIN** (`gabarit_niveau_0016.txt`, script
`scratchpad/gabarit.py` — un plateau par but actif, `A`=actif, `*`=but déjà rempli, chiffre=rang,
rien pré-rempli). L'utilisateur a dessiné les cases mortes sur les 15 plateaux, corrigé deux fois,
et énoncé la loi en trois temps :

> **1.** Les cases mortes se calculent **tous les buts supprimés SAUF l'actif** — sinon une case-but
>    est vivante d'office et la table ment (`checkVictoire` ne compare qu'aux caisses, aucune
>    protection naturelle contre ça).
> **2.** Une case ainsi morte redevient **du SOL** si elle est **ALIGNÉE** (même ligne OU même
>    colonne) avec le but actif.
> **3.** Le reste est inchangé (murs, buts déjà remplis = obstacles).

**Vérifiée exactement sur les 15 plateaux du gabarit** (script `scratchpad/juge_loi.py`) :

```
rang  0 (12,7)  predit [(8,11)(9,11)(10,11)(11,11)]  == dessin utilisateur
…
rang 14 (8,9)   predit []                             == dessin utilisateur
   🎯 LOI VÉRIFIÉE SUR LES 15 PLATEAUX
```

**PUIS PASSÉE AU JUGE SUR 24 PARTIES HUMAINES GAGNANTES** (`fp` étendu, murs seuls, ordre calculé
sauf mention) :

| | niveaux |
|---|---|
| **0 faux positif** (19 niveaux) | 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 16, 17, 19, 20, 23, 24, 25 |
| faux positifs, **guéris par l'ORDRE HUMAIN injecté** | **12** (108→0), **14** (147→0), **15** (151→0) |
| faux positifs, ordre non vérifié | 18 (93), 22 (406), 26 (14) |

**Les trois seuls faux positifs vérifiables sont EXACTEMENT les trois niveaux dont on savait par
ailleurs que l'ordre calculé est faux** (le 12 par la session du 2026-07-31, le 14/15 par le
gradient ci-dessus). ⚠️ **La loi n'est donc PAS un test d'ordre absolu — c'est un test de
COHÉRENCE entre un ordre et une partie**, prouvé sur le niveau 6 : deux parties humaines gagnantes
existent, colonne 2 d'abord (2026-08-01) et colonne 1 d'abord (2026-08-03, rejouée exprès pour ce
test) — **chacune valide SON ordre (0 FP) et condamne l'autre (62-78 FP)**. Le 6 admet deux ordres
de remplissage valides ; la loi ne sait pas trancher entre eux, elle sait seulement dire si un
ordre donné est conforme à une partie donnée.

**Conséquence pour 18/22/26** : ne PAS lire leurs faux positifs comme « ordre calculé faux » sans
vérifier — il faudrait, comme sur le 6, REJOUER l'ordre calculé à la main pour savoir s'il est
seulement DIFFÉRENT ou réellement infaisable.

**⚠️ RIEN N'EST CODÉ NI PROMU.** La loi est validée en JUSTESSE (0 FP partout où l'ordre et la
partie s'accordent), PAS en GAIN — aucune mesure d'états épargnés. Prochaine étape convenue :
câbler en régime séparé (comme le motif du §6.1), canari sur les 15 résolus, PUIS mesurer le gain
sur 12/14/15/16 avec leur ordre humain injecté.

**Outils neufs, tous dans `scratchpad/` (à rapatrier si retenus) :**
- `gabarit.py <niv>` — un plateau par but actif, rien pré-rempli, pour dessiner une règle sans que
  l'instrument ne la suggère (leçon du 2026-08-02 sur l'échec de la fiche « bénéficiaire »).
- `juge_loi.py` — juge une loi de cases mortes sur TOUTES les parties humaines gagnantes du
  répertoire, avec option d'ordre injecté. Réutilisable pour toute règle future du même genre.

**Reste ouvert :**
- [ ] Coder la loi en régime séparé et mesurer le GAIN (états épargnés) sur 12/14/15/16.
- [ ] Vérifier 18, 22, 26 en rejouant leur ordre CALCULÉ à la main (pas en le condamnant sur écart).
- [ ] Réparer les 284 ancres `coup N/M` périmées des fichiers d'intentions (2026-08-02) — calculable
  exactement, testé reproductible sur 5 niveaux depuis `f3bf1fb`, ZÉRO ancre perdue. Non fait
  aujourd'hui, la session a bifurqué sur la critique du solveur.

#### 🎯 Session du 2026-08-04 — LA LOI CODÉE ET JUGÉE, le GEL HORS TOUR, et une précédence d'espèce neuve

**Trois choses, dans cet ordre : la loi du 2026-08-03 passe du dessin au code ; sa seconde moitié
(le gel) apparaît en cours de route ; et la bisection d'une partie humaine du 16 produit une règle
CAISSE → BUT que le projet n'avait pas.**

---

**LA LOI, CODÉE — et elle était presque GRATUITE.** `distanceParBut` fait **déjà** un BFS à rebours
**par but**, sur les murs seuls (aucune caisse, aucun autre but en obstacle) — soit exactement la vue
sous laquelle la loi a été dessinée et jugée. Il ne restait qu'une réduction booléenne au chargement,
`mortesLoi[BUT * size + CASE]`, plus `rangDeBut`. Table PLATE et non un vecteur de vecteurs : le
solveur copie `Game` par candidate, un `QVector<QVector<bool>>` coûterait nbButs incréments de
compteur par copie là où la table plate n'en coûte qu'un.

**La règle finale, en quatre temps** (les trois derniers sont des précisions de l'utilisateur, données
à l'écran sur des captures du 16 puis du 6) :
1. **cases mortes calculées tous les buts supprimés SAUF l'actif** ;
2. une case ainsi morte redevient du sol si elle est **ALIGNÉE** avec le but actif — et
   **l'alignement s'arrête au premier MUR** : « aligné » veut dire qu'on pourrait encore pousser en
   ligne droite jusqu'au but ;
3. **la mort dynamique ne concerne QUE les cases-buts.** Une case ordinaire qui n'atteint pas le but
   actif reste un garage licite : la caisse qui s'y trouve attendra le but qu'elle sait servir. La
   condamner serait le §4 en énième déguisement ;
4. les buts de rang **inférieur** à l'actif sont **exemptés** (rangés à leur tour, donc obstacles).

⚠️ **Un coin n'est jamais exempté par l'alignement** : d'un coin on ne pousse nulle part, l'exemption
n'a donc pas de sens là. Sans ce point la loi ratait précisément les BUTS en coin, seuls endroits où
le corner deadlock n'est pas déjà connu — un but est sa propre graine du BFS à rebours, donc jamais
mort dans la table ordinaire.

⚠️ **RIEN NE DÉPARTAGE ENCORE LES DEUX VERSIONS DE L'ALIGNEMENT** — vérifié, pas supposé : le gabarit
du 16 rend **15 plateaux sur 15 avec l'arrêt au mur COMME avec la version littérale**. La version
retenue tient de l'énoncé de son auteur, pas d'un juge, et c'est la plus mordante des deux. Un niveau
où elles diffèrent reste à trouver.

**LE JUGE — `mesures/loi` (NEUF).** `loi <niv> <gabarit.txt>` compare la table CALCULÉE au dessin
FAIT À LA MAIN, case par case, et sort non nul au moindre écart. Il a trouvé un vrai bug : le
quatrième temps (buts déjà remplis = obstacles) n'était câblé que dans le SOLVEUR, si bien que
**l'overlay de l'UI montrait une autre règle que celle que le solveur appliquait**. Porté dans la
table, en un seul exemplaire. ⚠️ Ce juge compare le **SURPLUS** (loi moins table ordinaire) : c'est
ce que l'utilisateur a dessiné, le reste étant coupé par `checkDefaite` depuis toujours.

⚠️ **Le gabarit du 16 ne valide plus la loi, et c'est voulu** : 8 écarts, tous exactement les deux
buts en coin ajoutés à la demande — (12,11) aux rangs 0-3, (8,11) aux rangs 6-9. **Le dessin du
3 août est antérieur à la précision du 4.** À redessiner si on veut garder un juge vert.

---

**LE GEL HORS TOUR — la seconde moitié, et elle ne coûte rien à écrire.** Constat utilisateur sur un
plateau exporté du 16 : *« quatre caisses collées les unes aux autres sur du sol, c'est mort
assuré »*. Elles étaient sur des BUTS (rangs 10, 12, 13, 14) alors que l'actif était le rang 0 — donc
sur du **sol**, au sens de la loi. Or `caisseGelee`/`bloqueeSurAxe` travaillent déjà sur
`estCaisse()`, qui couvre `tcCaisse` **et** `tcGoalCaisse` : le seul obstacle était la boucle de
`checkDefaite`, qui ne présente que les `tcCaisse`. `Game::geleHorsTour(butActif)` lève cette
exemption pour les buts de rang supérieur. **Rien de neuf n'est calculé.**

Pourquoi les trois détecteurs existants sont aveugles à ce motif, chacun pour une raison juste au
niveau de la CAISSE et fausse au niveau de la RÉGION :

| détecteur | la ligne |
|---|---|
| `checkDefaite` (game.cpp:213) | « ne teste que les tcCaisse : une caisse gelée SUR un but est un morceau de la solution » |
| gate du corral-N (game.cpp:1126) | `if (frontHorsBut == 0) continue;` — les caisses-frontière étaient sur des buts, **la région est abandonnée avant toute preuve** |
| motif du paquet | défini sur un groupe de caisses **hors but** |

**Le plateau était bien MORT** : réduit à UNE caisse hors but (méthode de réduction de l'utilisateur,
2026-08-03), l'A\* pur épuise l'espace en **10 états**. Le juge `paquet` rendait « inconnu ».

⚠️ **Ce n'est pas une preuve** : une caisse gelée sur un but de rang supérieur remplit quand même ce
but, la partie reste gagnable dans l'absolu. Même statut que la loi — une exigence d'ORDRE — donc
régime séparé.

⚠️ **PREMIÈRE FOIS QUE L'ORACLE NE CONFIRME PAS UN DIAGNOSTIC HUMAIN** (le plan comptait 6 sur 6). Un
premier état annoté « deadlock créé, les caisses (8,10) et (9,10) ne peuvent plus être bougées » ne
tenait pas : les deux étaient poussables, `paquet` rendait inconnu, les réductions à 1-3 caisses
étaient toutes solubles, et A\* posait **6 caisses de plus** depuis là. L'utilisateur en a convenu et
a exporté le bon plateau. **Le mécanisme qui a permis de trancher est la réduction, pas l'A\* complet.**

---

**LE RÉGIME `loi` — canari, binaire contre binaire, hors du répertoire du projet :**

| niv | `coupl-plongeon` | `loi` + gel | |
|---|---|---|---|
| 0-5, 17 | — | **identiques, poussées intactes** | |
| **7** | 24 989 / 90 | **24 400 / 90** | **247 prunes de GEL** |
| 9 | 83 029 / 237 | **82 106** / 237 | 860 prunes de loi |
| **6** | 570 / 110 | **AUCUNE** | gel = **0** |

**Le 6 est perdu par la LOI, pas par le gel, et la loi a raison sur le fond.** Son ordre calculé pose
(2,5), (2,4), (2,3) aux rangs 0-1-2, or **on n'entre en (1,y) qu'en poussant vers l'ouest depuis
(2,y)** : colonne 2 remplie, la colonne 1 est inatteignable. La loi tue donc toute la colonne 1 dès
le rang 0. Cet ordre **est** infaisable — le 6 rejoint 12/14/15, les niveaux dont la loi détecte que
l'ordre calculé est faux. Le prix : en régime `loi` avec l'ordre par défaut, le 6 n'est plus résolu.

🔴 **UN VERDICT PUBLIÉ PUIS RETIRÉ, ET LE §7 EN EST LA CAUSE.** « Le 6 tombe à cause de la règle des
coins » a été annoncé, puis retiré, puis re-établi autrement — parce que **`ordre_niveau_0006.txt`
traînait à la racine et s'injectait dans tout run du 6 lancé de là**, et que son contenu a changé
entre deux séries. Deux mesures du même niveau, incomparables, sans que rien ne le signale. C'est
exactement ce que l'entrée du §1 redoutait pour ce mécanisme (« bruyant des deux côtés » — la ligne
`[ORDRE_FICHIER]` était bien là, personne ne l'a lue). **Toute mesure sur un niveau doit se lancer
d'un répertoire sans fichier d'ordre**, ou vérifier la ligne d'en-tête.

---

**LE 16 BISECTÉ — et le témoin d'incomplétude le plus net du projet.**

L'utilisateur joue le 16 en hybride et exporte deux positions. Miroir Python du parseur de journal,
**validé avant lecture** : rejeu des 238 coups de la dernière partie (45 poussées, 6 `[undo]`
appliqués), plateau final **identique à l'octet** à l'export. Puis une position par poussée, et une
bisection au solveur (`coupl-plongeon`, 30 s) :

| position | verdict |
|---|---|
| p45 (fin) | ✅ **résolu — 45 états, 147 poussées** |
| **p44** | ❌ **`AUCUNE` en UN SEUL état exploré** |
| p01, p23, p34, p39, p42, p43 | non résolues |

> **Le régime d'engagement de la macro n'enfile AUCUN enfant depuis p44, alors qu'une poussée légale
> mène à p45, qu'il résout en 45 états.** Le §6.0 énonce l'incomplétude du régime depuis toujours ;
> ici elle est démontrée **à un coup près**, sur un témoin minimal et reproductible. (A\* pur, complet,
> confirme que p44 est vivant : 5 M états, file qui monte, `max 9/15`.)

⚠️ Ces plateaux rechargent leur statique (§7) — mais le but actif recalculé est **(12,7) rang 0** sur
p043, p044 ET p045, donc la comparaison porte bien sur la même question.

**La poussée qui bascule tout** : `joueur (11,6)->(10,6) POUSSE caisse ->(9,6)`. Le joueur, venu par
le couloir droit jusqu'en (11,6), déloge la caisse de **(10,6) vers l'ouest**.

**LES DEUX SUBTILITÉS DU 16, énoncées par l'utilisateur et vérifiées :**
1. **Il faut STOCKER trois caisses** en (11,10), (9,10) et (9,9) avant de boucher la colonne. C'est
   une contrainte d'exécution, pas une préférence : aucune caisse n'entre par le couloir droit (elle
   mourrait en (11,6)), donc les trois arrivent par la gauche et remontent la colonne une par une —
   (12,10) → (12,9) → (12,8) → (12,7) — et **une fois (12,9) posée, plus rien ne remonte**. La
   rangée 11 ne sert à rien pour ça : rien n'en sort (la rangée 12 est un mur continu, donc aucune
   poussée vers le nord n'a d'appui), une caisse qui y descend n'en repart jamais. **La loi disait
   déjà exactement ça** — pour le but actif (12,7) elle déclare morte toute la rangée 11 et rien
   d'autre. Premier endroit du projet où une règle calculée énonce une stratégie humaine.
2. **La caisse (3,6) sert de passage entre le bas et le haut** et doit être manipulée plusieurs fois.
   Test à une seule variable : remise en (3,6) dans la position gagnante, celle-ci **redevient dure**
   (4,5 M dépilés, `max 12/15`, non résolue) là où elle se résolvait en 45 états.

⚠️ **Le stock N'EST PAS le verrou** : depuis la position de stock réellement jouée, `coupl-plongeon`
ET `loi` plafonnent au **même 8/15** en 180 s (file −22 % pour la loi, 879 081 prunes dont 4 440 gels
— son premier vrai terrain). La difficulté est en aval du stock.

---

**🎯 LA RÈGLE QUI EN SORT — PRÉCÉDENCE CAISSE → BUT.** Toutes les précédences du projet sont
but → but. Celle-ci est d'une autre espèce :

> **Soit une caisse C, et A(C) l'ensemble des cases d'appui dont le joueur a besoin pour la pousser
> dans une direction quelconque. Si remplir un but G prive le joueur de TOUT A(C), alors C doit avoir
> été déplacée AVANT que G ne soit rempli** — sinon elle gèle à vie, sur du sol, hors but.

Statique, `O(caisses × buts × plateau)`, calculable au chargement comme `precedenceGlobale`.
**Relaxation optimiste** (le BFS de marche ignore toutes les autres caisses) : une contrainte trouvée
est une PREUVE, un silence ne promet rien.

⚠️ **LE RAFFINEMENT SANS LEQUEL ELLE NE VOIT RIEN, et il tient en une ligne : une poussée qui TUE la
caisse n'est pas une issue.** Premier jet : 0 contrainte sur le 16. La caisse (10,6) a deux poussées
géométriques, et celle vers l'est — appui (9,6), trivialement atteignable — masquait tout ; mais elle
la dépose en (11,6), d'où rien ne sort jamais. En excluant les destinations qui sont des **cases
mortes**, la contrainte apparaît.

**Ce qu'elle rend (`mesures/porte`, NEUF) :**

| | résultat |
|---|---|
| **les 15 RÉSOLUS** (témoins) | **0 contrainte partout** |
| **les 18 non résolus** | **2 seulement** : le **16** (avant le rang **0**) et le **30** ((15,10) avant (16,8), rang 17) |
| sur le 16 | `caisse (10,6) : à dégager AVANT (12,7)r0 (12,8)r1 (12,9)r2 (12,10)r3` |
| **la partie humaine du 16** | ✅ **la respecte** — les quatre buts de la colonne sont VIDES au moment où (10,6) est dégagée |

**C'est la stratégie de l'utilisateur, redérivée mécaniquement en quelques millisecondes.** Et la
validation ne se limite pas au silence sur les témoins : une solution réelle vérifie la contrainte,
sur le seul niveau où la règle parle.

⚠️ **Muette sur 16 des 18 non résolus** — un motif précis, pas l'explication du mur. Ne pas
généraliser (§11.4, commis trois fois le 2026-08-01).
⚠️ **Elle ne doit JAMAIS couper** : c'est de l'ORDONNANCEMENT, pas de la solubilité (§6.2,
2026-07-30 — la précédence par paires a fait 9 niveaux sur 10 en faute au juge `fp` quand on a voulu
en faire un test de mort).

**Reste ouvert :**
- [ ] **Le solveur n'a aucun moyen d'obéir à cette précédence.** `ordreButs` ordonne des buts ; ici la
  contrainte dit « dégage telle caisse d'abord » — c'est la catégorie `OUVRIR` du corpus d'intentions,
  que rien dans le solveur n'exprime. **Point d'accroche existant** : le régime `ordre-dyn`, où
  `butActif()` rend le premier but non rempli *encore livrable*. Y ajouter « … et dont la contrainte
  de porte est satisfaite » rendrait (12,7) non éligible tant que (10,6) est en place. Petit, LOUD,
  mesurable sur le 16 immédiatement.
- [ ] **Le GAIN de la loi n'est mesuré nulle part** : 247 prunes de gel sur le 7 (−2,4 % d'états),
  860 sur le 9, file ÷1,3 sur le 16 — aucun niveau débloqué. La loi est validée en JUSTESSE, pas en
  RENDEMENT, et c'était déjà la réserve du 2026-08-03.
- [ ] **Redessiner le gabarit du 16** avec la règle des coins, sinon `mesures/loi` reste rouge et ne
  juge plus rien.
- [ ] **Le 6 en régime `loi`** : accepter la perte (l'ordre calculé est réellement infaisable) ou
  corriger l'ordre du 6. C'est le même arbitrage que 12/14/15.
- [ ] **Le 30** est le seul autre niveau à contrainte de porte, et elle y est souple (rang 17). À
  regarder si on cherche un second cas.

**État du code** (non commité) : `game.cpp`/`game.h` — `mortesLoi`/`rangDeBut`/`calculCasesMortesLoi`,
`casesMortesLoi` (le surplus, pour l'affichage), `caseMorteOrdinaire`, `geleHorsTour` ;
`solveurastar.*` — régime `loiOrdre`, `loiTropTot` aux DEUX points d'enfilage, stats `[LOI]` ;
`solveur.*` — type `AstarMacroCouplagePlongeonLoi` ; `mesures/bench.cpp` — mode `loi` ; `wgame.*` /
`mainwindow.*` — overlay **deux gris** (pâle = table ordinaire, foncé = surplus de la loi, recalculé
AU TRACÉ) + case à cocher. **Neufs** : `mesures/loi.cpp`/`.pro`, `mesures/porte.cpp`/`.pro`.
Miroir de rejeu du journal : jetable, non versionné.

#### ⏸️ Session du 2026-08-04 (suite) — LA CONTRAINTE DE PORTE GREFFÉE SUR `butActif()`

**C'est l'item que la session ci-dessus laissait en tête des ouverts** : la précédence
caisse → but existait comme DÉTECTEUR (`mesures/porte`) mais le solveur n'avait aucun moyen d'y
obéir. Point d'accroche retenu : le régime d'essai `ordre-dyn`, où `butActif()` rend déjà le premier
but non rempli *encore livrable* au lieu du premier tout court. On y ajoute *« … et dont la porte
n'est plus occupée »*.

**Le code, deux pièces, aucune variable d'environnement (§7) :**
1. **`Game::calculPorteRequis()`** — statique, au chargement comme `precedenceGlobale`. Pour chaque
   but, la liste des cases à dégager avant lui, en **CSR** (`porteCases` concaténées + `porteDebut`,
   deux vecteurs plats) : le solveur copie `Game` par candidate, un vecteur de vecteurs coûterait
   nbButs incréments de compteur par copie. Trace `[PORTE]` au chargement, **passive** — elle
   n'ajoute ni ne coupe aucun comportement, donc elle ne peut pas faire diverger l'app du bench.
2. **`porteBloquee(but)` testé à TROIS endroits** de la branche dynamique : le jalon (une caisse peut
   venir se garer sur une case de porte APRÈS l'élection du but), la boucle d'éligibilité, et le
   repli. Un but bloqué est **PASSÉ**, jamais coupé.

⚠️ **Le défaut n'est pas concerné, par construction** : tout vit dans la branche `ordreDynamique`.

**🔴 UN BUG POSÉ PUIS RATTRAPÉ, ET SA LEÇON EST GÉNÉRALE.** Premier jet : le test était sur le chemin
nominal mais **pas sur le repli** *« aucun but livrable »*, qui rend `ordreButs[0]` en dur. Mesuré sur
la trace du 16 : `(12,7)` — le but que la contrainte écarte — était élu **10 fois contre 7 avant la
greffe**. La greffe empirait ce qu'elle devait corriger.

> **Un garde-fou posé sur le chemin nominal et pas sur le repli ne tient pas : le repli sert quand ça
> va mal, c'est-à-dire exactement quand la contrainte compte.**

Corrigé avec un **dernier recours** qui ne peut jamais rester bloqué : si TOUS les buts sont bloqués,
on en rend un quand même (rendre −1 signifierait « gagné » à l'appelant).

⚠️ **ET LE COMPTE D'ÉLECTIONS DANS UNE TRACE NE PROUVE RIEN** : élire (12,7) une fois (10,6) dégagée
est parfaitement correct, et la trace `[ordre]` ne porte pas la position des caisses. D'où un **test
unitaire** ajouté à `mesures/porte` : armer l'ordre dynamique sur le plateau de DÉPART et demander
quel but est élu. Sur le 16 il rend **(8,11) rang 11, porte libre** — plus (12,7). C'est le seul
endroit où la greffe se vérifie sans lire une trace de plusieurs millions d'états.

**Canari :**

| juge | résultat |
|---|---|
| défaut `macro`, 0/1/2/3/5/6/7/17 | **identique** (4/14/412/499/9 123/570/24 376/24 786) |
| **`ordre-dyn`, avant contre après**, mêmes 8 niveaux | **identique à l'unité** — inerte là où il n'y a pas de contrainte, ce qui est le cas des 15 résolus |

**Le 16, à budget égal (120 s) :**

| | dépilés | file | max | `rangees` courant |
|---|---|---|---|---|
| `ordre-dyn` seul | 3 224 000 | 3 427 037 | 8/15 | 4 |
| **+ porte** | 2 287 000 | 3 106 289 | **9/15** | **6** |

⚠️ **Ça ne prouve rien** — le §6.6 a réfuté la progression à budget borné comme prédicteur (le 10 y
affiche 12 % et il tombe). Indice, pas verdict.

**⏸️ RUN SANS BUDGET — ARRÊTÉ AU PLAFOND MÉMOIRE, AUCUN VERDICT.** Relevé final, veilleur posé à
14 Go pour que l'arrêt soit VOLONTAIRE et laisse une trace propre plutôt que d'être subi :

| | |
|---|---|
| durée | **1 h 27** de CPU |
| dépilés / vus | **101 141 000** / **188 848 380** |
| file à l'arrêt | **120 422 428**, **+1 672 par millier — elle MONTE** |
| **max** | **11/15** |
| footprint | 10 Go à 46 min, 11 à 1 h 20, **12 → 14 Go en une minute** à la fin |

- **Le `max 11/15` est un record du projet sur ce niveau** : le plafond tenait à **7/15** depuis le
  début du chantier, et le motif du paquet à budget 2 000 l'avait poussé à 8/15, trois fois sur trois.
  C'est ce qui survit de ce run, et c'est tout.
- ⚠️ **AUCUN VERDICT, DANS AUCUN SENS — cinquième application de la règle** (31, 13/r07, 11, 18, et
  maintenant le 16 en `ordre-dyn`+porte). Ce qu'on peut écrire : *le 16 ne se résout pas en 101 M
  dépilements dans ce régime*. Rien de plus. En particulier, **rien ne dit que la contrainte de porte
  aide**, et rien ne dit qu'elle nuit.
- **188,8 M états vus** : deuxième plus gros run du projet, derrière les 217 M du 18 et **au-dessus
  des 87 M qui ont suffi à résoudre le 11**.
- 🔴 **Le mur est la MÉMOIRE, pas le temps** — 1 h 27 seulement, et le §6.5 se re-confirme une fois
  de plus. La montée finale est BRUTALE (12 → 14 Go en une minute) : un veilleur à intervalle d'une
  minute est à la limite du suffisant, et attendre la pression système aurait fait perdre le relevé.
  Les chantiers mémoire (arène — poste dominant —, hachage 128 bits, blocs pour `noeuds`/file)
  redeviennent le facteur limitant dès qu'un run de ce niveau va au bout de ses forces.

**❌ LE 30 — la greffe y est STRICTEMENT INERTE, et la prédiction le disait.** Sa contrainte porte sur
le **rang 17**, l'avant-dernier, et le solveur y plafonne à **2/18**. Il n'approche jamais du moment
où elle mordrait. Vérifié binaire contre binaire (worktree sur `HEAD`) : **traces `[ordre]`
identiques**, 7 lignes de part et d'autre. ⚠️ Les comptes d'états (3,86 M contre 3,82 M) ne se
comparent PAS — les deux runs sont tués au TEMPS, donc c'est la trace qui tranche, pas le compteur.
**La porte reste donc un cas unique, celui du 16, où elle n'a rien prouvé.**

**❌ ET LE RACCORD DES DEUX MOITIÉS EST RÉFUTÉ LE JOUR MÊME.** Constat de l'utilisateur en annotant le
chemin du record 11/15 : *« pourquoi la caisse est poussée en (9,11) maintenant, ce n'est pas une case
morte ? »*. Réponse : **dans `ordre-dyn`, ni la loi ni le gel ne sont câblés** — les deux moitiés du
jour vivaient dans deux régimes disjoints, et aucun run ne les avait jamais eues ensemble. D'où un
régime combiné, `ordre-loi`. Canari :

| niv | `ordre-dyn` seul | **+ loi** |
|---|---|---|
| 0, 1, 3 | — | identiques |
| **5, 6, 17** | résolus | **`AUCUNE`** |
| 2 | 131 poussées | **141** |
| 7 | 88 poussées | **92** |

**Chacune des deux moitiés prise SEULE est saine** — la loi seule ne perd que le 6 (raison
documentée plus haut), l'ordre dynamique seul résout les huit. C'est leur COMBINAISON qui casse.

> **La cause, mesurée** : sur le 17, **233 prunes dont 2 seulement de gel** ; sur le 5, 6 565 dont 9.
> C'est la **loi** qui coupe, pas le gel. Et c'est cohérent après coup : ses cases mortes sont
> indexées par le but ACTIF, or elle n'a jamais été validée que contre l'ordre **STATIQUE** — le
> gabarit du 16 a été dessiné avec ces rangs-là, `juge_loi.py` a jugé avec eux. L'ordre dynamique
> rechoisit depuis l'état courant et peut revenir en arrière : une case légitimement utilisée comme
> garage devient morte dès que le but actif change. **Le « 0 faux positif » de la loi ne se
> transporte pas à un ordre qui bouge.**

⚠️ **Leçon générale, et elle vaut au-delà de ce raccord** : une règle validée l'est *contre le régime
qui a servi à la valider*. Composer deux essais sains n'est pas sûr — il faut repasser le canari, et
ici il a suffi d'une passe de huit niveaux pour tuer l'idée. Régime conservé dans `solveur.h`, marqué
RÉFUTÉ, pour qu'il ne soit pas reproposé.

**Reste ouvert :**
- [ ] **La contrainte n'existe que dans `ordre-dyn`**, régime qui coûte plus cher partout où il a été
  mesuré (§6.2, 2026-07-31 : ×87 sur le 6, ×9 sur le 7). Si on veut la porte AILLEURS, il faut un
  `butActif()` capable de passer un but SANS l'ordre dynamique complet — donc un régime de plus, et
  cette fois en repassant le canari (cf. le raccord ci-dessus).
- [ ] **Re-valider la loi contre un ordre qui BOUGE**, si on tient au raccord : redessiner un gabarit
  avec l'ordre dynamique, ou juger la loi sur les états réellement traversés. Non fait, et pas
  évident — l'ordre dynamique n'a pas de « rangs » stables à dessiner.

#### 🎯 Session du 2026-08-04 (fin) — LE PLAN HUMAIN MESURÉ : ce qui sépare les résolus, et pourquoi le planificateur naïf est mort

**Constat de l'utilisateur, qui ouvre la session** : *« essayer de trouver des règles à partir de
différents cas marginaux, ça ne me paraît pas jouable […] le démêlage, le goal ordering, les portes,
les macros, tous ces concepts sont très bons, mais je pense qu'il faut les assembler différemment. »*
Les mesures qui suivent lui donnent raison sur le fond, et **corrigent deux fois le cadrage que je
proposais** — c'est le fait marquant de la session.

**Le corpus le permet sans écrire une ligne de solveur** : 25 parties humaines GAGNANTES rejouées
depuis les journaux hybrides (`mesures/taches.py`, `mesures/garage.py`, rapatriés du scratchpad — la
leçon de `juge_loi.py`, perdu le matin même). ⚠️ **Miroir validé avant lecture, sur quatre chiffres
déjà écrits ailleurs dans ce document** : le 4 rend **415 poussées / 57 choisies** (exact), le 17
rend **213** (l'optimum), le 9 rend **237** (le compte du solveur), le 16 rend 212 pour 213 états.

**LE DÉCOUPAGE EN TÂCHES.** Une tâche = une invocation de goal macro (une livraison, marquée
`[macro] LANCEE`/`TERMINEE` dans le journal), ou une suite maximale de poussées CHOISIES sur la même
caisse (une manœuvre).

| | tâches | manœuvres | **reprises** |
|---|---|---|---|
| **12 résolus** | 20 | 9,5 | **1,5** |
| **13 non résolus** | 39 | 28 | **8** |

Une **reprise** = une caisse manœuvrée dans **deux tâches séparées ou plus** : garée, puis reprise.

- ❌ **CORRECTION — j'avais avancé « moins de dix décisions par partie »**, tiré des 96 annotations
  d'intentions sur 11 niveaux, et j'en faisais l'argument porteur. **Faux d'un facteur trois** : le
  découpage mécanique donne une médiane de **30 tâches**. Les intentions comptaient des PLANS, dont
  chacun regroupe plusieurs manœuvres — **ce n'est pas la même unité.** Ce qui survit : 30 tâches
  contre 10⁸ états, l'écart de représentation reste de six ordres de grandeur.
- 🎯 **LE RÉSULTAT : les REPRISES séparent proprement, et aux deux bouts.** `reprises ≤ 2` → 8 niveaux,
  **tous résolus** ; `reprises ≥ 8` → 8 niveaux, **tous non résolus** ; entre les deux une bande mêlée
  (le 6 est résolu avec 4, le 12 ne l'est pas avec 3). **Ce qui sépare ce qu'on finit de ce qu'on ne
  finit pas n'est ni la taille ni le nombre de caisses : c'est le nombre de fois qu'une caisse doit
  être GARÉE puis REPRISE.**
- **Et c'est exactement l'opération que rien ne représente**, ce que quatre observations indépendantes
  disaient déjà sans qu'on les rapproche : le §4 a réfuté le découpage « une caisse à la fois »
  *précisément* parce qu'il interdit le parking temporaire ; la variante sèche de R1 a été tuée huit
  fois par les transits ; la macro ne connaît qu'une destination, le but actif ; et `GARER` était la
  catégorie fourre-tout du corpus d'intentions, tombée de 10 à 1 dès que l'annotateur a vu la suite.

**❌ OÙ GARE-T-ON ? LA MESURE QUI DEVAIT DÉCIDER DE L'ARCHITECTURE — et elle est NÉGATIVE.**
L'hypothèse à tester : si les destinations de garage se concentrent sur quelques cases, une recherche
au niveau des TÂCHES (`manœuvre(caisse, destination)`) a un branchement praticable ; sinon on a
déplacé le mur PSPACE d'un étage sans le réduire.

| | |
|---|---|
| garages relevés | **398** sur **297 cases distinctes** (1,34 par case) |
| murs voisins, cases de garage | **0,74** |
| murs voisins, toutes cases libres | **1,18** |
| garage sur une case-but | **0 %** partout, sauf 13 (33 %), 16 (29 %), 26 (16 %) |

**Aucune concentration, et on gare en espace OUVERT — l'inverse de l'intuition « une niche contre un
mur, hors des artères ».** Le planificateur naïf est donc mort : une tâche `manœuvre(caisse,
destination)` a un branchement de l'ordre du nombre de cases libres, et rien dans les données ne
donne le vocabulaire qui le réduirait.

⚠️ **Réserve de méthode, à ne pas cacher** : la « destination » mesurée est la case où la caisse
s'arrête quand le joueur passe à autre chose. On mesure peut-être *où la caisse était quand
l'attention a changé*, pas un choix délibéré — c'est la leçon du 2026-08-02, *« c'est toujours
l'unité de mesure »*. Le chiffre des murs voisins, lui, ne dépend pas de ce découpage.

**🎯 LA LECTURE QUI SURVIT, et elle est confirmée par celui qui joue :**

> **On ne choisit pas OÙ poser la caisse ; on la sort de là où elle gêne, et elle s'arrête où c'est
> commode.** — formulation retenue par l'utilisateur : *« c'est tout à fait ça »*.

Si les destinations s'éparpillent ET qu'on gare en espace ouvert, c'est que **la destination est
incidente**. Le paramètre d'une manœuvre n'est pas *où*, c'est **pourquoi** : *dégager cette case*,
*ouvrir ce passage*. Et ce paramètre-là est petit — c'est le vocabulaire `E`/`O` que la campagne
d'intentions a stabilisé (20 étiquettes sur 23 à `posees 0`). La destination se **calcule** alors,
au lieu d'être cherchée : le branchement passe de « cent cases » à « quelle gêne lever ».

⚠️ Ce n'est pas gratuit pour autant : une intention n'est utile que si elle se transforme en
GÉNÉRATION de coups. En score, elle retombe dans le guidage, fermé depuis le 2026-07-21. C'est le mur
que la campagne d'intentions n'a jamais franchi — *« ce que le corpus ne dit toujours pas, c'est à
quoi il sert »*.

**Trois exceptions qui contredisent l'éparpillement**, et ce sont trois non-résolus : sur le **25**,
une seule case absorbe **17 garages sur 28** ; sur le **20**, 11 sur 19 ; sur le **16**, 10 sur 42.
Là il y a bien une zone de dépôt au sens littéral — et c'est exactement le « stocker trois caisses
dans la zone d'embut » que l'utilisateur a énoncé pour le 16.

**Reste ouvert :**
- [ ] **Refaire la mesure des destinations à une AUTRE maille** — par exemple la case où la caisse
  reste le plus longtemps, plutôt que celle où la manœuvre s'arrête. La réserve ci-dessus dit que le
  résultat négatif pourrait être un artefact de découpage, et c'est bon marché à retester.
- [ ] **Les trois niveaux à zone de dépôt (16, 20, 25)** : y a-t-il une géométrie commune ? Ce serait
  le premier motif de « garage » caractérisable, et il porterait le stock du 16.
- [ ] ⚠️ **Ne pas relire les REPRISES comme un prédicteur** : elles se mesurent sur une partie humaine
  gagnante, donc a posteriori. Elles disent CE QUI est dur, pas qu'un plateau donné le sera.

#### 📖 Session du 2026-08-04 (lecture) — LES GADGETS DE CULBERSON, et le vocabulaire qui manquait au verrou

**Source** : Jonathan Laurent, *Complexité du jeu de Sokoban* (TIPE ENS, 18 juin 2012) — une
démonstration originale du résultat de Culberson (1997), lu par l'utilisateur puis discuté et
reconnecté au chantier du jour. Fichier local : `~/Documents/sokoban.pdf`.

**Le principe de la preuve.** Sokoban est PSPACE-complet parce qu'on peut émuler N'IMPORTE QUEL
automate linéairement borné (une machine de Turing à ruban fini) par un niveau : le niveau est soluble
ssi la machine accepte. La construction s'appuie sur des **gadgets** — des assemblages de cellules à
bornes d'accès, dont le comportement se caractérise entièrement par une fonction de transition
`f_G : États × Bornes → 𝒫(États × Bornes)` (un ensemble puissance : un gadget peut offrir un CHOIX,
pas seulement une trajectoire forcée). Le ressort qui force la simulation à être fidèle est la
**configuration irrécupérable** : le niveau est bâti pour qu'une solution puisse toujours défaire ses
propres poussées, donc toute poussée non réversible condamne la partie — c'est le même objet que nos
« deadlocks », vu depuis l'autre bout.

**LES QUATRE GADGETS, avec leur fonction de transition (extraite au format, `pdftotext -layout`
après `brew install poppler` — la première extraction, sans rendu propre, mélangeait les colonnes) :**

```
              a    b              a    b    r                a    a'   l   l'   r
         0    b0   -         0    -    -    r1        0      -    -    l'0  -   r1
         1    -    a0        1    b0   -    r1        1     {a'0,a'1} -  l'0  -  r1

        Diode          Inverseur         Transistor              Verrou
```

- **Diode** — UN seul état. `a→b` (reste en 0), `b→-` (bloqué). Sens unique pur, rien à retenir.
- **Inverseur** — deux états, alternance stricte. `0: a→b(→1), b→-` puis `1: a→-, b→a(→0)`.
- **Transistor** — deux états. `0: a→-, b→-, r→r(→1)` puis `1: a→b(→0), b→-, r→r(→1)`. Verrouillé,
  rien ne passe sauf visiter R (déverrouille). Déverrouillé, UN passage A→B consomme le déverrouillage
  et referme.
- **Verrou** — celui qui a résisté à la première lecture. La ligne `{a'0, a'1}` porte tout : en état
  1 (déverrouillé), passer par `a→a'` a **deux issues possibles**, écrites dans la fonction de
  transition elle-même — rester déverrouillé OU se reverrouiller. C'est le joueur qui choisit.
  `l→l'` vaut `l'0` sur les DEUX lignes : franchir L→L′ **verrouille toujours**, quel que soit l'état
  de départ — un second mécanisme, indépendant du couple A/A′, dédié à la remise à zéro.

**LE LIEN DIRECT AVEC LE CHANTIER DU JOUR :**

- **(10,6) du niveau 16 EST un transistor**, formalisé : état 0 = verrouillé (aucune sortie utile,
  cf. §6.2 « précédence caisse → but »), R = descendre le couloir droit jusqu'en (11,6), état 1 = un
  seul passage A→B possible ensuite (la caisse quitte définitivement (10,6)). `porteBloquee()` en est
  une version dégénérée : un booléen figé au chargement, là où le gadget est une fonction d'un état
  qui ÉVOLUE.
- **La rangée 11 du 16 est une diode dégénérée** : passage permis dans un seul sens (on y entre),
  aucun état à stocker (on n'en ressort jamais, il n'y a pas de second usage à préserver). C'est
  cohérent avec ce que la loi de l'ordre dit déjà de cette rangée pour le but actif (12,7).
- **Le VERROU est le vocabulaire qui manque au projet.** Tous nos détecteurs — corral unitaire,
  pince, paquet non livrable, gel hors tour, loi de l'ordre — sont des transistors ou des diodes :
  des booléens figés, jamais un composant à ÉTAT INTERNE qu'on peut re-basculer à volonté. La salle
  d'embut du 16 (« stocker trois caisses », §6.2) a la forme d'un verrou à PLUSIEURS crans plutôt
  qu'à deux états — un compteur borné, pas un binaire.

**CE QUE ÇA ÉCLAIRE DANS LES RÉSULTATS DU JOUR, sans rien changer à ce qui est déjà écrit :**

- **Pourquoi il n'existe pas de liste finie de règles locales** (le constat de l'utilisateur qui a
  ouvert la session du soir, « essayer de trouver des règles à partir de cas marginaux, ça ne me
  paraît pas jouable ») a un théorème derrière lui : la PSPACE-complétude EST l'énoncé qu'aucune
  collection finie de détecteurs locaux ne peut capturer le problème en général. Le §3 le disait déjà
  pour `h` (« toute borne qui capturerait le mou devrait résoudre un ordonnancement optimal ») ; c'est
  la même chose, dite au niveau des règles plutôt que de la fonction heuristique.
- **Le 192 plus DUR que le 190** en retirant des murs (§6.2, 2026-07-20) cesse d'être une surprise :
  Hearn–Demaine (2005, reformulation de Culberson via NCL) montrent que le résultat tient MÊME SANS
  AUCUN MUR INTÉRIEUR — la dureté ne vient donc pas de la géométrie des murs, mais de la RÉVERSIBILITÉ
  disponible dans l'espace ouvert.
- **Les REPRISES** (mesure du soir même, 1,5 sur les résolus contre 8 sur les non-résolus) sont
  exactement le pendant humain de la configuration irrécupérable : un niveau facile est un niveau où
  l'humain peut se permettre BEAUCOUP de garer-reprendre (verrous), un niveau dur est un niveau où
  chaque manœuvre est proche de l'irréversible (diodes, transistors qui ne se referment pas).

**⚠️ Ce que ça ne dit PAS.** La PSPACE-complétude est une borne PIRE CAS sur des instances
ADVERSARIALES — les niveaux de Culberson sont construits pour encoder une machine, les nôtres sont
dessinés par des humains pour être jouables. Rien n'interdit de résoudre le 16 ; ça interdit une
méthode générale et efficace, pas celle-ci en particulier.

**Reste ouvert :**
- [ ] **Décomposer 2 ou 3 niveaux à la main en gadgets** (transistor / diode / verrou), et compter la
  taille de l'espace d'états ABSTRAIT qui en résulte. C'est le test le moins cher pour juger si une
  recherche au niveau du « plan de tâches » (§ session du soir) est praticable : si le 16 fait 3-4
  composants à une poignée d'états chacun, la piste est réelle ; si ça part à vingt, on a déplacé le
  mur PSPACE d'un étage sans le réduire.
- [ ] Lire la construction du pont plan (§4.3 du document, croisement de deux couloirs) — non fait,
  hors sujet immédiat mais c'est la pièce qui manque pour route un circuit complexe sur un plateau 2D.

#### 🎯 Session du 2026-08-05 — LE 16 DÉCOMPOSÉ EN GADGETS (partiel) : deux transistors vérifiés, le stock encore un trou

**Reprise du premier item ouvert de la session lecture** (« décomposer 2-3 niveaux à la main en gadgets,
compter l'espace d'états ABSTRAIT qui en résulte »). Fait à la main sur le **16** (le plus avancé),
en recoupant trois sources déjà là : la géométrie brute (murs seuls du `.xsb`, script jetable), le
journal humain `hybride_niveau_0016.txt` (209 Ko, deux parties, 2026-08-03 et 2026-08-04), et les
résultats déjà mesurés le 2026-08-04 (`mesures/porte`, bisection p44/p45 du §6.2). **Aucune ligne de
solveur touchée** — c'est de la lecture de plateau et de journal, pas du code.

**Mesure 0, la plus utile : la constriction initiale.** Sur 81 cases de sol, **7 seulement sont
atteignables sans pousser une seule caisse** (flood-fill trivial, boîtes traitées comme obstacles).
Et sur le graphe de sol PUR (boîtes ignorées, murs seuls), **un seul point d'articulation existe dans
tout le niveau**. Autrement dit : ce plateau n'a presque aucun goulot de MUR — tous ses vrais goulots
sont des caisses assises sur un couloir à une case de large, sans détour possible. C'est une
illustration directe, chiffrée, du point Hearn–Demaine relevé dans la session lecture (« la dureté ne
vient pas de la géométrie des murs, mais de la réversibilité disponible dans l'espace ouvert ») — sur
CE niveau précis, pas sur l'énoncé général de la preuve.

**Gadget 1 — TRANSISTOR (10,6), déjà prouvé le 2026-08-04, maintenant relu à la lettre dans le
journal.** `mesures/porte` avait établi la contrainte caisse→but ; le journal confirme le mécanisme
géométrique exact : la SEULE case d'appui pour pousser C vers l'ouest est (11,6), et la SEULE façon
d'atteindre (11,6) est de monter la colonne VIDE (12,10)→(12,9)→(12,8)→(12,7)→(11,7)→(11,6) — donc de
passer PAR le but même qu'on n'a pas encore le droit de remplir. Le journal montre cette route au
mot près, à `posees 3/15` : `joueur (12,10)->(12,9)`, …, `(11,7)->(11,6)`, puis
`(11,6)->(10,6) POUSSE caisse ->(9,6)`. **Deux états abstraits** : {C bloque / C dégagée}. Une fois
dégagée, le gadget est consommé — pas de troisième état, pas de retour en arrière possible (rejouer
depuis là ne repropose jamais la question).

**Gadget 2 — TRANSISTOR (3,4)/(4,4), non documenté avant aujourd'hui.** Même forme que le gadget 1,
trouvé par la même méthode (constriction initiale) : la salle du haut (lignes 0-4, caisses
(2,3)(3,4)(4,4)(6,4)(6,5)(10,4)) est inatteignable depuis le départ (3,5) sans pousser (3,4) ou (4,4)
au nord au préalable — ce sont exactement 2 des 3 caisses candidates parmi les 7 cases de départ.
Confirmé par l'ordre réel : **la toute première poussée du journal** est exactement `joueur
(3,5)->(3,4) POUSSE caisse ->(3,3)`, et la caisse (4,4) est poussée à son tour quelques coups plus
tard (`(4,5)->(4,4) POUSSE caisse ->(4,3)`), dans la même phase d'ouverture avant toute autre action.
**Deux états abstraits**, même famille que le gadget 1 : binaire, jamais reverrouillé.

**Élément 3 — DIODE, rangée 11 (confirmé, pas nouveau).** Les 5 buts de rang 6, 7, 8, 9, 11
((12,11)(11,11)(10,11)(9,11)(8,11)) : descendre y est permis, remonter non — la rangée 12 est un mur
plein sur toute la largeur, donc aucun appui pour repousser une caisse vers le nord depuis la rangée
11. Pas un composant réutilisable au sens du gadget : un compteur monotone 0→5, déjà énoncé par la
loi de l'ordre du 2026-08-03/04. Mentionné ici pour mémoire, pas remesuré.

**Élément 4 — VERROU présumé sur (3,7)/(3,6), rôle NON tranché.** Le journal confirme que cette
caisse est bien manipulée plusieurs fois (poussée au nord vers (3,6), puis reposée au sud) — ce que
le §6.2 du 2026-08-04 disait déjà. Mais la géométrie brute montre un CONTOURNEMENT : la colonne 2
((2,6)-(2,7)-(2,8)) atteint (3,8) sans jamais toucher (3,7) — donc ce n'est PAS un verrou d'ACCÈS pur
comme les gadgets 1 et 2, la case n'est jamais strictement obligatoire pour la connexité. Hypothèse
non vérifiée : un verrou sur le TEMPS plutôt que l'espace (la caisse doit être hors d'un chemin
précis À UN INSTANT donné du plan, pas hors d'un chemin en général). **Laissé ouvert plutôt que forcé
dans un moule** — mieux vaut un gadget non classé qu'un mal classé.

**Élément 5 — le « stock » (11,10)(9,10)(9,9), TOUJOURS UN TROU.** Réel : `stock16.xsb` (position
jouée) capture exactement ces 3 buts remplis alors que le gadget 1 n'est PAS ENCORE franchi (la
caisse (10,6) y est toujours présente). Mais **aucun modèle d'états qui tienne** n'en est sorti
aujourd'hui : le journal montre au moins DEUX stratégies différentes pour amener une caisse en tête
de colonne — une route directe `(9,10)→(10,10)→(11,10)→(12,10)→…` vue dans un essai à `posees 2/15`
via `[macro] LANCEE`, et le détour par le gadget 1 vu dans la partie qui a fini par gagner. Un
automate écrit à partir de ça aujourd'hui serait de la fiction. Et c'est très exactement la pièce que
le 2026-08-04 avait déjà signalée comme le vrai mur : *« le stock N'EST PAS le verrou … la difficulté
est en aval du stock »*.

**Verdict provisoire (dépassé plus bas dans la même session) :** les DEUX gadgets semblaient totaliser
**2 × 2 = 4 états abstraits** — une poignée. Le dépouillement mécanique qui suit corrige ce chiffre.

---

**SUITE, LE JOUR MÊME — dépouillement mécanique du journal, et DEUX CORRECTIONS.** Premier item ouvert
ci-dessus, fait dans la foulée : script jetable (`scratchpad/gates16.py`) qui rejoue TOUS les `[mouv]`
de la session 1 du journal (**964 coups, 2026-08-03** — c'est bien celle que l'en-tête des intentions
citait), gère les `[undo]` par une pile d'opérations (annule exactement le dernier pas, poussée ou
marche), et recalcule la taille de la région atteignable par flood-fill **après chaque poussée**
(les marches seules ne peuvent pas la changer). Validé par construction : chaque poussée vérifie que
la caisse existe bien à la case attendue avant de la déplacer (assertion, pas de correction silencieuse).

**Résultat : 124 poussées sur 964 changent la taille de la région atteignable** — la session
s'arrête à `posees 9/15`, soit une progression aux deux tiers de la partie. Deux corrections aux
gadgets écrits plus haut, IMPORTANTES :

1. **❌ Le gadget 2 n'est PAS un transistor à 2 états, jamais reverrouillé — c'est un CONVOYEUR
   PARTAGÉ.** Le couloir (2,4)-(3,4)-(4,4)-(5,4)-(6,4)-(7,4) s'ouvre et se ferme **au moins cinq fois**
   dans les 964 coups (pas `571-574`, `651-654`, `727-730`, `794-796`, `832`), avec des amplitudes de
   +48 à +59 cases à chaque fois. Ce n'est pas UNE caisse qu'on pousse une fois pour de bon : c'est un
   couloir à une case de large que **CHAQUE caisse de la salle du haut doit traverser à son tour** pour
   rejoindre la salle des buts — la salle du haut contient 6 caisses ((2,3)(3,4)(4,4)(6,4)(6,5)(10,4)),
   et le couloir est retraversé une fois par livraison. Un vrai transistor se consomme ; celui-ci se
   RECHARGE à chaque caisse suivante. Requalifié : **CONVOYEUR À UNE VOIE, un état par caisse encore à
   faire passer** (6 états, pas 2).
2. **✅ Le gadget 4 — (3,7)/(3,6) — EST un verrou réel, confirmé plutôt qu'hypothétique.** Basculé
   **dix fois** dans les 964 coups (`46,194,231,236,379,384,433,438,485,490,529,534,605,610,685,690,
   759,764,825,830`), amplitude stable (~25-29 cases) à chaque bascule — signature d'un vrai verrou
   binaire réutilisé, pas d'un artefact de style de jeu. Le contournement par la colonne 2 que
   j'avais relevé (accès à (3,8) sans toucher (3,7)) n'empêche donc pas cette caisse d'être
   fonctionnellement un verrou : elle sert à autre chose que la connexité de (3,8), très probablement
   au même rôle que le gadget 2 — laisser passer, une par une, les caisses qui descendent de la salle
   du haut vers (8,6)/(9,6).
3. **⚠️ Le gadget 1 — (10,6) — n'est pas « consommé pour de bon » comme écrit plus haut.** La caisse
   *initiale* de (10,6) EST bien dégagée une fois pour toutes au pas 78 (`+27`, `10,6→9,6`) — ce que
   `mesures/porte` avait prouvé reste exact. Mais **la CASE (10,6) est réoccupée par une AUTRE caisse
   dès le pas 221** (`10,5→10,6`, `−38`) puis re-dégagée au pas 256 (`+27`) : elle sert de RELAIS de
   passage à répétition pour le trafic du couloir (8,6)/(9,6), au même titre que les gadgets 2 et 4.
   La contrainte prouvée par `porte` (cette caisse-LÀ, avant CE but-LÀ) reste vraie ; l'image du
   « transistor isolé, 2 états, fini » qui l'accompagnait ne l'est pas.
4. **Troisième corridor, non repéré à la première lecture** : (8,8)-(8,9)-(9,9), utilisé comme
   parking juste à l'entrée de la salle des buts, bascule **au moins 17 fois** avec des amplitudes de
   ±7 à ±45 — plus petit que les deux premiers mais avec la même signature répétitive.

**LA VRAIE FORME DU NIVEAU 16, donc, n'est pas « 3-4 transistors indépendants ».** C'est **UNE seule
voie physique** (la chaîne (2,4)…(7,4) → (8,6)/(9,6) → (10,6) → (8,8)/(8,9)/(9,9) → salle des buts)
que **chacune des 15 caisses doit franchir séquentiellement**, une à la fois — le mou n'est pas dans
le CHOIX d'un chemin (il n'y en a qu'un), il est dans **l'ORDRE et l'ENTRELACEMENT** des passages.
C'est cohérent avec la mesure 0 (un seul point d'articulation de mur) : le niveau n'a pas plusieurs
portes indépendantes, il a UN goulot que le trafic complet doit remonter, encore et encore.

⚠️ **Ce que ça change pour le compte d'états abstrait** : ce n'est plus un produit de quelques
automates à 2 états chacun (4, 8, 16…) — c'est plus proche d'un problème d'ORDONNANCEMENT sur une
ressource unique (la voie), avec 15 tâches en compétition pour l'emprunter. La taille abstraite
plausible n'est donc pas 2ⁿ mais plutôt de l'ordre du nombre de **permutations partielles** compatibles
avec la loi de l'ordre déjà connue — nettement plus que 4, mais potentiellement bien en-dessous des
10⁸ états bruts si la voie unique réduit chaque décision à « qui passe ensuite », un choix parmi les
caisses encore à livrer plutôt qu'un choix de destination.

**Reste ouvert :**
- [ ] **Corriger le vocabulaire** dans une relecture future de cette session (les gadgets 1/2/4 ne
  sont pas trois transistors isolés : c'est UN convoyeur à trois relais). Laissé tel quel ici — c'est
  la trace de la correction elle-même qui a de la valeur, pas une réécriture propre.
- [ ] **Modéliser le convoyeur comme un problème d'ORDONNANCEMENT à une ressource** (15 tâches, 1
  ressource partagée, précédences partielles déjà connues via la loi de l'ordre) plutôt que comme un
  produit de gadgets indépendants — c'est la vraie forme qui ressort du dépouillement.
- [ ] **L'élément 5 (stock) reste non modélisé** — mais il faut maintenant le relire à la lumière du
  convoyeur : peut-être n'est-ce pas un composant séparé, plutôt le point où le convoyeur alimente la
  colonne du gadget 1 pendant que d'autres livraisons continuent d'y transiter.
- [ ] Refaire la mesure 0 ET le dépouillement mécanique sur un DEUXIÈME niveau (14 ou 15, déjà gagnés
  à la main le 2026-08-03) pour savoir si « une seule voie, tout le trafic dessus » est une propriété
  du 16 ou du générateur de niveaux en général — c'est maintenant la question qui compte, pas la
  constriction initiale seule.
- [x] Script `scratchpad/gates16.py` — généralisé et rejoué sur 14/15/17 le jour même, cf. ci-dessous.

---

**SUITE, LE JOUR MÊME — 14, 15 ET 17 REJOUÉS AU MÊME SCRIPT, SUR UN CONSTAT À L'ŒIL DE
L'UTILISATEUR.** Constat qui ouvre cette suite : *« le 14 et le 15 ont un accès d'acheminement
principal, et 1 ou 2 [routes] pour 1 ou 2 caisses. Le 17 n'en a qu'un seul. »* Script généralisé
(`scratchpad/gates_generic.py`, prend niveau/plateau/journal en paramètres), rejoué sur la session la
plus avancée de chaque niveau (14 : 1216 coups, max 17/18 ; 15 : 889 coups, max 14/15 ; 17 : 551
coups, max 5/6 — toutes des parties de la campagne du 2026-08-03). Puis regroupement des paires de
cases rejouées ≥1 fois en composantes connexes (union-find), pour distinguer un vrai COULOIR (une
chaîne de cases) d'une coïncidence de deux gates isolées à la même case.

| niveau | sol atteignable au départ | composantes « PRINCIPAL » (≥5 traversées) | « secondaire » (3-4) | « mineur » (1-2, une route par caisse) |
|---|---|---|---|---|
| **17** (résolu) | 5/87 (6 %) | **2** (23 et 8 cases, jusqu'à ×11 et ×6) | 0 | 1 |
| **16** (non résolu) | 7/81 (9 %) | **2** (14 et 14 cases, jusqu'à ×20 et ×10) | 1 | 2 |
| **15** (non résolu) | 12/104 (12 %) | 0 | **1** (26 cases, jusqu'à ×4) | 7 |
| **14** (non résolu) | 53/121 (44 %) | 0 | **1** (6 cases, jusqu'à ×3) | 6 |

**LE CONSTAT À L'ŒIL TIENT, MESURÉ.** 17 et 16 sont dominés par un petit nombre de COULOIRS lourdement
réutilisés (jusqu'à ×20) et presque aucune route à usage unique ; 14 et 15 sont l'inverse — une seule
route un peu plus fréquentée que les autres (×3 à ×4, pas plus), et six à sept routes dédiées à une
ou deux caisses chacune. La bascule est nette, pas un dégradé bruité : aucun niveau n'est entre les
deux.

⚠️ **Mais ça ne sépare PAS résolu de non-résolu.** Le 17 (résolu) et le 16 (non résolu) sont dans la
MÊME catégorie structurelle (convoyeur dominant) ; le 14 et le 15 (tous deux non résolus) sont dans
l'autre. Ce qui sépare le 17 des trois autres, c'est sa taille — **6 caisses contre 15 à 18** — pas la
forme de son réseau de couloirs. **La structure du réseau et la difficulté sont deux axes
différents.**

**CE QUE ÇA DIT POUR LA QUESTION DE LA SESSION PRÉCÉDENTE** (« le modèle d'ordonnancement à ressource
partagée généralise-t-il ? ») : **partiellement, et pas uniformément.** Sur 16 et 17, oui — la quasi-
totalité du trafic passe par deux couloirs, un modèle « qui emprunte le couloir, dans quel ordre »
capture l'essentiel. Sur 14 et 15, non — la majorité des cases-gates ne sont traversées qu'une ou
deux fois : ce sont des portes ponctuelles (plus proches du gadget « transistor à 2 états, consommé
une fois » de la toute première lecture) plutôt qu'une ressource partagée à ordonnancer. **Les deux
familles de gadgets coexistent dans le même niveau 33 pièces** — le bon modèle n'est pas UN choix
entre convoyeur et transistor, c'est de reconnaître LEQUEL s'applique à quelle case, plateau par
plateau.

**Reste ouvert :**
- [ ] **Vérifier si la catégorie (convoyeur-dominant / transistors-dominants) est stable entre deux
  parties humaines gagnantes DIFFÉRENTES du même niveau** — ici chaque niveau n'a qu'une seule partie
  dépouillée. Si un second joueur du 16 route autrement et n'a presque aucun couloir réutilisé, la
  catégorie mesure le STYLE de jeu, pas la géométrie du plateau (même piège que le §6.2, 2026-08-03,
  sur les deux ordres valides du niveau 6).
- [ ] **Chercher ce qui, dans le PLATEAU SEUL (murs, sans jouer), prédit la catégorie** — la
  constriction initiale (mesure 0) ne suffit pas : 15 et 17 sont tous deux très constreints au départ
  (12 % et 6 %) mais dans des catégories opposées. Un candidat pas testé : le nombre de points
  d'articulation de mur (mesure 0 du 2026-08-05) rapporté au nombre de caisses.
- [ ] `scratchpad/gates_generic.py` — même sort que `gates16.py`, à rapatrier dans `mesures/` si
  cette lecture reprend sur d'autres niveaux, sinon perdu avec le scratchpad.

---

#### ⏸️ Session du 2026-08-05 (fin) — LE FIL GADGETS REFERMÉ, faute de pouvoir séparer

**Verdict de l'utilisateur sur les trois sessions du jour, et il est juste : « ça ne fait pas avancer
le schmilblick ».** Bilan honnête : le vocabulaire gadget (transistor/diode/verrou/convoyeur) est
réel et mesuré, mais **il ne sépare pas résolu de non-résolu** (17 et 16 dans la même famille
structurelle malgré des issues opposées) et **aucun chiffre d'espace d'états abstrait n'en est
jamais sorti** — la question posée par la session lecture du 2026-08-04 (« 3-4 composants ou vingt »)
reste sans réponse, et la piste s'arrête là faute de mieux. Fermé, pas effacé — comme le reste de ce
document quand une piste ne paie pas.

**REPRISE DU FIL QUI, LUI, SÉPARAIT : les REPRISES (§ session du 2026-08-04 fin, plus haut).** Rappel
du résultat qui tient toujours, non contredit depuis : `reprises ≤ 2` → 8 niveaux, **tous résolus** ;
`reprises ≥ 8` → 8 niveaux, **tous non résolus**. Une reprise = une caisse manœuvrée dans deux tâches
séparées ou plus (garée, puis reprise plus tard). C'est le seul signal du projet à ce jour qui
sépare proprement aux deux bouts sur un découpage mécanique (`mesures/taches.py`), pas une lecture à
l'œil.

**LA QUESTION À REPRENDRE, telle que laissée ouverte le 2026-08-04** : *pourquoi une caisse est-elle
reprise plutôt que livrée directement, et est-ce que ça se prédit AVANT de jouer* (depuis le plateau
seul, ou tôt dans une recherche) plutôt qu'a posteriori sur une partie gagnante. Deux réserves déjà
posées à ne pas oublier en repartant :
- ⚠️ **Les reprises se mesurent a posteriori** (§2026-08-04 fin) — sur une partie humaine gagnante,
  donc après coup. Elles disent CE QUI est dur, pas qu'un plateau donné le sera : ne pas les relire
  comme un prédicteur sans l'avoir vérifié.
- ⚠️ **La mesure des destinations de garage était NÉGATIVE** (même session) : pas de concentration
  sur quelques cases, on gare en espace ouvert. Donc la reprise n'est pas un problème de « où » —
  c'est un problème de « pourquoi » (dégager, ouvrir un passage), déjà rapproché du vocabulaire
  `E`/`O`/`G`/`A`/`T`/`R` de la campagne d'intentions.

**Reste ouvert, hérité tel quel de la session du 2026-08-04 (fin), à attaquer en premier :**
- [ ] Refaire la mesure des destinations à une AUTRE maille (case où la caisse reste le plus
  longtemps, plutôt que celle où la manœuvre s'arrête) — pas cher, pourrait changer le verdict négatif.
- [ ] Les trois niveaux à zone de dépôt repérée (16, 20, 25) : géométrie commune ?
- [ ] **Nouveau, posé aujourd'hui** : une reprise est-elle détectable AU MOMENT où elle a lieu (le
  coup qui gare une caisse "trop tôt") plutôt qu'après coup sur toute la partie ? Si oui, c'est un
  signal exploitable en cours de recherche, pas seulement un diagnostic rétrospectif.

#### 🎉 Session du 2026-08-06 — LE 27 TOMBE PAR L'ORDRE, et « trop tôt » est un problème de BUT, pas de CAISSE

**Point de départ, une question de l'utilisateur en lisant le premier cours** : *« le couplage
hongrois et le goal-ordering n'entreraient-ils pas en conflit ? »* La réponse tenait en trois
étages (les deux premiers déjà au plan, le troisième non) et a ouvert toute la journée.

---

**❌ LE TIE-BREAK ALIGNÉ SUR `ordreButs` — CODÉ, MESURÉ, RÉFUTÉ, RESTAURÉ.**

Constat de départ, vérifié au code : le score de départage de `getHeuristique` (game.cpp:1049)
prend les buts **dans leur ordre d'INDEX** — c'est-à-dire l'ordre où ils apparaissent dans le
`.xsb`. Il y a donc **trois ordres de remplissage** dans le solveur, et ils ne se parlent pas :

| ordre | qui le lit | d'où il vient |
|---|---|---|
| `ordreButs` | la macro seule, via `butActif()` | précédence + contiguïté, prouvé |
| l'appariement hongrois | `h`, le tie-break, la macro en régime `coupl-` | coût minimal, sans notion d'ordre |
| **l'index des buts** | le tie-break lexicographique | **l'ordre de déclaration dans le fichier** |

Essai : émettre les chiffres du score dans l'ordre de `ordreButs` au lieu de l'index (6 lignes).
Pur tie-break, donc optimalité intacte par construction. Binaire contre binaire, même arbre, une
seule variable, déterminisme vérifié en double tirage des deux côtés.

| run | `f = C*` | ref | modifié | effet |
|---|---|---|---|---|
| **1 astar** | 99,9 % | 1 755 | **926** | **÷1,90** |
| **6 astar** | **99,6 %** | 502 634 | 503 515 | **PERTE 0,18 %** |
| **4 macro** | **100,0 %** | 55 560 | 55 560 | **identique à l'unité** |
| 190 macro | 85,5 % | 145 368 | 146 252 | perte 0,6 % |
| 6 macro | 50,9 % | 570 | 545 | −4,4 % |
| 2 macro | 19,7 % | 412 | 406 | −1,5 % |
| 17 astar / 17 macro / 2 astar / 9 macro | 5,4 / 2,9 / 0,2 / 0,0 % | — | — | ±0,2 % |

**Canari intact partout** (4/97/131/134/355/143/110/90/237/213, 190=220, 191=250).

> ⚠️ **ET ÇA RÉFUTE LE PRÉDICTEUR DU §3.** Le plan lit la masse `f = C*` comme *« le gain suit la
> masse `f = C*`, ligne pour ligne »*. **Trois runs à ≥ 99,6 % rendent ÷1,90, une perte, et zéro
> exactement.** `f = C*` est une condition **nécessaire** — un tie-break ne peut rien gagner
> ailleurs — mais elle **n'est pas suffisante** : encore faut-il que le comparateur ait des ex æquo
> à départager. Le 4 en macro n'en a quasiment pas (le régime d'engagement n'enfile qu'une poignée
> d'enfants par état) ; le 1 en A\* pur en a plein.

Confondant vérifié avant de conclure : sur le 4, `ordreButs` **n'est pas** la permutation identité
(carte des rangs `f210/g543/h876/iba9/jedc`), donc le score a réellement changé. Le « identique à
l'unité » n'est pas un no-op déguisé. **`game.cpp` restauré à l'identique** (vérifié par `diff`).
Une seule orientation testée (premier-à-remplir = chiffre le plus significatif) ; l'inverse n'a pas
de sens.

---

**🎉 LE 27 EST RÉSOLU AVEC L'ORDRE HUMAIN INJECTÉ — deuxième niveau dont l'ordre est le verrou.**

Partie gagnée à la main en hybride (365 poussées, 20/20, rejeu validé contre `taches.rejoue()`),
ordre de pose définitif extrait, injecté par fichier, `coupl-plongeon` sans budget :

| | ordre injecté | témoin (ordre calculé) |
|---|---|---|
| dépilements | **332 359** ✅ **RÉSOLU**, 363 poussées | 31 648 000 — arrêté à la main |
| états vus | 567 115 | **87 490 395** |
| max | 20/20 | 17/20, **aucun record battu depuis 2,1 M** |

- ⚠️ **PAS une ligne de [scores.md](scores.md)** : obtenu par injection, donc **non reproductible
  avec le binaire par défaut**. Même statut que le 12 depuis juillet. Ce qui est prouvé, c'est que
  l'ordre était le verrou ; pas que le solveur sache résoudre le 27.
- ⚠️ **363 poussées n'est pas un canari** (régime plongeon, cf. le 21 : ±18 poussées en coupant le corral).
- ⚠️ **Le témoin n'a AUCUN verdict** — 6ᵉ application de la règle (31, 13/r07, 11, 18, 16, 27).
  87,5 M états vus, c'est l'ordre de grandeur du run qui a résolu le 11.

**LE DÉFAUT DE L'ORDRE CALCULÉ, et le défaut de juillet n'est plus celui-là.** L'ordre du 27 met
désormais (6,2) au rang **19** et non plus 16 : le tri topologique du 2026-07-30 a corrigé le
murage diagnostiqué le 2026-07-29. **Le défaut restant est ailleurs :**

```
CALCULÉ : (6,4)(6,3) | (5,4)(5,3) | (4,4)(4,3) | … — des PAIRES DE COLONNES, rangées 3 et 4 entrelacées
HUMAIN  : (6,4)(5,4)(4,4)(1,4)(3,4)(2,4) | (1,3)(2,3)(3,3)(4,3) | … — rangée 4 EN ENTIER, puis rangée 3
```

51 paires en désaccord sur 190. Et le calcul pose **(6,3) au rang 1 et (5,3) au rang 3**, alors que
ces cases absorbent **19 transits** pendant les dix premières poses : il bouche le couloir d'entrée
à la deuxième pose. **Les trois tests de précédence sont muets** (0 violation locale, globale et
par paires) — boucher un couloir de transit ne viole aucune arête, puisqu'une arête parle de
*faisabilité*, pas de *trafic*.

> **CANDIDAT, pas une loi (§11.4) : sur le 12 comme sur le 27, le calcul ENTRELACE deux lignes
> parallèles de buts là où l'humain en termine une avant d'attaquer l'autre.** Sur le 12 c'est le
> sens de la colonne 15, sur le 27 le découpage rangées/colonnes. Deux cibles mesurables pour la
> contiguïté de run, au lieu d'une.

⚠️ **UNE ERREUR DE DÉPOUILLEMENT COMMISE PUIS CORRIGÉE, et c'est encore l'UNITÉ DE MESURE.** Le
premier tableau annonçait « 14 buts sur 14 posés hors de leur rang » sur le chemin du record 10/20.
Faux : sur un chemin **inachevé**, « dernière arrivée sur un but » n'est pas « pose » — la caisse
peut repartir. Corrigé, les **10 poses sont le PRÉFIXE EXACT** `[0,1,2,3,4,5,6,7,8,9]` de l'ordre
injecté, **0 paire en désaccord** ; les 4 buts « hors rang » sont des **transits purs** — (6,2) 10
arrivées, (5,2) 9, (5,3) 9, (6,3) 1. **45 transits pour 10 poses.** C'est la neuvième mise à mort
de la variante sèche de R1, et la première sur une sortie de SOLVEUR (les huit autres venaient de
parties humaines).

---

**🎯 LE CORPUS ENTIER : sur 26 niveaux gagnés à la main, 9 ont un ordre calculé ≠ ordre joué.**

Comparaison à l'ordre calculé **d'aujourd'hui** (`mesures/ordre`), surtout pas à celui inscrit dans
les journaux — ceux-ci sont des traces de binaires d'avant le correctif multi-salles.

| | niveaux |
|---|---|
| **ordre identique (17)** | 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, **16**, 17, **19, 20, 23, 24, 25** |
| **ordre DIFFÉRENT (9)** | **6, 12, 13, 14, 15, 18, 22, 26, 27** |

Inversions : 22 → **148/351**, 13 → 52/120, 27 → 51/190, 15 → 41/105, 18 → 26/55, 14 → 24/153,
6 et 12 → 16, 26 → 1.

- ✅ **RECOUPEMENT INDÉPENDANT AVEC LA LOI DE L'ORDRE.** Le 2026-08-03, la loi passée sur 24 parties
  humaines trouvait des faux positifs sur exactement **12, 14, 15, 18, 22, 26**. **Les six sont dans
  ces neuf.** Deux instruments sans rapport, même verdict. Les trois que la liste ajoute ont chacun
  leur raison : le **6** (deux ordres valides, la partie du journal suit le sien), le **13** (la loi
  est muette malgré 52 inversions), le **27** (hors de cette campagne).
- 🎯 **ET LE SIGNAL EST FRANC :**

  | | ordre identique | ordre différent |
  |---|---|---|
  | **résolus par le solveur** | **11** | **1** (le 6) |
  | non résolus | 6 | 8 |

  Les six non-résolus à ordre identique sont **16, 19, 20, 23, 24, 25** — très exactement le
  « groupe qui interroge le plus » du 2026-08-01, qui ne reposait que sur 19/20/23. **Porté à six,
  et obtenu mécaniquement : leur verrou n'est pas l'ordre.**
- ⚠️ **Réserve sérieuse** : l'ordre calculé était **AFFICHÉ** pendant le jeu, donc « identique »
  mesure en partie la conformité volontaire (réserve déjà posée le 2026-08-01 pour 1/2/3/4). Ce qui
  la tempère : l'utilisateur a dévié sur neuf niveaux, l'instrument n'est pas purement
  auto-confirmant. Et 26 niveaux, pas 33 — pas de journal gagnant pour 0, 21, 28 à 33, ni le 29.

---

**❌ QUATRE INJECTIONS DE PLUS (14, 15, 22, 26) — AUCUNE NE RÉSOUT.** Ordres humains extraits des
journaux, injectés, `coupl-plongeon`, arrêtés à la main. Prédiction faite **avant** le lancement à
partir des reprises (§2026-08-04) : le 14 (5 reprises) avait la meilleure chance, le 22 (12) et le
15 (11) la moins bonne.

| niv | reprises | dépilements | états vus | max | verdict des plongeons |
|---|---|---|---|---|---|
| 14 | 5 | 18,9 M | 51,2 M | 15/18 | records **12, 13, 14, 15/18 MORTS** (A\* pur, `CORRAL=0`) |
| 15 | 11 | 12,7 M | 28,1 M | 10/15 | record 10/15 **MORT** (`échec en 1 état` sur budget 96 944) |
| 22 | 12 | 18,5 M | 56,4 M | 4/27 | budgets épuisés — aucun verdict |
| 26 | 8 | 31,3 M | 51,2 M | 7/13 | record 7/13 **MORT** (`échec en 2 états` sur budget 1 372) |

- 🎯 **Le 14 accumulait des records dans une lignée CONDAMNÉE depuis 9 785 dépilements** : ses
  quatre derniers records rejoués seuls rendent `AUCUNE 0` — mort à la racine, zéro état exploré,
  **identique avec `CORRAL=0`** donc pas un faux positif du corral. Même motif que le 27 en juillet
  (record 17/20 « de bon espoir », mort à la racine).
- **Le 22 passe de 1/27 à 4/27** — l'ordre injecté le décoince du démarrage, ce que rien d'autre
  n'avait obtenu. Il reste Groupe A.
- **Bilan de la campagne d'injection : 2 réussites (12, 27), 1 échec net (13, l'ordre humain fait
  PERDRE), 5 sans verdict (14, 15, 18, 22, 26).** « Ordre différent » n'est donc pas un prédicteur
  de déblocage — c'est une condition qui, seule, ne suffit pas.

---

**❌ UNE MÉTRIQUE PROPOSÉE ET TUÉE PAR SON CONFONDANT, dans l'heure.** Constat utilisateur sur le
14 : *« les caisses en (9,2) et (11,2) peuvent rester sans qu'on ne les ait touchées avant »*.
Vérifié, et c'est spectaculaire :

| caisse | 1ʳᵉ poussée | % de la partie écoulé | rang de pose |
|---|---|---|---|
| **(9,2)** | coup **229**/247 | **93 %** | 14/17 |
| **(11,2)** | coup **238**/247 | **96 %** | 16/17 |
| (15,7) | coup 243/247 | 98 % | 17/17 |

Trois caisses ne bougent pas d'un pouce pendant 93 % de la partie puis rentrent d'un trait (4 à 5
poussées). À l'autre bout, (2,3) part au coup 1 et n'arrive qu'au coup 185. **Deux populations :
une RÉSERVE et un JEU DE TRAVAIL.**

D'où une métrique « % de caisses jamais manœuvrées à la main », qui donnait **53 % sur les résolus
contre 15 % sur les non-résolus**. ❌ **Sans valeur : Spearman 0,91 avec le simple « % de poussées
faites par macro ».** Elle ne dit rien de plus que « sur les résolus, la macro fait le travail ».
La bonne maille reste celle laissée ouverte le 2026-08-05 (fraction de la partie passée sur la case
de DÉPART), indépendante de la disponibilité de la macro. Non faite.

---

**🎯 LE RÉSULTAT DE LA SESSION — « TROP TÔT » EST UN PROBLÈME DE BUT, PAS DE CAISSE.**

Question de l'utilisateur, qui est la bonne : *« je ne vois pas comment dire au solveur : il y a une
macro jouable sur cette caisse, mais c'est trop tôt »*. Avant de chercher un prédicat, on compte le
phénomène — le mode hybride journalise à chaque état le nombre de macros jouables, et on sait quelle
poussée a été jouée. **Vérité terrain gratuite, jamais dépouillée.**

⚠️ **Et l'utilisateur a désamorcé le piège avant qu'on y tombe** : *« sur le 14, le goal ordering
n'était pas bon, donc la macro déclenchait probablement à tort »*. C'est le **FAIT 6** (§7) — sans
séparer les deux populations, on mesurerait l'écart à `ordreButs`, pas le report.

| échantillon | macros offertes puis DÉCLINÉES | médiane par niveau |
|---|---|---|
| **ordre identique (17 niveaux)** | **82 / 1 139 poussées choisies = 7 %** | **0 %** |
| ordre différent (9 niveaux) | 1 197 / 1 506 = **79 %** | **89 %** |

Sur le 14 : **205 refus sur 205**. Sur le 27 : 93 %. Sur le 18 : 92 %. Sur le 13 : 89 %.

> **Quand le but actif est le bon, la macro offerte est la bonne et on la joue — médiane 0 % de
> refus sur dix-sept niveaux.** Le prédicat de report par CAISSE que la question appelait n'a
> presque rien à attraper. Ce qui paie, c'est l'ordre. Et ça consolide tout le reste de la journée :
> le 12 et le 27 tombent quand on corrige l'ordre, et les neuf niveaux à ordre différent sont
> exactement ceux où la macro tire à côté en permanence.

**MAIS IL RESTE UN SPÉCIMEN PUR, ET C'EST LE 16.** Ordre calculé **== ordre joué, 15/15 buts au même
rang**, et pourtant **50 refus sur 132 poussées choisies — 38 %**, contre 14 % au plus partout
ailleurs dans l'échantillon propre.

Discriminant appliqué aux 82 refus — la poussée jouée porte-t-elle sur la **même** caisse que celle
que la macro proposait (route refusée) ou sur une **autre** (moment refusé) ?

| | même caisse | autre caisse |
|---|---|---|
| **total corpus propre** | **7** (dont 5 sur le seul 17) | **75** |
| dont le 16 | **0** | **50** |

**Le détail du 16 donne le mécanisme complet.** La macro est offerte sur **la même caisse, (5,10),
trente-trois fois d'affilée**, de `posees 3` à `posees 10` — pendant que l'utilisateur répète

```
(7,4)→(8,4)  (8,4)→(9,4)  (9,4)→(9,5)  (9,5)→(9,6)  (9,6)→(8,6)
```

c'est-à-dire **le CONVOYEUR du 2026-08-05**, la voie unique que chacune des 15 caisses doit
franchir à son tour.

> 🔴 **ET VOILÀ POURQUOI LE 16 PLAFONNE.** `solveurastar.cpp:1061` ne génère les poussées simples
> que si `macrosOk == 0`. Tant que la macro sur (5,10) aboutit, **aucune** poussée simple n'est
> enfilée : le solveur est *forcé* de livrer (5,10) et **ne peut structurellement pas jouer le
> convoyage**. Ce n'est pas qu'il classe mal ces coups — **il ne les engendre pas.** Même
> diagnostic que le 13 (89 % hors régime), mais ici avec l'ordre **hors de cause** et sur une seule
> caisse identifiable.

C'est le spécimen que le plan cherchait depuis le 2026-08-04 (*« le stock reste non modélisé »*,
*« la difficulté est en aval du stock »*) : un seul niveau, une contrainte connue, 50 cas étiquetés.

**LE MÉCANISME POUR DIRE « PLUS TARD » EXISTE DÉJÀ, et il tient en une ligne** : refuser une macro
laisse `macrosOk` à 0, donc le repli sur les poussées simples s'enclenche tout seul. Et cette
famille de levier a la propriété qui manque à toutes les autres — **elle est LOUD et ne peut pas
mentir** : une macro refusée à tort ne produit jamais de fausse solution, elle ralentit
visiblement. C'est l'inverse exact de la variante sèche de R1, qui coupe et qu'on a tuée neuf fois.
Deux points d'accroche, un seul construit :

| dire « plus tard » sur… | état |
|---|---|
| un **BUT** — `butActif()` le saute | ✅ construit — c'est `porteBloquee()` dans `ordre-dyn` (2026-08-04) |
| une **CAISSE** — refuser la macro alors que le but est actif | ❌ pas construit ; le plus proche est `couplage`, qui *préfère* et se replie |

⚠️ **Le trou est le PRÉDICAT, pas le câblage** — et son coût est réel : pendant le report, le
solveur retombe sur les poussées simples, c'est-à-dire l'explosion combinatoire que le régime
d'engagement existe pour éviter. Sur le 16 ça porterait sur les états à `posees 3-10`, une bonne
part du run. **Le gain n'est pas acquis ; ce qui est acquis, c'est de savoir quoi tenter et où le
mesurer.**

---

**Reste ouvert :**
- [ ] **Le 16 est le terrain du report.** Sortir les 50 refus (caisse offerte, but actif, coup joué)
  et chercher ce qui, statiquement, distingue (5,10) d'une caisse livrable. Dépouillement, pas du code.
- [ ] **Corriger la contiguïté de run** — deux cibles mesurables désormais (12 et 27), même motif :
  ne pas entrelacer deux lignes parallèles de buts. C'est ce qui rendrait le 12 et le 27
  reproductibles **sans injection**, donc éligibles à [scores.md](scores.md).
- [ ] **Le 18 : son murage a BOUGÉ**, rang 10 → **rang 6, sur (6,6)**, bouché par (5,6)/(6,4)/(6,5)
  aux rangs 2/3/4. C'est le correctif multi-salles qui l'a déplacé, et l'item *« 18, 24, 25, 26 non
  mesurés »* du 2026-08-01 est donc fait pour le 18. `ordre` le classe **famille A** — *« un défaut
  de `ordreParPrecedence`, corrigeable »*. ⚠️ **Ça met en cause la conclusion du 2026-07-31**
  (*« le 18 restera muré quoi qu'on fasse, aucun ordre sain complet n'existe »*), qui portait sur
  l'ordre d'AVANT et n'a jamais été refaite. À reprendre avant de la citer.
- [ ] **Le 22 n'a pas de verdict** (budgets épuisés partout) et reste le plus gros désaccord d'ordre
  du corpus (148 inversions). Son ordre injecté le fait passer de 1/27 à 4/27.
- [ ] La maille « fraction de la partie passée sur la case de départ », qui remplacerait la métrique
  réfutée ci-dessus. Toujours pas faite.

**État du code** : ⚠️ **RIEN.** L'essai de tie-break a été **restauré** (`diff` vérifié), les runs
sont partis de répertoires isolés. Ce qui a changé dans le dépôt : **`ordre_niveau_0012.txt` et
`ordre_niveau_0027.txt` posés à la RACINE** à la demande de l'utilisateur (le 12 porte l'ordre de
**juillet**, colonne 15 bas→haut, le seul des deux qui résout). ⚠️ Avec `ordre_niveau_0006.txt` qui
y était déjà, **trois niveaux voient désormais tout `bench` lancé depuis la racine tourner à ordre
injecté** — bruyamment (`[ORDRE_FICHIER]` sur stderr), mais c'est le mécanisme qui a fait publier
puis retirer un verdict le 2026-08-04.

⚠️ **Outils de dépouillement écrits ce jour et restés dans le SCRATCHPAD** — `corpus_ordre.py`
(ordre calculé contre ordre joué sur tout le corpus), `decline.py` (macros offertes puis déclinées),
`refus.py` (même caisse / autre caisse), `pose27.py`. **C'est exactement le sort de `juge_loi.py`,
perdu le 2026-08-03** ; la règle du §1 dit de les rapatrier dans `mesures/` le jour même. **Non
fait.** Tous réutilisent le parseur de `mesures/taches.py` (`parties`/`rejoue`) plutôt que d'en
écrire un second, et chacun se valide en reproduisant son compte de poussées.

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
  distance nulle part sur le chemin) — ~~mais aucun cas confirmé de ce genre n'a encore été trouvé ;
  celui qu'on croyait tel s'est révélé être un second fork non exploré.~~
  ⚠️ **CONFIRMÉ le 2026-08-01** (fin du §6.2) : la caisse **(5,5) du niveau 20**, testée au clic droit
  contre **13 buts actifs différents**, ne peut avancer vers aucun — « aucune direction ne baisse la
  distance, même joueur placé où l'on veut ». C'est le premier cas prouvé, par un instrument qui
  relâche la contrainte de zone sur `avanceVersBut` lui-même. ⚠️ **UN seul cas, et il ne bloque
  rien** (cette caisse se manœuvre aux poussées simples, que le solveur génère bien là).

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
- ⚠️ **Un instrument d'observation peut mesurer l'OBSERVATEUR** (2026-08-01). Le mode hybride compte
  les états où aucune goal macro n'est jouable. Lu comme une mesure de difficulté du plateau, il a
  produit une conclusion fausse en une minute (« remplir la salle éloignée d'abord concentre le
  démêlage »). Il mesure en réalité l'**écart à `ordreButs`** : `butActif()` rend le premier but non
  rempli de l'ordre statique, donc dès qu'on joue autre chose, plus aucune macro n'est générée et le
  compteur sature — 884 états sur un seul but. **Avant de lire un compteur d'absence, vérifier ce
  qui le remet à zéro.** Ici le journal donnait la réponse gratuitement : il imprime le but actif à
  côté du compte, et c'est sa constance qui a démasqué l'artefact.
- ⚠️ **UNE NEUTRALISATION AU CLAVIER NE COUVRE PAS LA SOURIS** (2026-08-02). Pendant une session
  d'annotation, `eventFilter` neutralise `Backspace` et les flèches parce qu'ils modifient le plateau
  **sans toucher à `posPas`**, ce qui rend faux le numéro de coup écrit dans les intentions. Mais le
  **clic** du mode hybride fait marcher le perso par le même chemin, et il n'est pas neutralisé — le
  trou est dans le `KeyPress` du filtre, qui ne voit pas les événements souris. **La trace est déjà
  dans le corpus** : niveau 1, coup 8, deux frappes au **même numéro de coup** avec le joueur en
  (7,4) puis (7,3) — deux cases adjacentes, soit un pas de marche entre les deux. Contournement
  pendant la campagne : naviguer avec ◀ ▶, `N` ou le slider, **ne pas cliquer sur le plateau**.
  Règle générale : **quand on neutralise une entrée parce qu'elle contourne un compteur, énumérer
  TOUTES les entrées qui l'atteignent** — le commentaire du code liste consciencieusement les
  touches, et c'est cette liste qui a fait croire le problème réglé.
- ⚠️ **UN WIDGET QUI NE S'AFFICHE PAS SE DIAGNOSTIQUE EN IMPRIMANT SON ÉTAT, PAS EN CHANGEANT SON
  PLACEMENT** (2026-08-01). Une simple légende de touches a coûté **six corrections successives** :
  layout horizontal qui écrase un texte de trois lignes → `centralWidget` qui **EST** le plateau
  (WGame peint par-dessus) → fond forcé sans couleur de texte forcée (« tout gris » en thème sombre)
  → texte RichText avec entités et Unicode (un texte enrichi mal formé se rend **vide sans lever
  d'erreur**) → hauteur en constante alors que le texte était passé de 3 à 6 lignes (la barre
  d'état tronquait) → et enfin la vraie cause : **une condition de visibilité que j'avais moi-même
  écrite** (`setVisible(cbHybride->isChecked())`). À chaque tour, une hypothèse plausible remplaçait
  la précédente. **Ce qui a tranché en une seconde, c'est une capture d'écran** : barre d'état haute
  (donc le code s'exécute) + zone du widget vide (donc il est caché) ⇒ un seul suspect possible.
  Deux règles à en tirer : **une légende ne se conditionne pas**, et devant un symptôme visuel,
  imprimer `isVisible()`/`sizeHint()` coûte une minute là où six déductions coûtent une heure.
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
- **`mesures/compresse_journaux.py`** (neuf, 2026-08-02) — **le seul outil du projet qui RÉÉCRIT des
  données**, d'où trois précautions à ne pas retirer : rapport seul par défaut (`--ecrire` pour
  appliquer), **validation par REJEU sur le vrai plateau** (chaque coup légal, partie gagnée à
  l'arrivée — une partie qui ne valide pas est laissée intacte), et **idempotence vérifiée** (relancé
  sur un corpus déjà compressé, il ne retire plus rien). Trois passes, chacune un no-op sur la partie
  jouée : `[undo]` appliqués **avec le coup qu'ils annulent** (retirer le seul marqueur ferait
  rejouer les coups annulés — le parseur de l'app les applique), boucles de marche du perso retirées,
  **une seule partie gagnée conservée** par niveau. Les `[manque]` sortent dans
  `hybride_niveau_XXXX_manques.txt` au lieu d'être perdus. ⚠️ **Il décale les numéros de coup**, donc
  il marque les `_intentions.txt` « ANCRES PERIMEES » — sans quoi un fichier resterait faux en
  silence. Effet mesuré sur le corpus du 2026-08-02 : **119 711 → 47 080 lignes, 9,24 → 3,8 Mo**,
  21 parties gagnantes conservées.
  ⚠️ **Le piège qui a fait échouer 14 fichiers sur 21 au premier jet** : un journal contient
  PLUSIEURS parties, et traiter le fichier d'un bloc fait enjamber une frontière à un segment de
  marche — le rejeu de validation continue alors sur le plateau de la partie précédente. **Tout se
  fait partie par partie.** C'est la validation qui l'a vu, pas la relecture.
