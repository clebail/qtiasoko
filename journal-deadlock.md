# Journal — détection de deadlock

> **Journal de chantier, détaché de [plan.md](plan.md) le 2026-08-06.** Le document
> avait atteint 5 588 lignes et 386 Ko, dont 90 % de journaux de session ; on ne le
> relisait plus en entier. Ce fichier porte **§6.1** — corral unitaire, pince, corral-N, paquet non livrable.
>
> ⚠️ **La numérotation d'origine est conservée** (`§6.1` et ses sous-titres) : le plan
> et les autres journaux s'y réfèrent des dizaines de fois, et un renvoi qui ne désigne
> plus rien est pire que pas de renvoi. Les sessions restent dans l'ordre chronologique
> où elles ont été écrites.

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
