# attente — QUELLES CAISSES ATTENDENT, OÙ, ET COMBIEN DE TEMPS.
#
#   python3 attente.py [niveau ...]     # défaut : tous les journaux hybrides
#
# Question, posée par l'utilisateur le 2026-08-09 en distinguant DEUX notions que
# le projet confondait sous le mot « stock » :
#   1. « cette caisse ne gêne en rien, je la garde pour plus tard » — on n'y touche
#      pas, elle attend SUR SA CASE DE DÉPART. Coût zéro.
#   2. le STOCKAGE — on la déplace exprès vers un abri, elle attend AILLEURS, puis
#      on revient la chercher. C'est un détour payé.
#
# LA MAILLE : la plus longue immobilité d'une caisse SUR UNE CASE QUI N'EST PAS UN
# BUT, en pourcentage de la partie. Les deux filtres comptent, et chacun a été
# ajouté après une lecture fausse :
#   - « pas un but » — sans lui, une caisse LIVRÉE tôt et jamais retouchée sort en
#     tête (le 32 : (4,4) « immobile 96 % », en fait posée au coup 22 et finie).
#     Attendre sur son but n'est pas attendre, c'est avoir terminé.
#   - « case de départ ou non » — c'est ce qui SÉPARE les deux notions, et le
#     partage est net : le 14 n'a que des attentes sur place, le 16 que des attentes
#     ailleurs. Le niveau de référence du stockage sort tout seul du bon côté.
#
# ⚠️ Ne compte que les parties GAGNÉES et rejouables (rejeu validé coup par coup).
# ⚠️ Une poussée peut être FORCÉE par la géométrie : sur le 32, le joueur démarre en
# (1,4), seule case du plateau à n'avoir qu'un voisin libre, et doit pousser la
# caisse de (2,4) pour sortir. Elle attend ensuite en (3,4) — donc classée « ailleurs »
# alors qu'elle relève de la notion 1. Le raffinement (ne pas compter un déplacement
# sans alternative) n'est PAS fait.

import sys, os, re, glob
from taches import parties, charge, R


def attentes(niv):
    """Rend (liste, T) ; liste = (duree, case d'attente, case de depart, t0, t1),
    trie par duree decroissante. None si aucune partie gagnee rejouable."""
    chemin = f"{R}/hybride_niveau_{niv:04d}.txt"
    if not os.path.exists(chemin) or not os.path.exists(f"{R}/level{niv:04d}.xsb"):
        return None
    g0 = charge(f"{R}/level{niv:04d}.xsb")
    but = {(x, y) for y, r in enumerate(g0) for x, c in enumerate(r) if c in '.*+'}

    retenu = None
    for p in parties(chemin):
        g = [r[:] for r in g0]
        lib = lambda x, y: g[y][x] in ' .'
        cai = lambda x, y: g[y][x] in '$*'

        def vJ(x, y): g[y][x] = '.' if g[y][x] == '+' else ' '
        def pJ(x, y): g[y][x] = '+' if g[y][x] == '.' else '@'
        def vC(x, y): g[y][x] = '.' if g[y][x] == '*' else ' '
        def pC(x, y): g[y][x] = '*' if g[y][x] == '.' else '$'

        j = [(x, y) for y, r in enumerate(g) for x, c in enumerate(r) if c in '@+'][0]
        ident, n = {}, 0
        for y, r in enumerate(g):
            for x, c in enumerate(r):
                if c in '$*':
                    ident[(x, y)] = n; n += 1
        depart = {v: k for k, v in ident.items()}
        traj = {i: [depart[i]] for i in range(n)}
        quand = {i: [0] for i in range(n)}

        ok, t = True, 0
        for (p1, p2, p3, _m) in p:
            if j != p1: ok = False; break
            if p3 is not None:
                if not cai(*p2) or not lib(*p3): ok = False; break
                vC(*p2); pC(*p3)
                i = ident.pop(p2); ident[p3] = i
                traj[i].append(p3); quand[i].append(t + 1)
            elif not lib(*p2):
                ok = False; break
            vJ(*j); pJ(*p2); j = p2; t += 1

        if ok and all(c != '$' for r in g for c in r) and t:
            retenu = (traj, quand, depart, t)      # la DERNIÈRE partie gagnée

    if retenu is None:
        return None
    traj, quand, depart, T = retenu

    res = []
    for i in traj:
        for k, case in enumerate(traj[i]):
            if case in but:
                continue                            # sur un but = livrée, pas en attente
            a = quand[i][k]
            b = quand[i][k + 1] if k + 1 < len(quand[i]) else T
            res.append((b - a, case, depart[i], a, b))
    return sorted(res, reverse=True), T


def main():
    args = [int(a) for a in sys.argv[1:] if a.isdigit()]
    if not args:
        args = sorted(int(m.group(1))
                      for f in glob.glob(f"{R}/hybride_niveau_*.txt")
                      for m in [re.search(r'hybride_niveau_(\d+)\.txt$', f)] if m)
    print("ATTENTE = plus longue immobilité d'une caisse sur une case QUI N'EST PAS UN BUT")
    print("  « sur place » = elle attend là où elle a commencé  -> on n'y a pas touché")
    print("  « déplacée »  = elle attend là où on l'a mise      -> STOCKAGE, détour payé\n")
    for niv in args:
        r = attentes(niv)
        if r is None:
            continue
        res, T = r
        surPlace = [x for x in res if x[1] == x[2]]
        deplacee = [x for x in res if x[1] != x[2]]
        print(f"=== niveau {niv} — {T} coups ===")
        for nom, lot in (("sur place", surPlace), ("déplacée", deplacee)):
            if not lot:
                print(f"   {nom:9s} : aucune")
                continue
            bouts = ", ".join(f"{str(c[2])}"
                              + (f"→{c[1]}" if c[1] != c[2] else "")
                              + f" {c[0]*100//T}%"
                              for c in lot[:4])
            print(f"   {nom:9s} : {bouts}")
        print()


if __name__ == '__main__':
    main()
