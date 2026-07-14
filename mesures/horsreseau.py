#!/usr/bin/env python3
"""Teste l'identite :   mou = 2 x (passages HORS RESEAU)

    python3 horsreseau.py

Le RESEAU = l'union des trajets des caisses resolues SEULES (toute case empruntee
par au moins UNE caisse). C'est le domaine de h : h est la somme de ces trajets.

  - une poussee SUR le trajet solo fait descendre h de 1  -> PRODUCTIVE
  - une poussee HORS du reseau devie la caisse            -> NON PRODUCTIVE

Une deviation coute 2 poussees (sortir du reseau, puis y revenir) pour 1 seul
passage hors reseau. D'ou l'identite testee ici.

Lit les sorties de `mesures/passages` (deja calculees, aucun solveur relance).
"""
import re, subprocess, sys, os

BUILD = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build", "passages", "passages")

# niveau -> (sous-niveaux 1 caisse, C*, h)
CAS = {
    1:  (range(130, 136), 97,  95),
    2:  (range(140, 150), 131, 129),
    3:  (range(150, 161), 134, 128),
    17: (range(120, 126), 213, 201),
}


def carte(args):
    """Lance mesures/passages et rend la grille des passages (None = mur)."""
    out = subprocess.run([BUILD] + args, capture_output=True, text=True).stdout
    lignes = out.split("\n")
    # le bloc de carte : entre la ligne 'total : N poussees' et 'total des passages'
    i = next(i for i, l in enumerate(lignes) if l.startswith("total : "))
    j = next(i for i, l in enumerate(lignes) if l.startswith("total des passages"))
    brut = [l for l in lignes[i + 1:j] if l.strip()]

    g = []
    for l in brut:
        row = []
        for k in range(0, len(l), 3):
            c = l[k:k + 3]
            if c == "###":
                row.append(None)
            else:
                s = c.strip()
                row.append(0 if s == "." else int(s))
        g.append(row)
    return g


print("%-5s %-6s %-6s %-6s %-14s %s" %
      ("niv", "C*", "h", "mou", "hors reseau", "2 x hors reseau"))

for niv, (solos, cstar, h) in CAS.items():
    ref  = carte(["astar", str(niv)])
    solo = carte(["bfs"] + [str(n) for n in solos])

    hors = 0
    for y in range(len(ref)):
        for x in range(len(ref[y])):
            r = ref[y][x]
            s = solo[y][x] if y < len(solo) and x < len(solo[y]) else None
            if r is None or s is None:
                continue
            if s == 0 and r > 0:          # case que les solos n'empruntent JAMAIS
                hors += r

    mou = cstar - h
    ok = "OK" if 2 * hors == mou else "!! ECART"
    print("%-5d %-6d %-6d %-6d %-14d %-14d %s" % (niv, cstar, h, mou, hors, 2 * hors, ok))
