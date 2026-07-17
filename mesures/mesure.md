# Harnais de mesure du solveur

Outils en ligne de commande qui compilent les sources du solveur telles quelles et
les interrogent de l'extérieur. **Rien ici n'entre dans `qtiasoko.pro`** : ce n'est
pas du code produit, c'est de l'instrumentation.

Ils existent parce que ce projet s'est fait piéger plusieurs fois par des
raisonnements plausibles mais faux (§3bis, §6.B, §8.5, et l'étape 11). **Chaque
conclusion du plan doit pouvoir être re-mesurée**, en particulier après un
changement du solveur.

## Construire

```sh
mkdir -p build/bench && cd build/bench && qmake ../../bench.pro && make
```

(idem pour les autres.)

## Les outils, et la question à laquelle chacun répond

| outil | question | sortie clé |
|---|---|---|
| `bench <niv> [poids]` | Combien d'états / de poussées, et combien de mémoire ? | Le **canari** : `4 / 97 / 131 / 134 / 213` (niveaux 0/1/2/3/17). Avec `INSTRUM_F`, l'**histogramme des `f` au dépilement**. |
| `mou <niv> [n]` | Les états qu'A\* développe sont-ils du gaspillage ? | Échantillonne les états **réellement dépilés** (`DUMP_DEV`) et résout depuis chacun : sur un chemin optimal / hors chemin / **deadlock non détecté**. |
| `fp <niv> [variante] [astar\|macro]` | **Un élagage invente-t-il des morts ?** | `LIVRAISON=0 ./fp 17 4` : résout SANS le test, rejoue la solution gagnante et interroge le test sur chaque état traversé. Ces états sont solubles **par construction** → **toute détection est un faux positif prouvé**. C'est le juge qui a réfuté le test « but orphelin » (§6.1, 2026-07-21) là où l'échantillonnage de `mort` le déclarait sûr. |
| `diverge <niv>` | `h` désigne-t-elle le bon coup ? | Rang du bon enfant parmi les enfants classés par `h`, et mou de `h` le long du chemin optimal. |
| `paires <niv>` | Le mou vient-il d'une interaction entre 2 caisses ? | Matrice des suppléments d'interaction, par paire de caisses. |
| `trace <niv> [pas]` | À quoi ressemble la solution ? | La grille poussée par poussée, avec les poussées **non productives** (les manœuvres). |
| `passages <bfs\|astar> <niv...>` | **Les couloirs sont-ils prédictibles ?** | Carte des passages de caisse. Sur PLUSIEURS niveaux, en somme : superpose les caisses résolues seules. **C'est l'outil du §9.7** : l'écart avec la vraie solution vaut EXACTEMENT le mou de `h`. |
| `congestion <niv> <solo...>` | **De quoi le mou est-il fait ?** | Dissèque chaque poussée non productive : quelle caisse est écartée, quelle case elle libère, et qui attendait cette case. **C'est l'outil du §9.8.** |
| `derive.py <src> <base>` | — | Génère les sous-niveaux à 1 caisse + 1 but (`012x` ← 17, `013x` ← 1, `014x` ← 2, `015x` ← 3). |
| `goulots.py` | Où sont les passages obligés ? | Une carte PNG par niveau. ⚠️ Mesure la propagation par POUSSÉE, pas le flood-fill du joueur (voir les pièges). |
| `cumul.py`, `horsreseau.py` | — | Additionnent / comparent les cartes exportées par l'app (bouton « Export passages »). |

**L'app compte aussi les passages** : chaque case affiche le nombre de fois qu'une caisse
l'a occupée (1 sous chaque caisse au départ — elle y est déjà), et le bouton
**« Export passages »** écrit le tout en `.txt`.

## Pièges de mesure déjà rencontrés (ne pas les refaire)

- **Ne jamais comparer à un chiffre écrit dans un document.** Il vieillit en silence
  pendant que le code bouge. Comparer un binaire à un **autre binaire**, reconstruit
  depuis `HEAD` via `git worktree`. (Étape 11 : j'ai failli « corriger » une
  régression mémoire qui n'existait pas, et un « bug » de 267 états qui était une
  référence périmée.)
- **Échantillonner la bonne population.** `mou` a d'abord échantillonné l'ensemble
  `{f <= C*}` au lieu des états **dépilés** : 25× plus gros, car A\* s'arrête au but
  et n'en visite qu'une fraction. Résultat annoncé puis retiré : « 44 % de deadlocks
  non détectés » — un vrai chiffre sur la mauvaise population reste une erreur.
- **Un chiffre trop propre est un chiffre à ne pas croire.** `paires` a d'abord rendu
  un supplément de 1 sur **toutes** les paires : c'était le garde-fou de
  `getHeuristique()` (`game.cpp`, `n != nbButs` → repli sur l'ancienne borne), parce
  que les sous-niveaux gardaient tous les buts. Il faut des sous-problèmes **carrés**
  (2 caisses, 2 buts).
- **`ps rss` ment sur macOS** (le compresseur mémoire sort les pages de la RSS).
  Utiliser `/usr/bin/time -l` (« peak memory footprint ») ou `footprint -p PID`.
- **Une carte de trafic a perdu l'identité des caisses.** Elle agrège tout le monde, donc
  elle ne peut PAS servir à juger la congestion : une caisse écartée atterrit presque
  toujours sur le trajet d'une voisine, sans jamais « sortir du réseau ». Mesuré (§9.8) :
  niveau 17, **12 de mou pour 2 passages hors réseau**. Le critère est PAR CAISSE, jamais
  par case.
- **`timeout` n'existe pas sur macOS** (ni `gtimeout` sans Homebrew). Un script de balayage
  qui l'utilise fait échouer TOUS les niveaux instantanément et les compte comme des
  timeouts — y compris le niveau 0, qui se résout en 1 ms. Lancer en fond + `kill` après
  délai.
