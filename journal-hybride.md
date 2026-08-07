# Journal — la campagne hybride

> **Journal de chantier, détaché de [plan.md](plan.md) le 2026-08-06.** Le document
> avait atteint 5 588 lignes et 386 Ko, dont 90 % de journaux de session ; on ne le
> relisait plus en entier. Ce fichier porte **§6.2 (suite)** — les parties jouées à la main et annotées, du 2026-08-01 au 2026-08-06.
>
> ⚠️ **La numérotation d'origine est conservée** (`§6.2 (suite)` et ses sous-titres) : le plan
> et les autres journaux s'y réfèrent des dizaines de fois, et un renvoi qui ne désigne
> plus rien est pire que pas de renvoi. Les sessions restent dans l'ordre chronologique
> où elles ont été écrites.






<!-- INDEX DES SESSIONS -->

**17 sessions.** Verdict en tête : ✅ acquis · ❌ réfuté · ⏸️ sans verdict ·
🎯 résultat marquant · 🎉 niveau tombé · ⚠️ correction · 📖 lecture. Les titres sont
exacts, une recherche sur la date ou sur un mot du sujet tombe dessus.

| | date | sujet |
|---|---|---|
| 🎯 | 2026-08-01 1/7 | LE MODE HYBRIDE : cinq niveaux joués à la main, le solveur en tient le LOG |
| ❌ | 2026-08-01 2/7 | LE RANG DU COUP HUMAIN : le démêlage n'est PAS un problème de CLASSEMENT |
| 🎯 | 2026-08-01 3/7 | LE 13 GAGNÉ EN HYBRIDE : 89 % de la solution N'EST PAS DANS L'ARBRE |
| 🎉 | 2026-08-01 4/7 | LE 20 GAGNÉ À LA MAIN : le 13 est une SINGULARITÉ, et le détour non-monotone est CONFIRMÉ |
| 🎉 | 2026-08-01 5/7 | SEPT NON-RÉSOLUS GAGNÉS À LA MAIN, et trois généralisations mortes |
| ✅ | 2026-08-01 6/7 | LE CORRECTIF MULTI-SALLES CODÉ ET PROMU : ×7,5 sur le 10 |
| ❌ | 2026-08-01 7/7 | LE 12 : c'est le SENS de parcours qui décide, pas le groupement |
| ✅ | 2026-08-02 | LE CORPUS D'INTENTIONS SUR 8 NIVEAUX : le vocabulaire fermé TIENT |
| 🎯 | 2026-08-02 suite | ONZE NIVEAUX RÉ-ANNOTÉS : l'instrument décidait du vocabulaire |
| 🎯 | 2026-08-03 | QUATRE NIVEAUX GAGNÉS À LA MAIN (14, 15, 16, 17), et LA LOI DE L'ORDRE ENFIN TROUVÉE |
| 🎯 | 2026-08-04 1/4 | LA LOI CODÉE ET JUGÉE, le GEL HORS TOUR, et une précédence d'espèce neuve |
| ⏸️ | 2026-08-04 2/4 | LA CONTRAINTE DE PORTE GREFFÉE SUR `butActif()` |
| 🎯 | 2026-08-04 3/4 | LE PLAN HUMAIN MESURÉ : ce qui sépare les résolus, et pourquoi le planificateur naïf est mort |
| 📖 | 2026-08-04 4/4, lecture | LES GADGETS DE CULBERSON, et le vocabulaire qui manquait au verrou |
| 🎯 | 2026-08-05 | LE 16 DÉCOMPOSÉ EN GADGETS (partiel) : deux transistors vérifiés, le stock encore un trou |
| ⏸️ | 2026-08-05 fin | LE FIL GADGETS REFERMÉ, faute de pouvoir séparer |
| 🎉 | 2026-08-06 | LE 27 TOMBE PAR L'ORDRE, et « trop tôt » est un problème de BUT, pas de CAISSE |

<!-- FIN INDEX -->

#### 🎯 Session du 2026-08-01 (1/7) — LE MODE HYBRIDE : cinq niveaux joués à la main, le solveur en tient le LOG

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

**État du code** — commité depuis, `git log` fait foi ; seul le point de conception est gardé ici.
`solveur.h` expose `appuis` (`protected` → `public`) pour que l'UI descende les poussées en coups de
marche par la **même recette** que `reconstruire()`, en exemplaire unique. **Rien dans le solveur**,
aucune variable d'environnement (§7). Canari vérifié après coup : 4/97/131/134/213 poussées,
4/14/412/499/24 786 états.
➡️ ⚠️ **« Rien dans le solveur » n'est plus vrai depuis la session ci-dessous** (2026-08-01,
suite) : deux constantes (`CORRAL_BUDGET`, `corralActif`) sont passées de `solveurastar.cpp` à
`solveurastar.h` pour que le miroir de l'UI ne puisse pas dériver. Aucun changement de
comportement, vérifié binaire contre binaire.

- [ ] **Repli anytime pour la macro** : passe 1 avec macro plafonnée en états, passe 2 sans
  macro si le budget est épuisé. Le repli doit se déclencher sur le **budget**, pas sur
  l'échec (un cas lent n'émet jamais « aucune solution »). Borne surtout le temps des cas
  lents (8, 9).

#### ❌ Session du 2026-08-01 (2/7) — LE RANG DU COUP HUMAIN : le démêlage n'est PAS un problème de CLASSEMENT

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

#### 🎯 Session du 2026-08-01 (3/7) — LE 13 GAGNÉ EN HYBRIDE : 89 % de la solution N'EST PAS DANS L'ARBRE

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

#### 🎉 Session du 2026-08-01 (4/7) — LE 20 GAGNÉ À LA MAIN : le 13 est une SINGULARITÉ, et le détour non-monotone est CONFIRMÉ

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

#### 🎉 Session du 2026-08-01 (5/7) — SEPT NON-RÉSOLUS GAGNÉS À LA MAIN, et trois généralisations mortes

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

#### ✅ Session du 2026-08-01 (6/7) — LE CORRECTIF MULTI-SALLES CODÉ ET PROMU : ×7,5 sur le 10

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

#### ❌ Session du 2026-08-01 (7/7) — LE 12 : c'est le SENS de parcours qui décide, pas le groupement

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

**État du code de la journée du 2026-08-01** — commité depuis, `git log` fait foi pour l'inventaire
des fichiers. Ce qui doit survivre à `git log`, en revanche :
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

#### 🎯 Session du 2026-08-04 (1/4) — LA LOI CODÉE ET JUGÉE, le GEL HORS TOUR, et une précédence d'espèce neuve

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

#### ⏸️ Session du 2026-08-04 (2/4) — LA CONTRAINTE DE PORTE GREFFÉE SUR `butActif()`

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

#### 🎯 Session du 2026-08-04 (3/4) — LE PLAN HUMAIN MESURÉ : ce qui sépare les résolus, et pourquoi le planificateur naïf est mort

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

#### 📖 Session du 2026-08-04 (4/4, lecture) — LES GADGETS DE CULBERSON, et le vocabulaire qui manquait au verrou

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

**Reste ouvert, hérité tel quel de la session du 2026-08-04 (3/4), à attaquer en premier :**
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
