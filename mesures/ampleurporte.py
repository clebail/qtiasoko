# ampleurporte — L'AMPLEUR DU PORTE GÉNÉRALISÉ (§6.0, 2026-08-18, suite).
#
#   python3 ampleurporte.py [niveau ...]        # défaut : tous les journaux hybrides
#
# `stock.py cut` ne teste qu'UNE paire (caisse, but) par instantané — celle
# réellement jouée dans la partie humaine — et rend 7/106 tenues. Mais le scan
# EXHAUSTIF de `portegen` sur le niveau 27 avait déjà montré que le motif est
# bien plus fréquent qu'annoncé dès qu'on teste TOUTES les paires possibles à
# un instant donné (jusqu'à 24 coupes simultanées sur un seul état). Cet outil
# généralise cette observation à tous les niveaux journalisés.
#
# PROTOCOLE : rejoue la dernière partie GAGNÉE de chaque niveau (comme
# `fpporte.py`), et scanne exhaustivement TOUTES les paires
# (caisse non livrée × but non rempli) à chaque JALON — l'instant juste AVANT
# une livraison FINALE, càd le moment où `butActif()` (en régime ordre
# dynamique) choisirait le PROCHAIN but. C'est la cadence réelle où le porte
# généralisé interviendrait s'il était câblé (cf. `setOrdreDynamique`,
# game.h : « on ne rechoisit que lorsque le but actif vient d'être REMPLI »),
# pas un scan à CHAQUE poussée — un jalon par but, comme le solveur.
#
# ⚠️ Même prédicat que `portegen`/`stock.py cut`, validé bit-à-bit entre les
# deux (cf. journal du 2026-08-18) : reachable()/_acces(), G exclu de son
# propre décompte.
import sys, os, glob, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from taches import parties, R
from stock import reachable, _acces
from fpporte import rejoue


def scan_jalons(pas, but, g0):
    """Rend, pour chaque livraison FINALE (jalon), (posB, G, npaires, ncoupes,
    exemples) -- exemples = jusqu'à 3 paires coupées (posC_ou_but, cible)."""
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
            r0 = reachable(walls, boxset, jav, L, H)
            nonlivrees = [b for b in boxset if b not in but]
            nonremplis = [g for g in but if g not in boxset]
            npaires, ncoupes, exemples = 0, 0, []
            for c in nonlivrees:
                for g in nonremplis:
                    npaires += 1
                    boxset1 = (boxset - {c}) | {g}
                    r1 = reachable(walls, boxset1, jav, L, H)
                    nl2 = [b for b in boxset if b not in but and b != c]
                    coupees = [b for b in nl2 if _acces(b, r0) and not _acces(b, r1)]
                    butsperdus = [gg for gg in but if gg != g and gg not in boxset
                                  and gg in r0 and gg not in r1]
                    if coupees or butsperdus:
                        ncoupes += 1
                        if len(exemples) < 3:
                            exemples.append((c, g, len(coupees), len(butsperdus)))
            out.append((p2, p3, npaires, ncoupes, exemples))
        if p3 is not None:
            boxset.discard(p2); boxset.add(p3)
    return out


def main():
    args = [int(a) for a in sys.argv[1:] if a.isdigit()]
    niveaux = args or sorted(
        int(m.group(1)) for f in glob.glob(f"{R}/hybride_niveau_*.txt")
        for m in [re.search(r'hybride_niveau_(\d+)\.txt$', f)] if m)

    print("AMPLEURPORTE — scan exhaustif (caisse x but) à chaque jalon (avant chaque livraison)")
    print("  npaires = paires testées, ncoupes = combien sont COUPE par le porte généralisé\n")
    tot_jalons = tot_jalons_touches = tot_niv_touches = 0
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
        jalons = scan_jalons(pas, but, g0)
        if not jalons:
            continue
        touches = [(posB, G, npaires, ncoupes, ex) for (posB, G, npaires, ncoupes, ex) in jalons if ncoupes]
        tot_jalons += len(jalons)
        tot_jalons_touches += len(touches)
        if touches:
            tot_niv_touches += 1
            print(f"=== niveau {niv} — {len(touches)}/{len(jalons)} jalons avec >=1 coupe ===")
            for (posB, G, npaires, ncoupes, ex) in touches:
                exs = ", ".join(f"{c}->{g}({nc}c/{ng}b)" for (c, g, nc, ng) in ex)
                print(f"   avant livraison {posB}->{G} : {ncoupes}/{npaires} paires coupees  [{exs}]")
    print("-" * 60)
    print(f"total : {tot_jalons_touches}/{tot_jalons} jalons avec >=1 coupe, "
          f"sur {tot_niv_touches} niveaux touches")


if __name__ == '__main__':
    main()
