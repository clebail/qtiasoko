#!/usr/bin/env python3
"""
COMPRESSION DES JOURNAUX HYBRIDES  (2026-08-02)

Réduit `hybride_niveau_XXXX.txt` à ce qui sert vraiment, en trois passes, chacune
un NO-OP sur la partie jouée :

  1. `[undo]` APPLIQUÉS   — on retire la ligne d'annulation ET le coup qu'elle
     annule. ⚠️ Retirer la seule ligne `[undo]` changerait tous les rejeux : le
     parseur de l'app (`MainWindow::rejoueJournal`) les applique, donc un journal
     dont on aurait ôté les marqueurs rejouerait les coups annulés.
  2. BOUCLES DE MARCHE retirées — entre deux poussées, si le perso repasse par une
     case déjà visitée, tout ce qui est entre les deux passages est supprimé. Le
     plateau ET la position du joueur sont identiques après : no-op strict.
  3. UNE SEULE PARTIE GAGNÉE conservée — celle que l'app charge de toute façon (la
     DERNIÈRE contenant « etat gagne »). Les tentatives abandonnées partent.

Les blocs `[manque]` sont extraits dans `hybride_niveau_XXXX_manques.txt` plutôt que
supprimés : ils portent la CAUSE d'une macro absente, et 84 d'entre eux n'existaient
nulle part ailleurs (dont 38 sur le niveau 20, cités par plan.md).

    python3 mesures/compresse_journaux.py            # rapport seul, n'écrit RIEN
    python3 mesures/compresse_journaux.py --ecrire   # applique

⚠️ TROIS PIÈGES, tous rencontrés en écrivant ce script :

  * UN JOURNAL CONTIENT PLUSIEURS PARTIES. Traiter le fichier d'un bloc fait
    enjamber une frontière de partie à un segment de marche, et le rejeu de
    validation continue sur le plateau de la partie précédente. **Tout se fait
    partie par partie.** C'est ce qui a fait échouer 14 fichiers sur 21 au
    premier jet — et la validation l'a vu, ce qui est bien l'intérêt de l'avoir.
  * LA VALIDATION EST UN REJEU SUR LE VRAI PLATEAU, pas une comparaison de
    lignes : chaque coup doit être légal (case adjacente, pas un mur, poussée
    possible) et la partie doit finir GAGNÉE. Une partie qui ne valide pas est
    laissée INTACTE, jamais réécrite au jugé.
  * LES NUMÉROS DE COUP CHANGENT. Les `hybride_niveau_XXXX_intentions.txt`
    ancrent leurs annotations sur `coup N/M` : après compression elles ne
    désignent plus les mêmes coups. Le script les marque en tête plutôt que de
    laisser un fichier faux sans le dire.

Rien ici ne dépend du solveur : c'est du traitement de texte validé par un rejeu.
"""
import glob
import os
import re
import sys

RE_MOUV = re.compile(r"\[mouv\] joueur \((\d+),(\d+)\)->\((\d+),(\d+)\)")
RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def charge(niv):
    """Murs, caisses, buts et joueur du plateau de départ, lus dans le .xsb."""
    murs, boxes, buts, pl = set(), set(), set(), None
    with open(os.path.join(RACINE, f"level{niv:04d}.xsb"), encoding="utf-8") as f:
        for y, r in enumerate(f.read().split("\n")):
            for x, ch in enumerate(r):
                if ch == "#":
                    murs.add((x, y))
                elif ch in "$*":
                    boxes.add((x, y))
                elif ch in "@+":
                    pl = (x, y)
                if ch in ".*+":
                    buts.add((x, y))
    return murs, boxes, buts, pl


def rejoue(niv, lignes):
    """Rejoue une partie. Rend (poussées, gagnée) ou None si un coup est ILLÉGAL."""
    murs, boxes, buts, pl = charge(niv)
    boxes = set(boxes)
    n = 0
    for l in lignes:
        g = RE_MOUV.match(l)
        if not g:
            continue
        a = (int(g.group(1)), int(g.group(2)))
        b = (int(g.group(3)), int(g.group(4)))
        if a != pl or abs(a[0] - b[0]) + abs(a[1] - b[1]) != 1 or b in murs:
            return None
        if b in boxes:                       # poussée : la case derrière doit être libre
            c = (2 * b[0] - a[0], 2 * b[1] - a[1])
            if c in murs or c in boxes:
                return None
            boxes.discard(b)
            boxes.add(c)
            n += 1
        pl = b
    return n, boxes == buts


def en_blocs(lignes):
    """Découpe en blocs clos par un [mouv] : un coup emporte les lignes qui le
    précèdent (son [hybride], son [macro]…), donc le retirer les retire aussi."""
    blocs, pending = [], []
    for l in lignes:
        pending.append(l)
        if RE_MOUV.match(l):
            blocs.append(pending)
            pending = []
    return blocs, pending


def applique_undo(lignes):
    """Passe 1. Extrait aussi les [manque], rendus à part."""
    blocs, pending, manques, entete = [], [], [], None
    for l in lignes:
        if l.startswith("=== niveau"):
            entete = l
        if l.startswith("[manque]"):
            if l.startswith("[manque] SIGNALE") or "aucun but actif" in l:
                manques.append(("H", entete))
            manques.append(("L", l))
            continue
        if l.startswith("[undo]"):
            if blocs:
                blocs.pop()
            pending = []                     # ce qui suivait le coup annulé part avec lui
            continue
        pending.append(l)
        if RE_MOUV.match(l):
            blocs.append(pending)
            pending = []
    return [x for b in blocs for x in b] + pending, manques


def retire_boucles(sess):
    """Passe 2, sur UNE partie. Entre deux poussées, ne garde qu'un passage par case."""
    blocs, queue = en_blocs(sess)
    est_pousse = ["POUSSE caisse" in b[-1] for b in blocs]
    garde = [True] * len(blocs)
    i = 0
    while i < len(blocs):
        if est_pousse[i]:
            i += 1
            continue
        j = i
        while j < len(blocs) and not est_pousse[j]:
            j += 1
        m = [RE_MOUV.match(blocs[k][-1]).groups() for k in range(i, j)]
        chemin = [(m[0][0], m[0][1])] + [(g[2], g[3]) for g in m]
        simple, idx, pos = [chemin[0]], [], {chemin[0]: 0}
        for k in range(1, len(chemin)):
            c = chemin[k]
            if c in pos:                     # boucle : on tronque au premier passage
                t = pos[c]
                for q in simple[t + 1:]:
                    del pos[q]
                simple, idx = simple[:t + 1], idx[:t]
            else:
                pos[c] = len(simple)
                simple.append(c)
                idx.append(i + k - 1)
        gardes = set(idx)
        for k in range(i, j):
            garde[k] = k in gardes
        i = j
    return [x for k, b in enumerate(blocs) if garde[k] for x in b] + queue


def parties(lignes):
    """Indices de découpe aux en-têtes `=== niveau`."""
    idx = [i for i, l in enumerate(lignes) if l.startswith("=== niveau")]
    return [(idx[k], idx[k + 1] if k + 1 < len(idx) else len(lignes)) for k in range(len(idx))]


def main():
    ecrire = "--ecrire" in sys.argv
    total_av = total_ap = 0
    sans_gagnee, non_validees = [], []

    for f in sorted(glob.glob(os.path.join(RACINE, "hybride_niveau_*.txt"))):
        if "_intentions" in f or "_manques" in f:
            continue
        niv = int(re.search(r"(\d{4})", os.path.basename(f)).group(1))
        with open(f, encoding="utf-8", errors="replace") as fh:
            src = fh.read().split("\n")

        sans_undo, manques = applique_undo(src)

        # Passe 2 puis 3, partie par partie. On ne garde que la DERNIÈRE gagnée.
        retenue = None
        for a, b in parties(sans_undo):
            sess = sans_undo[a:b]
            if not any("etat gagne" in l for l in sess):
                continue
            ref = rejoue(niv, sess)
            if ref is None or not ref[1]:
                non_validees.append(niv)
                continue
            nette = retire_boucles(sess)
            if rejoue(niv, nette) != ref:    # la compression a changé la partie : on garde l'originale
                non_validees.append(niv)
                nette = sess
            retenue = [x for x in nette if x.strip() != ""]

        av = sum(1 for x in src if RE_MOUV.match(x))
        if retenue is None:
            sans_gagnee.append(niv)
            continue
        ap = sum(1 for x in retenue if RE_MOUV.match(x))
        total_av += av
        total_ap += ap
        print(f"  niv {niv:>2} : {len(src):>6} -> {len(retenue):>6} lignes | {av:>5} -> {ap:>5} coups")

        if ecrire:
            with open(f, "w", encoding="utf-8") as fh:
                fh.write("\n".join(retenue) + "\n")
            if manques:
                out, vu = [], None
                for k, v in manques:
                    if k == "H":
                        if v != vu:
                            out += ["", v]
                            vu = v
                    else:
                        out.append(v)
                with open(f.replace(".txt", "_manques.txt"), "w", encoding="utf-8") as fh:
                    fh.write("\n".join(out) + "\n")

    print(f"\n  TOTAL {total_av} -> {total_ap} coups")
    print(f"  journaux sans partie gagnée (intacts) : {sans_gagnee or 'aucun'}")
    print(f"  parties non validées (laissées telles quelles) : {sorted(set(non_validees)) or 'aucune'}")

    if not ecrire:
        print("\n  RAPPORT SEUL — relancer avec --ecrire pour appliquer.")
        return

    # Les annotations ancrent leurs `coup N/M` : après compression elles pointent ailleurs.
    n = 0
    for f in sorted(glob.glob(os.path.join(RACINE, "hybride_niveau_*_intentions.txt"))):
        with open(f, encoding="utf-8") as fh:
            t = fh.read()
        if "ANCRES PERIMEES" in t:
            continue
        with open(f, "w", encoding="utf-8") as fh:
            fh.write(
                "### ⚠️ ANCRES PERIMEES — le journal a ete compresse, les numeros « coup N/M »\n"
                "### ci-dessous ne designent plus les memes coups. A REFAIRE.\n" + t)
        n += 1
    print(f"  {n} fichier(s) d'intentions marque(s) ANCRES PERIMEES")


if __name__ == "__main__":
    main()
