# Leçon 1 — La langue : logique, quantificateurs, et ce qu'est une preuve

> **Pourquoi on commence ici, et pas par du calcul.** Tu sais déjà calculer et programmer. Ce qui te
> manque, c'est la *langue* dans laquelle les mathématiques s'écrivent : lire un énoncé sans
> l'interpréter de travers, et écrire une démonstration que quelqu'un d'autre peut vérifier.
> C'est une compétence de lecture et d'écriture, pas de calcul. Elle se travaille comme une langue.
>
> Et elle est immédiatement rentable : plusieurs des impasses coûteuses de `plan.md` sont, à la
> lettre, des erreurs de cette leçon. Elles sont citées au fil du texte.

---

## 1. Proposition, prédicat

Une **proposition** est un énoncé qui est **vrai ou faux**, pas les deux, et dont la valeur ne dépend
pas de qui le lit.

| énoncé | proposition ? |
|---|---|
| « Le niveau 16 a 15 caisses. » | ✅ oui (et elle est vraie) |
| « Le niveau 16 a 12 caisses. » | ✅ oui (et elle est fausse — une proposition fausse reste une proposition) |
| « Pousse la caisse vers le nord. » | ❌ non — c'est un ordre, ça n'est ni vrai ni faux |
| « Ce niveau est joli. » | ❌ non — ça dépend du lecteur |
| « h(n) ≤ h\*(n) » | ⚠️ **pas encore** |

Le dernier cas est important. `h(n) ≤ h*(n)` contient une variable libre, `n`. Tant qu'on ne dit pas
*quel* `n`, l'énoncé n'a pas de valeur de vérité. On appelle ça un **prédicat** : une proposition à
trou. On le note `P(n)`.

Un prédicat devient une proposition de deux façons : soit on **remplace** le trou par une valeur
précise (`P(état_initial_du_16)`), soit on le **quantifie** (« pour tout `n`… »). C'est l'objet de la
section 3, et c'est là que tout se joue.

> **En code.** Un prédicat, c'est une fonction qui rend un booléen : `bool estMorte(Case c)`. Elle
> n'est ni vraie ni fausse en soi ; elle le devient une fois appliquée à un argument.

---

## 2. Les connecteurs

Cinq symboles. Ils se combinent, comme des opérateurs.

| symbole | lecture | code |
|---|---|---|
| `¬P` | non P | `!p` |
| `P ∧ Q` | P **et** Q | `p && q` |
| `P ∨ Q` | P **ou** Q | `p \|\| q` |
| `P ⇒ Q` | P **implique** Q | *(pas d'équivalent direct — voir plus bas)* |
| `P ⇔ Q` | P **équivaut à** Q | `p == q` |

### Le « ou » est inclusif

`P ∨ Q` est vraie dès que **au moins une** des deux l'est, y compris **les deux**. C'est le `||` du C++,
pas le « ou » du français courant (« fromage ou dessert » = un seul). En mathématiques, il n'y a
jamais d'exclusivité implicite. Si on veut l'exclusion, on l'écrit.

> **Chez toi.** « Une case est morte si elle est dans un coin **ou** sur un mur sans but aligné. » Les
> deux peuvent être vraies en même temps sur la même case — et c'est bien ce que fait
> `calculCaseMorte`, qui marque la case sans se demander laquelle des deux raisons s'applique.

### L'implication, le point qui coince pour tout le monde

`P ⇒ Q` se lit « si P, alors Q ». Sa table de vérité :

| P | Q | P ⇒ Q |
|:-:|:-:|:-:|
| V | V | **V** |
| V | F | **F** |
| F | V | **V** |
| F | F | **V** |

**Une seule ligne la rend fausse : P vraie et Q fausse.** Autrement dit :

> `P ⇒ Q` affirme uniquement qu'on ne peut pas avoir P sans Q.
> **Elle ne dit rien du tout quand P est fausse.**

Les deux dernières lignes surprennent toujours. « Si 2 + 2 = 5, alors je suis le pape » est une
proposition **vraie**. Elle ne raconte rien, mais elle est vraie : sa prémisse étant fausse, elle ne
promet rien, donc elle ne peut pas être prise en défaut. On appelle ça une **vérité vide**
(*vacuous truth*).

Ce n'est pas une bizarrerie théorique, tu la croises en code toutes les semaines :

```cpp
std::all_of(v.begin(), v.end(), pred)   // rend TRUE sur un conteneur vide
std::any_of(v.begin(), v.end(), pred)   // rend FALSE sur un conteneur vide
```

Ce n'est pas un choix arbitraire de la bibliothèque standard : c'est la logique. « Toutes les caisses
de cet enclos sont mortes » est **vraie** quand l'enclos ne contient aucune caisse — il n'y a aucun
contre-exemple à produire. « Il existe une caisse morte dans cet enclos » est **fausse** pour la même
raison.

> ⚠️ **Un piège à connaître dès maintenant.** Une propriété « vraie » par vacuité est vraie, mais
> elle n'apporte aucune information. Si un test de sûreté passe sur un ensemble vide, il n'a rien
> validé. Le §7 de `plan.md` est plein de cas où un compteur à zéro a été lu comme un résultat : c'est
> le même phénomène.

---

## 3. Les quantificateurs

Deux symboles, et c'est le cœur de la leçon.

| symbole | lecture | comment on le prouve | comment on le réfute |
|---|---|---|---|
| `∀x, P(x)` | **pour tout** x, P(x) | argument valable pour un x **quelconque** | **un seul** contre-exemple |
| `∃x, P(x)` | **il existe** x tel que P(x) | **un seul** exemple | argument valable pour tout x |

Cette asymétrie est un outil de travail quotidien, pas une curiosité :

> **Réfuter un « pour tout » coûte un exemple. Le prouver coûte un argument général.**
> Et symétriquement pour « il existe ».

C'est très exactement le régime de ton `plan.md` : tuer une hypothèse est rapide (« le 10 est à 12 %
et il tombe » — un contre-exemple suffit à détruire « la progression prédit la solvabilité »), la
confirmer est hors de portée. Tu écris toi-même « éliminer de faux signaux est un résultat ». C'est
la logique qui te dit pourquoi c'est le seul résultat facilement accessible.

### Ton propre code, traduit

| ce que fait le code | énoncé formel |
|---|---|
| `h` est admissible | `∀n, h(n) ≤ h*(n)` |
| la case `c` est morte | `¬∃ suite de poussées amenant une caisse de c vers un but` |
| l'état `e` est un deadlock | `¬∃ solution depuis e` |
| le niveau est gagné | `∀ caisse b, b est sur un but` |

Remarque que « morte » et « deadlock » sont des **négations d'existence**. C'est pour ça qu'ils sont
chers à établir et faciles à réfuter : il suffit d'exhiber un chemin. Toute ta théorie des deadlocks
consiste à trouver des cas où l'on peut conclure `¬∃` **sans explorer**.

### L'ordre des quantificateurs — le point le plus important de la leçon

**`∀x ∃y, P(x,y)` et `∃y ∀x, P(x,y)` ne veulent pas dire la même chose.** Le second est bien plus
fort, et il implique le premier (jamais l'inverse).

- `∀ personne, ∃ un jour où elle est née` — vrai, banal.
- `∃ un jour où ∀ personne est née` — faux, absurde.

Même symboles, ordre différent, sens sans rapport.

**Et voilà pourquoi ton couplage hongrois vaut ×59.** Compare deux bornes inférieures :

1. **Chaque caisse va au but le plus proche, indépendamment.**
   `∀ caisse b, ∃ but g` — et on somme les distances. Chaque caisse choisit *son* but, sans se
   soucier des autres. Deux caisses peuvent élire le même but : c'est autorisé par cet énoncé.

2. **Il existe une affectation bijective caisses → buts.**
   `∃ bijection σ, ∀ caisse b, σ(b) est un but` — et on somme les distances de cette affectation, en
   prenant la meilleure. C'est le couplage hongrois.

L'énoncé 2 est **plus fort** que le 1 : il impose une contrainte supplémentaire (l'injectivité).
Toute affectation bijective est en particulier un choix individuel pour chaque caisse — donc :

```
somme des minima individuels  ≤  coût du couplage optimal  ≤  h*
```

Une borne inférieure est d'autant meilleure qu'elle est **grande** (sans dépasser `h*`). Le couplage
est donc au moins aussi bon, et souvent bien meilleur : dès que deux caisses se disputent le même
but, la version 1 sous-estime bêtement et la version 2 non.

**Ton gain de ×59 est intégralement contenu dans l'ordre de deux quantificateurs.** Tu l'as trouvé
par l'expérience ; c'est une propriété qui s'énonce et se démontre en trois lignes.

> Retiens ce mécanisme, on va s'en resservir tout le cours : **contraindre davantage le problème
> relâché fait monter la borne.** C'est l'unique moteur des modules 2 et 3.

---

## 4. La négation — l'outil de travail

Savoir nier un énoncé, c'est savoir **quoi tester pour le mettre en défaut**. Trois règles, à
connaître par cœur :

```
¬(∀x, P(x))   ≡   ∃x, ¬P(x)
¬(∃x, P(x))   ≡   ∀x, ¬P(x)
¬(P ⇒ Q)      ≡   P ∧ ¬Q
```

Les deux premières sont les **lois de De Morgan** pour les quantificateurs. Elles se lisent
naturellement : « il est faux que tous soient P » = « il y en a un qui n'est pas P ».

La troisième est celle qu'on oublie, et c'est la plus utile chez toi :

> **Pour réfuter `h est admissible` (`∀n, h(n) ≤ h*(n)`), il suffit d'exhiber UN état `n` avec
> `h(n) > h*(n)`.**

Un seul état. C'est un test exécutable, pas un raisonnement : prends un niveau que tu résous, calcule
`h*` exactement par ton solveur optimal, compare à `h` sur chaque état du chemin. Si une seule
comparaison casse, l'heuristique est inadmissible et tout ce qui en dépend s'écroule.

**Méthode de négation d'un énoncé long :** on descend de gauche à droite, chaque `∀` devient `∃`,
chaque `∃` devient `∀`, et on nie le cœur en dernier.

```
        ∀ε > 0, ∃N, ∀n ≥ N,  |u(n) − L| < ε
nié :   ∃ε > 0, ∀N, ∃n ≥ N,  |u(n) − L| ≥ ε
```

(C'est la définition de la limite d'une suite et sa négation. Tu n'en as pas besoin aujourd'hui ;
c'est pour te montrer que la mécanique est purement syntaxique — on ne réfléchit pas, on retourne les
symboles.)

---

## 5. Contraposée et réciproque — l'erreur qui t'a coûté cher

Soit l'implication `P ⇒ Q`.

| nom | énoncé | statut |
|---|---|---|
| **contraposée** | `¬Q ⇒ ¬P` | ✅ **toujours équivalente** à `P ⇒ Q` |
| **réciproque** | `Q ⇒ P` | ❌ **sans rapport** — parfois vraie, parfois fausse |

Vérifie la contraposée sur la table de vérité de la section 2 : les deux colonnes sont identiques,
ligne par ligne. C'est un fait, pas une convention. Et c'est utile : prouver `¬Q ⇒ ¬P` prouve
`P ⇒ Q`, et c'est parfois beaucoup plus facile.

La réciproque, elle, est un **énoncé différent**. « S'il pleut, le sol est mouillé » n'entraîne
absolument pas « si le sol est mouillé, il pleut » (quelqu'un a pu arroser).

### Le cas `corral > 0`, en toutes lettres

Ton `plan.md` §5 pose la règle : *« corral > 0 ne se prune pas »*. Voici pourquoi, formellement.

Ce que la détection de corral établit :

```
(1)   état mort   ⇒   corral > 0
```

Ce qu'un élagage sur `corral > 0` supposerait :

```
(2)   corral > 0   ⇒   état mort            ← c'est la RÉCIPROQUE de (1)
```

(2) ne se déduit pas de (1). Et (2) est **fausse** : il existe des états parfaitement solubles qui
présentent un corral non résolu. Élaguer dessus supprime des solutions — et, pire, **en silence** :
le solveur ne rend pas d'erreur, il rend « insoluble » sur un niveau soluble.

La contraposée de (1), elle, est vraie et exploitable :

```
      corral = 0   ⇒   état non mort
```

…mais elle ne sert à rien pour élaguer, puisqu'elle n'autorise à couper aucune branche.

> **La règle générale, à graver.** Un test de sûreté ne peut élaguer que s'il a la forme
> `test positif ⇒ état réellement mort`. Un test qui a seulement la forme
> `état mort ⇒ test positif` est un **indice**, jamais un **couteau**. Ton §6.4 le dit déjà pour le
> réseau de neurones (« comme guide, jamais comme coupeur ») — c'est la même distinction, et elle
> vaut pour n'importe quel détecteur, appris ou codé à la main.

---

## 6. Ce qu'est une preuve

Une **démonstration** est une suite finie d'affirmations, où chacune est justifiée par :

- une **définition**,
- une **hypothèse** de l'énoncé,
- ou un résultat **déjà démontré**.

Rien d'autre. Pas d'intuition, pas de « on voit bien que », pas de mesure expérimentale. Une preuve
est vérifiable par quelqu'un qui ne comprend pas le sujet, en suivant les justifications une par une.

Trois techniques couvrent l'immense majorité des cas :

| technique | pour prouver `P ⇒ Q` | on suppose | on cherche |
|---|---|---|---|
| **directe** | | `P` | à atteindre `Q` |
| **par contraposée** | | `¬Q` | à atteindre `¬P` |
| **par l'absurde** | | `P ∧ ¬Q` | **n'importe quelle** contradiction |

### Exemple travaillé, sur ton propre problème

> **Énoncé.** Si `h₁` et `h₂` sont deux heuristiques admissibles, alors `max(h₁, h₂)` est admissible.

**Démonstration.** Soit `n` un état **quelconque**.
Par admissibilité de `h₁`, on a `h₁(n) ≤ h*(n)`.
Par admissibilité de `h₂`, on a `h₂(n) ≤ h*(n)`.
Donc `h*(n)` majore les deux nombres `h₁(n)` et `h₂(n)`, donc il majore leur maximum :
`max(h₁(n), h₂(n)) ≤ h*(n)`.
Comme `n` était quelconque, cette inégalité vaut pour tout `n`. ∎

Trois choses à observer dans cette preuve, elles sont typiques :

1. On dit **« soit `n` quelconque »** au début. C'est comme ça qu'on prouve un `∀` : on prend un
   élément dont on ne suppose rien, et on ne s'autorise ensuite aucune propriété particulière. La
   dernière ligne (« comme `n` était quelconque ») n'est pas une formule de politesse, c'est
   l'étape qui transforme le résultat ponctuel en résultat universel.
2. Chaque ligne est justifiée par **l'hypothèse** ou par une propriété élémentaire des nombres.
3. Le `∎` (ou « CQFD ») marque la fin. Convention utile : le lecteur sait où s'arrête l'argument.

### Et le contre-exemple qui va avec

> **Énoncé.** Si `h₁` et `h₂` sont admissibles, `h₁ + h₂` est-elle admissible ?

**Non.** Il suffit d'un contre-exemple (section 3 : c'est un `∀` qu'on réfute).
Prenons `h₁ = h₂ = h*`. Chacune est admissible (`h*(n) ≤ h*(n)`, trivialement).
Leur somme vaut `2·h*(n)`, qui est `> h*(n)` dès que `h*(n) > 0`, c'est-à-dire sur tout état non
final. Donc la somme n'est pas admissible. ∎

> **Ce que ça ouvre.** On aimerait pourtant additionner : deux heuristiques qui « voient » des
> difficultés différentes devraient se cumuler, et `max` gaspille l'information de la plus petite. Le
> moyen de le faire **en restant admissible** s'appelle la **partition de coûts**, et c'est le
> module 3 de ce cours. Retiens juste qu'à ce stade, le seul combinateur sûr que tu connais est
> `max`.

---

## 7. Exercices

Réponds-moi directement, à ton rythme. Il n'y a pas de piège : si un énoncé te paraît ambigu, dis-le,
c'est une réponse valable (l'ambiguïté d'énoncé est un vrai sujet).

**Ex. 1 — Traduction et négation.**
Écris avec des quantificateurs : *« la case `c` est morte »*. Puis nie ton énoncé et dis, en français,
ce que la négation te demande d'exhiber pour prouver qu'une case n'est **pas** morte.

**Ex. 2 — Implication ou réciproque.**
Pour chacun de ces énoncés tirés de ton projet, dis s'il autorise un élagage sûr, et écris
l'implication exacte qui le justifie ou l'invalide :
- (a) « une caisse dans un coin non-but est morte »
- (b) « un état dont le couplage hongrois dépasse le budget restant est mort »
- (c) « un état où aucune goal macro n'est jouable est mort »

**Ex. 3 — Ordre des quantificateurs.**
Écris avec des quantificateurs : *« chaque caisse peut atteindre au moins un but »* et *« il existe
une affectation bijective des caisses aux buts »*. Laquelle implique l'autre ? Construis un petit
plateau (3×3 suffit) où la première est vraie et la seconde fausse.

**Ex. 4 — Preuve.**
Démontre que si `h₁` et `h₂` sont admissibles, alors `min(h₁, h₂)` l'est aussi. Puis explique en une
phrase pourquoi personne ne fait ça en pratique.

**Ex. 5 — Contre-exemple.**
`plan.md` affirme : *« aucun levier n'est universel sauf le couplage »* (§6.6). Écris cet énoncé
formellement. Que faudrait-il exhiber, exactement, pour le réfuter ? Et — question plus dure —
est-ce que le tableau du §6.6 le **prouve** ?

---

## Ce qu'il faut retenir

1. Un **prédicat** n'est pas une proposition : il lui manque un quantificateur.
2. `P ⇒ Q` ne dit **rien** quand `P` est fausse. Vrai sur un ensemble vide ≠ validé.
3. Prouver un `∀` demande un argument général ; le réfuter demande **un seul** contre-exemple.
4. **L'ordre des quantificateurs change le sens.** C'est là qu'est ton ×59.
5. La **contraposée** est équivalente. La **réciproque** ne l'est pas — et c'est le piège qui
   transforme un indice en élagage faux.
6. Une preuve est une chaîne de justifications vérifiable, pas une conviction.

**Prochaine leçon** — les ensembles, les fonctions, et la récurrence : les objets sur lesquels tout le
reste est construit. Puis on entre dans le module 1 (relaxation), où la question « d'où vient une
borne inférieure ? » reçoit sa réponse générale.
