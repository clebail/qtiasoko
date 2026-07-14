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
| `diverge <niv>` | `h` désigne-t-elle le bon coup ? | Rang du bon enfant parmi les enfants classés par `h`, et mou de `h` le long du chemin optimal. |
| `paires <niv>` | Le mou vient-il d'une interaction entre 2 caisses ? | Matrice des suppléments d'interaction, par paire de caisses. |
| `trace <niv> [pas]` | À quoi ressemble la solution ? | La grille poussée par poussée, avec les poussées **non productives** (les manœuvres). |
| `tunnels <niv>` | Quel est le potentiel des macro-poussées ? | Part des poussées atterrissant dans un couloir sans alternative. |

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
