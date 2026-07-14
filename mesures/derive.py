#!/usr/bin/env python3
"""Derive des sous-niveaux a 1 CAISSE + 1 BUT depuis un niveau complet.

    python3 derive.py 17 120      # level0017 -> level0120, 0121, ... (une par caisse)

Meme convention que les 010x (derives du niveau 2, cf. plan §7.-1) : la caisse k
est appariee au but k dans l'ordre de balayage (y puis x), le joueur est conserve,
toutes les autres caisses et tous les autres buts sont RETIRES.

On OTE des obstacles, on n'en ajoute jamais : une caisse retiree redevient du sol
('$' -> ' ') ou un but nu ('*' -> '.'), et un but retire redevient du sol. Ajouter
des murs a la place (les 011x) serait FAUX comme modele — cf. §8.5.
"""
import os, sys

RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def charge(num):
    with open(os.path.join(RACINE, "level%04d.xsb" % num)) as f:
        lignes = [l.rstrip("\n") for l in f if l.strip("\n")]
    L = max(len(l) for l in lignes)
    return [list(l.ljust(L)) for l in lignes]


def main(src, base):
    g = charge(src)
    H, L = len(g), len(g[0])

    # Ordre de balayage : y puis x.
    caisses = [(x, y) for y in range(H) for x in range(L) if g[y][x] in '$*']
    buts    = [(x, y) for y in range(H) for x in range(L) if g[y][x] in '.*+']

    if len(caisses) != len(buts):
        print("!! %d caisses pour %d buts — appariement impossible" % (len(caisses), len(buts)))
        return

    print("niveau %d : %d caisses, %d buts" % (src, len(caisses), len(buts)))

    for k, (cx, cy) in enumerate(caisses):
        bx, by = buts[k]
        n = [row[:] for row in g]

        # Retirer les autres caisses.
        for j, (x, y) in enumerate(caisses):
            if j == k:
                continue
            n[y][x] = '.' if g[y][x] == '*' else ' '

        # Retirer les autres buts. Un but portant la caisse gardee ou le joueur se
        # degrade en caisse / joueur nus.
        for j, (x, y) in enumerate(buts):
            if j == k:
                continue
            c = n[y][x]
            if   c == '.': n[y][x] = ' '
            elif c == '*': n[y][x] = '$'
            elif c == '+': n[y][x] = '@'

        # Reposer la caisse et le but retenus (ils ont pu etre effaces ci-dessus
        # si la caisse k etait sur un but, ou si le but k portait une autre caisse).
        n[by][bx] = '*' if (bx, by) == (cx, cy) else ('+' if n[by][bx] == '@' else '.')
        if (cx, cy) != (bx, by):
            n[cy][cx] = '$'

        dest = os.path.join(RACINE, "level%04d.xsb" % (base + k))
        with open(dest, "w") as f:
            for row in n:
                f.write("".join(row).rstrip() + "\n")

        print("  level%04d : caisse (%d,%d) -> but (%d,%d)" % (base + k, cx, cy, bx, by))


if __name__ == "__main__":
    src  = int(sys.argv[1]) if len(sys.argv) > 1 else 17
    base = int(sys.argv[2]) if len(sys.argv) > 2 else 120
    main(src, base)
