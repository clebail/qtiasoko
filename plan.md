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
  (solve mené au bout, états/poussées/commit relevés). À ce jour : **18 résolus sur les 33**
  (0-12, 17, 21, 26, 27 et 32 — le 11 le 2026-07-28, le 10/21/32 le 2026-07-29, et le **12, le 26 et
  le 27 les 2026-08-09/11**, tous trois par le régime `ordre-look`, cf. §6.2 et
  [journal-macro.md](journal-macro.md)).
  **Les 15 autres — 13 à 16, 18 à 20, 22 à 25, 28 à 31 — ne sont PAS résolus**, y compris
  ceux dont ce document parle beaucoup (18, 24, 25 au §6.2 ; 13-16 dans les tableaux de
  diagnostic du §6.3). Apparaître dans un tableau de mesure ne veut PAS dire résolu : `mort`,
  `macro` et la jauge `rangees` tournent justement sur des niveaux qu'on ne sait pas finir.
  ⚠️ **Le 26 et le 27 sont résolus par un RÉGIME SÉPARÉ** (`ordre-look`), jamais par le défaut —
  même statut que le 10, le 21 et le 32 avec le plongeon. Et `ordre-look` **casse le 32** : aucun
  régime ne résout aujourd'hui les 18 d'un seul tenant.
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
- ~~Deux modes d'échec (jauge `rangees`) : **Groupe A** ne démarre pas ; **Groupe B** plafonne à
  mi-chemin.~~ ❌ **PARTITION TOMBÉE le 2026-08-11** : le **12** était Groupe A et il est résolu, le
  **26** était Groupe B et il est résolu. Elle était « ce qui SURVIT » du profilage du §6.6 ; il n'en
  reste rien. Le mode d'échec réellement dominant aujourd'hui est **la MÉMOIRE** — trois des cinq
  niveaux de la série `ordre-look` y sont morts (§6.5).

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
| **LE RANG DE LA MACRO HUMAINE** (dans l'app + `rejeu`) | **(neuf, 2026-08-23)** `jugeMacro` (**`jugemacro.h`**, racine, **exemplaire unique** §7) rejoue les deux passes de `tenteMacro` PUIS l'enfilage — régime du couplage, corral unitaire, corral-N, et la clé du comparateur (f croissant, **g décroissant**, guidage). Quatre verdicts : **HORS PASSE COUPLAGE** (le couplage assigne le but à une autre caisse dont la macro aboutit ⇒ celle-ci n'est générée dans AUCUN état), **ECARTE** (faux positif prouvé sur une partie gagnée), **INTROUVABLE** (miroir en défaut), **rang R/N + df**. Comble le trou de `⚠ ECARTE`, qui ne jugeait que les poussées manuelles — 133 coups sur 140 y échappaient sur la partie du 200. Ligne `[macro-rang]` dans le journal hybride. ⚠️ Ne rejoue NI `loiTropTot` NI la dédup `meilleurG` : un rang dit « il enfilerait ceci ici », pas « il le développerait » |
| `pas0 <niv>` | **(neuf, 2026-08-01) POURQUOI AUCUNE MACRO N'EST DISPONIBLE**, sur le plateau de DÉPART. Pour chaque couple (caisse, but), rejoue le contrat EXACT de l'UI — `macroPeutDemarrer`, descente `macroVersButBacktrack` menée au bout, `!isPerdu` — et classe les échecs : *amorce puis bloque en (x,y)*, *détour non-monotone requis*, *joueur du mauvais côté*. Répond en une seconde à « le premier but choisi change-t-il quelque chose au démarrage ? » (sur le 12 : non, aucun des 15 n'est atteignable). ⚠️ **Le premier jet ne testait que `macroPeutDemarrer` et annonçait l'inverse** — amorcer n'est PAS aboutir. Outil de chantier<br>**(2026-08-07)** accepte un **chemin `.xsb`** (comme `bench`/`loi`/`ordre`) et deux modes. `champ` imprime **les deux champs de distance côte à côte** — le **BRUT** (`Game::champDistanceBrut`, la table précalculée telle quelle = ce que la macro croit devoir suivre) et le **JOUABLE** (ce que la descente monotone accepte) : les lire ensemble est le seul moyen de séparer « la table se trompe » de « la table a raison mais la descente ne sait pas l'exécuter ». `trace` rejoue la descente pas à pas avec, pour CHAQUE direction, la raison du refus (`MUR` / `caisse` / `appui HORS ZONE` / `NON MONOTONE`), puis se confronte à la vraie fonction. C'est ce couple qui a trouvé le bug du demi-tour (§6.3, 2026-08-07). Deux autres modes : `multi` (combien de macros DISTINCTES une caisse peut produire — mesuré : jusqu'à 4 chemins, **toujours 1 seul état**) et `detour` (l'écart au trajet solo, par recherche bornée à une seule caisse mobile ; ⚠️ **itinéraire, PAS une borne** — les autres caisses y sont des murs, donc surestimation, §4) |
| **injection d'ordre par FICHIER** | **(neuf, 2026-08-01)** `ordre_niveau_XXXX.txt` dans le répertoire courant écrase l'ordre calculé de ce niveau. Complète `ORDRE_HUMAIN`, qui est une variable d'environnement et **n'atteint donc pas l'app** lancée par un launcher (§7) : c'est le seul moyen de JOUER un ordre à la main en mode hybride et de voir où il coince. Même parseur, exemplaire unique. **Bruyant des deux côtés** (`[ORDRE_FICHIER]` sur stderr, et le journal hybride écrit `ordre de remplissage ⚠ INJECTE depuis …` au lieu de `calcule`) — un fichier oublié changerait sinon le comportement en silence, le pire cas du §7. Absent = rien ne change |
| **rejeu de journal + INTENTIONS** (dans l'app) | **(neuf, 2026-08-01) CAPTURER LE PLAN, PAS LE COUP.** Touche `L` : relit `hybride_niveau_XXXX.txt`, en extrait la **dernière partie GAGNÉE** (les `[undo]` retirent le dernier coup) et l'installe dans le rejeu pas à pas existant — aucune mécanique de navigation en double. `N` saute à la prochaine **poussée choisie** (macros et marche franchies d'un coup). Six touches d'intention en vocabulaire **FERMÉ** : `E` écarter du chemin d'une autre caisse · `O` ouvrir un passage joueur · `G` garer pour plus tard · `A` préparer un appui · `T` **sortir pour reprendre dans l'autre sens** (= le RECUL du §3) · `R` rapprocher · `?` je ne sais pas. **Une frappe par PLAN**, valable jusqu'à la suivante — c'est l'objet même : le rang d'un coup isolé ne peut pas voir un plan sur plusieurs coups. Sortie : `hybride_niveau_XXXX_intentions.txt`, avec le **numéro de coup** (sans lui les annotations seraient orphelines). ⚠️ Flèches et Retour arrière **neutralisés** pendant une session : ils modifient le plateau sans toucher à `posPas`, et le numéro de coup écrit devient faux |
| `image <niv|fichier.xsb> [sortie.png] [taille]` | **(neuf, 2026-08-14, idée utilisateur)** UN PLATEAU EN PNG, AVEC LES SPRITES DE L'UI. ⚠️ Réutilise les classes de l'APPLICATION (`Sprite`, `Sol`/`SolHors`, `Mur`, `Caisse`, `GoalCaisse`, `Goal`, `Player`) et **le même empilement de couches que `WGame::paintEvent`**, flood-fill dedans/dehors compris — redessiner à côté produirait une image qui RESSEMBLE au jeu sans en être, et c'est justement quand les deux divergent qu'on regarde une image. Raison d'être : on lit des `.xsb` en ASCII en permanence, et **la géométrie du 12 a été mal lue trois fois de suite, dans les deux sens**, alors que la réponse était dans le dessin. Seul harnais de `mesures/` qui tire des sources de l'application et exige `QT += gui` (`QT_QPA_PLATFORM=offscreen` sans écran) |
| `paquetcle` | **(neuf, 2026-08-13)** LE CODEC DE CLÉS EST-IL UNE BIJECTION ? Empaquetage/dépaquetage sur les dix tailles réelles de plateau — bords, valeurs identiques, cases croissantes, 200 k tirages aléatoires par taille — plus la **canonicité** (les bits de rab à zéro, sans quoi `memcmp` ment). 2 000 040 cas. ⚠️ À passer AVANT tout câblage : le canari ne verrait pas une clé subtilement fausse, il verrait un niveau non résolu ou rien du tout |
| `attente.py <niv>` | **(neuf, 2026-08-09) LES CAISSES QU'IL NE FAUT PAS TRAITER COMME LES AUTRES.** Plus longue immobilité d'une caisse **sur une case qui n'est PAS un but**, en % de la partie gagnée — puis un seul critère de partage : attend-elle **là où elle a commencé** (on n'y a pas touché : *« ne gêne en rien, je la garde pour plus tard »*) ou **là où on l'a mise** (**stockage**, détour payé) ? ⚠️ Le filtre « pas un but » est indispensable : sans lui une caisse LIVRÉE tôt sort en tête (le 32, « immobile 96 % » = posée au coup 22 et finie). Les deux niveaux de référence sortent aux extrêmes sans réglage : le **14** n'a que du « sur place » (97/94/87/80 %), le **16** que du « déplacé » (90/89/87/81 %) |
| `stock.py <niv> [mode]` | **(neuf, 2026-08-17)** POURQUOI UNE CAISSE EST TENUE. Prolonge `attente.py` : mine les parties gagnées et décompose les caisses **livrables mais différées** sur quatre tests contrefactuels (validés par rejeu, aucun solveur). Modes : `(défaut)` les deux signatures du §6.0 (différée / déplacée en plusieurs fois) · `passage` transit strict de la case-but · `cut` cut d'articulation (prédicat du porte généralisé) · `contention` corridor de livraison partagé (§3) · `depart` bouchon au départ · `bilan` la décomposition PORTE/CONGESTION/BOUCHON/ORDRE. ⚠️ Sur la **partie humaine** loguée, pas sur une trace de solveur. Résultat en [journal-hybride.md](journal-hybride.md), 2026-08-17 |
| `diverge`, `paires`, `trace`, `passages`, `congestion` | mou de `h`, interactions de paires, solution pas à pas, cartes de trajets |
| **historique des RECORDS + critique du solveur `C`** (dans l'app) | **(neuf, 2026-08-03) LE MIROIR DE L'ANNOTATION D'INTENTIONS, mais sur ce que le SOLVEUR fait.** Le solveur a DEUX points d'enfilage (recherche principale + `plonge()`) et `nouveauMaxCaisses` écrasait le chemin visionné à CHAQUE record — un sélecteur conserve tous les chemins d'un run, voir le record 7 ET le record 8 ne demande plus qu'un seul run. Touche `C` : boîte de texte LIBRE (pas de vocabulaire fermé — celui des intentions a mis deux sessions à se stabiliser, on ne le refait pas sans savoir ce qu'on y met), journal `solveur_niveau_XXXX_critique.txt`, plateau `.xsb` joint à chaque entrée pour que `mort`/A\* puisse juger l'état après coup. `C` inerte pendant une session d'intentions (deux journaux distincts, ne pas mélanger) |
| `bench <fichier.xsb> record` → `.chemin` | **(neuf, 2026-08-03)** à côté de chaque `.xsb` exporté, une lettre par coup (H/D/B/G, ordre de `EDirection`) : permet de rejouer le chemin d'un record HORS de l'app, pour le passer à `mort`/`fp` |
| `gabarit.py <niv>` | **(neuf, 2026-08-03, scratchpad)** un plateau ASCII par but ACTIF (buts déjà remplis affichés comme posés, rien pré-rempli) — support pour DESSINER une règle de cases mortes à la main sans que l'instrument ne suggère le vocabulaire (cf. §6.2) |
| `juge_loi.py` | **(neuf, 2026-08-03, scratchpad)** juge une loi de cases mortes contre TOUTES les parties humaines gagnantes d'un coup (murs seuls, ordre injectable) : toute caisse sur une case déclarée morte est un faux positif PROUVÉ. A validé la loi du §6.2 sur 21/24 parties, et localisé les 3 exceptions à des ordres faux. ⚠️ **PERDU avec le scratchpad de sa session** — les scratchpads sont éphémères, tout outil qui doit resservir se rapatrie dans `mesures/` le jour même |
| `porte <niv>` | **(neuf, 2026-08-04) LA PRÉCÉDENCE CAISSE → BUT** — d'espèce neuve, toutes les autres sont but → but. *Si remplir G prive le joueur de TOUS les appuis d'une caisse C, alors C doit avoir bougé avant G.* Statique, O(caisses × buts × plateau), relaxation optimiste (une contrainte est une preuve, un silence ne promet rien). ⚠️ Une poussée dont la destination est une case MORTE ne compte pas comme une issue — sans ce test l'outil est muet. Rend **0 sur les 15 résolus**, et 2 sur 18 non résolus : le 16 (avant le rang 0) et le 30 — *comptes du 2026-08-04, quand la carte était à 15/33* |
| `portegen <niv\|plateau.xsb>` | **(neuf, 2026-08-18) LE PORTE GÉNÉRALISÉ** — `Game::porteGeneraliseeCoupe(idxCaisse, idxBut)` (game.cpp). Généralise `porteBloquee` : au lieu des seuls appuis de LA caisse, teste si occuper un but coupe l'accès du joueur à N'IMPORTE QUELLE AUTRE caisse non livrée ou but non rempli — un point d'articulation du graphe de marche COURANT (toutes les caisses réellement posées comme obstacles), pas la géométrie du départ seule. ⚠️ **DYNAMIQUE** contrairement à `porteBloquee` : deux flood-fills par appel, à interroger sur un état (`.xsb` de milieu de partie, comme `bench`/`ordre`/`pas0`), pas seulement un départ. Validé bit-à-bit contre le mineur `stock.py cut` sur les 10 tenues du niveau 27 (2 positifs, 8 négatifs, 10/10 identiques) |
| `fpporte.py [niv…]` | **(neuf, 2026-08-18) LE JUGE FP DU PORTE GÉNÉRALISÉ** — même protocole que `fp`/`juge_loi.py` : rejoue la dernière partie GAGNÉE de chaque niveau, teste le prédicat sur chaque livraison réelle, toute détection est un faux positif prouvé. **0 FP sur 1 650 livraisons, 28 niveaux.** ⚠️ Ne teste QUE la DERNIÈRE poussée de chaque caisse (sa position finale) — les poses de PASSAGE (une caisse qui transite par plusieurs buts d'un couloir aligné avant sa destination réelle) ne sont pas des livraisons ; les confondre a produit ~100 faux positifs bidons au premier jet (cf. §7) |
| `ampleurporte.py` / `ampleurporte2.py [niv…]` | **(neuf, 2026-08-18) L'AMPLEUR DU MOTIF** — `ampleurporte.py` (premier jet, RÉFUTÉ comme signal) scanne TOUTES les paires (caisse × but) à chaque jalon : 71 % coupées, mais **ne discrimine rien** — résolus et non-résolus touchés aux mêmes taux (67-81 % partout), signal trivial de géométrie (coins disjoints). `ampleurporte2.py` restreint au SEUL but que `butActif()` choisirait réellement (rang minimal de `getOrdreButs()`, lu via l'outil `ordre` — jamais recalculé en Python, trop de règles) : **17/435 jalons (3,9 %)**, 9 niveaux sur 28 touchés, dont plusieurs non-résolus (13, 14, 15, 22, 25) avec des coupures parfois massives (10 caisses/7 buts sur le 25) |
| `rejeu <niveau.xsb> <journal.txt>` | **(neuf, 2026-08-22 ; ÉTENDU AUX MACROS le 2026-08-23)** LE JUGE D'UNE PARTIE HUMAINE, DE BOUT EN BOUT. Rejoue la dernière partie d'un journal hybride et interroge le moteur à chaque état : `isPerdu()` (toute détection = faux positif prouvé) et `remplissageOrdonne()`. **Depuis le 2026-08-23, il juge aussi chaque `[macro] LANCEE`** via `jugeMacro` (`jugemacro.h`, exemplaire unique partagé avec l'UI) et classe la macro humaine en **HORS PASSE COUPLAGE** / **ECARTE** / **rang**. Il couvre donc enfin les coups de macro, qui étaient 95 % de la partie du 200. ⚠️ **Il relit l'ORDRE dans l'en-tête du journal** (`PAR ALIGNEMENT` → `setOrdreAlignement`, `⚠ INJECTE` → avertissement) : une macro vise `butActif()`, donc rejouer sous un autre ordre rendrait tous les verdicts faux **sans bruit**, le rejeu des coups marchant très bien par ailleurs. Le garde a servi le jour même — la partie du 200 avait été jouée en align, et 11 macros sur 15 étaient jugées à côté |
| `zonembut [<niv\|fichier.xsb>]` | **(neuf, 2026-08-22, idée utilisateur) LES ZONES D'EMBUT, EN `.xsb`.** Une zone d'embut est l'ENCLOS qui enferme un bloc de buts adjacents : son sol, ses murs, ses PORTES. Sans argument, balaie les 32 niveaux ; écrit `zone_nivNN_zK.xsb` (convention de caractères de l'app, donc rechargeable par `image`/`bench`/l'app) et imprime les coordonnées des portes. **L'outil ne calcule rien** : tout est dans `Game::zonesEmbut()` (§7, exemplaire unique), il ne fait que dessiner. Quatre étages : `sallesDeButs` (déjà là) → GOULOTS (≤ 2 voisins libres, jamais un but) + PORTES DOUBLES (paire fermée par des murs aux deux bouts **et qui coupe le plateau** — sans ce second test, le couloir large de 2 du niveau 20 se ferait trancher à chaque rangée) → ABSORPTION des culs-de-sac (seuil balayé : 4/5/6 identiques) → **RESSERRAGE PAR COUPE** de 1 ou 2 cases **pas forcément voisines** (la bouche du 21 est {(9,9),(10,8)}, deux cases en diagonale), quand rien ne sépare la salle du reste. **CALÉ SUR UNE VÉRITÉ TERRAIN** — découpage à la main par l'utilisateur, puis **relecture des 35 zones en PNG**, qui a réfuté un mécanisme de plus (cf. §6.0). Résultat : **35 zones sur 32 niveaux**, les 508 buts couverts, 3 niveaux en portent plusieurs (10, 18, 25) ; la plus étalée fait **30 %** de l'intérieur, médiane **19 %** |

| `ordredp <niv\|fichier.xsb> [--avant x,y,… --apres x,y,…]` | **(neuf, 2026-08-21, non commité) LE GOAL-ORDERING PROUVÉ, PAS DEVINÉ.** DP par sous-ensembles (schéma Held-Karp) sur `Game::butMureLocalement`, qui ne dépend que du SOUS-ENSEMBLE de buts posés, jamais de l'ordre — donc « un ordre sans murage existe-t-il » est une pure accessibilité dans le treillis des 2^n parties, mémoïsable. Contrainte dure = la seule précédence PROUVÉE (`precedenceGlobale`, via `PrecedencePaires::atteintUneCaisse`) ; l'alignement (indice) est exclu exprès. Rend un ordre témoin (**SAT**) ou une preuve d'impossibilité (**UNSAT**, tout le treillis atteignable épuisé — jamais un budget ambigu). Remplace directement le retour arrière à budget fixe de `ordreParPrecedence`, dont le budget de 500 s'est révélé insuffisant même sur l'ordre PAR DÉFAUT (13, 23 — §6.0, 2026-08-21). ⚠️ Coût O(2^n) : praticable jusqu'à ~24-27 buts (niveau 22 : 610 270 états, 196 s) ; hors de portée pour le niveau 10 (32 buts) sans décomposition par composantes connexes. ⚠️ **PROUVE UNE CONDITION NÉCESSAIRE, PAS SUFFISANTE** — un ordre SAT peut rester injouable, `butMureLocalement` ignorant les caisses non livrées comme obstacles (réfuté en direct sur le 22, cf. §6.0) |

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
- **Deadlock de LIVRAISON, dit « but orphelin »** (§6.1, 2026-07-20/21, **retiré entièrement le
  2026-08-18** en reprenant le code en main) : un but vide qu'aucune caisse ne peut plus atteindre.
  Six variantes mesurées par `mesures/fp.cpp`, **cinq en faux positif prouvé** (une caisse déjà
  posée tenue pour un obstacle fixe, alors que le vrai jeu autorise à la ressortir — cassait le
  canari du niveau 2, 131→133 poussées) et **la sixième, seule sûre, ne capture rien de plus que
  `staticDeadlock`**. Aucun réglage ne pouvait sauver l'idée : les deux défauts des variantes
  dangereuses sont structurels (BFS non joueur-aware, caisses posées en obstacles permanents), pas
  un curseur à ajuster. Retiré en entier : `Game::butNonLivrable`, l'interrupteur `LIVRAISON`
  (`game.cpp`, `solveurastar.cpp`), et les variantes correspondantes dans `mesures/fp.cpp` (qui
  garde ses modes corral/gate/précédence, désormais le défaut).

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

> 🎯 **SESSION DU 2026-08-21 — LE GOAL-ORDERING N'EST PLUS LE FACTEUR LIMITANT ; DEUX
> CHANTIERS NOUVEAUX, DISTINCTS DE LA LOI DE L'ORDRE.** Partie de la conviction
> utilisateur *« la loi de l'ordre aidera, mais il faut d'abord un goal-ordering le
> plus parfait possible »* — la session finit par la déplacer : on sait aujourd'hui
> produire un ordre quasi-parfait, et ça ne suffit toujours pas à faire avancer le
> solveur. **Rien n'est commité** ; fichiers neufs listés au point 7.
>
> **1. LE BUDGET DE 500 DU RETOUR ARRIÈRE (`Game::ordreParPrecedence`, game.cpp) EST UN
> BUG, PAS UN COMPROMIS.** La recherche protégée (§ session du 2026-08-20, `pileTri`)
> sature son budget sur le niveau 10 en régime `loi` et retombe sur l'ancien glouton
> murant — mais en instrumentant (`TRACE_ORDRE`, compteur `budgetTri` restant), monter
> le budget à ~4000 la fait **converger vers EXACTEMENT l'ordre injecté à la main la
> veille** ((17,2) au rang 28). **Pur manque de budget, zéro nouvelle règle.**
> ⚠️ **Plus grave : le MÊME bug touche l'ordre PAR DÉFAUT** (pas seulement `loi`) sur
> les niveaux **13 et 23** — `ordre 13`/`ordre 23` sans rien activer rendent un ordre
> **MURÉ** (13 au rang 11, 23 au rang 17), parce que la recherche sature à 500 même en
> régime par défaut et retombe sur le glouton non protégé. Monté à 50 000, toujours pas
> de convergence sur ces deux-là (cf. point 3). Ces deux niveaux ne sont dans aucun
> canari (non résolus), donc rien ne l'avait signalé.
>
> **2. OUTIL NEUF (non commité) `mesures/ordredp.cpp`/`.pro` — DP PAR SOUS-ENSEMBLES,
> REMPLACE LE « BUDGET ÉPUISÉ » AMBIGU PAR UNE PREUVE.** Constat qui le permet :
> `Game::butMureLocalement(h, bloque)` ne dépend QUE de l'ensemble des buts déjà posés
> (`bloque`), JAMAIS de l'ORDRE dans lequel on les a posés (position de départ du
> joueur fixe, géométrie fixe). Donc « existe-t-il un ordre total sans murage » est une
> pure question d'ACCESSIBILITÉ dans le graphe des 2^n sous-ensembles — le schéma de
> calcul de Held-Karp (DP du TSP/chemin hamiltonien), simplifié : pas besoin de retenir
> « le dernier visité », seulement « quels buts sont posés ». BFS mémoïsé sur le
> bitmask, contrainte dure = la seule précédence PROUVÉE (`precedenceGlobale`, rejouée
> via l'exemplaire unique `PrecedencePaires::atteintUneCaisse` déjà partagé avec
> `ordre.cpp` — la précédence par ALIGNEMENT, indice et non preuve, est exclue exprès :
> elle biaiserait vers un ordre, elle ne prouverait rien sur l'EXISTENCE d'un ordre
> sûr). Rend soit un ordre témoin (SAT), soit **UNSAT PROUVÉ** (tout le treillis
> atteignable épuisé, pas un budget). ⚠️ Coût O(2^n) : praticable jusqu'à ~24-27 buts
> (niveau 22, 27 buts, 610 270 états explorés sur 134 M au plafond, ~196 s ; accepte des
> contraintes manuelles `--avant x,y,... --apres x,y,...` pour tester une hypothèse
> humaine sans relancer tout l'outil). **Hors de portée tel quel pour le niveau 10** (32
> buts) — décomposition par composantes connexes non tentée.
>
> **3. RÉSULTAT DU DP SUR LES 13 NON-RÉSOLUS TESTABLES (≤27 buts) : 11 SAT, 2 UNSAT
> PROUVÉ.** SAT (un ordre sans murage existe, souvent en quelques centaines à quelques
> milliers d'états) : **13, 14, 15, 16, 19, 20, 22, 24, 25, 28, 29, 30, 31** — 21 et 32
> revérifiés SAT en contrôle. **UNSAT PROUVÉ** (tout le treillis atteignable épuisé,
> aucune permutation ne marche) : **18** (confirme le diagnostic déjà écrit au point 8
> de la session du 2026-08-20 — sa solution RESSORT des caisses de buts déjà remplis,
> aucune permutation ne peut l'écrire) et **23** (même famille, non documenté avant
> aujourd'hui). Conséquence : sur 13 niveaux, **le goal-ordering n'est le facteur
> bloquant que pour 2 d'entre eux** — pour les 11 autres, un ordre sûr existe déjà.
>
> **4. MAIS MURAGE-FREE ≠ JOUABLE — RÉFUTÉ EN DIRECT SUR LE 22, DEUX FOIS DE SUITE**
> (diagnostic utilisateur, jeu à la main). Le premier ordre SAT du DP bute : l'utilisateur
> ne peut plus poser (13,9) ni (12,9). Un second ordre, sous contrainte manuelle
> (« 10,9 12,9 13,9 avant 7,8 8,8 9,8 10,8 »), bute à nouveau : **« le joueur ne peut
> plus atteindre la case d'appui »**. Cause identifiée : `butMureLocalement` ne bloque,
> dans son flood-fill d'accessibilité, QUE les murs et les buts déjà remplis — **jamais
> les autres caisses non livrées qui traînent sur le plateau**, alors que dans le vrai
> jeu chacune est un obstacle. C'est déjà écrit dans le commentaire de `game.h` (« le
> vrai jeu a davantage de caisses en transit, donc une zone joueur PLUS PETITE que
> celle calculée ici ») — deux ordres de suite viennent de le vérifier en pratique. Le
> DP prouve une condition NÉCESSAIRE, jamais SUFFISANTE.
>
> **5. LE 22 A UNE VICTOIRE HUMAINE DÉJÀ ENREGISTRÉE, JAMAIS EXPLOITÉE.**
> `hybride_niveau_0022.txt` contient une partie gagnée (1592 coups, ligne 3770 : `[hybride]
> aucun but actif (etat gagne)`), jamais mentionnée dans les tableaux du plan (le 22 est
> classé non-résolu au sens `scores.md`, une victoire humaine ne compte pas). Rejouée et
> validée par `taches.rejoue` (script Python ad hoc, à rapatrier si ça resert — cf. §7 sur
> les scratchpads perdus), l'ORDRE DE LIVRAISON FINALE réel :
> `(7,9)(8,9)(9,9)(13,9)(12,9)(7,6)(8,6)(9,6)(13,6)(12,6)(11,6)(8,7)(9,7)(13,7)(12,7)(11,7)
> (9,8)(8,8)(7,8)(11,8)(12,8)(13,8)(14,8)(10,6)(10,7)(10,8)(10,9)`.
> Confirme la moitié de l'hypothèse utilisateur ((13,9)/(12,9) tôt) et la corrige sur
> l'autre : **(10,9) sort en TOUT DERNIER**, avec toute la colonne x=10 réservée à la fin
> et remplie de haut en bas — vraisemblablement le vrai corridor d'accès du joueur,
> gardé ouvert jusqu'au bout. Confirmé sans murage par `ordre 22` (contrôle de
> cohérence : un ordre réellement joué ne peut pas violer une preuve).
>
> **6. INJECTÉ DANS LE VRAI SOLVEUR : ÉCHEC INSTRUCTIF, PAS UNE IMPASSE DE NIVEAU.**
> `ordre_niveau_0022.txt` (non commité, l'ordre du point 5) + `bench 22 coupl-plongeon` :
> **zéro progression en 40 000 états** (`max` reste à 0/27) — même symptôme que le 16 en
> juillet, la même cause probable (§6.0 du 2026-08-20, point 6 : le régime d'engagement
> ne génère AUCUNE poussée simple tant qu'une macro est jouable ailleurs, donc les
> poussées de préparation d'une partie humaine longue n'existent pas dans son arbre).
> `bench 22 relegue` (le correctif du 16) démarre (`max` 0→2 en 3000 dépilements) puis
> **plafonne à 2/27 pendant tout le budget de 25 min** (10,2 M dépilements, 18,9 M états
> vus, 1,25 Go). **Diagnostic par export + `pas0` (pas une déduction) :** l'état à
> `max=2` n'a PAS livré (7,9)/(8,9) (les rangs 0-1 de l'ordre injecté) mais **(13,8) et
> (14,8)** (rangs 22-23, presque la fin) — la relégation n'IMPOSE rien, elle
> dé-priorise juste, donc les poussées simples reléguées partent n'importe où. Résultat
> visible sur l'image exportée : le joueur s'isole tout en haut du plateau, le corridor
> d'alimentation du bas se reconfigure en amas, et `pas0` confirme qu'**aucun but n'est
> plus amorçable** depuis cet état. **Ce n'est donc pas la congestion irréductible du
> §3/§4** qui plafonne ici — c'est la relégation qui laisse le solveur diverger du plan
> avant même d'avoir essayé de le suivre.
> ⚠️ **Piège retrouvé en le refaisant** : charger un `.xsb` exporté PAR CHEMIN met
> `numNiveau=0`, donc `cheminOrdreInjecte` cherche `ordre_niveau_0000.txt`, pas
> `_0022.txt` — sans copier le fichier sous ce nom, `pas0`/`ordre` recalculent un ordre
> par défaut sans rapport (§7, « charger une position de milieu de partie recalcule
> tout le statique », variante inédite : ça vaut aussi pour l'injection).
>
> **7. FICHIERS NEUFS, NON COMMITÉS** : `mesures/ordredp.cpp` + `.pro` (le DP, avec son
> option `--avant/--apres` de contrainte manuelle et son log de progression chronométré
> `QElapsedTimer`), `mesures/ordre_niveau_0022.txt` (l'ordre miné du point 5, prêt à
> être rejoué ou archivé dans `mesures/ordres_humains/`).
>
> **CE QUI RESTE À FAIRE — DEUX CHANTIERS, DISTINCTS ET NI L'UN NI L'AUTRE N'EST LA LOI
> DE L'ORDRE :**
> - ✅ **CHANTIER A — FAIT le 2026-08-23** (mémoïsation par sous-ensembles ; ni un
>   budget plus haut ni le DP branché, cf. le bloc du 2026-08-23). ~~**CHANTIER A (sûr, petit)** — corriger le budget de `ordreParPrecedence` : soit le
>   monter (combien ? le 10 convergeait à ~4000, le 13/23 pas même à 50 000 — donc une
>   constante plus haute ne suffit pas partout), soit brancher le DP du point 2 en
>   remplacement pour les niveaux où 2^n reste praticable (≤ ~24 buts), avec repli sur
>   la recherche à budget seulement au-delà. Gain immédiat attendu : 10 réparé sans
>   injection manuelle ; 13/23 à revérifier une fois la vraie précédence calculée (ils
>   pourraient rester murés pour une AUTRE raison, cf. UNSAT du point 3 — non, 13 est
>   SAT, seul 23 est UNSAT parmi les deux ; donc 13 devrait se réparer, 23 restera muré
>   quoi qu'il arrive).
> - **CHANTIER B (gros, ouvert, condition réelle pour juger la loi de l'ordre)** — un
>   régime qui **respecte** un ordre donné, pas seulement qui le calcule puis regarde
>   ailleurs dès qu'une poussée simple est nécessaire. Le point 6 prouve que ni le
>   régime par défaut (aucune poussée simple hors macro) ni `relegue` (poussées simples
>   libres, juste dé-priorisées) ne suffisent : il faudrait quelque chose comme
>   « dé-prioriser une poussée simple proportionnellement à son écart à l'ordre demandé »
>   plutôt qu'un `bonusF` fixe indifférent à la cible. Tant que ce chantier n'est pas
>   fait, on ne peut pas dire si un bon ordre (prouvé, ou miné d'une partie humaine)
>   AIDERAIT le solveur — on sait seulement qu'aujourd'hui, il ne peut même pas essayer
>   de le suivre.

> 🆕 **2026-08-22 — LES ZONES D'EMBUT SONT CODÉES, ET CALÉES SUR UNE VÉRITÉ TERRAIN.**
> Demande utilisateur : un algo qui sorte, pour chaque niveau, un `.xsb` par ZONE
> D'EMBUT — l'enclos qui enferme un bloc de buts adjacents, avec ses murs, ses portes,
> et **les caisses seulement si elles sont déjà posées sur un embut**. Codé dans le
> moteur (`Game::zonesEmbut()`, game.cpp) + outil `mesures/zonembut` (§1).
> - **LA MÉTHODE EST L'ACQUIS PRINCIPAL** : plutôt que de faire arbitrer trois
>   définitions d'« enclos » dans le vide, l'utilisateur a **découpé 7 zones à la main
>   sur 5 niveaux** (1, 10, 16, 20, 25, choisis pour leurs géométries opposées). C'est
>   un jeu de validation, au même titre que les juges FP du §1 — et il a réfuté deux
>   règles avant qu'elles soient écrites.
> - **RÉSULTAT : 5/7 identiques AU CARACTÈRE PRÈS**, et les 2 restants ne diffèrent que
>   par des `#` contre des espaces — **jamais un embut ni une caisse déplacés**. Les 7
>   enclos sont donc justes ; seul le choix des murs à DESSINER autour diffère (l'outil
>   garde les murs qui touchent la zone, diagonales comprises ; le tracé à la main en
>   garde parfois d'autres).
> - **`sallesDeButs` (2026-08-01) suffisait déjà pour le découpage en zones** : il rend
>   le bon compte sur les 5 niveaux. ⚠️ Y compris le piège du **20**, où les dents de
>   mur (16,5)/(16,7)/(16,9)/(16,11) donnent l'illusion de six paquets alors que la
>   colonne x=17 est continue — une seule salle, une seule zone.
> - **DEUX RÈGLES RÉFUTÉES PAR LA VÉRITÉ TERRAIN, chacune par un niveau précis** :
>   *couper aux points d'articulation seuls* (le 25 sépare ses deux zones par une paire
>   de cases, aucune n'étant un point d'articulation) et *couper à tout goulot* (le 25
>   prolonge au contraire sa zone le long d'un couloir jusqu'au mur qui le bouche).
>   D'où les deux mécanismes du code : le **prolongement collinéaire** et la **porte
>   double** (paire fermée par des murs aux deux bouts ET qui COUPE le plateau — sans
>   ce second test, le couloir large de 2 du niveau 20 se ferait trancher à chaque
>   rangée).
> - **Le seuil de cul-de-sac est BALAYÉ, pas choisi** (méthode CORRAL_BUDGET, §6.2) :
>   1-2-3 → 4/7, **4-5-6 → 5/7**, 8 → **les deux zones du 25 fusionnent**, ce que la
>   vérité terrain interdit. Défaut 4, au milieu du plateau 4-5-6.
> - **AMPLEUR : 34 zones sur les 32 niveaux.** Deux niveaux seulement en portent
>   plusieurs (10 : 28+4 buts ; 25 : 17+2), et **18/24/26 en portaient deux à tort** —
>   deux salles de buts qui partagent un seul enclos sont une seule zone, d'où la
>   fusion. **Le niveau 8 est le cas d'école de la porte double** : il fuyait sur 54 %
>   du plateau par une entrée large de deux cases, il rend sa salle de buts seule
>   depuis.
> - **LA RÈGLE D'ABSORPTION EST CHOISIE PAR L'UTILISATEUR** (2026-08-22) : *« ce qui est
>   BOUCHÉ appartient à la zone, ce qui DÉBOUCHE n'y appartient pas »*. C'est ce que le
>   code fait déjà — une poche est avalée, un débouché est une porte. **Quatre variantes
>   ont été essayées pour la rendre scale-free, et les quatre sont RÉFUTÉES par mesure**,
>   toutes sur le jeu des 8 zones :
>   1. poche avalée si plus petite que la zone COURANTE → s'emballe (la zone grandit, le
>      plafond grandit) : le 21 prend les **94 cases** du plateau, zéro porte ;
>   2. même test figé sur le CŒUR de la salle → même 94 cases (le cœur est déjà trop gros
>      quand rien ne sépare la salle), **et les deux zones du 25 fusionnent** ;
>   3. graine = les BUTS seuls, sans l'étage des fragments → **0/8** (les salles perdent
>      le sol qui les entoure) ;
>   4. goulot compté en IGNORANT les buts voisins → **4/8**, casse le 16, et ne répare
>      même pas le 21.
>   **Le seuil absolu reste donc, et il porte un fait** : une poche est petite dans
>   l'absolu (4 cases au 20), un débouché est le reste du plateau (44 au 1). Les deux
>   modes sont franchement séparés ; le seuil tombe entre les deux.
> - ✅ **LE 21 EST TRANCHÉ — RESSERRAGE PAR COUPE (règle (i), choisie par l'utilisateur).**
>   Sa salle de buts n'a **aucun rétrécissement** vers le milieu du plateau — le découpage
>   à la main y traçait une frontière en plein sol libre ((8,4) à (8,8) sont libres,
>   vérifié case par case). Le seul point de coupe est la paire **{(9,9),(10,8)}**, deux
>   cases **NON adjacentes** ayant chacune trois voisins libres : invisible pour le goulot
>   comme pour la porte double. D'où un quatrième étage : quand la zone est trop large,
>   chercher une coupe de 1 ou 2 cases (voisines ou non) qui isole les buts, et ne garder
>   que leur côté. L'utilisateur a préféré ce résultat (**16 cases**) à son propre croquis.
>   ⚠️ **Trois garde-fous, chacun réparant un cas mesuré** : la coupe ne porte jamais sur
>   un but ; tous les buts de la salle restent du même côté ; **l'autre côté doit être
>   substantiel** — sans quoi la paire {(14,7),(15,6)} du niveau 1 « coupe » en isolant le
>   seul coin (14,6), et les salles se font charcuter par leurs propres angles.
> - ❌ **LE PROLONGEMENT COLLINÉAIRE EST RETIRÉ — le jour même où il avait été ajouté.**
>   Il courait tout droit jusqu'au mur suivant pour reproduire une zone du niveau 25.
>   **C'est la RELECTURE EN IMAGES qui l'a tué** : l'utilisateur a regardé la planche
>   contact et a signalé six anomalies d'un coup — le 2 à droite, le 9 à gauche, le 14 en
>   haut et en bas, le 15 à droite, le 25 à gauche, et le **18 rendu en une zone au lieu
>   de deux**. Toutes le même mécanisme : il embarquait le COULOIR DE SORTIE des salles.
>   Bilan mesuré sur les 8 zones de vérité terrain : **il en gagnait UNE et en perdait
>   cinq ailleurs**. Retiré sans interrupteur — un `qgetenv` planqué dans le moteur est le
>   piège §7. Seul reliquat assumé : la petite zone du 25 rend 3 cases au lieu des 5 du
>   croquis, ce que l'utilisateur a validé en images.
>   🎯 **LA LEÇON, et elle vaut au-delà de ce chantier** : le découpage à la main avait
>   validé ce mécanisme, la relecture en IMAGES l'a réfuté. Un jeu de validation de 8 cas
>   ne voit pas ce que 35 vignettes montrent d'un coup d'œil. **L'ASCII cachait ce que le
>   dessin a rendu évident** — même leçon que `mesures/image` en 2026-08-14 (« la
>   géométrie du 12 a été mal lue trois fois de suite »).
> - **AMPLEUR, ÉTAT FINAL : 35 zones sur les 32 niveaux, les 508 buts couverts**, trois
>   niveaux en portant plusieurs (10 : 28+4 · 18 : 4+7 · 25 : 17+2). Le resserrage avait
>   touché 5 zones (12, 13, 19, 21, 22), le retrait du prolongement en a resserré six
>   autres et **rendu au 18 ses deux zones**. L'étalement maximal est descendu de **55 %
>   → 38 % → 30 %** de l'intérieur (médiane 19 %) : les 35 zones sont enfin de la même
>   famille, ce qui était le critère de l'utilisateur (« le plus logique par rapport aux
>   autres »).
> 🎯 **2026-08-22 (suite 3) — L'ORDRE PROUVÉ, LA PORTE DE CHAQUE EMBUT, ET LE QUOTA
> PAR PORTE.** Objectif posé par l'utilisateur : *« trouver enfin un ordre solvable, et
> déterminer par quelle porte quel embut doit être rempli »*, avant de s'attaquer au
> démêlage. `zonembut <niv> ordre` (ou `ordre` seul pour les 32) **résout le niveau
> simple de chaque zone et LIT sa solution** — rien n'est déduit :
> - **l'ORDRE est PROUVÉ JOUABLE** puisqu'il vient d'une partie gagnée. C'est l'autre
>   moitié de l'asymétrie du §6.6 : `precedenceGlobale`/`butMureLocalement` disent ce qui
>   est INTERDIT (condition nécessaire), une solution dit ce qui MARCHE ;
> - **la PORTE de chaque embut**, et donc **le QUOTA par porte** — combien de caisses le
>   démêlage devra livrer de quel côté. C'est le cahier des charges de l'étape suivante.
> **RÉSULTAT : 25 zones sur 35**, sortie en `ordre_nivNN_zK.txt`. Les 10 restantes sont
> toutes en BUDGET dépassé, aucune insoluble.
> - ⚠️ **LES ZONES D'UN MÊME NIVEAU SONT INDÉPENDANTES** (constat utilisateur) : jamais
>   d'ordre global, un jeu de données par zone.
> - **La mesure valide la lecture humaine** : sur la zone 0 du 18, l'utilisateur avait dit
>   à la main « la porte du haut-gauche ne sert qu'au personnage, 3 caisses sur 4 passent
>   par celle de droite ». Le quota extrait est **(11,2)x3 (7,1)x1**.
> - Deux quotas instructifs : le **16** compte 14 pour 15 embuts (la 15ᵉ caisse est posée
>   au départ) ; le **10 zone 1** a deux portes et n'en utilise qu'UNE.
>
> 🧱 **LES 10 ZONES SANS ORDRE — CE QUI A ÉTÉ ESSAYÉ, ET CE QUI RESTE OUVERT.**
> - **Budget ×10 (30 s → 300 s par zone) : AUCUN gain.** 12, 13, 15, 20, 22, 25.1, 26,
>   28, 29, 32 rendent toujours rien. Cohérent avec le balayage A\* pur, où passer de
>   60 s à 240 s n'avait débloqué aucun niveau non plus. **On n'est pas au bord du mur,
>   on est loin derrière** — inutile d'allonger.
> - 🎯 **LES CAISSES DÉJÀ POSÉES SONT UN POINT DUR, ET LA MESURE EST NETTE** (question
>   utilisateur : *« il faut déplacer les caisses déjà en place, est-ce que le solveur
>   sait faire ça ? »*). Expérience : la zone du **15** privée de ses deux caisses posées
>   — mêmes embuts, même nombre de caisses, deux d'entre elles remises dans le couloir —
>   se résout en **171 états** ; avec elles, `coupl-plongeon` **ÉPUISE son espace**.
>   Trois zones du corpus en contiennent : 15 (2), 16 (1), 29 (5). **Le 15 et le 29 sont
>   sans ordre ; le 16 en a un.**
> - ⚠️ **MAIS L'EXPLICATION SIMPLE EST RÉFUTÉE** (scepticisme de l'utilisateur, vérifié
>   sur pièces). J'avais conclu « le régime macro ne ressort jamais une caisse d'un but,
>   §6.0 point 8 ». **Faux** : l'ordre extrait du 16 montre que sa caisse posée (9,10)
>   finit en (12,7) — donc elle bouge, et le régime a su le faire. Ce que le plan
>   documente est plus fin et CONDITIONNEL : « une macro engagée, le solveur ne génère
>   aucune poussée simple ». Le 16 tombe du bon côté, le 15 du mauvais.
> - ⚠️ **Un fait qui affaiblit encore l'explication** : sur le 15, `coupl-plongeon`
>   ÉPUISE l'espace là où `macro` SEUL ne l'épuise pas (il manque juste de temps). Les
>   deux portent pourtant le même moteur macro — c'est donc le **couplage ou le plongeon**
>   qui referme l'espace, pas seulement l'engagement. À creuser indépendamment.
> - **`relegue` essayé** (le correctif du §6.0 point 6, qui dé-priorise au lieu de
>   supprimer) : rien en 300 s. Non concluant, ni confirmation ni réfutation.
> ❌ **L'ORDRE PROUVÉ N'APPORTE RIEN — ET C'EST LE RÉSULTAT LE PLUS UTILE DU JOUR.**
> Les 25 ordres extraits ont été convertis au format d'injection (`ordre_niveau_XXXX.txt`,
> §1 — le parseur lit des paires `(x,y)`, il a fallu ne garder que le but et jeter la
> porte) et injectés dans les VRAIS niveaux. 22 niveaux avaient un ordre complet (toutes
> zones couvertes). Même binaire, même régime `coupl-plongeon`, budget 90 s, une seule
> variable.
> **Sur les 10 comparaisons concluantes : 9 IDENTIQUES À L'UNITÉ** (1, 2, 3, 4, 5, 7, 8,
> 9, 17 — mêmes états, mêmes poussées) **et la dixième diffère de 2 états** (le 6 :
> 570 → 568). Aucun niveau gagné.
> 🎯 **L'ordre extrait d'une PARTIE GAGNÉE coïncide avec celui que `ordreParPrecedence`
> CALCULE.** C'est une validation croisée par une voie totalement indépendante — une
> solution d'un côté, une précédence déduite de l'autre — de tout le travail sur l'ordre
> depuis juillet. Et surtout : **sur ces niveaux, l'ordre n'était pas le problème.** Le
> goulot est ailleurs (démêlage, ou génération des macros — cf. ci-dessous).
> - ⚠️ **La régression du 10 est un ARTEFACT DU PROTOCOLE, pas de l'ordre.** Le 10 a deux
>   zones et le format d'injection est un ordre TOTAL : la concaténation impose « remplir
>   entièrement la grande salle, PUIS la petite », alors que les zones sont INDÉPENDANTES
>   (constat utilisateur) et que leur entrelacement est libre. J'ai ajouté une contrainte
>   que la donnée ne porte pas — sur le niveau même où le §6.2 documente un correctif
>   multi-salles valant ×7,5. **Le mécanisme d'injection ne sait pas exprimer des ordres
>   indépendants ;** c'est sa limite, à corriger avant tout usage sur un multi-zones.
> - ⚠️ **11 niveaux sur 22 ne concluent d'aucun côté à 90 s** : la mesure ne dit RIEN sur
>   eux, et ne doit pas être lue comme si elle disait quelque chose.
>
> 🔬 **LA PARTIE HUMAINE DU 200, PASSÉE AU JUGE — TROIS SUSPECTS ÉLIMINÉS.**
> L'utilisateur a rejoué `level0200.xsb` en mode HYBRIDE et l'a GAGNÉ : 363 coups,
> 94 poussées, 15/15, dont **133 macros et seulement 7 poussées choisies à la main**.
> Outil neuf **`mesures/rejeu`** (+`.pro`) : rejoue la dernière partie d'un journal
> hybride et interroge le moteur à chaque état — protocole du juge FP (§1), sur une
> partie gagnée toute détection est un faux positif prouvé.
> - **0 faux positif de défaite** sur les 363 états. L'élagage des deadlocks est hors de
>   cause.
> - **0 `HORS REGIME MACRO`, 0 `⚠ ECARTE`** sur les 7 poussées manuelles : elles sont
>   toutes générables par le solveur et aucune n'est élaguée.
> - ❌ **`remplissageOrdonne` est DU CODE MORT** — déclarée dans `game.h:155`, appelée
>   NULLE PART dans `solveurastar.cpp`. Elle rejetterait pourtant ce chemin gagnant dès
>   le coup 1 (362 états sur 363 en violation), parce que les deux caisses PRÉ-POSÉES ne
>   sont pas sur les buts les plus profonds. À savoir si quelqu'un la réactive.
> - ❌ **LE COUPLAGE EST DISCULPÉ**, après l'avoir accusé à tort. L'isolation montrait
>   `couplage` et `coupl-plongeon` qui ÉPUISENT quand `macro` et `plongeon` ne concluent
>   pas — mais la lecture du code interdit qu'il coupe : `getHeuristique` ne sert qu'à
>   `f = g + poids·h` (aucun test sur `INF_COUPLAGE`), et le score de guidage n'est
>   qu'un DÉPARTAGE à `f` et `g` égaux (solveurastar.cpp:23). Un minorant ne retire pas
>   d'état, il réordonne. `couplage` épuise plus vite, voilà tout — une meilleure `h`
>   réexpanse moins. ⚠️ **Non vérifié empiriquement** : `macro` seul n'a pas épuisé en
>   600 s, la confirmation manque.
> ~~🎯 **CE QUI RESTE, ET C'EST LE VRAI RÉSULTAT** : l'espace engendré par le RÉGIME
> MACRO ne contient pas de solution pour cette zone [...] il est dans la **GÉNÉRATION DES
> MACROS elles-mêmes**.~~
> ❌ **RÉFUTÉ le 2026-08-23 — LE TROU ÉTAIT DANS L'ORDRE, et c'est une leçon de méthode
> coûteuse.** Trois suspects avaient été éliminés un par un (élagage, relégation,
> couplage) et la conclusion est tombée sur le quatrième **par élimination, sans jamais
> le tester**. Or l'ordre par défaut du 200 est MURÉ au rang 13 — `ordre level0200.xsb`
> le dit en une seconde, et cet outil existait depuis le 2026-07-29. **Le suspect qui
> reste après élimination n'est pas prouvé coupable** : il faut encore l'interroger, et
> c'était ici le moins cher des quatre. Même famille que le §7 (« nommer les variables
> AVANT de lire un écart ») : ici c'est un mécanisme non mesuré qui a hérité de la
> charge, faute d'avoir vérifié qu'on avait bien épuisé la liste.
> ⚠️ **Et l'instrumentation ne le voit pas** : le journal hybride confronte le solveur
> aux poussées choisies À LA MAIN (`⚠ ECARTE`), jamais aux MACROS jouées. Sur cette
> partie, 133 coups sur 140 échappent donc au juge. **Étendre `⚠ ECARTE` aux macros est
> le prochain pas**, et il est petit.
>
> 🎯 **2026-08-23 — LE CHANTIER A EST FAIT, ET LE 200 TOMBE EN 54 ÉTATS. LE BUDGET DE
> 500 N'ÉTAIT PAS TROP PETIT : LA RECHERCHE ÉTAIT EXPONENTIELLE POUR RIEN.**
> Parti d'un constat utilisateur — *« l'ordre qu'il utilise n'est pas bon ; celui du
> régime align est bon, mais ça ne résout pas non plus »* — et confirmé statiquement
> en une seconde par un outil qui existait depuis juillet :
>
> ```
> ❌ ORDRE MURÉ au rang 13, sur le but (7,7).
>    13. but (7,7)  approches viables=2  libres=0  <<< MURÉ
>    14. but (8,7)  approches viables=2  libres=0  <<< MURÉ
> ```
>
> **LA CAUSE, et elle n'est pas celle que le §6.0 du 2026-08-21 avait retenue.** Ce
> paragraphe proposait de « monter le budget » ou de « brancher le DP », après avoir
> mesuré que 50 000 ne suffisait toujours pas sur les 13 et 23. Le vrai défaut est
> ailleurs : tout ce dont dépend la suite de la recherche — `pret`, `pretPreuve`,
> `mureraitQuelquun`, donc `butMureLocalement` — est fonction du seul **ENSEMBLE** des
> buts déjà émis, jamais de l'ORDRE dans lequel on les a posés. C'est **exactement le
> constat qui fonde `mesures/ordredp`** (§6.0, point 2), mais il n'avait été exploité
> que pour bâtir un outil SÉPARÉ. Appliqué à la recherche existante, il ne demande pas
> un DP : il demande **une table**. Sans elle, un même sous-ensemble était redescendu
> `k!` fois et le budget partait là-dedans.
> - **Le correctif tient en trois lignes de logique** (`game.cpp`,
>   `ordreParPrecedence`) : un `QSet<quint64>` des sous-ensembles prouvés stériles ; on
>   n'y redescend plus, et surtout **on ne paie plus de budget pour eux**.
> - ⚠️ **La clé porte AUSSI `salleCourante`**, et l'oublier serait un vrai bug : deux
>   chemins menant au même sous-ensemble peuvent laisser le joueur dans des salles
>   différentes, et les passes 1/2 classent les candidats d'après elle. Avec la salle,
>   les deux états sont rigoureusement équivalents ; sans elle, on couperait des
>   branches valides.
> - ⚠️ **On ne mémoïse que l'ÉPUISEMENT RÉEL des candidats d'un étage, jamais un abandon
>   par budget.** Marquer « échoué » un sous-ensemble qu'on a seulement cessé d'explorer
>   ferait rater un ordre sain, et le murage reviendrait **sans que rien ne le signale**.
>   C'est la distinction UNSAT / budget que le §6.0 exigeait déjà de `ordredp`.
> - Le budget passe de 500 à **5 000**, en garde-fou dur. ⚠️ **La constante n'est PAS ce
>   qui répare** : à 500 la mémoïsation suffit déjà pour le 200 ; le 13, lui, demande les
>   deux — et **sans mémoïsation, 100 000 ne le répare pas** (mesuré). C'est la preuve
>   séparée que le levier est la table, pas le chiffre.
>
> **RÉSULTAT, canari d'ordre sur les 35 plateaux (binaire contre binaire) :**
>
> | | |
> |---|---|
> | ordres qui changent | **2 — le 13 et le 200**, tous deux **MURÉ → sain** |
> | les 34 autres | **identiques au caractère près** |
> | murages restants | **18 et 23 seulement** — soit **exactement les deux UNSAT PROUVÉS** du DP (§6.0 point 3) |
> | temps CPU de chargement (10, 13, 22, 23, 24, 200) | **inchangé**, dans le bruit (le 22 reste le pire à 4,6 s) |
>
> 🎯 **ET LE MURAGE PAR DÉFAUT DU CORPUS EST DÉSORMAIS EXACTEMENT L'ENSEMBLE DES UNSAT
> PROUVÉS.** Il ne reste plus un seul niveau muré par défaut de `ordreParPrecedence` : le
> 18 et le 23 le sont parce qu'aucune permutation ne marche, ce qui n'est plus un défaut
> d'algorithme mais une propriété du plateau. C'est la première fois que les deux
> lectures — la recherche du moteur et le DP de `mesures/ordredp` — **coïncident sur tout
> le corpus**.
>
> ✅ **`level0200.xsb` EST RÉSOLU : `etats=54 poussees=76 coups=335`**, ordre CALCULÉ
> (aucune injection), régime `coupl-plongeon`. Le banc d'essai que le plan tenait pour
> « hors de portée du solveur » depuis le 2026-08-22 tombe en 54 états dès que son ordre
> n'est plus muré. **Tout ce que ce document a écrit sur le 200 comme angle mort de la
> GÉNÉRATION DES MACROS est réfuté** (corrections apposées plus bas, sur les trois
> paragraphes concernés).
> - ⚠️ **Le 13 n'est PAS résolu pour autant** : ordre réparé, il rend `max 9/16` sans
>   conclure — la même valeur qu'au profilage de juillet. Un ordre sain est nécessaire,
>   il n'est pas suffisant (§6.6, l'asymétrie). Ne pas lire « 2 ordres réparés » comme
>   « 2 niveaux gagnés » : c'est **1 gagné, 1 débloqué à mi-chemin**.
> - ⚠️ **Ce que ça dit du juge de macros écrit le même jour** : ses 19 HORS PASSE
>   COUPLAGE restent un fait mesuré, mais **le 200 ne les portait pas comme cause**.
>   Le juge a bien fait son travail (0 ECARTE, 0 INTROUVABLE, il a disculpé l'élagage) ;
>   c'est l'interprétation qui allait trop vite, deux fois de suite, avant que le test
>   statique ne tranche.
> - ⚠️ **Et l'ordre align, lui, ne résout toujours pas** (constat utilisateur, vérifié) :
>   injecté par fichier, `coupl-plongeon` et `plongeon` rendent tous deux `max 11/15` en
>   600 s sans conclure — moins loin que l'ordre par défaut réparé, qui gagne. Un ordre
>   prouvé jouable par une partie humaine n'est donc pas le meilleur ordre pour le
>   solveur : deuxième illustration de l'asymétrie du §6.6, et la plus nette.
>
> **PÉRIMÈTRE** : `game.cpp` seul (`ordreParPrecedence` — mémoïsation + budget 5 000).
>
> 🔬 **LA RELANCE, CIBLÉE — ET SON BILAN EST MAIGRE, IL FAUT LE DIRE.** Le réflexe
> « relancer les 15 non-résolus » a été ÉCARTÉ par l'utilisateur, et il avait raison :
> pour 14 d'entre eux l'ordre est identique au caractère près et le code aussi, donc le
> run rejoue une trajectoire déterministe déjà mesurée. Un non-résolu n'a même pas de
> ligne dans `scores.md` — il n'y a pas de chiffre à rafraîchir. **Ce qui a rendu la
> relance utile, c'est de chercher D'ABORD où l'ordre change**, et pas seulement dans le
> régime par défaut :
>
> | régime | ordres qui changent |
> |---|---|
> | défaut | 13, 200 |
> | **align** | **10, 13, 20** |
> | look | 13, 200 |
>
> Trois runs, pas quinze. ⚠️ L'ordre align est **injecté par fichier** et le régime reste
> `coupl-plongeon` : armer `loi` armerait AUSSI `caseMorteLoi` (réfutée), soit deux
> variables — le piège du 2026-08-19.
> - ✅ **10 : `etats=571053 poussees=544 coups=1563`** — le chiffre du 2026-08-19 **à
>   l'unité près**, mais sans injection manuelle. **Le point 🔴 « l'ordre gagnant du 10
>   est INJECTÉ À LA MAIN » est CLOS** ; `mesures/ordres_humains/ordre_niveau_0010_appui-17-2.txt`
>   devient une archive, plus un outil de chantier. ⚠️ Toujours **2,3× au-dessus du
>   défaut** (249 913) : une injection automatisée, pas un gain.
> - ❌ **20 : `max 1/18` en 900 s**, ordre align réparé — exactement la valeur du
>   profilage de juillet. Murage levé, le niveau ne démarre toujours pas.
> - ❌ **13 : `max 9/16`**, dans les TROIS régimes (défaut, look à 20,8 M états/900 s) —
>   là aussi la valeur de juillet, inchangée.
>
> 🎯 **CONCLUSION, et elle vaut mieux que le gain** : la correction fait tomber **un banc
> d'essai (200) et zéro niveau du corpus**. Les deux non-résolus dont l'ordre était muré
> — 13 et 20 — ne bougent pas d'un but une fois réparés. C'est la **troisième et la plus
> nette illustration de l'asymétrie du §6.6** : un ordre sain est nécessaire, il n'est
> jamais suffisant. Et ça confirme par l'expérience ce que le DP annonçait au §6.0
> (point 3) : *« sur 13 niveaux, le goal-ordering n'est le facteur bloquant que pour 2
> d'entre eux »* — on sait maintenant que même pour ces 2, le réparer ne suffit pas.
> **Le goal-ordering est donc CLOS comme piste de déblocage** : il ne reste plus un seul
> niveau muré par défaut hors des deux UNSAT prouvés, et aucun des murages levés n'a
> rendu un niveau.

> ✅ **2026-08-23 — LE JUGE EST ÉTENDU AUX MACROS, ET IL NOMME LE DÉSACCORD DE
> GÉNÉRATION.** Le point 1 de « OÙ REPRENDRE » ci-dessous est FAIT. Le journal
> hybride ne confrontait le solveur qu'aux poussées choisies à la main ; sur la
> partie du 200, 133 coups sur 140 y échappaient. `jugeMacro` (**`jugemacro.h`**,
> racine — en **exemplaire unique** §7, appelé par l'UI *et* par `mesures/rejeu`)
> rejoue les deux passes de `tenteMacro` puis l'enfilage, à l'identique : régime du
> couplage, corral unitaire, corral-N au même `CORRAL_BUDGET`, et la clé du
> comparateur (f croissant, **g décroissant**, guidage).
> - **Ce que l'écran ne disait pas, et qui est tout le sujet.** `majMacrosJouables`
>   cercle une caisse dès que sa descente aboutit. Le solveur en fait DAVANTAGE, et
>   d'abord par le **RÉGIME DU COUPLAGE** : la passe 0 ne tente que
>   `caisseAssignee(but)` et, si SA descente aboutit, `break` — les autres caisses ne
>   sont **jamais tentées**. Une macro cerclée en vert peut donc n'exister dans aucun
>   état de l'arbre du solveur. Le commentaire de `solveurastar.cpp` dit « ce régime
>   ne RETIRE aucune branche, il en PRÉFÈRE une » : c'est vrai de l'espace ATTEIGNABLE,
>   c'est faux **à un état donné**, et c'est cette distinction que le juge mesure.
> - ⚠️ **Nuance de fidélité reproduite telle quelle** : `macrosOk` compte, dans le
>   solveur, les descentes qui ABOUTISSENT et non les enfants réellement enfilés
>   (l'incrément suit `enfiler()`, qui a pu élaguer). Une passe 0 dont l'unique enfant
>   meurt au corral coupe donc quand même la passe de repli.
> - ⚠️ **Le `g` n'est PAS commun aux frères** ici, contrairement aux poussées simples :
>   une macro enfile à `g + (nombre de poussées)`. C'est bien Δf, jamais Δh, qui se
>   transporte à la file — et les trois clés du comparateur mordent, là où le juge des
>   poussées simples n'en avait que deux.
>
> **LA MESURE, sur les 16 parties GAGNÉES en banque — 216 macros, aucun rejeu à la
> main** (les journaux étaient déjà là ; c'est l'outil qui manquait) :
>
> | verdict | compte | |
> |---|---|---|
> | **HORS PASSE COUPLAGE** (jamais générée à cet état) | **19 / 216 (8,8 %)** | sur **8 niveaux sur 16** |
> | **ECARTE** (faux positif d'élagage prouvé) | **0** | le corral est hors de cause |
> | **INTROUVABLE** (miroir en défaut) | **0** | l'overlay et le juge concordent partout |
> | retenues | 196 | **rang 1 dans 158 cas (81 %)**, rang moyen 1,3, pire 7 |
>
> 🎯 **QUAND LE SOLVEUR GÉNÈRE LA MACRO HUMAINE, IL LA CLASSE EN TÊTE** — 81 % de
> rang 1, `df = 0` presque partout. **Le guidage n'est donc pas le problème ; la
> génération l'est.** C'est la même forme de résultat que la session 2/7 du
> 2026-08-01 sur les poussées simples (« il ne peut pas départager »), et elle pointe
> dans la même direction : ce qui manque n'est pas un tie-break de plus.
> - **Le 200 et le 11 portent les taux les plus hauts** (26,7 % et 28,6 %) — et le 200
>   est justement le banc que l'humain gagne et que le solveur ne résout pas. ⚠️ **Deux
>   points ne font pas une loi** (§11.4, piège tombé en direct sur le mou) : le taux
>   **ne discrimine pas** résolus et non-résolus (le 3, le 4, le 26 sont résolus et en
>   portent ; le 20, le 25, le 13, le 15 sont non résolus et n'en portent aucun).
>   Ce n'est PAS le prédicteur que le §6.6 réclame.
> - ⚠️ **Et ce n'est pas une preuve d'incomplétude.** Le juge dit « à CET état, cette
>   branche n'est pas dans l'arbre » ; le solveur peut y revenir par un autre chemin.
>   L'incomplétude, elle, reste prouvée par l'autre bout (le 16 : `AUCUNE`, espace
>   ÉPUISÉ, §6.0 point 6). Ce que le juge apporte est le **mécanisme nommé et
>   localisable**, pas un théorème de plus.
> - ⚠️ **Deux étages ne sont PAS rejoués** et un verdict se lit avec : `loiTropTot`
>   (propre au régime `loi`, réfuté) et la **dédup `meilleurG`**, qui demande
>   l'historique d'un run entier. Un `rang` dit « le solveur enfilerait ceci ici », pas
>   « il le développerait ».
>
> 🔬 **L'EXPÉRIENCE QUE LA MESURE DÉSIGNAIT, FAITE LE JOUR MÊME — ET ELLE RÉFUTE
> L'INFÉRENCE FACILE.** Le couplage étant déjà un interrupteur (`plongeon` contre
> `coupl-plongeon` : macro + plongeon des deux côtés, le couplage pour seule variable),
> le tester ne demandait aucun code. Sur `level0200.xsb` :
>
> | régime | états | verdict | progression |
> |---|---|---|---|
> | `coupl-plongeon` | **18 630 enfilages** | **`AUCUNE`** | **`max 13/15`** |
> | `plongeon` (sans couplage) | 7,3 M vus (900 s, non conclu) | budget | **`max 13/15`** |
>
> - ✅ **Le couplage TRONQUE, c'est confirmé** : avec lui l'espace est ÉPUISÉ en
>   18 630 enfilages sur un niveau dont l'utilisateur possède une solution. C'est un
>   `AUCUNE` de plus qui ne prouve rien (§6.0, 2026-08-22), et on sait maintenant par
>   quel mécanisme. Ce n'est pas non plus le corral : **`PRUNES=0`, 183 durs jugés,
>   0 mort** sur ce run — cohérent avec les 0 ECARTE du juge.
> - ❌ **MAIS IL N'EST PAS LE VERROU DU 200, et c'est le résultat le plus utile.** Les
>   deux régimes plafonnent **au même endroit, 13/15**. Retirer le couplage transforme
>   un `AUCUNE` en un budget — il ouvre l'espace sans faire progresser d'un seul but.
>   Le mur du 200 est donc dans ses **deux derniers buts**, pas dans la troncature de
>   génération que le juge mesure.
> - ⚠️ **J'avais écrit l'inverse une heure plus tôt** (« le run confirme à l'échelle de
>   l'espace entier »), sur la seule foi du `AUCUNE` contre le non-épuisement — sans
>   avoir lu la progression, que `2>/dev/null` avalait. C'est **le piège de la jauge du
>   §1**, pris sur le fait : la ligne qui réfutait la conclusion partait sur `stderr` et
>   je l'avais jetée. Deux régimes qui diffèrent par leur mode de sortie peuvent être
>   identiques par ce qui compte.
>
> 🔬 **CE QUE ÇA OUVRE, dans l'ordre.** (1) **Le 200 devient un cas mieux posé** : deux
> régimes indépendants s'arrêtent à 13/15, donc la question n'est plus « pourquoi le
> solveur ne démarre pas » mais « qu'y a-t-il aux deux derniers buts ». L'état à 13/15
> s'exporte (`bench … record`) et se passe à `pas0`/`mort` — c'est le protocole qui a
> chiffré le plongeon avant de le coder. (2) **Le juge est prêt pour le corpus entier**
> dès qu'une partie gagnée de plus est enregistrée, et il ne coûte rien à relancer.
> (3) Le HORS PASSE COUPLAGE reste un fait mesuré à expliquer sur les 7 autres niveaux
> touchés — mais après le 200, et sans présumer qu'il y pèse davantage.
>
> ⚠️ **PIÈGE ÉVITÉ DE JUSTESSE, et il vaut pour tout rejeu de journal** : la partie du
> 200 a été jouée sous l'**ordre align**, et `rejeu` recalculait l'ordre par défaut. Une
> macro visant `butActif()`, **11 verdicts sur 15 étaient faux — et silencieusement**,
> puisque le rejeu des COUPS, lui, marchait parfaitement. Corrigé en lisant la SOURCE de
> l'ordre dans l'en-tête que l'UI écrit exprès (`calcule` / `⚠ PAR ALIGNEMENT` /
> `⚠ INJECTE depuis …`), plus un garde qui refuse de juger quand le but du journal et
> `butActif()` divergent. C'est le §7 (« charger une position recalcule tout le
> statique ») appliqué au journal — et la démonstration que l'annotation de source
> ajoutée le 2026-08-19 « pour la relecture humaine » servait en fait à une machine.
> Le 26 en porte encore une trace (1 macro non jugée, désync à mi-partie).
>
> **PÉRIMÈTRE** : `jugemacro.h` (neuf), `mainwindow.h`/`mainwindow.cpp`
> (`mesureRangMacro`, appelée depuis `joueMacro` — ligne `[macro-rang]` dans le journal),
> `mesures/rejeu.cpp`. **Aucun fichier du solveur n'est touché** et rien n'appelle
> `jugeMacro` depuis un chemin du moteur : le canari ne peut pas bouger, et il n'a donc
> pas été relancé — c'est dit plutôt que sous-entendu.

> 📌 **OÙ REPRENDRE (2026-08-23, fin de session) — remplace la liste du 2026-08-22
> juste en dessous, dont le point 1 est FAIT et dont le point 3 change de statut.**
> 1. **LES DEUX DERNIERS BUTS, PAS LE DÉMARRAGE.** Le 20 reste à `max 1/18` et le 13 à
>    `max 9/16` **ordre sain**, donc la question « pourquoi ne démarre-t-il pas » n'a plus
>    d'issue du côté de l'ordre. Le protocole qui a chiffré le plongeon AVANT de le coder
>    s'applique ici : exporter l'état de record (`bench <niv> <regime> record`), le passer
>    à `pas0` (quelle macro manque, et pourquoi) et à `mort` (l'état est-il seulement
>    vivant ?). **Un état qu'on regarde vaut mieux qu'un budget qu'on allonge.**
> 2. **CHERCHER UN PRÉDICTEUR** pour les 8 zones sans ordre et sans caisse pré-posée
>    (12, 13, 20, 22, 25.1, 26, 28, 32) — inchangé, et c'est la seule ligne vide du
>    tableau du §6.6. Hypothèse à VÉRIFIER : le rapport embuts/porte.
> 3. **LE DÉMÊLAGE**, avec les quotas par porte comme cahier des charges — inchangé, et
>    désormais **la seule grande piste restante** : le §3/§4 le dit irréductible, le
>    goal-ordering vient d'être clos, et la mémoire était un plafond, pas le problème.
> 4. Les **19 HORS PASSE COUPLAGE** du juge de macros restent un fait mesuré sans
>    explication, sur 7 niveaux autres que le 200. ⚠️ **Ne pas en refaire le suspect
>    principal** : c'est exactement ce qui vient d'être payé cher (§7, « le suspect qui
>    reste après élimination »). À traiter comme une observation à expliquer, après 1 et 2.
> ❌ **CE QU'IL NE FAUT PAS REFAIRE** : relancer avec plus de budget (réfuté ×4 et ×10 le
> 2026-08-22, puis ×2 sur le 13 le 2026-08-23) — **et relancer un corpus que la promotion
> ne touche pas** (§7, 2026-08-23 : chercher d'abord OÙ le changement mord).

> 📌 **OÙ REPRENDRE (2026-08-22, fin de session).** Par ordre de rendement attendu :
> 1. ✅ **FAIT le 2026-08-23 — voir le bloc ci-dessus.** ~~**ÉTENDRE `⚠ ECARTE` AUX MACROS** dans le journal hybride.~~ Aujourd'hui le juge ne
>    confronte le solveur qu'aux poussées choisies À LA MAIN : sur la partie du 200,
>    **133 coups sur 140 échappent au contrôle**. L'étendre dirait, en UNE partie, quelle
>    macro le solveur refuse de produire et à quel état. C'est le seul chemin connu vers
>    le fond du problème, et il se mesure sur 13×12 au lieu du niveau 18 complet.
> 2. **CHERCHER UN PRÉDICTEUR** pour les 8 zones sans ordre et sans caisse pré-posée
>    (12, 13, 20, 22, 25.1, 26, 28, 32). Hypothèse à VÉRIFIER, pas à supposer : le rapport
>    embuts/porte (20 embuts pour 1 porte sur le 28, 18 sur le 20). Ce serait le premier
>    prédicteur du §6.6 sur le rangement PUR.
> 3. **LE DÉMÊLAGE**, avec les quotas par porte comme cahier des charges.
> ❌ **CE QU'IL NE FAUT PAS REFAIRE** : relancer avec plus de budget. Réfuté deux fois le
> même jour — ×4 sur A\* pur (60 s → 240 s) et ×10 sur l'extraction (30 s → 300 s), zéro
> gain à chaque fois. On n'est pas au bord du mur.
>
> ~~🎯 **CE QUI EST GAGNÉ MALGRÉ TOUT : un banc d'essai minuscule.** `level0200.xsb`
> (13×12, 15 embuts, ni transport ni congestion) est **résolu à la main par
> l'utilisateur** et hors de portée du solveur. C'est le plus petit cas reproductible de
> l'angle mort qui bloque le niveau 18 depuis des semaines.~~
> ❌ **PÉRIMÉ le 2026-08-23 : le 200 EST RÉSOLU par le solveur, en 54 états.** Son
> ordre par défaut était MURÉ sur ses deux derniers buts ; l'ordre réparé, il tombe
> aussitôt. Ce n'était pas un angle mort de la macro, c'était un ordre faux — cf. le
> bloc du 2026-08-23 plus haut.
>
> 🔴 **ET LA CORRECTION DE MÉTHODE DU JOUR — `AUCUNE` N'EST PAS UNE PREUVE.**
> J'ai lu toute la journée les `AUCUNE` de `macro`/`coupl-plongeon` comme « prouvé
> insoluble », et j'ai déclaré tel cinq niveaux fabriqués. **C'est faux** : ces régimes
> embarquent le RÉGIME D'ENGAGEMENT, que le §6.0 (point 6) documente comme INCOMPLET —
> « il ne rend pas des niveaux plus lents, il les rend insolubles ». Leur `AUCUNE` dit
> « espace TRONQUÉ épuisé ».
> **C'est l'utilisateur qui l'a démontré en RÉSOLVANT À LA MAIN**, dans l'app, un niveau
> que j'avais déclaré insoluble (la zone du 15, exporté en `level0200.xsb`). Vérifié
> ensuite : A\* pur ne conclut pas dessus, alors qu'il épuise réellement l'ancienne
> version de la zone 0 du 18 (`level0201.xsb` d'alors). **Seul A\* pur fournit une
> vérité.** `extrait()` le câble : `coupl-plongeon` pour trouver vite, reprise en `Astar`
> dès qu'il rend `AUCUNE`, et le mot « insoluble » n'est employé que si A\* épuise.
> ⚠️ Les faux verdicts ont tout de même fait trouver de VRAIS défauts de construction
> (cf. ci-dessous) — mais le mot « prouvé » était usurpé, et il ne l'est plus.
> ~~🎯 **À GARDER : `level0200.xsb` est résolu à la main et PAS par le solveur.**~~
> ❌ **RÉFUTÉ le 2026-08-23 — il l'est, en 54 états, dès que son ordre n'est plus muré.**
> La formule « l'humain bat le solveur sur la seule question de l'ordre » était juste au
> mot près, et personne ne l'a lue au pied de la lettre : c'était bien l'ordre, et il
> suffisait de lancer `ordre level0200.xsb` pour le voir.
>
> 🧱 **LE GÉNÉRATEUR, ET LES PIÈGES QU'IL A COÛTÉ** (six tours de réglage avec
> l'utilisateur, croquis de référence `niveau1embutSeul.txt`) :
> - **une caisse ne peut pas TOURNER UN COIN dans un couloir d'une case** : la pousser
>   perpendiculairement demanderait au personnage de se tenir dans le mur. Posée du côté
>   opposé à la porte, elle est perdue d'avance ;
> - **les extrémités d'une voie butant sur une marge nulle sont inutilisables**, pour la
>   même raison ;
> - **deux voies étendues jusqu'aux angles s'y recouvrent** et y empilent des caisses en
>   paquet 2×2, immobiles ;
> - **un blanc dans un `.xsb` est AMBIGU** (sol de zone, porte, ou hors-zone) : détecter
>   les côtés ouverts sur les blancs de bordure a fait poser 3 caisses sur 4 d'un côté
>   sans porte. Il faut passer les VRAIES portes ;
> - **voies ALIGNÉES et non en quinconce** (constat utilisateur) : en quinconce la caisse
>   du fond retombe décalée et doit encore glisser — un déplacement avant toute poussée
>   utile ; **rien en face d'une porte**, pour la même raison ;
> - ⚠️ **une porte peut ne servir QU'AU PERSONNAGE** (zone 0 du 18) et aucune analyse
>   statique ne le dit — il faut pousser pour le savoir. **D'où la décision finale : 2
>   cases de marge PARTOUT**, non optimal mais robuste, qui laisse le solveur router.
> - ⚠️ Un piège plus vicieux que les autres : une boucle de réessai qui recalculait la
>   même chose rendait un plateau **VIDE**, et le solveur répondait `OK` dessus. Un
>   résultat faux qui ne se signale pas. Vérifier `caisses == embuts` sur chaque sortie.
>
> 🆕 **2026-08-22 (suite 2) — TROIS CORRECTIFS VENUS DE LA RELECTURE EN IMAGES.**
> - **L'ÉTAGÈRE** (constat utilisateur sur le 24) : `sallesDeButs()` regroupe les buts
>   4-ADJACENTS, ce qui coupe en deux le niveau 24 — ses deux paquets ((1,1)..(10,2) et
>   (13,1)/(13,2)) sont pourtant sur la **même rangée, adossés au même mur continu**
>   (la ligne y=0), séparés par deux cases vides. C'est une seule étagère. D'où une
>   seconde règle de regroupement, statique : buts alignés + tout libre entre eux + un
>   mur continu du même côté sur toute la longueur. **Effet mesuré sur les 32 niveaux :
>   le 24 et lui seul** (2 zones → 1, 22 embuts).
>   ⚠️ **La fusion est faite dans `zonesEmbut()`, PAS dans `sallesDeButs()`** — le
>   SOLVEUR utilise cette dernière pour le lookahead de rang 0 confiné à la salle de
>   tête (§6.2, le correctif multi-salles qui vaut ×7,5 sur le 10). La toucher
>   décalerait l'ordre de remplissage de tout le corpus.
> - **LE SEUIL DE CÔTÉ SUBSTANTIEL PASSE DE 4 À 3** : sur le 18, la coupe (4,6) était
>   rejetée d'UNE case — son côté gauche en fait exactement 4 — et la zone gardait un
>   **enclos sans aucun embut**, repéré à l'œil sur la planche contact. Balayé : 1, 2, 3
>   et 4 laissent les 8 zones de référence identiques ; seul le 18 bouge (18 → 9 cases).
>   ⚠️ Fausse piste écartée en chemin : « l'absorption rampe » (elle avale 4 cases, puis
>   4 de plus depuis la zone agrandie). C'est vrai — le garde est resté — mais ce n'était
>   PAS la cause ici : l'enclos du 18 était dans le cœur, pas dans l'absorption. Vérifié
>   à la trace avant de conclure.
> - **MARGE PAR CÔTÉ pour les niveaux simples** (idée utilisateur) : au lieu d'un anneau
>   uniforme élargi sur les 4 côtés dès qu'une rangée de caisses ne suffit pas, chaque
>   côté reçoit la marge qu'il lui faut, et les caisses vont d'abord sur les côtés
>   **OUVERTS** (ceux où débouche une porte) pour n'avoir qu'à glisser. Un côté sans
>   caisse garde 1 — de quoi laisser passer le personnage. **−12 % de surface totale**
>   sur les 35 niveaux (6 642 → 5 866 cases), jusqu'à −28 % sur le 24. Moins de cases
>   libres, c'est moins de configurations de caisses, donc un espace d'états plus petit.
>
> 🆕 **2026-08-22 (suite) — LES NIVEAUX SIMPLES : LE RANGEMENT SANS LE DÉMÊLAGE.**
> Demande utilisateur, référence dessinée à la main dans `niveau1embutSeul.txt`. À partir
> de chaque zone : on l'enrobe d'un COULOIR, on referme d'un MUR, et on pose une caisse
> par embut non rempli **dans la voie intérieure du couloir** — la voie extérieure reste
> libre pour le personnage, posé sur la case libre d'indice le plus faible. La zone GARDE
> SES MURS : une caisse n'entre que par une porte.
> **C'est le banc d'essai que le goal-ordering n'a jamais eu** : plus de transport (les
> caisses sont à pied d'œuvre), plus de congestion — il ne reste que « dans quel ordre
> remplir, et par où entrer ». Le §3/§4 sépare ces deux moitiés depuis toujours ; ces 35
> niveaux isolent la première.
> - **POURQUOI LA VOIE INTÉRIEURE NE PEUT PAS PRODUIRE DE DEADLOCK** : collée au mur de
>   la zone, une caisse ne peut être poussée que le LONG de la voie (la pousser vers la
>   zone la jette dans le mur ; la pousser vers l'extérieur demanderait au joueur de se
>   tenir DANS ce mur). Elle glisse donc jusqu'à une porte. ⚠️ **Un couloir d'UNE case
>   serait mortel** : une caisse posée dans un COUDE a ses deux voisins libres
>   perpendiculaires, le joueur ne peut jamais s'aligner, elle est morte avant le premier
>   coup. D'où le couloir de 2, élargi tant que les caisses ne rentrent pas.
> - **Vérifié** : 0 caisse en coin, 0 caisse contre le mur extérieur, sur les 35 niveaux.
> - 🎯 **ET LE RÉSULTAT EST DÉJÀ UN FAIT** : `bench <fichier> macro` résout **27/35 en
>   moins de 10 s, la plupart en une poignée d'états** (7 sur le niveau 1, 11 sur le 2,
>   29 sur la grande zone du 10) — mais **8 résistent** : 12, 13, 15, 20, 22, 26, 28, 29.
>   Le 12 et le 13 tournent encore à 60 s avec un `tableG` à 47 % (≈ 1 M d'entrées). Or il
>   n'y a RIEN d'autre à faire dans ces niveaux que ranger : ni détour, ni congestion, ni
>   caisse à écarter. **La difficulté qui reste est l'ordre de remplissage à l'état pur**,
>   et c'est exactement la moitié que le plan dit irréductible depuis le §3.
> - Sortie : `simple_nivNN_zK.xsb` + `.png`, planche `_planche_simples.png`.
>
> - **SORTIE CONSERVÉE** dans `mesures/zones_20260822/` : les 34 `.xsb`, leur `_resume.txt`
>   (portes comprises), les 34 PNG et la planche contact — à la manière de
>   `fixtures_20260820/`, un scratchpad étant éphémère (§1). C'est le témoin si la règle
>   bouge.
> - **EN IMAGES (2026-08-22, demande utilisateur)** : rendu par `mesures/image` (sprites
>   de l'UI), plus `_planche_contact.png` — une vignette par zone, légendée embuts /
>   cases / portes. **Deux corrections au passage dans `image.cpp`**, toutes deux
>   révélées par les plateaux de zone, qui n'ont PAS de joueur :
>   · le perso était dessiné **inconditionnellement** à `getPlayerPoint()`, donc dans le
>     coin (0,0) — sur un mur — quand le plateau n'en a pas. On relit la case au lieu de
>     croire le point ;
>   · sa copie privée de `calculeInterieur` est remplacée par `Game::interieur()` (§7,
>     exemplaire unique) : elle floodait depuis ce même joueur fantôme.
>   ⚠️ Une TROISIÈME copie du flood subsiste dans `wgame.cpp` (l'UI), non touchée — elle
>   ne voit que des plateaux avec joueur, mais c'est le doublon suivant à résorber.
> - **INVARIANCE VÉRIFIÉE** : le même niveau chargé en cours de partie
>   (`plateau_niveau21.xsb`, milieu de partie, caisses déplacées) rend **la même zone à
>   la case près** que `level0021.xsb`. Attendu — une zone ne dépend que des murs et des
>   buts —, mais c'est le genre d'évidence qui se révèle fausse (§7).
> - ⚠️ **CE N'EST PAS UNE PREUVE, et rien ne doit en dépendre dans `checkDefaite`.**
>   Une zone est une lecture de la géométrie, pas un théorème : rien n'y dit qu'une
>   caisse ne doit pas SORTIR de la zone, et le point 8 ci-dessous (le niveau 18)
>   rappelle qu'une partie gagnante ressort 12 fois une caisse de son but. Tout usage
>   futur passe par un juge FP (§1) avant câblage.
> - **Canari** : le changement est purement ADDITIF (deux fonctions neuves,
>   `zonesEmbut` et `interieur`, qu'aucun chemin existant n'appelle). Vérifié binaire
>   contre binaire (`git worktree` sur `d9632f4`) sur **0/1/2/3/17 en `astar` ET
>   `macro`** : identique à l'unité — états, poussées, coups, et l'histogramme des `f`.
>   Les poussées retombent sur le canari écrit au §1 (4 / 97 / 131 / 134 / 213).
>   ⚠️ **Piège de canari rencontré** : le binaire témoin porte son `LEVELS_DIR` en dur
>   vers le worktree ; une fois le worktree retiré il ne rend plus RIEN, et la
>   comparaison annonce « DIFFÉRENT » sur un binaire pourtant sain. Comparer en passant
>   les plateaux **par chemin** (`bench level0001.xsb …`), ou garder le worktree tant
>   qu'on mesure.
> - **Coût de l'outil** : 16 s pour les 32 niveaux, dont 4,7 s sur le seul niveau 22 (le
>   plus grand, 167 cases d'intérieur) — c'est la recherche de coupe, quadratique en
>   cases. Sans objet pour le solveur : `zonesEmbut()` n'est appelée par aucun chemin du
>   moteur, seulement par l'outil.
>
> 🔴 **SESSION DU 2026-08-20 — LA LOI DE L'ORDRE EST RÉFUTÉE, ET LE RÉGIME D'ENGAGEMENT
> EST INCOMPLET.** Rien n'est commité ; l'arbre de travail porte tout ce qui suit.
> Session interrompue (orage) — ce bloc est le point de reprise.
>
> **1. LE MURAGE LOCAL, CÂBLÉ DANS `ordreParPrecedence` (game.cpp/game.h).**
> Parti du plateau du 21 exporté par l'utilisateur : **mort à 7/13**, deux buts —
> (11,10) et (11,11) — qu'aucune poussée ne peut plus atteindre, la colonne x=13 étant
> gelée par pure géométrie. Cause : l'ordre align remplit (13,10)/(13,11) — les APPUIS
> des seules manœuvres restantes — avant les buts qu'ils desservent. Même espèce que le
> verrou du 10 ((17,2), 2026-08-19).
> - `Game::butMureLocalement(idxBut, bloque)` — la précédence par approches du §6.2,
>   remontée de `mesures/ordre.cpp` dans le moteur en **exemplaire unique** (§7) ;
>   l'outil l'appelle et confronte son propre détail au verdict du moteur.
> - Câblé DEUX fois : dans la garde anti-échouage du glouton **et** dans la post-passe
>   de tri topologique. ⚠️ **Le premier est INERTE** (cartes de rangs identiques sur les
>   35 niveaux) : c'est la POST-PASSE qui décide de l'ordre dès qu'il y a des arêtes de
>   précédence, elle défaisait ce que le glouton avait bien choisi. Quatre déductions
>   perdues avant d'aller **imprimer** l'ordre installé.
> - La post-passe est passée du glouton à une **recherche avec retour arrière** (budget
>   500) : le garde myope déplaçait simplement le murage ((11,10) sauvé au rang 8
>   condamnait (13,10) au rang 9).
> - `precedenceAlignement` est un **indice**, `precedenceGlobale` une **preuve** : quand
>   respecter l'indice mure un but, la preuve gagne (passe 3 du tri).
> - **RÉSULTAT** : ordre par défaut identique sur 33/35 (seuls 18 et 22, tous deux
>   murés auparavant, changent) ; en align, 12/21/31 réparés, 20 toujours muré.
>   **CANARI 29 mesures binaire contre binaire (worktree sur `92f3ecd`) : 29 identiques.**
>   Coût du ctor `Game(Level)` inchangé (22 : 4,64 → 4,42 s CPU).
>
> **2. L'ANOMALIE DES +3 EST TRANCHÉE — `setOrdreAlignement` fait bien ce qu'on croit.**
> Le test que le plan réclamait, sur le 17, trois runs une seule variable :
> A défaut+ordre défaut = **18 636** · B `loi` = **18 639** · C défaut + ordre align
> **injecté** = **18 639**. **B = C à l'unité** ⇒ le régime applique réellement son
> ordre, et **le régime `loi` en lui-même est GRATUIT** : tout ce qu'il produit vient de
> l'ordre, jamais de `caseMorteLoi`. Les lignes « neutre » du 2026-08-19 tiennent.
>
> **3. LE 21 EST REPRIS PAR `loi`, MAIS NE BAT PAS LE DÉFAUT.** Sans injection :
> `loi` = **2 922 397 / 159 p.** (545 s) là où le régime ne rendait RIEN en 2 400 s
> hier. Ligne de base re-mesurée ici : défaut = **2 922 383 / 159 p.** (544 s), et
> défaut + align injecté = **2 922 397**, soit **B = C** encore. Le garde guérit une
> blessure que `precedenceAlignement` s'infligeait ; il n'améliore pas le défaut.
> Le 11 rend **13 913 050 / 243**, exactement le chiffre du plan, à l'unité.
>
> **4. LE 32 : C'EST L'ORDRE, PROUVÉ PAR ISOLATION.** Même binaire, même régime
> `coupl-plongeon`, seule variable l'ordre : défaut = **6 591 366 / 153 p.** en 676 s ;
> ordre align **injecté** = **rien en 1 800 s**, 15,9 M états vus, mur `max 10/15` —
> exactement le mur que le plan attribuait au régime `loi`. Ce n'est donc ni
> `caseMorteLoi` (PRUNES=0 ici) ni le murage local (le 32 n'en a jamais eu) :
> **`precedenceAlignement` produit un mauvais ordre sur ce niveau.** ⚠️ 1 800 s est un
> budget, pas une preuve (§6.6).
>
> **5. ❌ `caseMorteLoi` EST RÉFUTÉE — 19 NIVEAUX SUR 29 EN FAUX POSITIF PROUVÉ.**
> Outils neufs, rapatriés dans `mesures/` le jour même (§1 : `juge_loi.py` avait été
> perdu avec son scratchpad) : **`mesures/jugeloi.cpp`** (+`.pro`) rejoue une partie
> GAGNÉE et applique le contrat EXACT de `SolveurAStar::loiTropTot` ; toute détection
> est un faux positif **prouvé**. **`mesures/poussees_journal.py`** en extrait les
> poussées, partie validée par rejeu.
> Faux positifs (défaut / align) : 4 → 0/9 · 5 → 0/5 · **6 → 21/17** · 7 → 0/21 ·
> 8 → 0/2 · 9 → 0/9 · 12 → 0/6 · 14 → 35/0 · 15 → 52/30 · 16 → 39/0 · **18 → 22/22** ·
> 19 → 0/19 · **22 → 116/24** · 23 → 0/10 · 24 → 0/46 · 26 → 3/18 · **27 → 79/57** ·
> 32 → 27/21. Zéro sur 1, 2, 3, 10, 11, 13, 17, 20, 25, 190.
> ⚠️ **Le niveau 6 — le seul que la loi était censée réparer — en porte 21.**
> **LA CAUSE, lue dans le code puis vérifiée** : `calculCasesMortesLoi` n'exempte que
> les buts de rang INFÉRIEUR au but actif et juge les buts de rang SUPÉRIEUR, avec ce
> commentaire assumé : « une caisse posée là est hors de son tour, et c'est exactement
> ce que la loi vise ». C'est-à-dire **interdire de remplir dans le désordre** — l'idée
> que le §4 a réfutée (« le désordre valide n'est pas de la redondance → niveau 1 rendu
> insoluble »). Le même fichier refuse pourtant ce raisonnement quinze lignes plus haut
> pour les cases ordinaires (« c'est le §4 en énième déguisement »).
> Cas type, niveau 18 : la caisse (14,9) est livrée sur son but (9,3) à la **44ᵉ
> poussée** d'une partie gagnante de 132 et n'en bouge plus ; la loi la condamne dès
> **le coup 39**, parce que (9,3) est le rang **10** alors que l'actif est le rang 0.
> Le run mesure `PRUNES = 1 101 923 / 28,9 M enfilages (3,82 %)` et progresse MOINS que
> le témoin (`max 6/11` contre `8/11`). **Le noyau valide de la loi est déjà implémenté
> ailleurs et prouvé sûr : `porteGeneraliseeCoupe` (0 FP sur 1 650 livraisons).**
>
> **6. 🎯 LE RÉGIME D'ENGAGEMENT N'EST PAS SOUS-OPTIMAL, IL EST INCOMPLET.**
> Le §6.3 le notait comme une perte d'OPTIMALITÉ. C'est une perte de COMPLÉTUDE, et
> elle rend des niveaux insolubles. Deux preuves indépendantes :
> - **16** : l'utilisateur le gagne à la main sous l'ordre align (196 poussées,
>   validées par rejeu) ; `bench 16 loi` rend **AUCUNE** — espace ÉPUISÉ, pas un budget
>   — en 811 s. Le journal dit pourquoi : sur 55 poussées vraiment choisies, **23
>   (42 %)** portent `HORS REGIME MACRO : 1 macro engagee, le solveur ne genere aucune
>   poussee simple`.
> - **18** : une macro est jouable **DÈS LA RACINE** (caisse (11,5) → but actif (7,4),
>   5 poussées), donc le solveur ne génère qu'elle ; **aucune des dix premières
>   poussées de la partie humaine gagnante n'existe dans son arbre**.
> **CORRECTIF CODÉ — RELÉGATION DES POUSSÉES SIMPLES** (§6.4 forme (a) : dé-prioriser,
> jamais élaguer). `enfiler` prend un `bonusF` (0 partout ailleurs) ajouté à `f` et
> **jamais à `g`** — la dédup `meilleurG` continue de raisonner sur le vrai coût. Deux
> régimes SÉPARÉS : `AstarMacroCouplagePlongeonRelegue` (ordre défaut) et
> `...LoiRelegue` (ordre align). Pénalité = `Solveur::penaliteRelegation`, posée par le
> HARNAIS via `RELEG_F` — **jamais lue par un `qgetenv` du solveur** (§7, CORRAL_DETECT).
> **MESURÉ sur le 18** : témoin `max 8/11` (11,98 M états) — valeur restée identique de
> 120 s en juillet à 1 200 s ce soir — contre **`relegue` F=2 : `max 9/11`** en
> **9,42 M états**, donc plus loin avec MOINS d'états. Le plateau le plus stable du
> corpus cède. ⚠️ Le niveau n'est PAS résolu, et `max` est un mauvais prédicteur —
> démontré le jour même : l'état à **7/11** du 18 est à **6 états** de la victoire,
> quand le solveur explore des 8/11 qui sont des impasses.
> **BALAYAGE DE LA PÉNALITÉ (méthode CORRAL_BUDGET, §6.2), sur le 18, budget 1 200 s :**
> témoin `max 8/11` (11,98 M) · **`F=2` → `max 9/11` (9,42 M)** · `F=64` → `max 8/11`
> (13,21 M). **La pénalité est donc un vrai levier, et 2 bat 64** — cohérent avec le §3
> (le mou est toujours PAIR, les paliers de `f` valent 2, donc `F=2` = « un recul de
> retard » et `F=64` relègue si loin que la poussée simple n'est jamais dépilée).
> ⚠️ Trois points seulement : ne pas figer 2 sans balayer 4 et 6, et sur d'autres niveaux.
> ⚠️ Le premier jet du balayage — faire varier la FIXTURE à `F=2` — était le mauvais
> plan d'expérience : `p25` du 16 est à 12 caisses de la fin, c'est le niveau entier.
> ⚠️ La relégation est appliquée UNE FOIS, à l'enfilage ; elle n'est PAS cumulative le
> long d'une branche. Rendre cumulatif est réfuté d'avance : la partie humaine du 16
> contient 23 poussées simples jouées macro engagée, donc `23·K` d'inflation.
>
> **7. 🔴 LE MURAGE PAR ACCÈS DU JOUEUR (contre-exemple utilisateur, non stabilisé).**
> Le test d'occupation seul est insuffisant : la poche haute du 18
> `{(8,2),(9,2),(9,1),(10,1),(9,3),(10,3)}` n'a que deux portes, (8,1) et (10,2).
> Remplir (10,3) exige une caisse en (10,2) — qui bouche la porte droite — et le joueur
> en (10,1), donc DEDANS, accessible seulement par (8,1). L'ordre pose (8,1) au rang 8
> et (10,3) au rang 9 : **mort au rang 9**, et les deux cases de l'approche n'étant même
> pas des buts, le test d'occupation n'y voyait rien.
> `butMureLocalement` prend donc un troisième temps : l'appui doit être **joignable par
> le joueur**, obstacles = murs + buts posés + **la caisse d'approche elle-même**.
> Relaxation optimiste depuis la position de DÉPART, donc sound.
> **Ampleur** : ordre par défaut changé sur **2 niveaux seulement** (18, 26) ; murage
> RÉVÉLÉ sur le **23** (`ok → r17`, ordre inchangé : il existait, invisible) ; le 13
> passe de `r14` à `r12`. ⚠️ **NE RÉPARE PAS LE 18** et y dégrade l'ordre (muré r4).
> ⚠️ **NI CANARI NI MESURE DE TEMPS depuis ce changement, et le 26 est RÉSOLU et son
> ordre a bougé : à ne pas garder en l'état.**
>
> **8. 🎯 POURQUOI LE 18 RÉSISTE, ET C'EST STRUCTUREL.** Sa partie gagnante **ressort
> les caisses des buts** : (7,5) posé 5 fois / **4 sorties**, (8,1) 4 fois / **3
> sorties**, (5,6) 3/2, (6,5) 3/2, (6,6) 2/1 — **12 sorties de but sur 132 poussées**.
> Un `ordreButs` est une PERMUTATION : chaque but rempli une fois, définitivement.
> **Aucun ordre ne peut décrire cette solution.** D'où : la recherche gardée se vide
> jusqu'au rang 0 (elle cherche ce qui n'existe pas) ; (8,1) ne peut aller ni avant
> (le poser mure (10,3)) ni après (le poser après rend (8,1) non livrable) ; et la
> macro, qui pousse VERS un but et ne l'en ressort jamais, est structurellement muette.
> Même angle mort que le « but orphelin » du §4.
>
> **9. PIÈGES DE LA SESSION, à ne pas refaire.**
> - ⚠️ **`pkill -f` / `pgrep -f` tuent le shell qui les lance** : le motif figure dans
>   sa propre ligne de commande — **et le texte d'un heredoc en fait partie**. Trois
>   auto-kills, dont un qui a fait croire un script lancé alors qu'il n'avait jamais
>   été écrit. Utiliser `grep '[b]ench'` et séparer l'écriture du script de son
>   lancement.
> - ⚠️ **Lire une sortie avec `tail` et conclure sur « le premier »** : le premier faux
>   positif du 18 était au coup **39**, pas 114. Toute une explication (« la caisse n'a
>   pas bougé, c'est le juge qui a changé ») bâtie sur cette troncature, et fausse.
> - ⚠️ **Ne jamais désigner une case en la lisant sur une IMAGE** : deux caisses mal
>   identifiées d'affilée. `mesures/image` imprime désormais son verdict en clair
>   (`[loi] but actif`, cases mortes, caisses concernées). Même leçon que le §7 sur le
>   widget invisible.
> - ⚠️ **Les binaires de `mesures/` sont des Mach-O arm64** (macOS) : sur Linux ils se
>   font exécuter par `sh` et rendent « Syntax error ». Tout rebâtir.
> - ⚠️ `bench 2 astar` rend **582 469** états ici contre **590 066** écrit au §1 pour
>   l'étalon USok — même valeur sur les deux binaires, donc pas une régression.
>   **L'étalon USok du plan ne vaut pas sur cette machine**, à recalibrer avant tout
>   chronométrage. (Le 21 et le 11, eux, se rejouent à l'unité près.)
>
> **10. FIXTURES CONSERVÉES** dans `mesures/fixtures_20260820/` (sorties du scratchpad,
> qui est éphémère — §1) : les trois plateaux du 16 (`niv16_p19/p60/p180.xsb`), l'état
> élagué du 18 (`niv18_elague_coup114.xsb`) et ses deux PNG annotés, la liste de
> poussées du 18 (`p18.txt`, entrée de `jugeloi`), et les trois ordres align injectables
> (16, 21, 32). ⚠️ Le plateau mort du 21 est `plateau_niveau21.xsb` à la racine.
>
> **OUTILS NEUFS** (tous dans `mesures/`) : `jugeloi.cpp`/`.pro`,
> `poussees_journal.py`, et `image` étendu — `--rejeu <poussees.txt> --stop <n> --loi
> --align`. ⚠️ `--rejeu` charge le VRAI niveau puis applique les poussées : charger un
> `.xsb` de milieu de partie recalculerait `ordreButs` et `mortesLoi` pour ce plateau-là
> (§7) et l'image montrerait un autre but actif que celui du run.
>
> **11. CAMPAGNE NON-RÉSOLUS (ordre changé aujourd'hui), budget 1 200 s, aucun résolu :**
> 18 → défaut `max 8/11`, `loi` `max 6/11` · 22 → défaut `max 8/27` (contre `max 1/27`
> au profilage de juillet — mouvement réel, l'ordre par défaut n'est plus muré), `loi`
> `max 3/27` · 31 → 12/20 des deux côtés · 20 → défaut `max 1/18`.
> **`loi` est strictement PIRE partout où il diffère** — cohérent avec le §5 ci-dessus.
>
> **12. LE 16, MINÉ (partie gagnée à la main par l'utilisateur, 196 poussées).**
> L'ordre joué **est l'ordre align**, donc sa jouabilité est PROUVÉE (§6.6 :
> 0 inversion prouve, un grand nombre ne prouve rien) — première preuve de ce genre
> pour `precedenceAlignement`. ⚠️ **Jouable n'est pas solvable** : un ordre prouvé bon ne
> garantit pas que le solveur ira au bout, un ordre prouvé mauvais garantit qu'il n'ira
> pas (§6.6, asymétrie du 2026-08-22) — et `bench 16 loi` sous cet ordre prouvé rend
> bien AUCUNE. Profil macro : aucune macro des poussées 0→19, trous
> jusqu'à 60, puis **120 poussées d'affilée** avec macro, un nœud de 5 poussées à
> 180-184. **Fixtures exportées** : `p19` → `bench … macro` rend **AUCUNE** (0,05 s) ;
> `p60` → **OK en 106 états** ; `p180` → OK en 8 états. **Toute la difficulté du 16 est
> dans ses 60 premières poussées.**
> 🎯 **ET LE DÉMÊLAGE EST LE STOCKAGE** (§6.0, chantier STOCK) : les **8 mises en stock**
> de la partie tombent toutes entre les poussées **3 et 48**, aucune ensuite — les 148
> poussées restantes ne font que livrer et REPRENDRE le stock. Mettre une caisse en
> stock, c'est pousser vers une case qui n'est PAS un but : hors du vocabulaire de la
> macro, donc exactement la poussée simple que le régime d'engagement refuse de générer.
> Les deux chantiers — STOCK et complétude de la macro — sont le même verrou vu des
> deux bouts.


> ⚠️ **CETTE FEUILLE DE ROUTE EST HISTORIQUE — relire d'abord ce qui suit** (2026-08-11). Le plongeon
> ci-dessous a été codé, promu, et a fait tomber 10/11/21/32 ; l'ordre a fait tomber 12/26/27 par le
> régime `ordre-look`. **Le prochain chantier n'est plus ici** : c'est la **MÉMOIRE** (§6.5), devenue
> le mode d'échec dominant — trois des cinq derniers niveaux relancés y sont morts, dont un à 213,7 M
> états pour 14 Go. L'arène est le premier poste nommé par le §6.5, et il n'a jamais été attaqué.
> Le second est le **STOCK** (journal-hybride, 2026-08-09) : deux notions distinctes et chiffrées —
> « garder pour plus tard » et le stockage — qu'aucune ligne du solveur n'exprime.
>
> ✅ **REPRIS ET MINÉ le 2026-08-17 — « garder pour plus tard » se scinde en TROIS mécanismes.**
> La prochaine étape convenue (miner les deux signatures, sans solveur) est faite, et le mining a
> débordé son cadre. Outil neuf `mesures/stock.py` (six modes), piège `taches.py` corrigé (chemin
> dérivé du script). Décomposition des **106 caisses tenues** (livrables ≥ 30 coups) des 28 parties
> gagnées, sur quatre tests contrefactuels validés par rejeu :
> - **PORTE** (poser sur le but bloque un passage : transit strict OU cut d'articulation) : **24**,
>   dont **7 cuts** francs — (16,2)/27 mure 4 buts. **Statique, donc CODABLE** (extension de
>   `porte.cpp`), et les 7 cuts sont le jeu de validation prêt.
> - **CONGESTION** (le CORRIDOR de livraison est encore emprunté par d'autres) : **31**. Le
>   contre-exemple (10,10)/22 (tenue 218c, (b) au porte) l'a fait émerger : sa colonne de livraison est
>   l'autoroute de 15 caisses. C'est le démêlage **§3/§4, IRRÉDUCTIBLE** — aucune borne géométrique.
> - **BOUCHON** (la caisse au départ ouvre une région si on la retire) : **2/106**, hypothèse
>   « départ bloque » **réfutée** comme mécanisme courant.
> - **ORDRE libre** (rien ne force) : **66 %** — le solveur les réordonne déjà (§7 : `diff` brut = écart
>   à `ordreButs`).
>
> **Bilan : un tiers des tenues relève d'un passage (33 %) ; de ce tiers, une moitié est statique-codable
> (PORTE), l'autre est congestion irréductible ; deux tiers sont de l'ordre libre.** Cohérent avec la
> thèse du plan (§3/§4). Seul actionnable : le **PORTE généralisé statique** (articulation-cut),
> non encore codé. Détail, chiffres et pièges de mesure en [journal-hybride.md](journal-hybride.md),
> session du 2026-08-17. ⚠️ Pièges relevés : compter la reachability du but qu'on occupe exprès (artefact
> 90→7) ; la fenêtre du transit rate ce qui précède la livrabilité ; les salves par temps comptent la marche.
>
> ✅ **CODÉ ET VALIDÉ le 2026-08-18 — `Game::porteGeneraliseeCoupe` (game.cpp), toujours HORS
> SOLVEUR.** Généralise `porteBloquee` : plus de flood-fill « murs + G + C seuls », deux flood-fills
> sur l'état COURANT (toutes les caisses réellement posées comme obstacles), et le test porte sur
> N'IMPORTE QUELLE caisse non livrée / but non rempli, pas seulement les appuis de la caisse
> testée. Deux validations, dans cet ordre :
> 1. **Bit-à-bit contre `stock.py cut`** (outil neuf `mesures/portegen`, cf. §1) : les 10 tenues du
>    niveau 27 exportées en `.xsb` de milieu de partie, 10/10 verdicts identiques entre le mineur
>    Python (validé sur les vraies parties) et le prédicat C++ intégré au moteur — dont les deux
>    positifs, (16,2)→(4,1) qui mure 4 buts et (4,10)→(5,2).
> 2. **`fpporte.py` (juge FP, cf. §1), 0 faux positif sur 1 650 livraisons, 28 niveaux.** Même
>    protocole que `fp`/`juge_loi.py` : toute détection sur un coup réellement joué dans une partie
>    GAGNÉE est une preuve de faux positif. Piège capté en le construisant, cf. §7 (« une pose sur
>    un but n'est pas une livraison »).
>
> Le prédicat est donc établi comme SÛR au même titre que `porteBloquee` — un DÉLAI de
> remplissage, jamais une preuve de mort.
>
> ✅ **AMPLEUR MESURÉE le 2026-08-18 (outils `ampleurporte`/`ampleurporte2`, §1).** Premier réflexe
> réfuté : scanner TOUTES les paires (caisse × but) à chaque jalon rend 71 % de coupes — mais **ne
> discrimine rien**, résolus et non-résolus touchés aux mêmes taux (67-81 % partout). C'est un fait
> trivial de géométrie (des coins disjoints du plateau ne se voient jamais), pas un signal. La
> mesure qui compte restreint le test au SEUL but que `butActif()` choisirait réellement — rang
> minimal de `ordreButs`, lu STATIQUEMENT via l'outil `ordre` (jamais recalculé en Python, trop de
> règles pour être rejouées fidèlement hors du moteur) — sans présumer quelle caisse le remplirait
> (occuper G comme un mur de plus, aucune case libérée). Résultat : **17/435 jalons (3,9 %)**, sur
> **9 niveaux touchés sur 28** — dont des non-résolus notables (**13, 14, 15, 22, 25**), avec
> parfois des coupures MASSIVES quand ça touche (10 caisses + 7 buts d'un coup sur le 25, 10
> caisses + 4 buts sur le 22). **Un signal réel mais modeste, pas concentré sur les non-résolus**
> (le 6, le 26, le 27 et le 32 — tous résolus — l'ont aussi) : cohérent avec §3/§4, le porte
> généralisé attaque un COIN du problème (comme le corral en son temps), pas le mur PSPACE.
>
> ✅ **CÂBLÉ le 2026-08-18 — `Game::porteGeneraliseeBloquee`, dans les trois points d'appel de
> `porteBloquee` au sein de `butActif()`.** ⚠️ Le design naïf (« occuper G sans savoir quelle caisse
> le remplirait », `idxCaisse=-1`) est **RÉFUTÉ AVANT câblage** : `fpporte.py` en variante « sans
> libération » trouve **1 faux positif prouvé sur le niveau 25** — la case que la vraie caisse
> libère en partant rouvre exactement le passage qu'on croyait couper. Corrigé en balayant TOUTES
> les caisses non livrées et en ne bloquant que si TOUTES coupent (`game.cpp`, juste après
> `porteGeneraliseeCoupe`) : sûr par construction, puisque la caisse RÉELLEMENT jouée dans une
> partie gagnante fait partie du balayage et y est toujours trouvée sûre (déduit du 0 FP/1650 de
> `fpporte.py` en variante AVEC libération). **Canari revérifié binaire contre binaire** (`git
> worktree` sur HEAD) sur 0,1,2,3,4,5,6,7,9,17,190,191 en régime `macro` : identique à l'unité —
> attendu par construction, le code câblé n'est jamais atteint hors de `ordreDynamique`.
> **Essai sur 22 et 25 (`ordre-dyn`) : NON CONCLUANT, interrompu pour préserver la machine** (RAM
> quasi épuisée, swap plein). Avant coupure : le 22 a montré `max 8/27` (contre `max 1` au
> profilage de juillet — premier vrai mouvement sur ce niveau), le 25 `max 2/19`, aucun des deux
> plafonds confirmé définitif ni aucune solution. **Rien de tranché** : à refaire avec surveillance
> mémoire et un seul run à la fois si le chantier reprend.

> 🧭 **BILAN DE TRAJECTOIRE (2026-08-18, idée utilisateur) — LA RÉDUCTION D'ÉTATS MARQUE LE PAS
> DEPUIS `ordre-look`.** Point explicitement demandé : distinguer un vrai LEVIER (réduit le nombre
> d'états à explorer) d'un simple AGRANDISSEMENT DE BUDGET (mémoire, temps) qui permet seulement
> d'aller plus loin en force brute sur un espace inchangé. Relu chronologiquement :
> - **Le dernier vrai levier structurel est `ordre-look`** (2026-08-08/09, §6.2) : a fait tomber
>   12, 26 et 27 d'un coup — des niveaux auparavant insolubles, pas juste accélérés.
> - **Les dix jours qui suivent (2026-08-11 à 08-17) sont presque exclusivement de la mémoire**
>   (`TableG` en deux tableaux −25 %, arène empaquetée −46 %, `noeuds` restructuré −15 %, plafond
>   `QVector` à 2 Go levé) — cf. §6.5, dont le **propre bilan dit déjà** « la mémoire était un
>   PLAFOND, pas le problème [...] on a acheté de l'espace d'exploration, pas des idées ». Le run
>   le plus gros jamais lancé (470 M états, niveau 29) n'a débloqué **aucun niveau**. C'est un
>   agrandissement de budget, pas un levier — la distinction que ce bilan reprend explicitement.
> - **Le porte généralisé (aujourd'hui) est la première tentative de levier depuis `ordre-look`,
>   et sa propre mesure d'ampleur le classe petit** : 9 niveaux touchés sur 28, jamais plus de 4
>   jalons par niveau (§6.0 ci-dessus). Pas dans la même catégorie que goal-ordering ou corral —
>   localisé, pas structurel.
> - **Raison structurelle probable, pas juste une mauvaise passe** : le mining du stock
>   (2026-08-17, ci-dessus) décompose déjà les caisses tenues par un mécanisme de PASSAGE en deux
>   moitiés égales — PORTE (statique, codable, c'est ce qu'on vient de câbler) et **CONGESTION,
>   explicitement qualifiée d'IRRÉDUCTIBLE** (démêlage PSPACE pur, §3/§4, aucune borne géométrique
>   ne la capture). La moitié statique-codable du gisement vient d'être exploitée ; l'autre moitié
>   n'est PAS un levier possible par définition — seul le budget de recherche (mémoire, anytime)
>   peut la traverser, jamais la court-circuiter.
> - **Ce qui reste non essayé** : le RN comme MINEUR HORS-LIGNE de motifs sûrs (§6.4) — jamais
>   câblé, jamais tenté. C'est la seule piste du plan qui ouvrirait une CLASSE de leviers statiques
>   nouvelle (motifs découverts automatiquement) plutôt que d'en dériver un de plus à la main sur un
>   gisement qui donne des rendements décroissants (loi de l'ordre ÷2,98 sur un niveau et zéro sur
>   un autre §6.6 ; porte généralisé sur 9/28 aujourd'hui).
>
> 🚧 **EN COURS le 2026-08-19 — restauration de la loi de l'ordre + découverte d'un vrai bug
> dans `ordreParPrecedence`.** Point de reprise détaillé, à lire avant de continuer.
> ⚠️ **Ce bloc disait « NON COMMITÉ, SESSION INTERROMPUE » — c'est PÉRIMÉ** : tout ce qu'il
> énumère est commité dans **`92f3ecd`** (« Reprise de la loi de l'ordre »), à la liste de
> fichiers près. Corrigé le 2026-08-19 (session du soir, cf. la suite plus bas).
>
> **Point de départ** : demande utilisateur de restaurer « les cases mortes dynamiques »
> retirées le 2026-08-18 (§6.6 ci-dessous). Deux mécanismes distincts s'y cachaient
> (`caseMorteLoi` et `geleHorsTour`) — testés ISOLÉS l'un de l'autre pour la première fois :
> - **`geleHorsTour` seul** (régime jetable, retiré depuis) : casse AUSSI le niveau 6
>   (`AUCUNE`, 62 prunes). Réfute l'hypothèse qui semblait ressortir du raccord du
>   2026-08-04 (« gel=0 sur le 6, c'est la loi qui coupe, pas le gel ») — cette mesure ne
>   disculpait rien, la loi masquait déjà les états où le gel aurait mordu.
> - **`caseMorteLoi` seule** (régime `AstarMacroCouplagePlongeonLoi`, conservé, cf.
>   solveur.h/solveurastar.h) : casse ÉGALEMENT le niveau 6 au premier essai. Mais cette
>   fois la cause a été **trouvée et corrigée**, pas juste constatée.
>
> **LA VRAIE CAUSE, prouvée par test minimal** : ce n'est PAS `caseMorteLoi` qui est fautive
> — c'est **`Game::ordreParPrecedence()`** (game.cpp) qui produit un ordre erroné sur le
> niveau 6. Preuve : déplacer SEULEMENT le but (2,3) du rang 2 au rang 9 (dernier), sans
> rien changer d'autre, suffit à faire résoudre le niveau (`ORDRE_HUMAIN`, testé à la
> main). Le niveau 6 a deux colonnes de buts parallèles (x=1 et x=2) séparées par un mur
> à x=0 : une caisse en colonne 2 PEUT encore rejoindre la colonne 1 (poussée vers l'ouest),
> l'inverse est géométriquement IMPOSSIBLE. `ordreParPrecedence` traite pourtant la colonne 2
> en premier, parce que son dernier critère de tie-break (`d`, distance de poussée depuis la
> caisse la plus proche — `game.cpp` vers la ligne 2088, `if (da != dc) return (da < dc);`)
> préfère systématiquement « le plus proche », et la colonne 2 est mécaniquement UNE poussée
> plus proche que la colonne 1 à chaque rangée. Ce critère est juste quand la distance reflète
> l'enclavement (le niveau 1 s'en sort parce que son critère `mur`, prioritaire, capture déjà
> la bonne colonne — la distance n'y tranche jamais) ; il est backward face à un passage à
> SENS UNIQUE, où « un pas de plus » signifie « plus jamais accessible », pas « moins urgent ».
> Vérifié avec `TRACE_ORDRE=1 mesures/ordre` (imprime les clés de tri à chaque rang).
>
> **LE CORRECTIF, scopé en régime d'essai** (jamais l'ordre par défaut) :
> - `Game::alignementLoi(cell, idxBut)` — factorisé hors de `calculCasesMortesLoi()`, le
>   même test (aligné + pas un coin) réutilisable ailleurs (§7 : une règle à deux endroits
>   diverge).
> - `Game::precedenceAlignement()` — nouvelle précédence but-à-but : si B ne peut pas
>   atteindre A (`distanceParBut`) ni s'aligner avec lui, ET que A, LUI, peut atteindre B
>   (asymétrie prouvée), alors B doit précéder A. Fusionnée dans `attente()`
>   (`ordreParPrecedence`), le tie-break de TÊTE — mais **seulement si `Game::ordreAlignement`
>   est armé** (`setOrdreAlignement()`, même mécanique que `setOrdreLookahead`).
> - Câblé UNIQUEMENT par le nouveau régime solveur `AstarMacroCouplagePlongeonLoi`
>   (`Solveur::creer`, solveur.cpp) : `depart.setOrdreAlignement(true)` avant de construire
>   le `SolveurAStar` avec `loi=true`. Aucun autre régime n'y touche.
>
> **DEUX BUGS TROUVÉS ET CORRIGÉS EN COURS DE ROUTE, à ne pas reproduire :**
> 1. ❌ **Fusion INCONDITIONNELLE dans `attente()`** (premier jet, sans le drapeau) : corrige
>    le 6, mais CASSE SÉVÈREMENT le niveau 10 (32 buts) — 2/32 caisses posées en 227 000
>    états quand la référence en pose 32 en 250 000 — et fait apparaître un murage LOCAL sur
>    6 niveaux auparavant propres (2, 8, 9, 12, 21, 32). Reverti, re-câblé derrière le
>    drapeau `ordreAlignement`. **Leçon reconfirmée : un signal qui répare un niveau via le
>    tie-break de tête de `ordreParPrecedence` doit être scopé, jamais fusionné dans l'ordre
>    par défaut sans repasser le canari des 35 niveaux.**
> 2. ❌ **`rangDeBut`/`mortesLoi` restaient périmés après le recalcul d'`ordreButs`** :
>    `setOrdreLookahead` a le MÊME angle mort (jamais débusqué faute d'avoir été combiné à la
>    loi) — recalculer `ordreButs` sans recalculer `rangDeBut` laisse `caseMorteLoi` juger
>    avec l'ANCIEN rang pendant que `butActif()` lit le NOUVEL ordre. `setOrdreAlignement`
>    appelle maintenant `calculCasesMortesLoi()` juste après `installeOrdreParPrecedence()`.
>    Sans ce correctif, `bench 6 loi` retombait à `AUCUNE` malgré l'ordre corrigé.
> 3. ❌ **Arête asymétrique manquante** (trouvé par l'utilisateur, en lisant l'export du
>    niveau 10 : « il prend des caisses de la petite salle pour les mettre dans la grande »).
>    Le premier jet de `precedenceAlignement` ajoutait une arête dès que B n'atteint pas A,
>    SANS vérifier que A atteint B en retour — condamnant aussi bien un vrai passage à sens
>    unique qu'une paire de buts simplement dans des salles DISJOINTES (aucun rapport de
>    précédence réel). Mesuré sur le niveau 10 (salle 28 + salle 4) : la petite salle
>    héritait d'une dette `attente` de 29-30 (quasi tous les autres buts), reléguée en fin
>    d'ordre sans raison. Corrigé en exigeant l'asymétrie dans les DEUX sens.
>
> **RÉSULTAT VALIDÉ** : niveau 6 résolu en régime `loi`, SANS INJECTION, 557 états/110
> poussées (optimal, identique au défaut). Canari binaire contre binaire intact — 0-9, 17,
> 190, 191 en macro/coupl-plongeon, ET le niveau 10 en régime PAR DÉFAUT (`coupl-plongeon`,
> jamais affecté par `ordreAlignement`) — tous identiques à l'unité près.
>
> 🔴 ~~**OUVERT, PAS TRANCHÉ — le niveau 10 en régime `loi` LUI-MÊME ne converge pas.**~~
> ✅ **TRANCHÉ le 2026-08-19 au soir — la cause est un ORDRE FAUX, et elle est prouvée.** Le
> paragraphe qui suit reste pour son récit et ses mesures ; sa conclusion est dépassée, lire
> la session du soir plus bas (« LE VERROU DU 10 »). Deux
> tentatives : la première (avant le correctif d'asymétrie) plafonnait à `max 1/32` après
> 37 000 dépilements (tuée à 300 s) puis à `max 1/32` encore après 281 000 dépilements/896 000
> états vus (tuée à ~5 min, `h(reste)` ne descend jamais). La seconde (après le correctif)
> est bien meilleure — `max 22/32` atteint vers 1,7-2,7 M dépilements — mais reste ARRÊTÉE
> MANUELLEMENT avant conclusion (ni `OK` ni `AUCUNE`), à la demande de l'utilisateur pour
> écrire ce point d'étape.
> - **Indice fort, NON VÉRIFIÉ** : `[LOI] enfilages=20 567 278 PRUNES=0 (0,00 %)` sur
>   l'intégralité du second run — la loi n'a JAMAIS rien coupé. Sur le niveau 6, en
>   comparaison, `PRUNES=62` sur seulement 2269 enfilages. Hypothèse de l'utilisateur,
>   plausible et non réfutée : **la macro ne s'engage quasiment jamais sur ce niveau**, la
>   recherche retombe en poussées simples qui ignorent totalement `ordreButs` — ce qui
>   expliquerait des buts posés hors tour observés dans l'export à 22/32 ((2,10) = rang 31,
>   (16,4)/(16,5) = rangs 26-27, alors que seuls les rangs 0-21 auraient dû être faits).
> - **À VÉRIFIER EN PREMIER À LA REPRISE** : `pas0 <fichier.xsb> trace` sur les fixtures
>   `record_niv10_r*.xsb` déjà exportées (répertoire `records10/` du scratchpad de CETTE
>   session — **PROBABLEMENT PERDU**, le scratchpad est éphémère ; réexporter avec
>   `bench 10 loi record` si besoin, cf. §1). Si `pas0` confirme qu'aucune macro n'est
>   disponible à ces états, le problème n'est plus dans l'ORDRE (déjà corrigé) mais dans le
>   RÉGIME D'ENGAGEMENT de la macro sur ce niveau précis — un chantier différent.
> - **État 22/32 mort ou vivant, non tranché** : un A* complet (`bench <fichier.xsb> astar`,
>   preuve exhaustive) a été lancé sur `record_niv10_r22_g0354.xsb` mais tué avant conclusion
>   (deux tentatives, 120 s puis fond perdu — jamais laissé aller au bout).
>
> **Fichiers modifiés, NON COMMITÉS à la coupure** : `game.h`/`game.cpp` (`alignementLoi`,
> `precedenceAlignement`, `ordreAlignement`, `setOrdreAlignement`, `calculCasesMortesLoi` +
> `caseMorteLoi`/`mortesLoi` restaurés), `solveur.h`/`solveur.cpp` (régime
> `AstarMacroCouplagePlongeonLoi`), `solveurastar.h`/`solveurastar.cpp` (`loiTropTot`,
> `StatsLoi`), `mesures/bench.cpp` (mode `loi`), `mesures/ordre.cpp` (mode `align`),
> `wgame.h`/`wgame.cpp` + `mainwindow.h`/`mainwindow.cpp`/`mainwindow.ui` (checkbox « Cases
> mortes (loi de l'ordre) » restaurée, déclarée dans le `.ui` cette fois — pas construite en
> code, sur demande explicite). `geleHorsTour`/`caissesGeleesHorsTour` NE SONT PAS
> restaurés (réfutés, cf. plus haut) : zéro trace dans le code actuel, vérifié.
>
> 🎯 **LE VERROU DU 10, TROUVÉ ET PROUVÉ (2026-08-19 au soir, diagnostic utilisateur) —
> « remplir la colonne 17 en premier, ok, mais pas jusqu'en haut, sinon le personnage ne peut
> plus se retourner pour pousser en colonne 15 ».** Lu à l'œil sur la géométrie, confirmé par
> accessibilité pure (aucun solveur). Mesures à **`92f3ecd`** (+ modifs UI non commitées, qui ne
> touchent pas le solveur).
>
> **LA GÉOMÉTRIE.** Le bloc de buts `{rangée 1 en x=15-17, col 15 y=2..8, col 16 y=2..9,
> col 17 y=2..14}` ne communique avec le reste du plateau que par **UNE case : (15,2)**, depuis
> (14,2) — `(14,1)` est un mur, la rangée 0 aussi. La rangée 1 (x=15,16,17, du sol libre, PAS des
> buts) est la seule plate-forme d'où l'on peut pousser vers le BAS dans les trois colonnes, et
> elle n'est atteignable qu'en traversant la rangée 2.
>
> **POURQUOI LA COLONNE 15 EST LE CAS PARTICULIER.** Pour livrer un but de la colonne 15 il faut
> une caisse en (15,2) ET le joueur en (15,1), donc DEDANS. Or :
> - poussée vers l'EST depuis (14,2) : la caisse arrive en (15,2), le joueur reste **dehors** — et
>   la porte est désormais bouchée par sa propre caisse. Mesuré : `(15,1)` inatteignable.
> - poussée vers l'OUEST depuis (17,2) : la caisse va de (16,2) à (15,2), le joueur atterrit en
>   (16,2), **dedans**, et boucle (16,1) → (15,1). Mesuré : `(15,1)` atteignable.
>
> La seconde est la SEULE qui marche, et son appui est **(17,2)**. Les colonnes 16 et 17 se servent
> par poussée est (le joueur y atterrit toujours dedans) : elles n'ont pas le problème.
> ⚠️ **(17,2) n'est donc pas « le dernier but de la colonne 17 », c'est l'APPUI de l'unique
> manœuvre de livraison vers la colonne 15.** L'ordre align le remplit au rang 17, avant les cinq
> buts de la colonne 15 (rangs 18-22) : il condamne la colonne 15 dès le rang 18.
>
> **LA PREUVE PAR L'EXPÉRIENCE, trois runs, une seule variable (l'ordre) :**
>
> | run | états | poussées | verdict |
> |---|---|---|---|
> | `loi` + ordre align **calculé** | 8 423 358 vus **et ça continuait** | — | **mur à `max 22/32`**, tué |
> | `loi` + ordre **corrigé, injecté** | **571 053** | **544** | ✅ **RÉSOLU** |
> | défaut (`coupl-plongeon`) | **249 913** | 544 | ✅ résolu |
>
> Le témoin muré reproduit **exactement** le `max 22/32` consigné plus haut pour le run d'août : la
> panne est la même, et elle survit à 5,5× le budget qui suffit à l'ordre corrigé. L'hypothèse
> concurrente (« le run d'août avait juste été arrêté trop tôt ») est donc **réfutée**.
> L'ordre corrigé (injecté) : `(3,10) (3,11) (2,11) (15,8) (16,9) (17,14)…(17,3) (15,7)…(15,3)
> (16,8)…(16,3) (17,2) (16,2) (15,2) (2,10)` — (17,2) déplacé du rang 17 au rang 28, et il doit
> rester AVANT (16,2)/(15,2) puisque la caisse qui le remplit transite par ces deux cases.
>
> ⚠️ **MAIS L'ORDRE CORRIGÉ NE BAT PAS LA LIGNE DE BASE** : 571 053 contre 249 913 états, à
> poussées égales (544) et par le MÊME plongeon gagnant (record 13/32, 2 919 états de plongeon des
> deux côtés — ils atteignent la même porte de sortie). Raison, visible en diffant les deux ordres
> plutôt que les deux régimes : **l'ordre PAR DÉFAUT respecte déjà la contrainte** — il remplit la
> colonne 15 en PREMIER, pendant que (17,2) et (16,2) sont encore libres, donc la manœuvre ouest
> est disponible tout du long. La correction **guérit une blessure que `precedenceAlignement`
> s'inflige** ; elle n'améliore rien par rapport au défaut.
>
> ❌ **ET LE RÉGIME `loi` CASSE LE 21 ET LE 32** (campagne du soir, `loi` contre `coupl-plongeon`,
> même binaire, sur les résolus > 10 qui le sont PAR LE DÉFAUT — le 12/26/27 sont exclus, ils ne
> tombent que par `ordre-look`, et le 26 ne tient pas sur une machine à 3 Go libres) :
>
> | niveau | `loi` | défaut | |
> |---|---|---|---|
> | 17 | 18 639 / 213 p. | 18 636 / 213 p. | neutre |
> | **21** | **rien en 2 400 s**, mur `max 10/13` | **2 922 383 / 159 p. en 542 s** | ❌ |
> | **32** | **rien en 2 400 s**, mur `max 10/15` | **6 591 366 / 153 p. en 687 s** | ❌ |
> | 11 | 13 913 050 / 243 p. | 13 913 047 / 243 p. | neutre |
>
> Le 21 et le 32 sont **exactement deux des six niveaux** que le plan notait déjà comme muraillés
> par la fusion INCONDITIONNELLE (« 2, 8, 9, 12, 21, 32 »). Scoper derrière `ordreAlignement` a
> protégé l'ordre PAR DÉFAUT — **la casse est intacte À L'INTÉRIEUR du régime `loi`**, où elle
> n'avait jamais été mesurée. ⚠️ 2 400 s est un BUDGET, pas une preuve : on peut dire « ne résout
> pas en 4,4× le temps du défaut », pas « ne résout jamais » (§6.6, la progression à budget borné).
>
> ⚠️ **`PRUNES=0` PARTOUT** — 10, 11, 17, 21, 32, sur des dizaines de millions d'enfilages
> (105 M sur le 32). `caseMorteLoi` n'a **jamais rien coupé** hors du niveau 6. Donc **tout ce que
> le régime `loi` produit ici, en bien comme en mal, vient de l'ORDRE**, jamais de la loi. Et ça
> réfute l'hypothèse consignée plus haut (« la loi ne coupe rien PARCE QUE l'ordre est muré ») :
> le `PRUNES=0` survit intact à un ordre qui, lui, gagne.
>
> 🔴 **OUVERT — L'ANOMALIE DES +3, à tester EN PREMIER demain (deux secondes).** Le 11 réordonne
> **8 buts sur 14** et rend **+3 états** ; le 17 en réordonne 2 sur 6 et rend **+3** aussi.
> Exactement +3 sur deux niveaux sans rapport, ça ressemble à un artefact et non à un effet
> d'ordre. **Si c'en est un, les lignes « neutre » du tableau ci-dessus ne valent rien** et il n'en
> reste que deux exploitables. **Le test** : injecter l'ordre align par fichier sur le **17** et le
> passer en régime **DÉFAUT** (1 s par run). S'il rend 18 639, le régime applique bien l'ordre et
> le neutre est réel ; s'il rend autre chose, `setOrdreAlignement` ne fait pas ce qu'on croit dans
> le solveur.
>
> 🔴 **OUVERT — l'ordre gagnant du 10 est INJECTÉ À LA MAIN.** `precedenceAlignement` continue de
> produire celui qui condamne la colonne 15 ; le niveau ne passe que si `ordre_niveau_0010.txt`
> est présent, ce qui est un outil de chantier, pas une résolution. La règle à dériver n'est PAS
> l'alignement mais la précédence **caisse → but** de `porte` : *un but qui sert d'appui à
> l'unique manœuvre de livraison d'un autre doit être rempli après lui*. C'est le cas DYNAMIQUE
> (invisible au départ, il n'apparaît qu'une fois la colonne 17 posée), donc du ressort de
> `porteGeneraliseeCoupe` — aujourd'hui câblée seulement comme *délai* dans `butActif()` sous
> `ordreDynamique`. Voilà pourquoi l'outil `porte` rend 0 sur le 10 et ne pouvait pas le voir.
>
> ✅ **UI, commité à part** : case à cocher « Ordre par alignement (régime loi) » (déclarée dans le
> `.ui`, comme `cbMortesLoi`) qui arme `setOrdreAlignement()` sur le Game de l'UI — sans elle
> l'interface affichait toujours l'ordre par défaut, même solveur lancé en régime `loi`, puisqu'il
> travaille sur sa propre copie. ⚠️ **Le drapeau est posé AU CHARGEMENT, et la bascule RECHARGE le
> niveau** : `ordreParPrecedence` lit `playerPoint` (garde `LIVR_DURE=3`) et les buts déjà posés,
> donc un recalcul en cours de partie rendrait un AUTRE ordre que celui du départ. Le journal
> hybride annonce désormais `calcule ⚠ PAR ALIGNEMENT (regime loi)` (accesseur `getOrdreAlignement()`
> ajouté exprès) — sans quoi les deux ordres produiraient des traces indiscernables, alors que 29
> buts sur 32 changent de rang sur le 10. Et les aplats gris des cases mortes passent désormais
> PAR-DESSUS le violet de la zone joueur (demande utilisateur) : la zone couvre presque tout le
> plateau accessible, le gris s'y noyait — deux aplats translucides ne se départagent que par
> l'ordre de peinture.
>
> **Objectif affirmé par l'utilisateur, à garder en tête pour la suite** : *« Je veux que les
> niveaux se résolvent avec un même et seul solveur [...] je suis sûr que le 16 notamment ne
> sera jamais résolu [sans la loi] »* — refus explicite de conclure « ça corrige le 6 mais pas
> le 10, donc c'est un correctif ciblé » sans avoir vraiment épuisé la piste. Ne pas abandonner
> le régime `loi` sur la seule base d'un run à budget borné (§6.6 : « la progression à budget
> borné ne prédit RIEN » — déjà mesuré comme piège sur CE niveau, en juillet).
>
> ⏸️ **RECADRAGE INITIAL (2026-08-17, avant le mining) — conservé pour mémoire :**
> - Le cas « garder pour plus tard » se scinde en deux, et un seul compte : *on ne la pose pas
>   maintenant parce que ça fermerait un passage* (généralise l'outil `porte`, qui ne teste que la
>   caisse perdant SES PROPRES appuis, au cas où une AUTRE caisse perd son passage). L'autre lecture
>   — « on pourrait la poser mais on ne le fait pas » — **n'est pas un cas** : sans conséquence, rien
>   à modéliser.
> - Rectifié ensuite : la caisse n'attend pas forcément « n'importe où hors du passage » mais à un
>   **emplacement précis** — sauf que l'exemple vérifié (14, caisses (9,2) et (11,2), cf.
>   `mesures/attente.py`) montre qu'ici l'emplacement précis, c'est **le point de départ lui-même** :
>   (9,2) attend 1061/1216 coups (87 %) et (11,2) 1145/1216 (94 %), **jamais déplacées** avant la
>   toute fin. Pas de recherche de case à faire pour ce couple — juste « ne pas y toucher » et savoir
>   QUAND il devient enfin nécessaire de les bouger.
> - **Prochaine étape convenue** : miner les parties gagnées pour repérer deux signatures, toutes
>   deux lisibles dans les données DÉJÀ loguées, sans toucher au solveur :
>   1. *poussable sur un but mais pas jouée tout de suite* — le mode hybride logue déjà
>      `macro jouable : caisse (x,y) en N poussées` à chaque pas ; reste à repérer les caisses pour
>      qui cette ligne apparaît plusieurs fois avant d'être enfin jouée ;
>   2. *déplacée en plusieurs fois* (pas un trajet net vers le but) — `attente.py` reconstruit déjà
>      `traj[i]`, la trajectoire complète de chaque caisse ; reste à compter les ruptures
>      mouvement/immobilité/remouvement.
> - **Données disponibles** : 11 des 15 non-résolus ont une partie gagnée rejouable exploitable
>   (13, 14, 15, 16, 18, 19, 20, 22, 23, 24, 25) ; 4 n'en ont aucune (**28, 29, 30, 31** — jamais
>   terminés à la main, le 31 à peine entamé, 374 o).
> - ✅ **Piège corrigé le 2026-08-17** : `mesures/taches.py` avait `R="/Users/corentin/perso/qtiasoko"`
>   codé en dur (macOS). Passé à `R = dirname(dirname(__file__))`, surchargeable par `QTIASOKO_ROOT`.
>   Plus de monkey-patch. (Les deux signatures ci-dessus sont maintenant minées par `stock.py`, qui
>   réutilise ce parseur réparé.)

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

> ✅ **DÉCOMPOSITION MESURÉE le 2026-08-11 — le tableau ci-dessous était CALCULÉ, il ne l'est plus.**
> Instrumentation `[MEM]` posée dans `solveurastar.cpp` (fonction unique, appelée aux trois sorties
> **et avec la jauge** — les trois niveaux morts de mémoire ont tous été TUÉS, donc aucune sortie de
> fin ne les aurait décrits). Relevé sur le **29** à **212,5 M états vus**, six fois plus gros que le
> niveau 8 qui avait servi au calcul de juillet :
>
> | poste | §6.5 calculé (niv. 8, 17,7 M) | **mesuré** (niv. 29, 212,5 M) | |
> |---|---|---|---|
> | **arène** | 45 % | **43 %** | 6 891 Mo |
> | `meilleurG` / `TableG` | 18 % | **26 %** | 4 096 Mo |
> | file | 27 % | **19 %** | 3 072 Mo |
> | `noeuds` | 13 % | **11 %** | 1 792 Mo |
> | **total** | — | **15 851 Mo** | **78,2 o/état vu** |
>
> **Le modèle tenait.** L'arène domine, et « premier poste à attaquer » reste juste. Deux choses que le
> calcul de juillet ne disait pas :
> - **`TableG` pèse plus que prévu** — 4 096 Mo, une puissance de deux EXACTE : la table vient de
>   doubler et la moitié de ses cellules n'a jamais servi. C'est ce qui explique l'écart entre les
>   **15,8 Go comptabilisés et les 14 Go résidents** (`footprint`) : de la capacité allouée jamais
>   touchée, donc jamais paginée. ⚠️ **Un doublement de `TableG` réserve 2 Go d'un coup** — sur une
>   machine de 18 Go, cette allocation décide seule du moment où le mur tombe.
> - **L'arène est pleine à ~100 %** : 212 486 340 clés de 17 shorts = 7,2 Go utiles pour 6 891 Mo
>   alloués. **Elle ne peut donc pas être optimisée, seulement ÉVITÉE** — réduire ce poste veut dire
>   ne plus garder la clé complète de chaque état vu, c'est-à-dire le hachage 128 bits mis en réserve
>   par ce même paragraphe.
>
> ✅ **PREMIER GAIN, le 2026-08-11 : `TableG` en DEUX TABLEAUX PARALLÈLES, −25 %.**
> `Slot{Cle; qint32 g}` faisait 8 octets. `g` est un nombre de POUSSÉES — 639 au maximum jamais
> observé — donc un `quint16` suffit ; mais un `struct{quint32;quint16}` est **repadé à 8** par
> l'alignement, la structure annulait le gain. Séparés en `vector<quint32> offsets` +
> `vector<quint16> gs` : **6 octets par cellule**.
> Bénéfice second, qui vaut peut-être autant : **la boucle de sondage ne compare QUE l'offset**, donc
> elle ne touche que le tableau de 4 octets — **16 cellules par ligne de cache au lieu de 8**.
>
> | | mesuré |
> |---|---|
> | `tableG` sur le niveau 9 | 8 Mo → **6 Mo** |
> | total par état vu | 94,3 → **90,9 o** |
> | extrapolé au run du 29 | 4 096 → **~3 072 Mo**, **1 024 Mo rendus** |
> | temps (USok, binaire contre binaire depuis `68b7991`) | **×1,00** sur 2 astar, 9/4/17 macro |
> | canari | **les douze identiques à l'état près** |
>
> Deux gardes `Q_ASSERT_X` sur `g` (dans `setG` et `insere`) : le §7 collectionne les troncatures
> muettes, et celle-ci ne planterait pas — elle **mentirait**. Un `g` tronqué rend un chemin plus
> court que le réel, donc une solution qui n'existe pas, et le canari n'y verrait rien.
>
> ⚠️ **LA POINTE DE RÉHACHAGE : mesurée, mais son caractère fatal N'EST PAS démontré.** `rehache()`
> alloue la nouvelle table avant de libérer l'ancienne : la pointe vaut **exactement 1,50 fois la
> cible**, tracé sur les six doublements du niveau 9. Sur le 29 (avant les 6 octets), le passage de
> 2 048 à 4 096 Mo demandait **6 144 Mo en un instant** ; après, 4 608.
> **J'ai d'abord écrit que c'était « probablement ce qui tue les runs » — la mesure ne le soutient
> pas** : `footprint` rendait **14 Go résidents pour 15 Go de pic** sur le 29, trop peu pour une
> pointe à 1,5×. Le bond est réel, sa létalité est une hypothèse.
>
> ❌ **LE DIMENSIONNEMENT UNIQUE, codé puis RETIRÉ le 2026-08-11.** Dimensionner la table une fois
> sur un budget dérivé de la RAM supprime la pointe *et* le coût CPU du réhachage. Retiré pour deux
> raisons :
> 1. **Il PARIE** — rien ne distingue « ce run sera énorme » de « ce run vient de dépasser le seuil ».
>    Un niveau terminant naturellement à 384 Mo de table sautait à 1 536 Mo, quatre fois trop.
>    ⚠️ Premier jet encore pire : il réservait **1 536 Mo pour les 22 clés du niveau 1** — le commentaire
>    annonçait un seuil que le code n'avait pas. *Un commentaire qui décrit une garde absente est pire
>    qu'aucun commentaire.*
> 2. **On paierait un sur-dimensionnement CERTAIN contre un bénéfice HYPOTHÉTIQUE** (cf. ci-dessus).
>
> **Ne pas le reproposer sans avoir d'abord montré qu'une pointe de réhachage TUE réellement un run.**
> ⚠️ Et noter que le facteur de croissance ne peut pas résoudre ça : avec un facteur `k`, la pointe
> vaut `(1+1/k)` fois la cible et la charge retombe à `70 %/k`. `k=2` → 1,50× et 35 % ; `k=8` → 1,12×
> et 8,8 %. **Grandir plus réduit la pointe et aggrave le gaspillage permanent — aucun `k` ne gagne
> sur les deux.**
>
> ✅ **VALIDÉ À L'ÉCHELLE ET CHIFFRÉ EN PORTÉE, le 2026-08-12/13.**
> - **Canari à 103 M états** : le **26** rend `etats=103640691 poussees=197 coups=639`, **identique au
>   dernier état**. C'était la vérification qui manquait — les douze niveaux du canari plafonnent à
>   325 k états, et une table de hachage peut se comporter jusqu'à 6 Mo puis diverger à 1,5 Go.
> - **Portée : +24 % sur le 29** — 213,7 M → **265,7 M états vus**, et un record de plus (`max` 10/16
>   → **11/16**). Coût par état : **78,2 → 66,3 o**.
>   ⚠️ **J'avais prédit +7 %.** L'erreur : la comparaison était faite à un instant où `tableG` venait
>   de doubler, donc à sa charge la plus défavorable (35 %) et à son gaspillage maximum. **Comparer
>   deux runs à un instant pris au hasard dans un cycle de doublement ne mesure rien** — il faut ou
>   bien le point d'arrêt, ou bien une moyenne sur le cycle.
> - ⚠️ **Sur le 26, le gain ne se voit PAS en mémoire finale** : `tableG` rend bien ses 512 Mo, mais le
>   total reste à ~7 Go, la recherche ayant réinvesti la place dans l'arène avant de plonger. Le gain
>   se lit en **portée**, pas en pic — sur un niveau qui termine, il n'y a rien à voir.
> - **Non mesuré : le TEMPS à grande échelle.** Les USok (×1,00) portaient sur des runs où la table
>   tient en cache. À 1,5 Go chaque sonde est un défaut de cache, et c'est là que la séparation
>   devait payer. Le chiffrer coûte trois heures (référence à reconstruire + deux runs du 26).
>
> ✅ **SECOND GAIN, le 2026-08-13 : L'ARÈNE EMPAQUETÉE, −46 %.** Une clé est une suite d'indices de
> CASE rangés sur 16 bits, alors que `ceil(log2(taille du plateau))` vaut **6 à 9 bits** sur les 35
> niveaux. Empaquetés à cette largeur : **46 % de l'arène**, prédiction confirmée à 46 % mesurés.
>
> **Pourquoi le bit-packing et pas un delta+varint**, qui gagnerait autant : le packing garde une
> longueur **FIXE**. L'arène conserve son pas fixe, `CleEq` reste un memcmp — sur **17 octets au lieu
> de 34**, donc plus rapide — et le codage est canonique, donc deux états égaux ont les mêmes octets.
> Un varint cassait les trois.
>
> | mesuré sur le 26, au même point (103,6 M états) | avant | après |
> |---|---|---|
> | **total** | 7 081 Mo | **5 576 Mo** (−21 %) |
> | **arène** | 3 241 Mo (46 %) | **1 736 Mo (31 %)** (−46 %) |
> | octets par état | 61,2 | **48,2** |
> | temps (USok, 5 mesures) | — | **×1,00** (0,98 à 1,02) |
> | canari | — | **les douze + le BFS + le 26 à 103,6 M états, identiques** |
>
> Le décodage ne coûte rien parce qu'il est payé par la comparaison : `CleEq`/`CleHash` parcourent
> moitié moins d'octets, et ils tournent une à quatre fois par sonde.
>
> ⚠️ **Test de BIJECTION avant tout câblage** (`mesures/paquetcle`) : 2 000 040 cas sur les dix tailles
> réelles, bords compris, plus la vérification que les bits de rab sont à zéro — sans quoi deux clés
> égales pourraient différer et le memcmp mentirait. **Le canari n'aurait pas vu une clé subtilement
> fausse** : il aurait vu un niveau non résolu, ou rien. C'est la leçon du §7 sur `decodeCle`, dont le
> bug a faussé `mou` pendant des semaines sans qu'aucune mesure ne le signale.
>
> 🎯 **BILAN DES DEUX CHANTIERS : 78,2 → 66,3 → 48,2 octets par état, soit −38 %.**
> **Et il n'y a PLUS de poste dominant** : sur le 26, arène 31 %, `TableG` 28 %, `noeuds` 28 %.
> La logique « premier poste à attaquer » de ce paragraphe s'arrête donc ici, faute de premier poste.
> ⚠️ Ce classement vaut pour **13 buts** ; sur le 29 (16 buts) l'arène pesait 51 % et `noeuds` 12 % —
> la répartition suit la longueur de clé, donc le nombre de caisses. À relire là-bas.

> ✅ **TROISIÈME GAIN, le 2026-08-17 : `noeuds` EN DEUX TABLEAUX + CROISSANCE DOUCE, −15 % au total.**
> Même diagnostic que `TableG` en août : `Solveur::Noeud` était `{qint32 parent; quint16 idxCaisse;
> quint8 dir}`, 7 octets utiles **repadés à 8** par l'alignement du `qint32`. Séparé en
> `ArbreNoeuds` — `std::vector<quint32> parents` + `std::vector<quint16> caisseDir` (case et
> direction tassées dans un seul champ, `case << 2 | dir`) — soit **6 octets, −25 %**. Bénéfice
> second, comme pour `TableG` : `reconstruire()` ne lit QUE `parents` en remontant la chaîne, donc
> 16 parents par ligne de cache au lieu de 8 noeuds entiers.
>
> **Deuxième pièce, plus grosse que prévu : la CROISSANCE DOUCE (×1,25 au lieu de ×2) sur `noeuds`
> ET la file d'A\*.** Un doublement laisse en moyenne 33 % de capacité vide (pointe à 50 % juste
> après le doublement) ; à ×1,25 c'est ~11 %. Mesuré en direct sur le 29 au moment du chantier : la
> file portait 78,5 M éléments dans 134,2 M de capacité, soit **1 264 Mo alloués et jamais écrits**
> rien que sur ce conteneur. La fonction `reserveDouce` (`solveur.h`) porte le facteur, appliquée
> aux deux vecteurs de `ArbreNoeuds` et à la file de `solveurastar.cpp`.
> ⚠️ **Ce levier est d'une nature différente des précédents** : il ne touche NI la structure d'une
> clé NI aucune décision du solveur, seulement la capacité réservée. Contrairement à un tie-break ou
> à un élagage, **le canari ne peut PAS bouger** — s'il bouge, c'est un bug, pas un arbitrage. Il
> réduit aussi la POINTE de réallocation (le mécanisme qui avait fait ajouter la garde
> `std::bad_alloc` sur `TableG`) : à ×1,25 la coexistence ancien+nouveau vaut 1,45x la cible contre
> 1,50x à ×2 sur `TableG`, et *bien davantage* sur `noeuds`/la file où l'ancien ET le nouveau
> vecteur sont désormais deux fois plus petits qu'avec le `Noeud` à 8 octets.
>
> | mesuré sur le 29, au même point (236 M clés, binaire contre binaire) | avant (`5e740dd`) | après |
> |---|---|---|
> | **total** | 12 239 Mo | **10 387 Mo** (**−15,1 %**) |
> | octets par état vu | 54,4 | **46,2** |
> | `noeuds` (capacité) | 2 048 Mo | **1 406 Mo** (−31,3 %) |
> | file (capacité) | 3 072 Mo | **1 863 Mo** (−39,4 %) |
> | temps CPU (`bench 4 macro`, 4 mesures entrelacées, `/usr/bin/time -f %U`) | 8,10 s (moy.) | **8,01 s** — ×1,00, dans le bruit |
> | canari | **29 mesures** (0-9, 17, 190, 191 × macro/couplage + 0-2 astar), **bit-à-bit identiques** |
>
> Le gain sur `noeuds` (−31,3 %) dépasse les −25 % mécaniques de la structure : le reste vient de la
> capacité moins gaspillée. Sur la file, dont la structure `SElement` n'a pas changé, **la totalité
> du −39,4 % vient de la seule croissance douce** — c'est la preuve séparée que les deux effets sont
> distincts et s'additionnent.
> ⚠️ **Comparaison faite au DERNIER point commun** (236 M clés = le plafond de la référence avant
> qu'elle ne soit arrêtée manuellement) plutôt qu'à un instant choisi au hasard dans un cycle de
> croissance — la leçon du run à +7 %/+24 % du 2026-08-12/13 juste au-dessus.
> ⚠️ **Chronométrer le CPU pas le mural, deux fois plus qu'ailleurs ici** : les premières mesures de
> temps ont été prises pendant qu'un run de 200+ M états tournait en fond sur la même machine (12
> coeurs, donc pas de contention de scheduler, mais assez de trafic mémoire/cache pour faire
> ressortir un point à 14,88 s contre 8,1 s ailleurs) — écarté par les mesures entrelacées une fois
> le run de fond arrêté.
> **Pas encore commité** — la garde d'assertion de `ArbreNoeuds::ajoute` (14 bits pour l'index de
> case, 2 pour la direction) suit la même règle que `TableG::setG` (§7) : un débordement mentirait
> plutôt que de planter.

> 🎯 **LE VRAI MUR N'ÉTAIT PAS LA MÉMOIRE : `QVector` PLAFONNE À 2 Go** (2026-08-13).
> Le 29 mourait sur `std::bad_alloc` **trois runs de suite, au même dépilement** (180 338 000) —
> signature d'une cause déterministe, pas d'une pression système. Diagnostic par conteneur :
> `[BADALLOC] NOEUDS : 167 772 160 -> 335 544 320 entrées (1 280 -> 2 560 Mo) REFUSE`, alors que le
> total n'était que de **7 212 Mo sur 18 Go**.
> Test en isolation : **`QVector` échoue à 2 048 Mo là où `std::vector` passe**, et à 4 096 aussi —
> limite des tailles en `int` de Qt 5, indépendante de la mémoire libre. Le solveur mourait avec
> **11 Go disponibles**. Corrigé en passant `Solveur::noeuds` en `std::vector`.
> ⚠️ **Ce plafond empêchait les deux gains mémoire de servir à quoi que ce soit.**
>
> ⚠️ **TROIS HYPOTHÈSES FAUSSES AVANT LA BONNE, et leur point commun.**
> 1. *« La pointe de réhachage tue les runs »* — l'arithmétique tombait juste (18 898 Mo sur 18 Go),
>    mais le repli posé sur `TableG` **n'a jamais imprimé sa ligne**. **Un calcul cohérent n'est pas
>    une preuve d'identité** : les quatre conteneurs ont des tailles voisines et doublent tous, donc
>    n'importe lequel produit à peu près la même arithmétique.
> 2. *« `footprint` ne voit pas de pic, donc il n'y en a pas »* — l'allocation ÉCHOUE, donc les pages
>    ne deviennent jamais résidentes. **L'instrument ne pouvait pas voir ce qu'on lui demandait.**
> 3. *« Un seuil haut réduira la pointe »* — sauter à la moitié du budget donne **exactement** la
>    pointe du doublement. Codé avant que le calcul ne le montre.
> Ce qui a tranché à chaque fois : aller **nommer** le fait (diagnostic par conteneur, puis test de
> `QVector` en isolation) au lieu de le déduire.
>
> 🎯 **BILAN DE PORTÉE : +77 %, ET ZÉRO NIVEAU.** 78,2 → **45,4 octets par état** (−42 %, `TableG` et
> arène cumulés) ; portée 265,7 → **470,3 M états vus**. De très loin le plus gros run du projet — le
> précédent record était le 26 à 103,6 M. **Et il n'apporte rien** : records du 29 à 9/16 (6,1 M
> dépilements), 10/16 (35,9 M), 11/16 (167,9 M), puis **RIEN entre 167,9 M et 331,5 M**. 163 millions
> de dépilements sans un seul nouveau record. Les trois plongeons échouent **par espace épuisé** (12,
> 15 et 40 états sur des budgets de 121 k, 719 k et 3,3 M) : ce sont des **branches mortes**, comme
> les neuf records du 12 en juillet.
> **La mémoire était un PLAFOND, pas le problème.** Cohérent avec le §3 (le mou est un résidu
> d'ordonnancement) et le §4 (le démêlage est le mur PSPACE) : on a acheté de l'espace d'exploration,
> pas des idées.

> ❌ **DEUX AUTRES LEVIERS ÉVALUÉS, dont un réfuté.** Le coût du sondage linéaire a été mesuré en
> fonction de la charge (`INSTRUM_SONDE`, 1,5 M appels) : **1,89 sonde à 35 %, 4,40 à 70 %** — la
> courbe théorique `(1+1/(1-α)²)/2` s'y cale. Conséquences :
> - **Monter le seuil de 70 % à 85 % est RÉFUTÉ** : la formule donne **22,7 sondes**, soit ×5 sur la
>   boucle la plus chaude après le flood-fill. On échangerait 2 Go contre un solveur cinq fois plus
>   lent à cet endroit.
> - **Croissance ×1,5 au lieu de ×2** : garderait la charge entre 47 et 70 % au lieu de 35-70 %,
>   ~1 Go au mur pour ~+17 % de sondes. Défendable, non fait — exige un modulo au lieu du masque.

> ⚠️ **ET UN PIÈGE DE MESURE À NE PAS REFAIRE** : la même instrumentation sur le **niveau 9**
> (325 k états) donne `noeuds` à **29 %** et l'arène à 34 % — j'ai failli réorienter le chantier vers
> les chaînes de macro sur cette base. À 212 M états c'est 11 % et 43 %. **Un petit run ne dit RIEN de
> la décomposition** : la granularité des blocs (65 536 clés) y écrase tout. Mesurer à l'échelle où le
> mur tombe, jamais ailleurs.

> 🎯 **CONFIRMÉ ET DEVENU DOMINANT le 2026-08-11.** Sur la série des cinq niveaux relancés en
> `ordre-look`, **trois sont morts de la MÉMOIRE et non du temps** (25, 29, 31). Le modèle de ce
> paragraphe, marqué « calculés, pas mesurés », est validé sur **quatre tailles** — 61 Mo par million
> d'états à 13 buts (prédit : 63), 65 à 16, 85 à 19, 88 à 20. La croissance avec le nombre de caisses
> est celle qu'il annonce. **Et le plafond réel de la machine est plus HAUT qu'estimé : 213,7 M états
> pour 14 Go** (pic 15 Go sur 18 Go), contre ~150 M supposés — parce que le coût par état BAISSE en
> cours de run (les arènes allouent par blocs : 131 Mo/M à 9,9 M états, 88 à 34,6 M sur le même run).
> ⚠️ Toute extrapolation mémoire faite tôt dans un run **surestime d'un facteur deux**.
> 🎯 **ET PLUS ON REPOUSSE LE MUR, PLUS L'ARÈNE DOMINE** : 43 % à 212 M états, **51 % à 265 M** sur le
> 29 (46 % sur le 26). Elle croît linéairement avec les états vus, là où `TableG` et la file avancent
> par paliers. C'est donc la cible suivante, et la seule qui reste à cette taille.
> Conséquence : « l'arène est le premier poste à attaquer si le mur redevient bloquant » n'est plus
> une éventualité. Détail en [journal-macro.md](journal-macro.md), 2026-08-09/11.

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
| **loi de l'ordre** (2026-08-07) | **÷2,98** (12) | **inconnue** | ⚠️ **AUCUN — seule ligne vide du tableau** |
| **`ordre-look`** (2026-08-09) | **3 niveaux gagnés** (12, 26, 27) | **inconnue** | ⚠️ **AUCUN** — il GAGNE 12/26/27, est **NEUTRE** sur le 31 (ordre différent, trajectoire identique au dépilement près) et **CASSE** le 32. Change l'ordre de 9 niveaux sur 35 ; les 26 autres sont bit-à-bit identiques |

**Aucun levier n'est universel sauf le couplage.** Le §6.1 l'écrit déjà noir sur blanc pour le
corral (« la fréquence prédit le gain, exactement ») ; ce tableau ne fait que constater que c'est
vrai partout.

> ⚠️ **UN ÉCART À L'ORDRE HUMAIN N'EST PAS UNE ERREUR DE L'ORDRE CALCULÉ** (2026-08-07).
> `mesures/corpus_ordre.py` compare l'ordre calculé aux parties gagnées à la main : **19 des 26
> niveaux du corpus sont identiques**, 7 s'en écartent. Il est tentant de lire ces 7 comme des bugs —
> **c'est faux, et le 12 l'a démontré en une partie**. Son ordre calculé, réputé fautif tout l'été, a
> été rejoué à la main : **il gagne**. L'asymétrie est totale — *0 inversion PROUVE la jouabilité, un
> grand nombre ne prouve rien*, il dit seulement qu'aucune partie enregistrée ne l'a suivi. Les 7
> restants (22, 13, 27, 15, 18, 14, 26) sont **non vérifiés**, pas réfutés. Corollaire : le verrou du
> 12 n'est PAS son ordre — `pas0` montre que les deux ordres échouent au démarrage **à l'identique**
> (même caisse, même case de blocage). Détail en [journal-ordre.md](journal-ordre.md), 2026-08-07 suite.

> ⚠️ **ET L'ASYMÉTRIE SE RETOURNE UNE FOIS L'ORDRE INSTALLÉ DANS LE SOLVEUR** (idée utilisateur,
> 2026-08-22). Sur la JOUABILITÉ, c'est 0 inversion qui prouve (ci-dessus). Sur le SOLVE, c'est
> l'inverse : **un ordre prouvé BON ne garantit pas que le solveur ira au bout — un ordre prouvé
> MAUVAIS garantit qu'il n'ira pas.** Un bon ordre n'est qu'une condition parmi d'autres (il reste
> tout le régime : macro, couplage, mémoire, budget) ; un ordre qui MURE un but ferme la seule
> porte de sortie, quoi que fasse le reste.
> - Côté « bon, et pourtant rien » : le **16** (§6.0, 2026-08-20) — son ordre align est prouvé
>   jouable par une partie humaine gagnante de 196 poussées qui le suit, et `bench 16 loi` rend
>   pourtant **AUCUNE**, espace **ÉPUISÉ** (pas un budget), à cause du régime d'engagement.
> - Côté « mauvais, donc mort » : le **murage local** (§6.0, points 1 et 7) — le plateau du 21
>   mort à 7/13 avec deux buts qu'aucune poussée ne peut plus atteindre, le verrou (17,2) du 10,
>   la poche haute du 18. Là, le verdict tombe **sans lancer le solveur**.
>
> - 🎯 **LA DÉMONSTRATION LA PLUS FORTE, 2026-08-23** : le murage a été RÉPARÉ sur les
>   quatre plateaux où il restait (10 et 20 en align, 13 dans les trois régimes, 200 par
>   défaut). **Un seul tombe — le 200, un banc d'essai** —, le 10 était déjà résolu par
>   ailleurs, et **le 13 comme le 20 ne progressent pas d'un seul but** (`max 9/16` et
>   `max 1/18`, les valeurs de juillet). Le corpus n'a plus un seul ordre muré hors des
>   deux UNSAT prouvés, et ça n'a rendu aucun niveau. **Le côté « bon, et pourtant rien »
>   de cette asymétrie n'est donc pas une exception du 16 : c'est le cas GÉNÉRAL.**
>   Corollaire pour la suite : **le goal-ordering est clos comme piste de déblocage**, ce
>   qui ne retire rien à sa valeur de test d'ÉLIMINATION ci-dessous.
>
> **Conséquence sur le plan d'expérience, et c'est la raison d'être de cette note** : chercher le
> MURAGE est le seul test d'ordre qui rende un verdict DÉFINITIF pour un coût quasi nul — il se
> décide statiquement. Vouloir au contraire valider un ordre par un run qui aboutit, c'est faire
> porter à l'ordre le succès de tout le solveur, et un budget épuisé ne réfute rien (§6.0 point 4 :
> 1 800 s sur le 32 avec l'ordre align injecté, « un budget, pas une preuve »). Donc : **trier les
> ordres par élimination** (murage, murage par accès du joueur), jamais par sélection.

> ⚠️ **LA LOI DE L'ORDRE EST LE CONTRE-EXEMPLE VIVANT DE CE TABLEAU** (2026-08-07). Mesurée pour la
> première fois en GAIN — et non plus seulement en justesse — elle rend **÷2,98 en états et ÷2,06 en
> mémoire sur le 12** (2 097 523 → 704 591, à 212 poussées identiques) et **exactement ZÉRO sur le
> 27** (332 359 des deux côtés, pas un état d'écart). Les deux runs sont sous ordre humain injecté,
> même binaire, une seule variable. **On ne sait pas dire ce qui sépare les deux cas**, alors que
> chaque autre ligne du tableau a produit son prédicteur en même temps que son gain. Première mesure
> à faire, et la moins chère : relever les stats `[LOI]` (enfilages / PRUNES / gel hors tour) sur les
> deux runs — elles partent avec la jauge, donc elles sont déjà là. Détail en
> [journal-hybride.md](journal-hybride.md), session du 2026-08-07.

> ❌ **LOI DE L'ORDRE RETIRÉE ENTIÈREMENT le 2026-08-18** (idée utilisateur, en reprenant en main le
> code du solveur). Le contre-exemple ci-dessus n'a jamais été résolu, et la mesure du jour l'a
> aggravé : `bench 6 loi` rend **`AUCUNE`** — le régime rend **insoluble un niveau RÉSOLU par
> défaut**, pas juste plus lent. Un commentaire déjà présent dans `solveur.h` (§ régime
> `AstarMacroCouplagePlongeonOrdreLoi`, retiré le même jour) l'avait déjà mesuré sans que la
> conclusion soit tirée jusqu'au bout : « la loi seule ne perd que le 6 », classé comme un cas isolé
> plutôt que comme un défaut structurel. Cohérent avec ce que la règle dit d'elle-même dans son propre
> commentaire de code (retiré aussi) : ni `caseMorteLoi` ni `geleHorsTour` ne sont des élagages
> PROUVÉS — une exigence d'ordre, jamais un théorème de géométrie, donc rien ne garantissait qu'elle
> ne coupe pas la seule branche gagnante d'un niveau. **Retiré en entier** : le câblage solveur
> (`loiOrdre`/`loiTropTot`/`StatsLoi`, les régimes `...Loi`/`...OrdreLoi`), la table `Game`
> (`mortesLoi`/`caseMorteLoi`/`casesMortesLoi`/`geleHorsTour`/`calculCasesMortesLoi`), l'overlay UI
> (case à cocher « cases mortes (loi de l'ordre) ») et l'outil `mesures/loi.cpp`. `rangDeBut`/
> `rangDuBut` sont CONSERVÉS (extraits dans `calculRangDeBut()`) : utilisés par `porteBloquee` et par
> les outils `porte`/`ordre`, indépendamment de la loi. Canari revérifié intact.

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

**Ce que ça débloquerait concrètement** : un profilage dirait lesquels des non résolus sont
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
(1/27), ~~**12** (1/15)~~.
> ⚠️ **LE 12 N'EN EST PAS — corrigé le 2026-08-07.** Sans budget, il atteint **10/15** sous son ordre
> calculé (mode `record`). Son `max 1/15` à 120 s était un artefact du budget, c'est-à-dire
> exactement le piège que cette même session énonçait deux paragraphes plus haut (« la progression à
> budget borné ne prédit RIEN ») — appliqué ici à la partition Groupe A / Groupe B elle-même. **Les
> quatre autres n'ont pas été revérifiés** et sont donc suspects au même titre.
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

> ⚠️ **ET LA PARTITION ELLE-MÊME EST TOMBÉE le 2026-08-11.** Le bilan ci-dessous concluait que seule
> survivait « la partition Groupe A / Groupe B ». Elle ne survit plus : le **12** était Groupe A
> (« ne démarre pas ») et il est résolu ; le **26** était Groupe B (7/13, pente +1080) et il est résolu
> — en **103,6 M états et 1 h 30**, quand tous les profilages tournaient à **120 secondes**. Aucun
> budget de deux minutes ne pouvait le voir venir. **Il ne reste donc RIEN de ce profilage.**
>
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
- ⚠️ **UNE POSE SUR UN BUT N'EST PAS UNE LIVRAISON — une caisse peut TRANSITER par
  plusieurs buts avant sa destination réelle** (2026-08-18, en construisant `fpporte.py`). Un
  couloir de buts alignés (niveau 10, colonne x=17) fait glisser chaque nouvelle caisse jusqu'au
  but le plus profond encore libre, en passant par tous les buts plus proches SANS s'y arrêter —
  le journal humain montre `POUSSE caisse ->(16,2) [POSE]` puis, deux coups plus tard,
  `POUSSE caisse ->(17,2) [POSE]`, la MÊME caisse continuant son chemin. Un premier juge FP qui
  testait le porte généralisé (cf. §1 `portegen`) sur CHAQUE pose a rendu ~100 « faux positifs » —
  tous bidons, la caisse n'étant jamais restée sur ces buts intermédiaires. Corrigé en ne testant
  que la DERNIÈRE poussée de chaque caisse (sa position finale, `dernier_coup_de` dans
  `fpporte.py`) : 0 FP sur 1 650 livraisons. Même famille que le piège « compter G elle-même »
  de `stock.py cut` (§6.0, 2026-08-17) : le prédicat était juste, c'est la définition de
  « livrée » qui était trop large.
- ⚠️ **LE FICHIER D'ORDRE INJECTÉ N'A PAS DE SYNTAXE DE COMMENTAIRE** (2026-08-19). Le parseur est
  un `QRegularExpression` **global sur tout le contenu** (`game.cpp`, dans `calculDistancePoussee`) :
  il ramasse `(x,y)` où qu'il soit, en-tête compris. Un fichier `ordre_niveau_XXXX.txt` documenté
  par un en-tête `#` qui cite des coordonnées voit ces coordonnées lues comme des buts, puis la
  vraie liste déclenche `cité DEUX FOIS` → `voulu.clear()` → **ordre calculé CONSERVÉ**. Pris en
  écrivant l'en-tête de `mesures/ordres_humains/ordre_niveau_0010_appui-17-2.txt`, où expliquer le
  rôle du but (17,2) suffisait à casser le fichier. Sauvé par le fait que le refus est BRUYANT — la
  leçon vaut surtout pour ce qu'elle dit du reste : **écrire les coordonnées en toutes lettres
  (`x=17 y=2`) dans tout commentaire de ces fichiers**, et vérifier par
  `grep -oE '\([0-9]+,[0-9]+\)' fichier | wc -l` avant de s'en servir.
- ⚠️ **COMPARER DEUX RÉGIMES QUAND L'ORDRE CHANGE AUSSI N'ISOLE RIEN** (2026-08-19, correction
  utilisateur en direct). J'ai comparé `loi` + ordre injecté (571 053 états) au défaut + ordre par
  défaut (249 913) et conclu « le régime `loi` est 2,3× pire ». **Faux : deux variables bougeaient**,
  le régime ET l'ordre — et comme `PRUNES=0`, la loi ne faisait rien du tout, l'écart était
  intégralement dû à l'ordre. Le témoin qui tranche existait dans la même campagne et disait
  l'inverse : sur le 17, `loi` contre défaut rend **+3 états sur 18 636 (+0,016 %)**, donc le régime
  en lui-même est gratuit. **Règle : nommer les variables AVANT de lire un écart** — un régime qui
  recalcule l'ordre en change deux à la fois, et le sous-produit (l'ordre) peut peser cent fois plus
  que l'objet mesuré (l'élagage). Même famille que le §6.6 sur les prédicteurs, appliquée à
  l'attribution d'un coût plutôt qu'à celle d'un gain.
- ⚠️ **UNE LIGNE DE `scores.md` PEUT ÊTRE PÉRIMÉE D'UN FACTEUR 8** (2026-08-19). Le niveau 10 y
  porte **2 175 724 états** (`703f851`, 2026-07-29, confirmé sur Linux à l'unité) ; le même
  `bench 10 coupl-plongeon` sur `92f3ecd` rend **249 913**. Le chiffre n'est pas faux, il est
  VIEUX — les chantiers mémoire d'août et `ordre-look` sont passés entre les deux. J'ai annoncé un
  « ÷3,81 » fondé dessus avant que le témoin ne le réduise en miettes. C'est la règle « jamais à un
  chiffre écrit » (§1) prise en défaut sur le fichier qui fait justement foi : **`scores.md` fait foi
  sur QUI est résolu, pas sur COMBIEN ça coûte aujourd'hui.** Tout écart chiffré se remesure binaire
  contre binaire, y compris contre ce fichier-là.
- ⚠️ **UN SCAN EXHAUSTIF SANS FILTRE MESURE LA GÉOMÉTRIE, PAS LE PROBLÈME** (2026-08-18, en
  mesurant l'ampleur du porte généralisé). Tester TOUTES les paires (caisse × but) à chaque
  jalon rend 71 % de coupes (`ampleurporte.py`) — un chiffre qui a l'air massif mais qui NE
  DISCRIMINE RIEN : résolus et non-résolus sont touchés aux mêmes taux (67-81 % partout). La
  raison est bête une fois vue : la plupart des paires testées sont des caisses et des buts dans
  des coins du plateau qui n'ont jamais eu vocation à interagir — le test mesure alors « des
  régions disjointes ne se voient pas », un fait trivial de géométrie. Restreindre au SEUL but
  que le solveur choisirait réellement (rang minimal de `ordreButs`, cf. `ampleurporte2.py`) fait
  tomber le chiffre à 3,9 % — la mesure utile. **Un scan exhaustif est un test de PRÉSENCE, pas de
  PERTINENCE : sans filtrer sur ce que le solveur regarderait VRAIMENT, un grand nombre ne prouve
  rien** — même leçon que le §6.6 sur les prédicteurs a posteriori, appliquée ici à une mesure
  d'ampleur plutôt qu'à un gain.
- ⚠️ **LE SUSPECT QUI RESTE APRÈS ÉLIMINATION N'EST PAS PROUVÉ COUPABLE** (2026-08-23, la
  leçon la plus chère de la journée). Le 2026-08-22, quatre causes possibles au blocage du
  200 : élagage, relégation, couplage, génération des macros. Les trois premières ont été
  éliminées **une par une, sur pièces** — et la conclusion est tombée sur la quatrième
  **sans jamais la tester**, écrite en gras comme « le vrai résultat ». Or la vraie cause
  n'était dans aucune des quatre : **l'ordre par défaut du 200 était MURÉ**, et
  `ordre level0200.xsb` le disait en une seconde, avec un outil qui existait depuis le
  2026-07-29. Le raisonnement par élimination ne vaut que si la liste est complète, et rien
  ne garantit qu'elle l'est. **Interroger le suspect restant coûtait ici moins cher que
  d'éliminer n'importe lequel des trois autres.** Même famille que « nommer les variables
  AVANT de lire un écart » (2026-08-19), appliquée à l'attribution d'une CAUSE.
- ⚠️ **`2>/dev/null` SUR UN BENCH JETTE LA LIGNE QUI TRANCHE** (2026-08-23). Comparant deux
  régimes sur le 200, l'un rendait `AUCUNE` en 18 630 enfilages et l'autre tournait sans
  conclure : j'en ai conclu que le premier tronquait l'espace, et c'était juste — mais j'ai
  ajouté qu'il était donc « le verrou », ce qui était faux. **Les deux plafonnaient au même
  `max 13/15`**, et cette ligne partait sur `stderr`, dans le `/dev/null` de ma propre
  commande. Le §1 dit déjà « la jauge part sur stderr, et un pipe l'avale » à propos des
  niveaux qu'on ne résout pas ; il faut le lire aussi pour les **comparaisons de régimes** :
  deux runs qui diffèrent par leur mode de SORTIE peuvent être identiques par ce qui compte.
  Rediriger vers un fichier, toujours — `2>jauge.txt`, jamais `2>/dev/null`.
- ⚠️ **RELANCER UN CORPUS QUAND RIEN N'A CHANGÉ POUR LUI NE MESURE RIEN** (2026-08-23,
  objection de l'utilisateur, retenue). Après la correction d'ordre, le réflexe « relancer
  les 15 non-résolus » (§0 : « relancer après chaque promotion ») allait consommer 2 h 30 de
  machine — alors que le canari d'ordre venait d'établir que **14 des 15 gardaient un ordre
  identique au caractère près**, pour un code par ailleurs inchangé : trajectoire
  déterministe, résultat connu d'avance. Et l'argument « ça rafraîchit des chiffres périmés »
  ne tenait pas : **un non-résolu n'a pas de ligne dans `scores.md`**, il n'y a rien à
  rafraîchir. La règle du §0 reste juste, mais elle se lit « relancer ce que la promotion
  TOUCHE » : chercher d'abord OÙ le changement mord — ici en balayant les trois régimes
  d'ordre, ce qui a fait apparaître le 10 et le 20, invisibles dans le régime par défaut.
  **Trois runs au lieu de quinze, et ce sont les trois qui disaient quelque chose.**
- ⚠️ **REJOUER UN JOURNAL SOUS UN AUTRE ORDRE QUE CELUI DE LA PARTIE EST FAUX EN SILENCE**
  (2026-08-23, en étendant `mesures/rejeu` aux macros). Une macro vise `butActif()`, donc
  tout verdict la concernant dépend de `ordreButs`. La partie du 200 avait été jouée sous
  l'ordre align ; `rejeu` recalculait l'ordre par défaut, et **11 verdicts sur 15 étaient
  faux** — sans le moindre signe, puisque le rejeu des COUPS, lui, marchait parfaitement
  (les coups sont des directions, ils ne dépendent d'aucun ordre). Corrigé en lisant la
  SOURCE dans l'en-tête que l'UI écrit exprès (`calcule` / `⚠ PAR ALIGNEMENT` / `⚠ INJECTE
  depuis …`), plus un garde qui refuse de juger quand le but du journal et `butActif()`
  divergent. **L'annotation de source ajoutée le 2026-08-19 « pour la relecture humaine »
  servait en fait à une machine** — argument à garder quand on hésite à faire dire à une
  trace d'où vient ce qu'elle contient.
- ⚠️ **UNE MÉMOÏSATION D'ÉCHECS DOIT DISTINGUER L'ÉPUISEMENT DE L'ABANDON** (2026-08-23, en
  mémoïsant `ordreParPrecedence`). Marquer « stérile » un sous-ensemble dont on a réellement
  essayé tous les candidats est une preuve ; le marquer parce qu'on a **cessé de l'explorer
  faute de budget** ferait rater un ordre sain, et le murage reviendrait **sans que rien ne
  le signale** — ni le canari (qui ne verrait qu'un niveau non résolu de plus) ni l'outil
  `ordre` (qui dirait « muré » sans dire pourquoi). C'est la distinction UNSAT / budget que
  le §6.0 exigeait déjà de `ordredp`, et elle vaut partout où l'on met un résultat en cache :
  **on ne cache un verdict que s'il est prouvé, jamais s'il est seulement constaté.**
  Second piège de la même famille, évité par construction : la clé doit porter TOUT ce dont
  la suite dépend — ici `salleCourante` en plus du sous-ensemble, sans quoi on couperait des
  branches valides.

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
- **Le régime `ordre-look`** (`AstarMacroCouplagePlongeonLook`, 2026-08-08, `5aeae01`) — a fait tomber
  le **12, le 26 et le 27**. Trois pièces : `Game::ordreLookahead` (drapeau, copié dans les ctors de
  copie ET de déplacement comme `ordreDynamique`), `Game::setOrdreLookahead()` qui **recalcule
  `ordreButs` sur place** (donc à poser sur l'état de DÉPART, avant la recherche), et
  `Game::installeOrdreParPrecedence()` — exemplaire unique de l'installation, extrait exprès de
  `calculDistancePoussee` pour que le recalcul rejoue le même repli rebours.
  La règle elle-même est **au rang 0 seulement**, **après** les clés de contiguïté, et **confinée à la
  salle de tête** ; chacune des trois restrictions a été payée par une régression mesurée (190, 10, 32
  — cf. [journal-ordre.md](journal-ordre.md), 2026-08-08). ⚠️ C'est un **re-tri d'un sous-ensemble
  APRÈS le tri**, pas une clé du comparateur : une clé qui ne s'applique qu'entre certains couples
  n'est pas un ordre strict faible et `std::sort` partirait en comportement indéfini.
  ⚠️ `setOrdreLookahead` **refuse de recalculer si un ordre est injecté** (fichier ou `ORDRE_HUMAIN`)
  et le dit sur stderr — sans ce garde, le régime écrasait l'injection en silence (§7).
- **`jugemacro.h`** (neuf, 2026-08-23) — `jugeMacro()`, le verdict « le solveur produirait-il
  CETTE macro, à CET état ? », en **exemplaire unique** partagé par l'UI (`mainwindow.cpp`,
  ligne `[macro-rang]`) et `mesures/rejeu`. À la racine et non dans `mesures/`, pour que la
  dépendance aille de l'outil vers l'app et jamais l'inverse. Il n'est appelé par **aucun
  chemin du moteur** : le canari ne peut pas bouger.
- ⚠️ **`Game::ordreParPrecedence` porte une MÉMOÏSATION PAR SOUS-ENSEMBLES** (2026-08-23) —
  `butMureLocalement` ne dépend que de l'ENSEMBLE des buts posés, jamais de leur ordre, donc
  un sous-ensemble prouvé stérile ne se ré-explore pas. **La clé porte aussi `salleCourante`**
  (les passes 1/2 classent les candidats d'après elle) et **seul l'épuisement RÉEL s'inscrit,
  jamais un abandon par budget** — l'inverse ferait rater un ordre sain en silence. C'est ce
  qui a réparé le 13, le 20 (en align), le 10 (en align, l'ordre jadis injecté à la main) et
  le 200.
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
