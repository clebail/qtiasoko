# fpporte — LE JUGE FP DU PORTE GÉNÉRALISÉ (§6.0, 2026-08-18).
#
#   python3 fpporte.py [niveau ...]        # défaut : tous les journaux hybrides
#
# Même méthodologie que `mesures/fp` (§6.1) et `mesures/juge_loi.py` (§6.2) :
# rejoue la DERNIÈRE partie gagnée de chaque niveau et interroge le prédicat sur
# CHACUN de ses états réels — toutes soluble par construction (la partie GAGNE),
# donc toute détection positive est un FAUX POSITIF PROUVÉ. À passer avant de
# câbler `Game::porteGeneraliseeCoupe` (game.cpp) dans `butActif()`, comme
# `porteBloquee` l'est déjà.
#
# Le prédicat lui-même (reachable/_acces) est celui déjà validé dans `stock.py`
# (mode `cut`) et confronté bit-à-bit à l'implémentation C++ du moteur
# (`mesures/portegen`) sur les 10 tenues du niveau 27 — 10/10 identiques,
# cf. journal-hybride.md 2026-08-18. Ce juge-ci teste la même formule mais sur
# TOUTE livraison de TOUTE partie gagnée, pas seulement les caisses tenues.
#
# ⚠️ PIÈGE CAPTÉ EN CONSTRUISANT CE JUGE : ne tester QUE la DERNIÈRE poussée de
# chaque caisse (sa position FINALE), jamais une pose intermédiaire. Un couloir
# de buts alignés fait TRANSITER une caisse par plusieurs cases-buts avant sa
# destination réelle (cf. niveau 10, colonne x=17 : chaque nouvelle caisse
# glisse jusqu'au but le plus profond encore libre, en passant par tous les
# buts plus proches sans s'y arrêter). Tester ces poses de PASSAGE comme des
# livraisons a produit ~100 « faux positifs » bidons au premier jet — la caisse
# n'est jamais restée là, donc aucun cut ne s'est jamais produit. Même famille
# de piège que le « cut à 90 » de `stock.py` (compter G elle-même) : le
# prédicat est correct, c'est la définition de « livrée » qui était trop large.
import sys, os, glob, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from taches import charge, parties, R
from stock import reachable, _acces


def rejoue(niv, coups):
    """Rejoue une partie candidate. Rend (gagne, pas, but, g0) ; pas = liste de
    (joueur_avant, p2, p3) — p3 = case d'arrivée de la caisse si poussée."""
    g0 = charge(f"{R}/level{niv:04d}.xsb")
    but = {(x, y) for y, r in enumerate(g0) for x, c in enumerate(r) if c in '.*+'}
    g = [row[:] for row in g0]
    lib = lambda x, y: g[y][x] in ' .'
    cai = lambda x, y: g[y][x] in '$*'
    def videJ(x, y): g[y][x] = '.' if g[y][x] == '+' else ' '
    def poseJ(x, y): g[y][x] = '+' if g[y][x] == '.' else '@'
    def videC(x, y): g[y][x] = '.' if g[y][x] == '*' else ' '
    def poseC(x, y): g[y][x] = '*' if g[y][x] == '.' else '$'
    j = [(x, y) for y, r in enumerate(g) for x, c in enumerate(r) if c in '@+'][0]
    pas = []
    for (p1, p2, p3, mid) in coups:
        if j != p1:
            return None
        if p3 is not None:
            if not cai(*p2) or not lib(*p3):
                return None
            videC(*p2); poseC(*p3)
        elif not lib(*p2):
            return None
        pas.append((j, p2, p3))
        videJ(*j); poseJ(*p2); j = p2
    gagne = bool(pas) and all(c != '$' for r in g for c in r)
    return (pas, but, g0) if gagne else None


def teste(pas, but, g0):
    """Teste le prédicat cut sur chaque livraison FINALE (dernière poussée de
    chaque caisse, identifiée en repérant le dernier index de coup où elle est
    poussée). Rend la liste des faux positifs trouvés."""
    walls = g0
    L, H = len(g0[0]), len(g0)
    boxset = {(x, y) for y, r in enumerate(g0) for x, c in enumerate(r) if c in '$*'}

    ident, n = {}, 0
    for y, r in enumerate(g0):
        for x, c in enumerate(r):
            if c in '$*':
                ident[(x, y)] = n; n += 1
    dernier_coup_de = {}
    for k, (jav, p2, p3) in enumerate(pas):
        if p3 is not None:
            i = ident.pop(p2); ident[p3] = i
            dernier_coup_de[i] = k
    finaux = set(dernier_coup_de.values())

    ident2, n2 = {}, 0
    for y, r in enumerate(g0):
        for x, c in enumerate(r):
            if c in '$*':
                ident2[(x, y)] = n2; n2 += 1
    fps = []
    for k, (jav, p2, p3) in enumerate(pas):
        if p3 is not None:
            i = ident2.pop(p2); ident2[p3] = i
            if k in finaux and p3 in but:
                r0 = reachable(walls, boxset, jav, L, H)
                boxset1 = (boxset - {p2}) | {p3}
                r1 = reachable(walls, boxset1, jav, L, H)
                nonlivrees = [b for b in boxset if b not in but and b != p2]
                coupees = [b for b in nonlivrees if _acces(b, r0) and not _acces(b, r1)]
                butsperdus = [gg for gg in but if gg != p3 and gg not in boxset
                              and gg in r0 and gg not in r1]
                if coupees or butsperdus:
                    fps.append((p2, p3, coupees, butsperdus))
            boxset.discard(p2); boxset.add(p3)
    return fps


def main():
    args = [int(a) for a in sys.argv[1:] if a.isdigit()]
    niveaux = args or sorted(
        int(m.group(1)) for f in glob.glob(f"{R}/hybride_niveau_*.txt")
        for m in [re.search(r'hybride_niveau_(\d+)\.txt$', f)] if m)

    print("FPPORTE — juge FP du porte généralisé : rejeu de partie(s) GAGNÉE(S)")
    print("  toute détection sur un coup réellement joué et gagnant = faux positif prouvé\n")
    total_fp, total_livr, nniv = 0, 0, 0
    for niv in niveaux:
        lv = f"{R}/level{niv:04d}.xsb"
        jr = f"{R}/hybride_niveau_{niv:04d}.txt"
        if not os.path.exists(lv) or not os.path.exists(jr):
            continue
        best = None
        for p in parties(jr):
            r = rejoue(niv, p)
            if r is not None:
                best = r
        if best is None:
            continue
        pas, but, g0 = best
        fps = teste(pas, but, g0)
        nliv = sum(1 for (jav, p2, p3) in pas if p3 is not None and p3 in but)
        total_livr += nliv
        nniv += 1
        if fps:
            print(f"niveau {niv} : {len(fps)} FAUX POSITIF(S) sur {nliv} livraisons finales")
            for (posB, G, cb, cg) in fps:
                print(f"   pose {posB} -> {G} : coupe {len(cb)} caisse(s) {cb}, {len(cg)} but(s) {cg}")
            total_fp += len(fps)
    print("-" * 60)
    print(f"total : {total_fp} faux positifs sur {total_livr} livraisons finales, {nniv} niveaux journalises")


if __name__ == '__main__':
    main()
