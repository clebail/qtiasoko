#!/usr/bin/env python3
"""Extrait les poussées d'une partie GAGNÉE du journal hybride, au format de `jugeloi`.

    poussees_journal.py <niv> [index]   ->  une ligne "<case> <dir>" par poussée

`case` est l'index de case (x + y*largeur) de la caisse poussée, `dir` l'ordre de
`Game::EDirection` (0=Haut 1=Droite 2=Bas 3=Gauche). Sans `index`, prend la DERNIÈRE
partie gagnée du journal.

⚠️ La partie est VALIDÉE PAR REJEU avant d'être émise (mesures/taches.py) : un journal
contient plusieurs parties, des `[undo]`, et des tentatives abandonnées. Émettre une
partie non gagnée ferait juger un chemin qui ne prouve rien — tout l'argument du juge
repose sur « ces états sont solubles par construction ».

⚠️ La LARGEUR vient du fichier de niveau, pas du journal : `jugeloi` indexe ses cases
avec la largeur du `Level`, et un décalage d'une colonne ferait pousser une autre
caisse sans que rien ne le signale (§7, `idxCaisse` est un index de CASE).
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import taches

DIRS = {(0, -1): 0, (1, 0): 1, (0, 1): 2, (-1, 0): 3}   # ordre de EDirection


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    niv = int(sys.argv[1])
    voulu = int(sys.argv[2]) if len(sys.argv) > 2 else None

    grille = taches.charge(f"{taches.R}/level{niv:04d}.xsb")
    largeur = max(len(r) for r in grille)

    parties = taches.parties(f"{taches.R}/hybride_niveau_{niv:04d}.txt")
    gagnees = []
    for k, p in enumerate(parties):
        if not p:
            continue
        gagne, _ = taches.rejoue(niv, p)
        if gagne:
            gagnees.append((k, p))
    if not gagnees:
        sys.exit(f"poussees_journal: aucune partie GAGNEE pour le niveau {niv}")

    if voulu is None:
        k, coups = gagnees[-1]
    else:
        sel = [(k, p) for k, p in gagnees if k == voulu]
        if not sel:
            sys.exit(f"poussees_journal: la partie {voulu} du niveau {niv} n'est pas gagnee")
        k, coups = sel[0]

    n = 0
    for (p1, p2, p3, mid) in coups:
        if p3 is None:
            continue                      # marche du joueur : aucune poussée
        d = DIRS.get((p3[0] - p2[0], p3[1] - p2[1]))
        if d is None:
            sys.exit(f"poussees_journal: poussee non orthogonale {p2}->{p3}")
        print(f"{p2[0] + p2[1] * largeur} {d}")
        n += 1
    print(f"# niveau {niv}, partie {k}, {n} poussees, gagnee et validee par rejeu",
          file=sys.stderr)


if __name__ == "__main__":
    main()
