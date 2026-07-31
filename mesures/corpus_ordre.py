# corpus_ordre — L'ORDRE QUE L'UTILISATEUR A RÉELLEMENT JOUÉ, contre l'ordre CALCULÉ.
#
#   python3 corpus_ordre.py            # rapport seul
#   python3 corpus_ordre.py --ecrire   # + écrit les ordres dans mesures/ordres_humains/
#
# Question : « quelles corrections d'ordre ai-je appliquées sur les niveaux gagnés
# à la main ? ». Elles ne sont nulle part en clair — le dépôt ne porte que trois
# fichiers `ordre_niveau_XXXX.txt` (6, 12, 27), le reste vit à l'intérieur des
# parties gagnées des journaux hybrides.
#
# ⚠️ TROISIÈME ÉCRITURE DE CET OUTIL. Les deux premières (`corpus_ordre.py` le
# 2026-08-06, `juge_loi.py` le 2026-08-03) sont mortes avec leur scratchpad. D'où
# ce fichier-ci dans `mesures/`, comme le §1 l'exige.
#
# ⚠️ N'ÉCRIT JAMAIS À LA RACINE, même avec --ecrire : un `ordre_niveau_XXXX.txt`
# posé dans le répertoire courant CHANGE le comportement du solveur (§7), et un
# fichier déposé par un outil de mesure serait exactement le « fichier oublié dans
# un coin » que l'injection bruyante existe pour éviter. Les ordres sortent dans
# `mesures/ordres_humains/`, à copier à la main si on veut les jouer.
#
# LA DÉFINITION, et elle n'est pas neutre : l'ORDRE DE POSE DÉFINITIF. Une caisse
# peut entrer et sortir d'un but plusieurs fois (le §4 rappelle que le parking
# temporaire et le ressortir-d'un-but sont indispensables — la partie gagnante du
# 13 fait 84 départs de but pour 16 poses). Ce qui compte est donc la DERNIÈRE
# arrivée sur chaque but, celle qui n'est plus défaite. C'est la même définition
# que celle employée pour extraire l'ordre du 27 le 2026-08-06.

import sys, os, re, glob, subprocess
from taches import parties, charge, R

ECRIRE = '--ecrire' in sys.argv
SORTIE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ordres_humains')


def pose_definitive(niv, coups):
    """Rejoue la partie et rend (gagnee, [cases de but dans l'ordre de pose
    DEFINITIVE]). Rejeu autonome — on ne réutilise pas taches.rejoue(), qui suit
    l'identité des caisses et pas l'occupation des buts."""
    g = charge(f"{R}/level{niv:04d}.xsb")
    but = {(x, y) for y, r in enumerate(g) for x, c in enumerate(r) if c in '.*+'}
    lib = lambda x, y: g[y][x] in ' .'
    cai = lambda x, y: g[y][x] in '$*'

    def videJ(x, y): g[y][x] = '.' if g[y][x] == '+' else ' '
    def poseJ(x, y): g[y][x] = '+' if g[y][x] == '.' else '@'
    def videC(x, y): g[y][x] = '.' if g[y][x] == '*' else ' '
    def poseC(x, y): g[y][x] = '*' if g[y][x] == '.' else '$'

    j = [(x, y) for y, r in enumerate(g) for x, c in enumerate(r) if c in '@+'][0]
    derniere = {}          # but -> instant de la dernière arrivée
    t = 0
    for (p1, p2, p3, _mid) in coups:
        if j != p1:
            return (False, [])
        if p3 is not None:
            if not cai(*p2) or not lib(*p3):
                return (False, [])
            videC(*p2); poseC(*p3)
            t += 1
            if p3 in but:
                derniere[p3] = t
            if p2 in but and p2 in derniere:
                del derniere[p2]          # la caisse RESSORT : la pose n'était pas définitive
        else:
            if not lib(*p2):
                return (False, [])
        videJ(*j); poseJ(*p2); j = p2

    gagne = all(c != '$' for r in g for c in r)
    ordre = [b for b, _ in sorted(derniere.items(), key=lambda kv: kv[1])]
    return (gagne, ordre)


def ordre_calcule(niv):
    """Le déroulé de `mesures/ordre <niv>`, en cases de but. Rend [] si l'outil
    n'est pas construit — on ne devine pas l'ordre, on l'interroge."""
    exe = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ordre')
    if not os.path.exists(exe):
        return []
    # ⚠️ lancé depuis mesures/, JAMAIS depuis la racine : là-bas les
    # `ordre_niveau_00{06,12,27}.txt` écraseraient l'ordre calculé en silence et
    # on comparerait l'ordre humain… à lui-même.
    r = subprocess.run([exe, str(niv)], capture_output=True, text=True,
                       cwd=os.path.dirname(exe)).stdout
    return [(int(a), int(b)) for a, b in
            re.findall(r'^\s*\d+\.\s+but\s+\((\d+),\s*(\d+)\)', r, re.M)]


def inversions(a, b):
    """Nombre de paires classées à l'envers entre deux permutations (Kendall).
    0 = ordres identiques."""
    rang = {v: i for i, v in enumerate(b)}
    s = [rang[v] for v in a if v in rang]
    return sum(1 for i in range(len(s)) for k in range(i + 1, len(s)) if s[i] > s[k])


def main():
    if ECRIRE:
        os.makedirs(SORTIE, exist_ok=True)
    print(f"{'niv':>4} {'buts':>5} {'joué':>5} {'inv':>5} {'/max':>6}  {'verdict':<40} fichier")
    print("-" * 110)
    resume = []
    for f in sorted(glob.glob(f"{R}/hybride_niveau_*.txt")):
        b = os.path.basename(f)
        if any(k in b for k in ('_intentions', '_manques', 'ordre_calcule')):
            continue
        m = re.search(r'hybride_niveau_(\d+)\.txt', b)
        if not m or not os.path.exists(f"{R}/level{int(m.group(1)):04d}.xsb"):
            continue
        niv = int(m.group(1))

        # ⚠️ TOUTES les parties gagnées, pas seulement la dernière. Un journal en
        # contient plusieurs, et elles n'emploient pas forcément le même ordre : le 6
        # en admet DEUX valides (2026-08-03, colonne 2 d'abord puis colonne 1 d'abord),
        # chacun validant le sien et condamnant l'autre au juge de la loi. Ne garder
        # que la dernière effacerait l'alternative — et c'est l'alternative qui a de la
        # valeur, puisqu'une règle corrigée n'a qu'à retrouver L'UNE d'entre elles.
        vus, ordres = set(), []
        for p in parties(f):
            ok, ordre = pose_definitive(niv, p)
            if ok and ordre and tuple(ordre) not in vus:
                vus.add(tuple(ordre))
                ordres.append(ordre)
        if not ordres:
            continue

        calc = ordre_calcule(niv)
        n = len(calc) if calc else len(ordres[0])
        maxi = n * (n - 1) // 2

        # Le score du niveau est celui de son MEILLEUR ordre : une règle corrigée n'a
        # pas à retrouver toutes les variantes, une seule suffit à la valider.
        scores = [(inversions(o, calc) if calc else -1, o) for o in ordres]
        scores.sort(key=lambda t: (t[0] < 0, t[0]))
        inv, meilleur = scores[0]
        complet = (len(meilleur) == n)
        verdict = ('identique' if inv == 0 else f'{inv} inversion(s)') if inv >= 0 else 'ordre calculé absent'
        if len(ordres) > 1:
            verdict += f'  [{len(ordres)} ordres : ' + '/'.join(str(s[0]) for s in scores) + ']'
        if not complet:
            verdict += f' (partiel {len(meilleur)}/{n})'

        nom = ''
        if ECRIRE and complet:
            for k, (_s, o) in enumerate(scores):
                if len(o) != n:
                    continue
                nom = f'ordre_niveau_{niv:04d}.txt' if k == 0 else f'ordre_niveau_{niv:04d}_v{k+1}.txt'
                with open(os.path.join(SORTIE, nom), 'w') as fh:
                    fh.write(' '.join(f'({x},{y})' for x, y in o) + '\n')
            nom = f'ordre_niveau_{niv:04d}.txt' + (f' (+{len(scores)-1})' if len(scores) > 1 else '')
        print(f"{niv:>4} {n:>5} {len(meilleur):>5} {inv:>5} {maxi:>6}  {verdict:<40} {nom}")
        resume.append((niv, inv, maxi, complet))

    print("-" * 92)
    ident = [r for r in resume if r[1] == 0]
    diff = [r for r in resume if r[1] > 0]
    print(f"{len(resume)} niveau(x) avec une partie gagnée rejouable")
    print(f"  ordre joué == ordre calculé : {len(ident)} -> {[r[0] for r in ident]}")
    print(f"  ordre joué  ≠ ordre calculé : {len(diff)} -> "
          + ', '.join(f'{r[0]} ({r[1]})' for r in sorted(diff, key=lambda r: -r[1])))
    if ECRIRE:
        print(f"\nOrdres écrits dans {SORTIE}/ — À COPIER À LA MAIN à la racine pour les jouer.")
    else:
        print("\n(rapport seul ; --ecrire pour sortir les ordres dans mesures/ordres_humains/)")


if __name__ == '__main__':
    main()
