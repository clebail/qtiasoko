#!/usr/bin/env python3
"""Cartes de GOULOTS : combien de CAISSES doivent forcement passer par chaque case ?

    python3 goulots.py            # les 32 niveaux -> mesures/goulots/

Pour chaque case c du plateau : on la bouche, et on compte combien de caisses
perdent alors TOUT acces a un but. Ce nombre est le nombre de caisses qui
DEVAIENT passer par c.

⚠️ PIEGE DEJA COMMIS (premiere version de ce script, corrigee) : ne PAS mesurer
l'accessibilite du JOUEUR. Un flood-fill depuis le joueur, caisses transparentes,
repond a « quelles cases empechent le JOUEUR d'atteindre les buts » — ce qui n'a
rien a voir. Sur le niveau 17, il marquait la colonne x=4 comme passage oblige des
6 caisses : c'est la seule SORTIE DU JOUEUR (il demarre en bas a gauche), mais la
caisse de droite monte par x=13 et n'y passe jamais.

Une caisse ne circule pas comme un joueur. Elle avance de p vers p+d seulement si
la case d'arrivee p+d ET la case d'appui p-d sont libres (le joueur doit tenir
derriere). C'est la mecanique de calculDistancePoussee() dans game.cpp, et c'est
elle qu'on propage ici.

Relaxation assumee : on ignore si le joueur peut REJOINDRE la case d'appui (il
faudrait le suivre pas a pas). On ne fait donc qu'OTER des contraintes -> le
nombre de caisses bloquees est une borne INFERIEURE. Une case marquee « 6 caisses »
en bloque au moins 6 ; elle pourrait en bloquer plus.

Pur Python, aucun solveur.
"""
import os
from collections import deque

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap, BoundaryNorm
from matplotlib.patches import Rectangle, Circle
from matplotlib.cm import ScalarMappable

RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SORTIE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "goulots")

# Rampe sequentielle bleue (une seule teinte : c'est une magnitude), DISCRETISEE —
# une couleur par nombre de caisses.
RAMPE_BASE = ["#cde2fb", "#b7d3f6", "#9ec5f4", "#86b6ef", "#6da7ec", "#5598e7",
              "#3987e5", "#2a78d6", "#256abf", "#1c5cab", "#184f95", "#104281",
              "#0d366b"]
SURFACE  = "#fcfcfb"
MUR      = "#3f3e3b"
INK      = "#0b0b0b"
INK_DOUX = "#52514e"
ALERTE   = "#c0392b"

DIRS = ((0, -1), (1, 0), (0, 1), (-1, 0))


def charge(num):
    p = os.path.join(RACINE, "level%04d.xsb" % num)
    if not os.path.exists(p):
        return None
    with open(p) as f:
        lignes = [l.rstrip("\n") for l in f if l.strip("\n")]
    L = max(len(l) for l in lignes)
    return [l.ljust(L) for l in lignes]


def analyse(g):
    H, L = len(g), len(g[0])
    mur = lambda x, y: not (0 <= x < L and 0 <= y < H) or g[y][x] == '#'

    buts    = {(x, y) for y in range(H) for x in range(L) if g[y][x] in '.*+'}
    caisses = [(x, y) for y in range(H) for x in range(L) if g[y][x] in '$*']
    joueurs = [(x, y) for y in range(H) for x in range(L) if g[y][x] in '@+']
    if not buts or not caisses or not joueurs:
        return None

    def atteintUnBut(depart, bouchee):
        """La caisse partie de 'depart' peut-elle encore atteindre un but ?
        Propagation par POUSSEE : arrivee libre ET case d'appui libre."""
        if depart == bouchee:
            return False
        if depart in buts:
            return True
        vus = {depart}
        q = deque([depart])
        while q:
            x, y = q.popleft()
            for dx, dy in DIRS:
                arr = (x + dx, y + dy)          # ou la caisse atterrit
                app = (x - dx, y - dy)          # ou le joueur doit se tenir
                if arr in vus:                  continue
                if arr == bouchee or app == bouchee:  continue
                if mur(*arr) or mur(*app):      continue
                if arr in buts:                 return True
                vus.add(arr); q.append(arr)
        return False

    # Cases du plateau : celles qu'au moins une caisse peut occuper (ou un but).
    plateau = set()
    for c in caisses:
        vus = {c}
        q = deque([c])
        while q:
            x, y = q.popleft()
            for dx, dy in DIRS:
                arr = (x + dx, y + dy)
                app = (x - dx, y - dy)
                if arr in vus or mur(*arr) or mur(*app):
                    continue
                vus.add(arr); q.append(arr)
        plateau |= vus
    plateau |= buts

    goulots = {}
    for c in plateau:
        bloquees = sum(1 for k in caisses if not atteintUnBut(k, c))
        if bloquees:
            goulots[c] = bloquees

    return goulots, buts, caisses, joueurs[0], L, H


def dessine(num, g, res):
    goulots, buts, caisses, joueur, L, H = res
    n = len(caisses)
    vmax = max(goulots.values()) if goulots else 1

    # une couleur DISCRETE par nombre de caisses
    pas = max(1, (len(RAMPE_BASE) - 1) // max(vmax, 1))
    couleurs = [RAMPE_BASE[min(len(RAMPE_BASE) - 1, i * pas)] for i in range(vmax)]
    cmap = ListedColormap(couleurs)
    norm = BoundaryNorm(range(1, vmax + 2), cmap.N)

    fig, ax = plt.subplots(figsize=(max(7, L * .62), max(6, H * .62 + 2.6)))
    fig.patch.set_facecolor(SURFACE); ax.set_facecolor(SURFACE)

    for y in range(H):
        for x in range(L):
            if g[y][x] == '#':
                ax.add_patch(Rectangle((x, y), 1, 1, facecolor=MUR,
                                       edgecolor=SURFACE, linewidth=1.2))
                continue
            v = goulots.get((x, y), 0)
            face = cmap(norm(v)) if v else SURFACE
            ax.add_patch(Rectangle((x, y), 1, 1, facecolor=face,
                                   edgecolor="#dedcd6", linewidth=.9))
            if v:
                ax.text(x + .5, y + .5, str(v), ha="center", va="center",
                        fontsize=10, fontweight="bold",
                        color="#ffffff" if v / max(vmax, 1) >= .55 else INK)
            if (x, y) in buts:
                ax.add_patch(Circle((x + .5, y + .5), .34, fill=False,
                                    edgecolor=INK, linewidth=1.7, zorder=5))
            if (x, y) in caisses:
                ax.add_patch(Rectangle((x + .27, y + .27), .46, .46, fill=False,
                                       edgecolor=INK, linewidth=1.7, zorder=6))
            if (x, y) == joueur:
                ax.add_patch(Circle((x + .5, y + .5), .17, facecolor=INK_DOUX, zorder=7))

    ax.set_xlim(-.3, L + .3); ax.set_ylim(H + .3, -2.7)
    ax.set_aspect("equal"); ax.axis("off")

    total = sum(1 for v in goulots.values() if v == n)
    ax.text(-.3, -2.0, "Niveau %d — passages obliges" % num,
            fontsize=15, fontweight="bold", color=INK, va="bottom")
    ax.text(-.3, -1.2,
            "%d caisses, %d buts.  Chiffre = caisses qui perdent TOUT but si la case est bouchee."
            % (n, len(buts)), fontsize=9.5, color=INK_DOUX, va="bottom")
    ax.text(-.3, -0.45,
            ("%d case(s) bloquent les %d caisses — goulot total." % (total, n))
            if total else
            ("Aucune case ne bloque les %d caisses a la fois." % n),
            fontsize=9.5, color=ALERTE if total else INK_DOUX,
            fontweight="bold" if total else "normal", va="bottom")

    sm = ScalarMappable(cmap=cmap, norm=norm)
    cb = fig.colorbar(sm, ax=ax, fraction=.030, pad=.02,
                      ticks=[i + .5 for i in range(1, vmax + 1)])
    cb.ax.set_yticklabels([str(i) for i in range(1, vmax + 1)])
    cb.set_label("caisses bloquees", fontsize=9, color=INK_DOUX)
    cb.ax.tick_params(labelsize=8, colors=INK_DOUX, length=0)
    cb.outline.set_visible(False)

    fig.tight_layout()
    fig.savefig(os.path.join(SORTIE, "niveau%02d.png" % num), dpi=140,
                facecolor=SURFACE)
    plt.close(fig)
    return total, vmax, n


if __name__ == "__main__":
    os.makedirs(SORTIE, exist_ok=True)
    print("%-5s %-8s %-7s %-11s %s" % ("niv", "caisses", "max", "goulot tot.", "verdict"))
    for num in range(1, 33):
        g = charge(num)
        if not g:
            continue
        res = analyse(g)
        if not res:
            print("%-5d (illisible)" % num); continue
        total, vmax, n = dessine(num, g, res)
        print("%-5d %-8d %-7d %-11d %s" % (
            num, n, vmax, total,
            "GOULOT TOTAL" if total else ("max %d/%d" % (vmax, n))))
    print("\nPNG dans mesures/goulots/")
