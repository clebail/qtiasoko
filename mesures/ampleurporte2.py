# ampleurporte2 — L'AMPLEUR DU PORTE GÉNÉRALISÉ, RESTREINTE AU BUT ACTIF
# STATIQUE (§6.0, 2026-08-18, suite de ampleurporte.py).
#
#   python3 ampleurporte2.py [niveau ...]
#
# ⚠️ `ampleurporte.py` (premier jet) scannait TOUTES les paires (caisse × but)
# à chaque jalon : 310/435 (71 %) coupées, MAIS le chiffre ne discrimine RIEN
# — résolus et non-résolus sont touchés aux mêmes taux (67 % à 81 % partout).
# Il mesure surtout un fait trivial de géométrie (des coins disjoints du
# plateau ne se voient pas), pas un signal utile pour juger le levier.
#
# Cette version restreint le test au SEUL but que `butActif()` choisirait
# réellement — le rang minimal de `ordreButs` (STATIQUE, calculé une fois au
# chargement, cf. game.h) parmi les buts encore non remplis — et teste si
# l'occuper coupe l'accès à une caisse non livrée ou un autre but non rempli,
# SANS présumer quelle caisse le remplirait (aucune case libérée : occuper G
# comme un mur de plus, rien d'autre ne change). C'est la question que
# `porteBloquee`/`butActif()` posent réellement : « le but candidat est-il
# mûr ? », pas « existe-t-il une paire arbitraire qui coince ».
#
# Les rangs viennent de l'outil `ordre` (déjà compilé, mesures/build/ordre) —
# jamais recalculés en Python : ordreButs dépend de règles trop nombreuses
# (précédence par approches, contiguïté de run, repli rebours) pour être
# rejouées fidèlement hors du moteur.
import sys, os, glob, re, subprocess
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from taches import parties, R
from stock import reachable
from fpporte import rejoue

ORDRE_BIN = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build/ordre/ordre")


def rangs_but(niv):
    """Rend {(x,y): rang} via l'outil C++ `ordre`, ou None si absent/erreur."""
    try:
        out = subprocess.run([ORDRE_BIN, str(niv)], capture_output=True, text=True, timeout=30).stdout
    except Exception:
        return None
    rangs = {}
    for m in re.finditer(r'^\s*(\d+)\.\s+but\s+\((\d+),(\d+)\)', out, re.MULTILINE):
        rangs[(int(m.group(2)), int(m.group(3)))] = int(m.group(1))
    return rangs or None


def _acces(cell, reach):
    x, y = cell
    return any((x + dx, y + dy) in reach for dx, dy in ((0, -1), (1, 0), (0, 1), (-1, 0)))


def scan_jalons_actif(pas, but, g0, rangs):
    """À chaque jalon (avant une livraison finale), teste UNIQUEMENT le but de
    rang minimal parmi les non-remplis. Rend (posB, G_joue, but_actif, bloque,
    coupees, butsperdus)."""
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

    out = []
    for k, (jav, p2, p3) in enumerate(pas):
        if p3 is not None and k in finaux and p3 in but:
            nonremplis = [g for g in but if g not in boxset]
            if nonremplis:
                actif = min(nonremplis, key=lambda g: rangs.get(g, 1 << 30))
                r0 = reachable(walls, boxset, jav, L, H)
                boxset1 = boxset | {actif}     # occupe G, aucune case liberee
                r1 = reachable(walls, boxset1, jav, L, H)
                nonlivrees = [b for b in boxset if b not in but]
                coupees = [b for b in nonlivrees if _acces(b, r0) and not _acces(b, r1)]
                butsperdus = [gg for gg in but if gg != actif and gg not in boxset
                              and gg in r0 and gg not in r1]
                out.append((p2, p3, actif, bool(coupees or butsperdus), coupees, butsperdus))
        if p3 is not None:
            boxset.discard(p2); boxset.add(p3)
    return out


def main():
    args = [int(a) for a in sys.argv[1:] if a.isdigit()]
    niveaux = args or sorted(
        int(m.group(1)) for f in glob.glob(f"{R}/hybride_niveau_*.txt")
        for m in [re.search(r'hybride_niveau_(\d+)\.txt$', f)] if m)

    print("AMPLEURPORTE2 — le but ACTIF statique (rang minimal) est-il bloque a chaque jalon ?\n")
    tot_jalons = tot_bloques = tot_niv = 0
    for niv in niveaux:
        lv = f"{R}/level{niv:04d}.xsb"
        jr = f"{R}/hybride_niveau_{niv:04d}.txt"
        if not os.path.exists(lv) or not os.path.exists(jr):
            continue
        rangs = rangs_but(niv)
        if rangs is None:
            continue
        best = None
        for p in parties(jr):
            r = rejoue(niv, p)
            if r is not None:
                best = r
        if best is None:
            continue
        pas, but, g0 = best
        jalons = scan_jalons_actif(pas, but, g0, rangs)
        if not jalons:
            continue
        bloques = [j for j in jalons if j[3]]
        tot_jalons += len(jalons)
        tot_bloques += len(bloques)
        tot_niv += 1
        marque = " <==" if bloques else ""
        print(f"niveau {niv:>4} : {len(bloques):>2}/{len(jalons):>2} jalons ou le but ACTIF est bloque{marque}")
        for (posB, Gjoue, actif, bloque, cb, cg) in bloques:
            print(f"      but actif {actif} bloque (coupe {len(cb)} caisse(s), {len(cg)} but(s)) "
                  f"-- pendant que le jeu livrait {posB}->{Gjoue}")
    print("-" * 70)
    print(f"total : {tot_bloques}/{tot_jalons} jalons ou le but ACTIF statique est bloque, "
          f"sur {tot_niv} niveaux")


if __name__ == '__main__':
    main()
