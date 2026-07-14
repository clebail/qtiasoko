#!/usr/bin/env python3
"""Superpose les cartes de passages exportees par l'app (bouton « Export passages »).

    python3 cumul.py passages_niveau120.txt ... passages_niveau125.txt

Additionne case par case, et compare a une carte de reference si on en donne une
en dernier avec --ref.

But : voir si les couloirs de la vraie solution (les 6 caisses ensemble) se
retrouvent quand chaque caisse est resolue SEULE. Si oui, les couloirs sont une
propriete du NIVEAU. Sinon, ils naissent du fait que les caisses se genent.
"""
import sys, re


def lit(path):
    """Rend (grille, passages) : deux listes de listes, alignees."""
    with open(path, encoding="utf-8", errors="replace") as f:
        lignes = f.read().split("\n")

    ig = lignes.index("-- grille --")
    ip = next(i for i, l in enumerate(lignes) if l.startswith("-- passages"))

    grille = lignes[ig + 1: ip - 1]
    grille = [l for l in grille if l.strip("") != "" or True]
    grille = [l for l in grille if l != ""]

    # bloc des passages : jusqu'a la ligne vide ou le 'total'
    brut = []
    for l in lignes[ip + 1:]:
        if l.strip() == "" or l.startswith("total"):
            break
        brut.append(l)

    # 3 caracteres par case
    passages = []
    for l in brut:
        row = []
        for i in range(0, len(l), 3):
            cell = l[i:i + 3]
            if cell == "###":
                row.append(None)          # mur
            else:
                s = cell.strip()
                row.append(0 if s == "." else int(s))
        passages.append(row)

    return grille, passages


def main(paths, ref=None):
    cartes = [lit(p) for p in paths]
    H = len(cartes[0][1])
    L = max(len(r) for r in cartes[0][1])

    cumul = [[None] * L for _ in range(H)]
    for _, p in cartes:
        for y in range(len(p)):
            for x in range(len(p[y])):
                if p[y][x] is None:
                    cumul[y][x] = None
                else:
                    cumul[y][x] = (cumul[y][x] or 0) + p[y][x]

    def rend(carte):
        out = []
        for y in range(H):
            l = ""
            for x in range(L):
                v = carte[y][x] if x < len(carte[y]) else None
                if v is None:  l += "###"
                elif v == 0:   l += "  ."
                else:          l += "%3d" % v
            out.append(l)
        return out

    print("=== SOMME des %d sous-niveaux (chaque caisse resolue SEULE) ===" % len(paths))
    for l in rend(cumul):
        print(l)
    tot = sum(v for r in cumul for v in r if v)
    pic = max(v for r in cumul for v in r if v)
    print("\ntotal : %d   pic : %d" % (tot, pic))

    if ref:
        _, pr = lit(ref)
        print("\n=== REFERENCE : %s (les 6 caisses ensemble) ===" % ref)
        for l in rend([[pr[y][x] if y < len(pr) and x < len(pr[y]) else None
                        for x in range(L)] for y in range(H)]):
            print(l)
        tr = sum(v for r in pr for v in r if v)
        pkr = max(v for r in pr for v in r if v)
        print("\ntotal : %d   pic : %d" % (tr, pkr))

        print("\n=== ECART  (somme des solos) - (solution reelle) ===")
        diff = [[None] * L for _ in range(H)]
        for y in range(H):
            for x in range(L):
                a = cumul[y][x]
                b = pr[y][x] if y < len(pr) and x < len(pr[y]) else None
                if a is None or b is None:
                    diff[y][x] = None
                else:
                    diff[y][x] = a - b
        for y in range(H):
            l = ""
            for x in range(L):
                v = diff[y][x]
                if v is None:  l += "###"
                elif v == 0:   l += "  ."
                else:          l += "%+3d" % v
            print(l)


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    ref = None
    if "--ref" in sys.argv:
        ref = args[-1]
        args = args[:-1]
    main(args, ref)
