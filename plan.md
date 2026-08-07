# Plan solveur Sokoban

> **Ce document a été condensé le 2026-07-17.** On y garde : les gains **mesurés** et la
> technique qui les a produits, les pistes **restantes**, et les pièges à ne pas refaire.
> Le récit des impasses et des allers-retours a été coupé (l'historique est dans git).
>
> **Et DÉCOUPÉ le 2026-08-06**, à 5 588 lignes et 386 Ko — dont 90 % de journaux de session,
> si bien qu'on ne le relisait plus en entier. Les récits de chantier sont partis dans quatre
> fichiers (§6.1 à §6.3, cf. l'index sous le §6.0) ; **rien n'a été supprimé**, le découpage a
> été vérifié en recollant les morceaux contre la version commitée, à la ligne près. Ce qui
> reste ici est ce qu'on relit à chaque reprise : la carte, les outils, les résultats acquis,
> ce qui est réfuté, et les pièges.

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
| **mode HYBRIDE** (dans l'app) | **(neuf, 2026-08-01) LE SOLVEUR LOGUE UNE PARTIE HUMAINE.** ⚠️ **Il LOGUE, il n'ANNOTE pas** — le mot compte : annoter suppose un jugement, et s'il savait juger un coup il saurait résoudre le niveau. C'est mesuré des deux côtés — le §3 pour `h` (« toute borne qui capturerait le mou devrait résoudre un ordonnancement optimal ») et la session 2/7 du 2026-08-01 pour le classement (ton coup est 1ᵉʳ dans 47 % des cas, jamais au-delà du 18ᵉ, `df = 0` deux fois sur trois : il ne peut pas départager). Le solveur enregistre son état ; **c'est l'humain qui juge**, et seulement par la touche `C` (critique du chemin du solveur). Case à cocher : l'ordre de remplissage s'affiche en chiffres sur les buts, et à CHAQUE coup joué à la main l'UI rejoue le **régime d'engagement du solveur** (`solveurastar.cpp:331-343` — `getCaissesDeplacable` → `macroPeutDemarrer` → `macroVersButBacktrack` + `!isPerdu`) et surligne les macros jouables. Clic sur une caisse cerclée = la macro se joue ; clic sur une case libre = le perso y marche ; **clic DROIT = « il aurait dû y avoir une macro ici »**, qui consigne la CAUSE (poussable dans aucune direction / échec au pas 0 / descente bloquée en (x,y) avec N restants / aboutit mais `perdu`) + le plateau. Tout part dans `hybride_niveau_XXXX.txt` (un par niveau, en AJOUT, flush par ligne). C'est le seul outil qui répond à « l'ordre est-il BIEN JOUÉ ? », là où `ordre` ne répond qu'à « est-il FAISABLE ? »<br>**(2026-08-01, suite) LE RANG DU COUP HUMAIN** — à chaque poussée vraiment choisie (hors macro, hors rejeu), l'UI rejoue l'**enfilage** du solveur sur l'état d'avant (mêmes enfants, mêmes élagages dans le même ordre, même clé de tri que le comparateur) et journalise `[rang] (x,y) Dir \| rang R/N \| h .. f .. \| meilleur (x,y) Dir \| df ±k`. Trois variantes : `HORS REGIME MACRO` (le solveur ne générerait aucune poussée simple — c'est la mesure du désaccord), `⚠ ECARTE par le solveur` (**faux positif d'élagage PROUVÉ** si la partie est gagnée : c'est le juge `fp` étendu aux niveaux NON RÉSOLUS), `⚠ INTROUVABLE` (le miroir a divergé du solveur). ⚠️ Rang **parmi les frères**, pas dans la file globale : ce qui s'y transporte, c'est `df` |
| `pas0 <niv>` | **(neuf, 2026-08-01) POURQUOI AUCUNE MACRO N'EST DISPONIBLE**, sur le plateau de DÉPART. Pour chaque couple (caisse, but), rejoue le contrat EXACT de l'UI — `macroPeutDemarrer`, descente `macroVersButBacktrack` menée au bout, `!isPerdu` — et classe les échecs : *amorce puis bloque en (x,y)*, *détour non-monotone requis*, *joueur du mauvais côté*. Répond en une seconde à « le premier but choisi change-t-il quelque chose au démarrage ? » (sur le 12 : non, aucun des 15 n'est atteignable). ⚠️ **Le premier jet ne testait que `macroPeutDemarrer` et annonçait l'inverse** — amorcer n'est PAS aboutir. Outil de chantier<br>**(2026-08-07)** accepte un **chemin `.xsb`** (comme `bench`/`loi`/`ordre`) et deux modes. `champ` imprime **les deux champs de distance côte à côte** — le **BRUT** (`Game::champDistanceBrut`, la table précalculée telle quelle = ce que la macro croit devoir suivre) et le **JOUABLE** (ce que la descente monotone accepte) : les lire ensemble est le seul moyen de séparer « la table se trompe » de « la table a raison mais la descente ne sait pas l'exécuter ». `trace` rejoue la descente pas à pas avec, pour CHAQUE direction, la raison du refus (`MUR` / `caisse` / `appui HORS ZONE` / `NON MONOTONE`), puis se confronte à la vraie fonction. C'est ce couple qui a trouvé le bug du demi-tour (§6.3, 2026-08-07). Deux autres modes : `multi` (combien de macros DISTINCTES une caisse peut produire — mesuré : jusqu'à 4 chemins, **toujours 1 seul état**) et `detour` (l'écart au trajet solo, par recherche bornée à une seule caisse mobile ; ⚠️ **itinéraire, PAS une borne** — les autres caisses y sont des murs, donc surestimation, §4) |
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
- **Tolérance au DÉTOUR dans la goal macro** (2026-08-07, `pas0 … detour` sur 25 fixtures) :
  relâcher la descente monotone pour lui laisser un budget de poussées en trop ajouterait **5 macros
  sur 205 caisses poussables**, et les 7 gains sont **tous** sur les 3 plateaux exportés le jour même
  — **zéro** sur les 22 fixtures antérieures. **86 % des caisses n'atteignent pas le but même à +8** :
  le mode d'échec dominant de la macro n'est pas « l'itinéraire est trop long », c'est « il faut
  d'abord dégager une AUTRE caisse » — que la macro ne peut pas faire, elle en déplace une seule.
  ⚠️ L'écart au trajet solo est **toujours PAIR** (§3, `Δh = ±1`) : les paliers sont 2, 4, 6, et un
  détour à +2 est exactement UN recul. Détail en [journal-macro.md](journal-macro.md), 2026-08-07 2/2.
- **Plusieurs macros par CAISSE** (même date, `pas0 … multi`) : les forks produisent jusqu'à 4 chemins
  monotones mais **toujours un seul ÉTAT** — même longueur par construction, même configuration de
  caisses, et la clé ne retient que la ZONE du joueur. Enfiler les branches ne ferait que des doublons.
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

### 6.1 à 6.3 — les journaux de chantier, dans des fichiers séparés

Le **récit des sessions** y vit ; les **résultats acquis** restent ici. Découpé le 2026-08-06,
sans rien supprimer (cf. l'en-tête du document). La numérotation `§6.x` est conservée dans les
journaux : les renvois croisés continuent donc de désigner quelque chose.

| chantier | journal | ce qu'on y trouve |
|---|---|---|
| **§6.1** deadlock | [journal-deadlock.md](journal-deadlock.md) | corral unitaire, pince, corral-N, motif du paquet |
| **§6.2** ordre | [journal-ordre.md](journal-ordre.md) | goal-ordering, précédences, multi-salles (juillet) |
| **§6.2** hybride | [journal-hybride.md](journal-hybride.md) | parties à la main, intentions, gadgets, injections d'ordre (août) |
| **§6.3** macro | [journal-macro.md](journal-macro.md) | coût par état, backtracking sur forks, plongeon sur record, `deltaf` |

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
- ⚠️ **LE JOUEUR N'EST PAS UN OBSTACLE, et `isLibre()` dit le contraire** (2026-08-07). `isLibre` ne
  rend vrai que pour `tcNone` et `tcGoal` : la case du joueur (`tcPlayer`/`tcGoalPlayer`) est donc
  **occupée** pour elle. Mais `pousse()` téléporte le joueur sur l'appui avant que la caisse n'avance
  — sa case est libre au moment qui compte. `getCaissesDeplacable` avait l'exemption et la
  commentait ; **`avanceVersBut`, le contrat de descente de la macro, ne l'avait pas**. Conséquence
  mesurée : la macro refusait **tout DEMI-TOUR**, puisque après une poussée le joueur est par
  construction sur la case d'où la caisse vient — c'est-à-dire tout **RECUL** au sens du §3, les
  poussées qui portent la totalité du mou. Corrigé (§6.3, journal du 2026-08-07) : canari intact,
  17 en ÷1,33, aucun niveau dégradé. **Règle générale : une règle écrite à deux endroits diverge ;
  chercher l'autre exemplaire AVANT de conclure qu'une condition est juste** — et ici c'est la
  RÉPLIQUE du contrat dans un harnais, confrontée à l'original, qui a localisé l'écart, là où trois
  relectures de la fonction avaient conclu qu'elle était correcte.
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
