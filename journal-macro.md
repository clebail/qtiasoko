# Journal — goal macro et plongeon

> **Journal de chantier, détaché de [plan.md](plan.md) le 2026-08-06.** Le document
> avait atteint 5 588 lignes et 386 Ko, dont 90 % de journaux de session ; on ne le
> relisait plus en entier. Ce fichier porte **§6.3** — coût par état, forks, plongeon sur record, deltaf.
>
> ⚠️ **La numérotation d'origine est conservée** (`§6.3` et ses sous-titres) : le plan
> et les autres journaux s'y réfèrent des dizaines de fois, et un renvoi qui ne désigne
> plus rien est pire que pas de renvoi. Les sessions restent dans l'ordre chronologique
> où elles ont été écrites.






<!-- INDEX DES SESSIONS -->

**15 sessions.** Verdict en tête : ✅ acquis · ❌ réfuté · ⏸️ sans verdict ·
🎯 résultat marquant · 🎉 niveau tombé · ⚠️ correction · 📖 lecture. Les titres sont
exacts, une recherche sur la date ou sur un mot du sujet tombe dessus.

| | date | sujet |
|---|---|---|
| ✅ | 2026-07-21 suite 2 | le COÛT PAR ÉTAT de la goal macro (outil `macro`) |
| ⏸️ | 2026-07-23 | pourquoi la macro échoue si souvent : `echecBloque`, pas les forks |
| ✅ | 2026-07-23 suite | LE 8 TOMBE, sans aucune modif de code |
| 🎯 | 2026-07-24 | la macro PROMEUT-elle ses enfants ? (outil `deltaf`, à `6b0a024`) |
| ⏸️ | 2026-07-24 suite | le régime « BUT DU COUPLAGE » codé, mesuré, EN ATTENTE du 12 |
| 🎯 | 2026-07-28 1/4 | MESURE PRÉALABLE du PLONGEON (avant toute ligne de solveur) |
| ✅ | 2026-07-28 2/4 | PLONGEON CODÉ et MESURÉ : la prédiction tombe juste |
| ❌➡️✅ | 2026-07-28 3/4 | LE SEUIL EN % RÉFUTÉ, remplacé par un BUDGET RELATIF |
|  |  | 🎉🎉 2026-07-28 (4/4) — **LE NIVEAU 11 EST RÉSOLU**, deux fois le même jour |
| ✅ | 2026-07-29 1/4 | LE PLONGEON À L'ÉPREUVE DE DEUX NIVEAUX NEUFS (10, 21) |
| ⚠️➡️❌ | 2026-07-29 2/4 | le log des plongeons N'EST PAS un détecteur de branche morte |
| 🎯 | 2026-07-29 3/4 | le log des plongeons, ce qu'il dit vraiment |
| ⏸️ | 2026-07-29 4/4 | LE 31 : 188 M états vus, arrêté sans verdict |
| 🎯 | 2026-08-07 1/2 | LE JOUEUR PRIS POUR UN OBSTACLE : la macro ne savait jouer aucun DEMI-TOUR |
| ❌ | 2026-08-07 2/2 | LA TOLÉRANCE AU DÉTOUR mesurée puis RÉFUTÉE — 5 macros sur 205 |

<!-- FIN INDEX -->

### 6.3 La goal macro et le plongeon sur record

> ⚠️ **Cette section n'avait AUCUN titre dans `plan.md`** — ses sessions vivaient sous le
> §6.2, alors que le document la cite **38 fois**. C'est ce qui faisait paraître le §6.2
> démesuré. Le titre est ajouté ici ; le contenu n'a pas bougé.

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

#### 🎯 Session du 2026-07-28 (1/4) — MESURE PRÉALABLE du PLONGEON (avant toute ligne de solveur)

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

#### ✅ Session du 2026-07-28 (2/4) — PLONGEON CODÉ et MESURÉ : la prédiction tombe juste

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

#### ❌➡️✅ Session du 2026-07-28 (3/4) — LE SEUIL EN % RÉFUTÉ, remplacé par un BUDGET RELATIF

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

#### 🎉🎉 2026-07-28 (4/4) — **LE NIVEAU 11 EST RÉSOLU**, deux fois le même jour

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

#### ✅ Session du 2026-07-29 (1/4) — LE PLONGEON À L'ÉPREUVE DE DEUX NIVEAUX NEUFS (10, 21)

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

#### ⚠️➡️❌ Session du 2026-07-29 (2/4) — le log des plongeons N'EST PAS un détecteur de branche morte

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

#### 🎯 Session du 2026-07-29 (3/4) — le log des plongeons, ce qu'il dit vraiment

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

#### ⏸️ Session du 2026-07-29 (4/4) — LE 31 : 188 M états vus, arrêté sans verdict

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

#### 🎯 Session du 2026-08-07 (1/2) — LE JOUEUR PRIS POUR UN OBSTACLE : la macro ne savait jouer aucun DEMI-TOUR

**Point de départ, constat utilisateur sur un plateau exporté du niveau 2** (une seule caisse restante
en (11,7), un seul but vide en (2,1)) : *« la caisse en (11,7) ne déclenche pas de macro. Le chemin
précalculé passe par la ligne 7, colonne 11 ⇒ 5. Pourquoi la macro ne passe pas par la colonne 11 ? »*

**La colonne 11 n'était pas le sujet — et le précalcul avait raison.** `distanceParBut` pour le but
(2,1) rend `(11,7)=17`, `(10,7)=16`, `(11,6)=**18**` : monter coûte **+1**. La ligne 7 est bien la
route la plus courte. Le défaut était en aval, à la sixième poussée.

**🔴 LE BUG, une ligne, dans le contrat de descente `avanceVersBut` (`game.cpp:2736`) :**

```cpp
if (!isLibre(devant)) return -1;         // arrivée occupée (mur / autre caisse)
```

`isLibre` ne rend vrai que pour `tcNone` et `tcGoal` (`game.cpp:642`). La case où se tient le **joueur**
est `tcPlayer`/`tcGoalPlayer` — donc **fausse**. Or le joueur n'est jamais un obstacle pour une
poussée : `pousse()` le téléporte sur la case d'appui avant que la caisse n'avance.

**Et l'autre exemplaire de la règle l'avait, en le commentant** (`getCaissesDeplacable`, `game.cpp:661`) :

```cpp
// Le joueur libère sa propre case en marchant vers le point de
// poussée avant de pousser : elle compte comme libre même si
// elle est actuellement occupée par lui.
if(isLibre(idxDestination) || idxDestination == idxPlayer) {
```

**Deux exemplaires de la même règle, un seul l'appliquait.** Les poussées simples savaient, la macro
non. C'est le motif du §7 (« énumérer TOUTES les entrées qui atteignent le compteur »), sous une
cinquième forme : une exemption écrite à un endroit et pas à l'autre.

> **CE QUE ÇA COÛTAIT EXACTEMENT, ET C'EST STRUCTUREL.** Après une poussée, le joueur est **par
> construction** sur la case d'où la caisse vient. Toute poussée qui ramène la caisse en arrière a donc
> pour destination la case du joueur — **le test refusait tout DEMI-TOUR, sans exception**. Or le
> demi-tour est le **RECUL** du §3 : les 0,8 à 2,8 % de poussées qui portent la **totalité du mou**, et
> dont `moureel` a mesuré que la caisse revient sur la case libérée **9 fois sur 10**. La macro ne
> pouvait en jouer aucun.

Sur le plateau d'origine, la trace le montre au pas 6 — le test de monotonie PASSAIT :

```
pas 6 : caisse (5,7)  reste 11  joueur (6,7)
        Droite -> (6,7) : libre=0  ❌   (rApres=3, dpb=11, attendu 11)
```

**Le correctif**, aligné sur l'exemplaire qui avait la règle :

```cpp
const int idxPlayer = playerPoint.x() + playerPoint.y() * largeur;
if (!isLibre(devant) && devant != idxPlayer) return -1;
```

C'est un **RELÂCHEMENT** : plus de macros disponibles, jamais moins. Il ne peut pas produire de fausse
solution (`pousse()` valide le coup, `zone[appui]` valide l'appui), mais il change l'arbre partout où
la macro tourne — d'où le canari, obligatoire.

**✅ CANARI INTACT ET GAIN NET** (macro, binaire contre binaire, ancien reconstruit depuis `1308642`
par `git worktree`, macOS arm64) :

| niv | états REF | états NEW | | coups REF → NEW |
|---|---|---|---|---|
| **17** | 24 786 | **18 636** | **÷1,33** | 569 → **561** |
| **2** | 412 | **364** | ÷1,13 | 541 → **523** |
| **9** | 354 623 | **325 250** | ÷1,09 | 678 = |
| **4** | 55 560 | **55 095** | −0,8 % | 945 → **943** |
| 0, 1, 3, 5, 6, 7, 190, 191 | — | identiques | — | = |

**Poussées identiques sur les douze** : 4/97/131/134/355/143/110/90/237/213, 190=220, 191=250.
**Aucun niveau dégradé.** Les **coups** baissent à poussées égales : même solution, moins de marche —
c'est le demi-tour joué en macro au lieu d'être reconstruit en poussées simples.

---

**⚠️ LE SECOND PLATEAU N'EST PAS LE MÊME PROBLÈME — et il ne faut pas le corriger.** L'utilisateur a
ensuite exporté le même niveau 2 avec **toutes les caisses restantes** (6 hors but). Là, aucune macro,
pour aucune caisse. Ce n'est pas un bug : mesuré par retrait progressif des caisses, but actif (1,1).

| plateau | macro | où (11,7) s'arrête |
|---|---|---|
| toutes les caisses | **NON** | (10,7) — (9,7) et (10,6) sont des caisses |
| sans (9,7) | **NON** | plus loin, sur (7,7) |
| sans (9,7), (7,7) | **NON** | (5,7) — `appui (4,7) HORS ZONE` |
| sans (9,7), (7,7), (4,7) | **OUI** | **ARRIVÉE au but** |

- **La troisième caisse ne bloque pas sur le trajet.** (4,7) n'est jamais traversée : c'est la case où
  le joueur doit se tenir pour faire le **demi-tour** en (5,7). Une caisse garée **à côté** du chemin
  suffit à tuer la macro, et rien dans le trajet ne le laisse voir.
- **Contrat réel de la macro, à écrire noir sur blanc** : elle exige que **tout le trajet solo soit
  libre, PLUS les appuis de ses demi-tours**. Elle ne sait pas contourner d'une case.
- La table dit vrai jusqu'au bout, vérifié à la main sur ce plateau : colonne 11 = 4+5+2+3+3+2 = **19** ;
  ligne 7 = 16 de trajet **+ 2 pour le demi-tour en (5,7)** = **18**. `distanceParBut` **encode le
  demi-tour** — c'est bien la descente, et elle seule, qui ne savait pas l'exécuter.

> 🎯 **LA FORMULATION QUI MANQUAIT AU PLAN : `h` ET L'ITINÉRAIRE DE LA MACRO PARTAGENT UNE TABLE, ALORS
> QU'ILS N'ONT PAS LA MÊME CONTRAINTE.** `h` doit ignorer les autres caisses sous peine d'être
> inadmissible — c'est « caisses = murs », réfuté au §4 **comme borne**. La macro, elle, n'est qu'un
> **GUIDE** : elle a le droit de les voir. Un itinéraire faux ne produit jamais de fausse solution, il
> fait échouer la macro **visiblement** — exactement la propriété LOUD que la session du 2026-08-06
> cherchait pour le report de macro. **Rien n'est codé, rien n'est mesuré** : le coût (un champ de
> distance par état) est le vrai sujet, et il n'a pas été estimé.

---

**📖 LEÇON DE MÉTHODE — c'est la RÉPLIQUE du contrat qui a localisé le bug, pas sa lecture.** Le
raisonnement au crayon sur `avanceVersBut` a conclu **trois fois** que la poussée devait passer (région
correcte, zone correcte, monotonie correcte). Ce qui a tranché : réécrire la descente dans `pas0` avec
l'**API publique seule**, puis la confronter à la vraie fonction sur le même couple. La réplique
arrivait au but, l'originale non — **l'écart entre deux implémentations de la même règle est un
localisateur, là où relire une fonction ne l'est pas.**

⚠️ **Un piège de lecture au passage** : `macroVersButBacktrack` rend `poussees.size() == 0` sur TOUT
échec (le vecteur n'est affecté qu'en cas de succès). J'y ai lu « il échoue avant d'avoir joué » alors
qu'il échouait au sixième pas. **La taille du chemin rendu ne dit rien du point d'échec** ; seul
`essais` (nombre de branches) est informatif.

**État du code** (non commité, sur `ordre-dynamique`, base `1308642`) :
- `game.cpp` / `game.h` — le correctif d'`avanceVersBut` ; **`Game::champDistanceBrut(indexBut)`**,
  accesseur const sur `distanceParBut` vu de la région du joueur courant. Même motif que
  `getOrdreButs()` : un accesseur de mesure plutôt qu'un `qgetenv` de debug dans le chemin chaud (§7).
- `mesures/pas0.cpp` — accepte un **chemin `.xsb`** (comme `bench`/`loi`/`ordre`), plus deux modes.
  `champ` imprime **les deux champs côte à côte** — le BRUT (ce que la macro croit devoir suivre) et le
  JOUABLE (ce que la descente accepte) : les lire ensemble est le seul moyen de séparer « la table se
  trompe » de « la table a raison mais la descente ne sait pas l'exécuter ». `trace` rejoue la descente
  pas à pas en donnant, pour chaque direction, la raison du refus (`MUR` / `caisse` / `appui HORS ZONE`
  / `NON MONOTONE`), et se confronte à la vraie fonction en fin de sortie.
  ⚠️ Charger une fixture de milieu de partie **recalcule `ordreButs`** (piège §7) — vérifié ici que le
  but actif est le même que dans l'app (rang 4 de l'ordre du niveau 2, `(1,1)`) avant de conclure.

**Reste ouvert :**
- [ ] **Relancer les non-résolus.** Le §0 le dit : la frontière bouge quand les leviers changent, et
  le 10 et le 21 sont tombés sans une ligne de code neuve. Ce correctif rend disponibles des macros
  qui ne l'étaient nulle part — c'est exactement le cas de figure.
- [ ] **L'itinéraire de la macro séparé de la table de `h`** (encadré ci-dessus). À discuter avant de
  coder ; chiffrer d'abord le coût d'un champ de distance par état.
- [ ] **Chercher les autres exemplaires de la règle du joueur-obstacle.** Les deux trouvés sont
  `avanceVersBut` et `getCaissesDeplacable` ; rien ne garantit qu'il n'y en ait pas un troisième
  (`diagnosticPas0` et `macroPeutDemarrer` délèguent, donc sont couverts).

#### ❌ Session du 2026-08-07 (2/2) — LA TOLÉRANCE AU DÉTOUR : mesurée, puis RÉFUTÉE le jour même

**Point de départ, constat utilisateur sur un plateau exporté du niveau 3** (5 caisses en colonne 10,
5 buts vides, but actif `(2,8)` — rang 6 de l'ordre du 3, identique dans la fixture et dans l'app) :
*« les caisses en (10,3), (10,5) et (10,6) devraient déclencher une macro, mais effectivement un poil
plus longue que l'optimum ».*

**L'intuition était juste sur ce plateau.** Le trajet solo descend la **colonne 10**, bouchée par les
quatre autres caisses ; la seule issue est la **colonne 11**, refusée par la descente monotone. Mesuré
par recherche bornée où seule la caisse suivie bouge (mode `detour`, ci-dessous) :

| caisse | trajet solo | réel | écart |
|---|---|---|---|
| (10,3) | 13 | 17 | **+4** |
| (10,5) | 11 | 13 | **+2** |
| (10,6) | 10 | 12 | **+2** |

(10,2) et (10,4) sont absentes : elles ne sont poussables dans **aucune** direction (masque
`getCaissesDeplacable` à 0), coincées entre leurs voisines.

**⚠️ L'ÉCART EST TOUJOURS PAIR, et c'est le §3 qui le dit.** `Δh` ne vaut jamais que −1 ou +1 le long
d'un chemin ; un trajet de longueur `L` partant de `d₀` et finissant à 0 vérifie donc
`L ≡ d₀ (mod 2)`. Les paliers de détour sont **2, 4, 6** — il n'y a pas de réglage fin à chercher. Et
la conversion est directe (`mou = 2 × reculs`) : **un détour à +2 est exactement UN recul**. Ce que la
macro ne sait pas jouer ici, ce n'est pas une bizarrerie de son contrat, c'est **la congestion
elle-même, dans l'unité où le §3 la mesure**.

**❌ LE BALAYAGE SUR LES 25 FIXTURES DISPONIBLES RÉFUTE LA GÉNÉRALISATION** (190-199, les neuf records
du 13, plateau02, plateau02_2, plateau03, plateau16, plateau16_2, stock16 ; budget +8) :

| | caisses poussables |
|---|---|
| examinées | **205** |
| atteignent le but actif à **+0** — la macro les a déjà | 22 |
| atteignent à **+2** | **5** |
| atteignent à +4 | 1 |
| atteignent à +6 | 1 |
| **n'atteignent pas, même à +8** | **176 (86 %)** |

> **Un budget de détour ajouterait 5 macros sur 205. Et les 7 gains sont TOUS sur les trois plateaux
> exportés ce jour-là** — sur les 22 fixtures antérieures, il en ajoute **exactement zéro**. Le coût,
> lui, serait une recherche bornée par caisse et par état, alors que le §6.3 a optimisé dans l'autre
> sens (pré-test avant copie, 48,5 % des tentatives mortes au pas 0 sur le 11). **Piste fermée.**

**🎯 CE QUE LE 86 % APPREND, ET C'EST LE VRAI RÉSULTAT DE LA SESSION.** Ces 176 caisses ne sont pas
bloquées par un chemin trop long : elles **n'atteignent pas le but du tout** tant que les autres
caisses ne bougent pas. Le mode d'échec dominant de la macro n'est donc **pas** « l'itinéraire est un
peu plus long », c'est **« il faut d'abord dégager quelqu'un d'autre »** — et aucun budget de détour ne
corrige ça, par construction : la macro déplace UNE caisse. C'est le démêlage, c'est-à-dire le mur
PSPACE du §4. Premier chiffre mis sur ce mode d'échec.

**Deux réserves, à garder avec le chiffre :**
- **Les autres caisses sont traitées en MURS** dans cette recherche. Légitime pour un *itinéraire*
  (« cette caisse peut-elle y aller sans qu'on touche aux autres ? »), mais ça SURESTIME la
  difficulté — c'est le piège « caisses = murs » du §4, ici assumé et borné à sa seule lecture valide.
  Le 86 % est donc un majorant du « il faut dégager quelqu'un », pas une mesure exacte.
- **L'échantillon est biaisé DANS LE SENS QUI AURAIT DÛ FAVORISER L'IDÉE** : les trois plateaux du
  jour ont été exportés *parce que* la macro y échouait. Que le gain soit nul sur les 22 autres n'en
  est que plus net.

**⚠️ PIÈGE DE DÉPOUILLEMENT — la première agrégation était FAUSSE et parfaitement plausible.** Elle
annonçait **98,2 % à +0 et 1,3 % à +2**, ce qui se lisait comme « la macro fait déjà presque tout ».
Cause : le parseur retenait les lignes commençant par `( x, y)`, or **le tableau des buts en fin de
sortie commence pareil** — 11 lignes de buts par fixture comptées comme des caisses à écart 0.
**Ce qui l'a démasquée, c'est une colonne CONSTANTE** : « 15, 15, 15, 15, 14… » sur les level019x,
soit le nombre de BUTS et non de caisses, sur des plateaux dont le nombre de caisses varie. Même
leçon que le §7 (« vérifier la cohérence interne d'un relevé avant de raisonner dessus ») : ici
l'incohérence était lisible sans rien relancer.

**État du code** : `mesures/pas0.cpp` seulement — deux modes neufs, rien dans le solveur.
- `multi [x,y]` — combien de macros DISTINCTES une caisse peut-elle produire ? Énumère toutes les
  descentes monotones et compte les **états** distincts. Mesuré : jusqu'à **4 chemins**, **toujours
  1 seul état**. Structurel : toutes les descentes monotones ont la MÊME longueur (chaque pas retire
  exactement 1), la configuration de caisses finale est identique, et la clé ne retient que la **ZONE**
  du joueur — deux chemins ne divergent que si leurs dernières poussées viennent de côtés
  **déconnectés une fois la caisse posée**. Jamais observé sur 17 fixtures. **Enfiler les forks ne
  produirait donc que des doublons** que la dédup rejetterait.
- `detour [tout|x,y] [budget]` — l'écart au trajet solo, par recherche bornée à une seule caisse
  mobile. ⚠️ **Ce n'est PAS une borne pour `h`** (caisses en murs = surestimation) : c'est un
  itinéraire. La distinction est celle de la session 1/2.

**Reste ouvert :**
- [ ] **Le vrai levier reste « dire *plus tard* sur une CAISSE »** (journal-hybride, 2026-08-06) :
  refuser une macro laisse `macrosOk` à 0 et enclenche le repli tout seul. Le trou est le PRÉDICAT,
  pas le câblage — et le 86 % ci-dessus dit que c'est bien là que ça se joue, pas dans l'itinéraire.
- [ ] La macro **PARTIELLE** (s'arrêter en route au lieu de tout jeter) n'a pas été mesurée. Sur le
  plateau du 2, la caisse (11,7) avance de deux poussées puis se mure, et cette avance est perdue.
