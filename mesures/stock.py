# stock — LES DEUX SIGNATURES DU « STOCK », minées dans les parties gagnées.
#
#   python3 stock.py [niveau ...]       # défaut : tous les journaux hybrides
#
# Prolonge attente.py (2026-08-09). L'utilisateur a séparé deux notions que le
# projet confondait sous « stock » ; ce sont deux SIGNATURES distinctes, toutes
# deux lisibles dans les données DÉJÀ loguées, sans toucher au solveur (§6.0) :
#
#   (1) LIVRABLE MAIS DIFFÉRÉE — « on pourrait la poser, on ne le fait pas ».
#       Le mode hybride logue à chaque pas `macro jouable : caisse (x,y)` : la
#       caisse qui, MAINTENANT, boucle le but actif d'un seul geste. Si le joueur
#       fait autre chose pendant que cette ligne persiste, il GARDE la caisse pour
#       plus tard. Mesuré : nombre de coups où la caisse est livrable sans être
#       livrée (`diff`), avant sa livraison finale.
#       ⚠️ K ∈ {0,1} : à chaque pas, au plus UNE caisse est livrable (vers le but
#       actif seul — c'est le régime d'engagement de la macro, cf. §7 « mesure
#       l'écart à ordreButs »). `diff` élevé = le joueur joue hors de l'ordre
#       statique, pas forcément qu'il « stocke » : à CROISER avec porte/attente.
#
#   (2) DÉPLACÉE EN PLUSIEURS FOIS — « détour payé », le vrai stockage.
#       Un trajet net vers le but = une seule salve de poussées. Une caisse garée
#       puis reprise a DEUX salves ou plus, séparées par une immobilité. Mesuré :
#       nombre de ruptures mouvement / immobilité / re-mouvement (`salves-1`).
#
# ⚠️ Ne compte que les parties GAGNÉES et rejouables (rejeu validé coup par coup),
# comme attente.py. Identité de caisse suivie de case en case depuis le départ.

import sys, os, re, glob
from taches import charge, R

re_mouv = re.compile(
    r'^\[mouv\] joueur \((\d+),(\d+)\)->\((\d+),(\d+)\)'
    r'(?: POUSSE caisse ->\((\d+),(\d+)\))?')
re_deliv = re.compile(r'macro jouable : caisse \((\d+),(\d+)\)')


def parties_riches(journal):
    """Comme taches.parties, mais chaque coup porte la caisse LIVRABLE annoncée
    par le [hybride] qui le précède. Rend une liste de parties ; chaque partie
    = liste de (deliv_or_None, (p1, p2, p3))."""
    out, cur, pend = [], None, None
    for l in open(journal, errors='replace'):
        l = l.rstrip('\n')
        if l.startswith('=== niveau'):
            if cur is not None: out.append(cur)
            cur, pend = [], None
            continue
        if cur is None:
            continue
        if l.startswith('[undo]'):
            if cur: cur.pop()
            pend = None
            continue
        if l.startswith('[hybride] posees'):           # ligne d'état, pré-coup
            m = re_deliv.search(l)
            pend = (int(m.group(1)), int(m.group(2))) if m else None
            continue
        m = re_mouv.match(l)
        if m:
            g = m.groups()
            p1 = (int(g[0]), int(g[1])); p2 = (int(g[2]), int(g[3]))
            p3 = None if g[4] is None else (int(g[4]), int(g[5]))
            cur.append((pend, (p1, p2, p3)))
            pend = None                                 # consommé : un macro
            continue                                    # interne ne le garde pas
    if cur is not None:
        out.append(cur)
    return out


def analyse(niv):
    """Rejoue la DERNIÈRE partie gagnée et rend, par caisse :
       (depart, fin, sur_but, diff, livree_apres_diff, salves, ruptures).
    None si aucune partie gagnée rejouable."""
    jr = f"{R}/hybride_niveau_{niv:04d}.txt"
    lv = f"{R}/level{niv:04d}.xsb"
    if not os.path.exists(jr) or not os.path.exists(lv):
        return None
    g0 = charge(lv)
    but = {(x, y) for y, r in enumerate(g0) for x, c in enumerate(r) if c in '.*+'}

    retenu = None
    for p in parties_riches(jr):
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
        traj = {i: [depart[i]] for i in range(n)}       # positions à chaque poussée
        gpush = {i: [] for i in range(n)}               # rangs GLOBAUX de poussée
        diff = {i: 0 for i in range(n)}                 # coups livrable-mais-pas-livrée

        tfirst = {i: None for i in range(n)}    # 1er coup où la caisse est livrable
        tarr = {i: 0 for i in range(n)}         # coup de son arrivée FINALE
        # journal complet des coups (p2 = case où va le joueur, p3 = case caisse)
        pas = []

        ok, t, np = True, 0, 0                           # np = compteur de poussées
        for (deliv, (p1, p2, p3)) in p:
            if j != p1: ok = False; break
            # la caisse livrable (état AVANT le coup) est-elle poussée CE coup ?
            dbox = ident.get(deliv) if deliv is not None else None
            pousse_ici = None
            if p3 is not None:
                if not cai(*p2) or not lib(*p3): ok = False; break
                pousse_ici = ident.get(p2)
                vC(*p2); pC(*p3)
                i = ident.pop(p2); ident[p3] = i
                traj[i].append(p3); gpush[i].append(np); np += 1
                tarr[i] = t
            elif not lib(*p2):
                ok = False; break
            if dbox is not None:
                if tfirst[dbox] is None:
                    tfirst[dbox] = t
                if dbox != pousse_ici:
                    diff[dbox] += 1            # livrable, mais on joue ailleurs
            pas.append((p2, p3))
            vJ(*j); pJ(*p2); j = p2; t += 1

        if ok and all(c != '$' for r in g for c in r) and t:
            retenu = (traj, gpush, depart, diff, tfirst, tarr, pas, t)

    if retenu is None:
        return None
    traj, gpush, depart, diff, tfirst, tarr, pas, T = retenu

    res = []
    for i in traj:
        fin = traj[i][-1]
        # UNE SALVE = suite de poussées consécutives dans l'ordre GLOBAL des
        # poussées : tant qu'aucune AUTRE caisse ne bouge entre deux, c'est le
        # même geste (une macro, un run à la main). Un trou = la caisse a été
        # GARÉE pendant qu'on poussait ailleurs → une rupture. Indépendant des
        # pas de marche, qui n'incrémentent pas le compteur de poussées.
        gs = gpush[i]
        salves = sum(1 for k, v in enumerate(gs) if k == 0 or v != gs[k - 1] + 1)
        livree = fin in but
        # REJEU CONTREFACTUEL (a vs b). B tenue livrable de tfirst à tarr : si on
        # la posait tôt, G=fin serait occupée pendant toute la fenêtre. Le jeu
        # humain touche-t-il G dans [tfirst, tarr) ? Un pas touche G si le joueur
        # y marche (p2==G) ou si une caisse y est poussée (p3==G). >0 => poser B
        # tôt aurait bloqué un passage RÉELLEMENT emprunté => (a) prouvé.
        touchesG, premier = 0, None
        if livree and tfirst[i] is not None and tfirst[i] < tarr[i]:
            for tt in range(tfirst[i], tarr[i]):
                p2, p3 = pas[tt]
                if p2 == fin or p3 == fin:
                    touchesG += 1
                    if premier is None: premier = tt
        # `diff` n'est parlant que si la caisse finit livrée (sinon jamais son but)
        res.append({
            'id': i, 'dep': depart[i], 'fin': fin, 'livree': livree,
            'diff': diff[i], 'salves': salves, 'ruptures': max(0, salves - 1),
            'fenetre': (tarr[i] - tfirst[i]) if (livree and tfirst[i] is not None) else 0,
            'touchesG': touchesG, 'premierTouch': premier,
        })
    return res, T


def liste_niveaux(args):
    return args or sorted(
        int(m.group(1))
        for f in glob.glob(f"{R}/hybride_niveau_*.txt")
        for m in [re.search(r'hybride_niveau_(\d+)\.txt$', f)] if m)


def main_passage(args):
    """Rejeu contrefactuel : les caisses tenues livrables bloquent-elles un passage ?
    Ne montre que les caisses réellement tenues (diff >= SEUIL coups)."""
    SEUIL = 30
    print("PASSAGE — rejeu contrefactuel (a) ferme un passage  vs  (b) ordre mauvais")
    print(f"  caisses tenues livrables >= {SEUIL} coups. G = but final de la caisse.")
    print("  touchesG = pas du jeu humain touchant G pendant la tenue (marche ou poussée)")
    print("     > 0  => poser tôt bloquait un passage EMPRUNTÉ            => (a)")
    print("     = 0  => G jamais utilisée pendant la tenue                => (b)\n")
    na, nb = 0, 0
    for niv in liste_niveaux(args):
        r = analyse(niv)
        if r is None:
            continue
        res, T = r
        tenues = sorted((c for c in res if c['livree'] and c['diff'] >= SEUIL),
                        key=lambda c: -c['diff'])
        if not tenues:
            continue
        print(f"=== niveau {niv} — {T} coups ===")
        for c in tenues:
            verdict = "(a) BLOQUE" if c['touchesG'] > 0 else "(b) ordre ?"
            pt = f", 1er au coup {c['premierTouch']}" if c['premierTouch'] is not None else ""
            print(f"   {str(c['dep']):>8} livrable {c['diff']:>3}c, fenêtre {c['fenetre']:>4}c "
                  f"-> G={c['fin']} touchée {c['touchesG']:>3}x  {verdict}{pt}")
            if c['touchesG'] > 0: na += 1
            else: nb += 1
        print()
    print("-" * 60)
    print(f"tenues (>= {SEUIL}c) : {na} en (a) ferme un passage, {nb} en (b) ordre ?")


def reachable(walls, boxset, player, L, H):
    """Cases où le joueur peut MARCHER (murs + caisses bloquent). Ensemble de cases."""
    from collections import deque
    seen = {player}; dq = deque([player])
    while dq:
        x, y = dq.popleft()
        for dx, dy in ((0, -1), (1, 0), (0, 1), (-1, 0)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < L and 0 <= ny < H and (nx, ny) not in seen \
               and walls[ny][nx] != '#' and (nx, ny) not in boxset:
                seen.add((nx, ny)); dq.append((nx, ny))
    return seen


def _acces(C, reach):
    x, y = C
    return any((x + dx, y + dy) in reach for dx, dy in ((0, -1), (1, 0), (0, 1), (-1, 0)))


def snapshots_tenues(niv, seuil):
    """Rejoue la partie gagnante et rend, pour chaque caisse tenue (diff>=seuil,
    livrée), un instantané au coup tfirst : (dep, G, diff, joueur, boxset, posB).
    boxset = positions des caisses à cet instant. Rend aussi (walls, but, L, H)."""
    base = analyse(niv)
    if base is None:
        return None
    res, _T = base
    G = {c['id']: c['fin'] for c in res}
    livree = {c['id']: c['livree'] for c in res}
    diff = {c['id']: c['diff'] for c in res}
    vises = {c['id'] for c in res
             if livree[c['id']] and diff[c['id']] >= seuil}
    if not vises:
        return None

    jr = f"{R}/hybride_niveau_{niv:04d}.txt"
    g0 = charge(f"{R}/level{niv:04d}.xsb")
    walls = [[c if c == '#' else ' ' for c in r] for r in g0]
    but = {(x, y) for y, r in enumerate(g0) for x, c in enumerate(r) if c in '.*+'}
    H, L = len(g0), max(len(r) for r in g0)

    snaps = None
    for p in parties_riches(jr):
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
                if c in '$*': ident[(x, y)] = n; n += 1
        vus, loc = set(), {}                        # instantanés récoltés
        ok, t = True, 0
        for (deliv, (p1, p2, p3)) in p:
            if j != p1: ok = False; break
            dbox = ident.get(deliv) if deliv is not None else None
            if dbox is not None and dbox in vises and dbox not in vus:
                vus.add(dbox)
                boxset = frozenset(ident.keys())
                loc[dbox] = (j, boxset, [k for k, v in ident.items() if v == dbox][0])
            if p3 is not None:
                if not cai(*p2) or not lib(*p3): ok = False; break
                vC(*p2); pC(*p3)
                i = ident.pop(p2); ident[p3] = i
            elif not lib(*p2):
                ok = False; break
            vJ(*j); pJ(*p2); j = p2; t += 1
        if ok and all(c != '$' for r in g for c in r) and t:
            snaps = loc

    if snaps is None:
        return None
    out = []
    for bid, (joueur, boxset, posB) in snaps.items():
        out.append({'id': bid, 'G': G[bid], 'diff': diff[bid],
                    'joueur': joueur, 'boxset': boxset, 'posB': posB})
    return out, walls, but, L, H


def main_cut(args):
    """Test de CUT : poser B sur G déconnecte-t-il le joueur de l'accès à une
    caisse non livrée / à un but non rempli ? Prédicat du porte généralisé."""
    SEUIL = 30
    print("CUT — poser B sur G coupe-t-il l'accès à une caisse non livrée / un but ?")
    print(f"  snapshot au coup où B devient livrable. Caisses tenues >= {SEUIL}c.")
    print("  coupe = # caisses non livrées dont l'accès (marche vers un côté) est perdu")
    print("  + buts non remplis rendus inatteignables.  > 0 => (a) par CUT\n")
    na, nb = 0, 0
    for niv in liste_niveaux(args):
        s = snapshots_tenues(niv, SEUIL)
        if s is None:
            continue
        tenues, walls, but, L, H = s
        lignes = []
        for c in sorted(tenues, key=lambda c: -c['diff']):
            G, posB, joueur, boxset = c['G'], c['posB'], c['joueur'], c['boxset']
            r0 = reachable(walls, boxset, joueur, L, H)
            boxset1 = (boxset - {posB}) | {G}
            r1 = reachable(walls, boxset1, joueur, L, H)
            nonlivrees = [b for b in boxset if b not in but and b != posB]
            coupees = [b for b in nonlivrees if _acces(b, r0) and not _acces(b, r1)]
            # ⚠️ EXCLURE G du décompte : poser B sur G rend trivialement la case G
            # non-marchable, mais l'OCCUPER est le but recherché, pas un cut. Sans
            # cette exclusion, 90 % des cas sortaient « coupe 1 but » = G elle-même.
            # On ne compte que les AUTRES buts non remplis rendus inatteignables.
            butsperdus = [gg for gg in but if gg != G and gg not in boxset
                          and gg in r0 and gg not in r1]
            n = len(coupees) + len(butsperdus)
            verdict = "(a) CUT" if n > 0 else "(b)"
            lignes.append((c['diff'], posB, G, n, len(coupees), len(butsperdus), verdict))
            if n > 0: na += 1
            else: nb += 1
        if lignes:
            print(f"=== niveau {niv} ===")
            for d, posB, G, n, nc, nb2, verdict in lignes:
                print(f"   {str(posB):>8} livrable {d:>3}c -> G={str(G):>8} : "
                      f"coupe {n:>2} ({nc} caisses, {nb2} buts)  {verdict}")
            print()
    print("-" * 60)
    print(f"tenues (>= {SEUIL}c) : {na} en (a) CUT, {nb} en (b)")


def replay_events(niv):
    """Rejoue la partie gagnante. Rend (dep, events, tfirst, tdeliv, G, livree, diff)
    ou None. events = liste (t, boxid, case occupée après la poussée)."""
    base = analyse(niv)
    if base is None:
        return None
    res, _T = base
    G = {c['id']: c['fin'] for c in res}
    livree = {c['id']: c['livree'] for c in res}
    diff = {c['id']: c['diff'] for c in res}
    jr = f"{R}/hybride_niveau_{niv:04d}.txt"
    g0 = charge(f"{R}/level{niv:04d}.xsb")
    keep = None
    for p in parties_riches(jr):
        g = [r[:] for r in g0]
        lib = lambda x, y: g[y][x] in ' .'; cai = lambda x, y: g[y][x] in '$*'
        def vJ(x, y): g[y][x] = '.' if g[y][x] == '+' else ' '
        def pJ(x, y): g[y][x] = '+' if g[y][x] == '.' else '@'
        def vC(x, y): g[y][x] = '.' if g[y][x] == '*' else ' '
        def pC(x, y): g[y][x] = '*' if g[y][x] == '.' else '$'
        j = [(x, y) for y, r in enumerate(g) for x, c in enumerate(r) if c in '@+'][0]
        ident, n = {}, 0
        for y, r in enumerate(g):
            for x, c in enumerate(r):
                if c in '$*': ident[(x, y)] = n; n += 1
        dep = {v: k for k, v in ident.items()}
        events, tfirst, tdeliv = [], {}, {}
        ok, t = True, 0
        for (deliv, (p1, p2, p3)) in p:
            if j != p1: ok = False; break
            db = ident.get(deliv) if deliv is not None else None
            if db is not None and db not in tfirst: tfirst[db] = t
            if p3 is not None:
                if not cai(*p2) or not lib(*p3): ok = False; break
                bid = ident[p2]; vC(*p2); pC(*p3); ident[p3] = ident.pop(p2)
                events.append((t, bid, p3)); tdeliv[bid] = t
            elif not lib(*p2): ok = False; break
            vJ(*j); pJ(*p2); j = p2; t += 1
        if ok and all(c != '$' for r in g for c in r) and t:
            keep = (dep, events, tfirst, tdeliv, G, livree, diff)
    return keep


def main_contention(args):
    """Test de CONTENTION DE CORRIDOR : la caisse tenue est-elle bloquée parce que
    son TRAJET de livraison est encore emprunté par d'autres caisses ? (§3/§4).
    Distinct du porte : ce n'est pas la case-but qui bloque, c'est le chemin pour
    l'atteindre — pur ordonnancement, pas une contrainte statique."""
    SEUIL = 30
    print("CONTENTION — la caisse tenue attend-elle que son CORRIDOR de livraison se libère ?")
    print(f"  P = cases traversées par la caisse pour aller à G. Tenues >= {SEUIL}c.")
    print("  busy = # autres caisses passant sur P APRÈS que la caisse soit livrable")
    print("     > 0 => livrer tôt bloquerait un corridor actif => CONTENTION (§3)\n")
    nc, nn = 0, 0
    for niv in liste_niveaux(args):
        rv = replay_events(niv)
        if rv is None: continue
        dep, events, tfirst, tdeliv, G, livree, diff = rv
        occ = {}                                     # cell -> liste (t, boxid)
        path = {}                                    # boxid -> cases traversées
        for (t, b, c) in events:
            occ.setdefault(c, []).append((t, b))
            path.setdefault(b, set()).add(c)
        tenues = sorted((b for b in diff
                         if livree.get(b) and diff[b] >= SEUIL
                         and tfirst.get(b) is not None),
                        key=lambda b: -diff[b])
        lignes = []
        for b in tenues:
            P = path.get(b, set()) | {dep[b]}
            tf, td = tfirst[b], tdeliv.get(b, 1 << 30)
            # autres caisses passant sur P dans [tfirst, tdeliv)
            busy = set(); last = None
            for c in P:
                for (t, bb) in occ.get(c, ()):
                    if bb != b and tf <= t < td:
                        busy.add(bb)
                        last = t if last is None else max(last, t)
            lignes.append((diff[b], dep[b], G[b], len(busy), last, td))
            if busy: nc += 1
            else: nn += 1
        if lignes:
            print(f"=== niveau {niv} ===")
            for d, dp, gg, nb, last, td in lignes:
                v = f"CONTENTION (dernier t={last}, livrée t={td})" if nb else "libre"
                print(f"   {str(dp):>8} tenue {d:>3}c -> G={str(gg):>8} : "
                      f"corridor busy par {nb:>2} caisses  {v}")
            print()
    print("-" * 60)
    print(f"tenues (>= {SEUIL}c) : {nc} en CONTENTION de corridor, {nn} corridor libre")


def main_bilan(args):
    """DÉCOMPOSITION des caisses tenues sur les 3 mécanismes mesurés :
       - PORTE : poser sur G bloque un passage (transit strict OU cut d'articulation)
       - CONGESTION : le corridor de livraison est encore emprunté (contention §3)
       - ni l'un ni l'autre => vrai ORDRE (rien ne force la tenue)."""
    SEUIL = 30
    print(f"BILAN — pourquoi les caisses tenues (>= {SEUIL}c) sont-elles tenues ?")
    print("  PORTE = transit strict de G  OU  cut d'articulation")
    print("  CONGESTION = corridor de livraison busy après que la caisse soit livrable\n")
    tot = {'porte': 0, 'cong': 0, 'both': 0, 'ordre': 0, 'n': 0}
    for niv in liste_niveaux(args):
        base = analyse(niv)
        if base is None: continue
        res, _T = base
        held = {c['id'] for c in res if c['livree'] and c['diff'] >= SEUIL}
        if not held: continue
        info = {c['id']: c for c in res}
        # transit
        transit = {i: info[i]['touchesG'] > 0 for i in held}
        # cut
        cut = {i: False for i in held}
        s = snapshots_tenues(niv, SEUIL)
        if s is not None:
            tenues, walls, but, L, H = s
            for c in tenues:
                if c['id'] not in held: continue
                r0 = reachable(walls, c['boxset'], c['joueur'], L, H)
                b1 = (c['boxset'] - {c['posB']}) | {c['G']}
                r1 = reachable(walls, b1, c['joueur'], L, H)
                nl = [b for b in c['boxset'] if b not in but and b != c['posB']]
                coup = any(_acces(b, r0) and not _acces(b, r1) for b in nl)
                bp = any(gg != c['G'] and gg not in c['boxset'] and gg in r0 and gg not in r1
                         for gg in but)
                cut[c['id']] = coup or bp
        # contention
        cong = {i: False for i in held}
        rv = replay_events(niv)
        if rv is not None:
            dep, events, tfirst, tdeliv, G, livree, diff = rv
            occ, path = {}, {}
            for (t, b, cc) in events:
                occ.setdefault(cc, []).append((t, b)); path.setdefault(b, set()).add(cc)
            for i in held:
                if tfirst.get(i) is None: continue
                P = path.get(i, set()) | {dep[i]}
                tf, td = tfirst[i], tdeliv.get(i, 1 << 30)
                cong[i] = any(bb != i and tf <= t < td
                              for cc in P for (t, bb) in occ.get(cc, ()))
        lignes = []
        for i in sorted(held, key=lambda i: -info[i]['diff']):
            porte = transit[i] or cut[i]
            klass = ('PORTE+CONG' if porte and cong[i] else 'PORTE' if porte
                     else 'CONGESTION' if cong[i] else 'ordre ?')
            tot['n'] += 1
            tot['both' if porte and cong[i] else 'porte' if porte
                else 'cong' if cong[i] else 'ordre'] += 1
            lignes.append((info[i]['diff'], info[i]['dep'], info[i]['fin'],
                           transit[i], cut[i], cong[i], klass))
        print(f"=== niveau {niv} ===")
        for d, dp, gg, tr, cu, co, klass in lignes:
            tags = ("T" if tr else "-") + ("C" if cu else "-") + ("G" if co else "-")
            print(f"   {str(dp):>8} tenue {d:>3}c -> {str(gg):>8}  [{tags}]  {klass}")
        print()
    print("-" * 60)
    n = max(1, tot['n'])
    print(f"{tot['n']} caisses tenues : "
          f"PORTE seul {tot['porte']} ({100*tot['porte']//n}%), "
          f"CONGESTION seule {tot['cong']} ({100*tot['cong']//n}%), "
          f"les deux {tot['both']} ({100*tot['both']//n}%), "
          f"ni l'un ni l'autre {tot['ordre']} ({100*tot['ordre']//n}%)")
    exp = tot['porte'] + tot['cong'] + tot['both']
    print(f"expliquées par un mécanisme de PASSAGE : {exp}/{tot['n']} ({100*exp//n}%)")


def main_depart(args):
    """4e mécanisme, symétrique du transit : la caisse tenue est-elle un BOUCHON ?
    La RETIRER de sa case (la livrer) ouvre-t-elle au joueur l'accès à une caisse
    non livrée / un but jusque-là inatteignable ? Si oui, sa livraison est un
    prérequis pour ouvrir une région => contrainte de timing, pas ordre libre."""
    SEUIL = 30
    print("DEPART — la caisse tenue est-elle un BOUCHON (la retirer ouvre une région) ?")
    print(f"  snapshot au coup où B devient livrable. Tenues >= {SEUIL}c.")
    print("  ouvre = # caisses non livrées / buts que RETIRER B rend atteignables")
    print("     > 0 => livrer B ouvre une région => sa tenue a un coût de timing\n")
    na, nb = 0, 0
    for niv in liste_niveaux(args):
        s = snapshots_tenues(niv, SEUIL)
        if s is None: continue
        tenues, walls, but, L, H = s
        lignes = []
        for c in sorted(tenues, key=lambda c: -c['diff']):
            posB, joueur, boxset = c['posB'], c['joueur'], c['boxset']
            r0 = reachable(walls, boxset, joueur, L, H)
            r1 = reachable(walls, boxset - {posB}, joueur, L, H)  # B retirée
            nl = [b for b in boxset if b not in but and b != posB]
            ouv_c = [b for b in nl if not _acces(b, r0) and _acces(b, r1)]
            ouv_g = [gg for gg in but if gg != posB and gg not in boxset
                     and gg not in r0 and gg in r1]
            n = len(ouv_c) + len(ouv_g)
            lignes.append((c['diff'], posB, c['G'], n, len(ouv_c), len(ouv_g)))
            if n > 0: na += 1
            else: nb += 1
        if lignes:
            print(f"=== niveau {niv} ===")
            for d, posB, G, n, oc, og in lignes:
                v = "BOUCHON" if n > 0 else "-"
                print(f"   {str(posB):>8} tenue {d:>3}c -> G={str(G):>8} : "
                      f"ouvre {n:>2} ({oc} caisses, {og} buts)  {v}")
            print()
    print("-" * 60)
    print(f"tenues (>= {SEUIL}c) : {na} BOUCHON, {nb} sans effet d'ouverture")


def main():
    if 'depart' in sys.argv:
        return main_depart([int(a) for a in sys.argv[1:] if a.isdigit()])
    if 'bilan' in sys.argv:
        return main_bilan([int(a) for a in sys.argv[1:] if a.isdigit()])
    if 'contention' in sys.argv:
        return main_contention([int(a) for a in sys.argv[1:] if a.isdigit()])
    if 'cut' in sys.argv:
        return main_cut([int(a) for a in sys.argv[1:] if a.isdigit()])
    if 'passage' in sys.argv:
        return main_passage([int(a) for a in sys.argv[1:] if a.isdigit()])
    args = [int(a) for a in sys.argv[1:] if a.isdigit()]
    if not args:
        args = sorted(int(m.group(1))
                      for f in glob.glob(f"{R}/hybride_niveau_*.txt")
                      for m in [re.search(r'hybride_niveau_(\d+)\.txt$', f)] if m)

    print("STOCK — deux signatures minées dans les parties gagnées")
    print("  (1) diff   = coups où la caisse est LIVRABLE (macro jouable) mais pas livrée")
    print("  (2) rupt   = ruptures mouvement/immobilité/re-mouvement (déplacée en plusieurs fois)\n")

    glob_diff, glob_rupt, nn = [], [], 0
    for niv in args:
        r = analyse(niv)
        if r is None:
            continue
        res, T = r
        nn += 1
        # signature 1 : les caisses livrées différées, triées par diff
        s1 = sorted((c for c in res if c['livree'] and c['diff'] > 0),
                    key=lambda c: -c['diff'])
        # signature 2 : les caisses déplacées en plusieurs fois
        s2 = sorted((c for c in res if c['ruptures'] > 0),
                    key=lambda c: -c['ruptures'])
        print(f"=== niveau {niv} — {T} coups, {len(res)} caisses ===")
        if s1:
            bout = ", ".join(f"{c['dep']} {c['diff']} ({c['diff']*100//T}%)" for c in s1[:5])
            print(f"   (1) différées : {bout}")
        else:
            print(f"   (1) différées : aucune")
        if s2:
            bout = ", ".join(f"{c['dep']}→{c['fin']} ×{c['salves']}" for c in s2[:5])
            print(f"   (2) en plusieurs fois : {bout}")
        else:
            print(f"   (2) en plusieurs fois : aucune")
        glob_diff += [c['diff'] for c in res if c['livree']]
        glob_rupt += [c['ruptures'] for c in res]
        print()

    if nn:
        import statistics as st
        d_nz = [x for x in glob_diff if x > 0]
        print("-" * 60)
        print(f"{nn} niveaux — caisses livrées : {len(glob_diff)}")
        print(f"   différées (diff>0) : {len(d_nz)}/{len(glob_diff)} "
              f"({100*len(d_nz)//max(1,len(glob_diff))}%), médiane des non nuls "
              f"{st.median(d_nz):.0f} coups" if d_nz else "   aucune différée")
        r_nz = [x for x in glob_rupt if x > 0]
        print(f"   en plusieurs fois (rupt>0) : {len(r_nz)}/{len(glob_rupt)} "
              f"({100*len(r_nz)//max(1,len(glob_rupt))}%)")


if __name__ == '__main__':
    main()
