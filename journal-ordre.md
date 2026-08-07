# Journal — ordre de remplissage

> **Journal de chantier, détaché de [plan.md](plan.md) le 2026-08-06.** Le document
> avait atteint 5 588 lignes et 386 Ko, dont 90 % de journaux de session ; on ne le
> relisait plus en entier. Ce fichier porte **§6.2** — le goal-ordering, du 2026-07-17 au 2026-07-31.
>
> ⚠️ **La numérotation d'origine est conservée** (`§6.2` et ses sous-titres) : le plan
> et les autres journaux s'y réfèrent des dizaines de fois, et un renvoi qui ne désigne
> plus rien est pire que pas de renvoi. Les sessions restent dans l'ordre chronologique
> où elles ont été écrites.






<!-- INDEX DES SESSIONS -->

**11 sessions.** Verdict en tête : ✅ acquis · ❌ réfuté · ⏸️ sans verdict ·
🎯 résultat marquant · 🎉 niveau tombé · ⚠️ correction · 📖 lecture. Les titres sont
exacts, une recherche sur la date ou sur un mot du sujet tombe dessus.

| | date | sujet |
|---|---|---|
| ❌ | 2026-07-19 | tout reverté, aucune approche ne reproduit l'ordre humain du 11 |
| ✅ | 2026-07-20 | l'ordre humain est TROUVÉ, mesuré, et il paie |
| ⏸️ | 2026-07-20 suite | la règle codée, testée, PAS ENCORE bonne. À reprendre. |
| ✅ | 2026-07-20 suite 2 | ordre CODÉ, mesuré, PROMU en défaut. Chantier bouclé. |
| 🎯 | 2026-07-29 | LE GOAL-ORDERING N'AVAIT PAS FINI SON TRAVAIL (outil `ordre`) |
| ✅ | 2026-07-29 suite | FAMILLE A CORRIGÉE : le glouton ne force plus, il recule |
| ✅ | 2026-07-30 | FAMILLE B PORTÉE DANS LE SOLVEUR (précédence globale) |
| ❌ | 2026-07-30 suite | LE 13 JUGÉ : ordre SAIN, niveau muré quand même |
| ⏸️ | 2026-07-31 | LE TEST EN DUR RETIRÉ, l'escalade RÉFUTÉE, la piste déplacée |
| ⏸️ | 2026-07-31 soir | ORDRE DYNAMIQUE, MASQUAGE, INJECTION : où on en est |
| ❌ | 2026-07-31 nuit | L'ORDRE HUMAIN DU 13 : relevé sur partie gagnante, et il fait PERDRE |

<!-- FIN INDEX -->

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

**État du code** — commité depuis. Deux choses qui ne se lisent pas dans `git log` : le **qDebug de
mouvements** de `mainwindow.cpp` est **gardé** (c'est lui qui a servi deux fois, cf. 2026-07-31
nuit), et le `#include <cmath>` / `std::ceil` corrige une faute **préexistante** de compilation sur
Linux, sans rapport avec le solveur. ~~**Reprendre ici** : détection de couloirs~~ — ✅ **réglé à la
session suivante**, et autrement que prévu : la contiguïté de run a suffi, sans détecter de couloir.

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

**État du code** — commité depuis. Ce qui reste utile à savoir : le régime d'essai **`ordre-dyn`**
et l'**injection `ORDRE_HUMAIN`** sont des outils de **chantier**, jetables, à retirer avec la
campagne.

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
