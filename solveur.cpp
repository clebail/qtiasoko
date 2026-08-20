#include <QPoint>
#include "solveur.h"
#include "solveurbfs.h"
#include "solveurastar.h"
#include "astar.h"

const Game::SDirection Solveur::appuis[NB_DIRECTION] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};

Solveur::Solveur(const Game& etatDepart, QObject* parent) : QThread(parent), depart(etatDepart) {
    qRegisterMetaType<QList<Game::EDirection>>("QList<Game::EDirection>");
}

QVector<Solveur::SType> Solveur::types() {
    return {
        {Bfs, "BFS (optimal)"},
        {Astar, "A* (optimal)"},
        {AstarPondere, "A* pondéré (rapide, approché)"},
        {AstarMacro, "A* macro (rapide)"},
        {AstarMacroCouplage, "A* macro — but du couplage"},
        {AstarMacroPlongeon, "A* macro — plongeon sur record (essai)"},
        {AstarMacroCouplagePlongeon, "A* macro — couplage + plongeon (essai)"},
        {AstarMacroCouplagePlongeonOrdre, "A* macro — couplage + plongeon + ordre dynamique (essai)"},
        {AstarMacroCouplagePlongeonLook, "A* macro — couplage + plongeon + ordre lookahead rang 0 (essai)"},
        {AstarMacroCouplagePlongeonLoi, "A* macro — couplage + plongeon + loi de l'ordre, isolee (essai)"},
        {AstarMacroCouplagePlongeonRelegue, "A* macro — couplage + plongeon + poussees simples RELEGUEES (essai)"},
        {AstarMacroCouplagePlongeonLoiRelegue, "A* macro — couplage + plongeon + loi + poussees simples RELEGUEES (essai)"}
    };
}

int Solveur::penaliteRelegation = 2;

Solveur* Solveur::creer(EType type, const Game& etatDepart, QObject* parent) {
    switch (type) {
        case Bfs:          return new SolveurBFS(etatDepart, parent);
        case Astar:        return new SolveurAStar(etatDepart, 1, false, parent);
        // w=2 mesuré comme le meilleur compromis : w=3 et w=5 explorent PLUS
        // d'états que w=2 (une h trop gonflée fait perdre le fil au lieu de guider).
        case AstarPondere: return new SolveurAStar(etatDepart, 2, false, parent);
        // Goal macro (§10.5) : optimal sur les petits niveaux, approché sur les gros
        // congestionnés (le trajet solo peut y différer du réel), mais résout tout.
        case AstarMacro:   return new SolveurAStar(etatDepart, 1, true, parent);
        // Régime d'essai (§6.3, 2026-07-24) : même solveur, la macro vise d'abord
        // la caisse que le couplage assigne au but actif.
        case AstarMacroCouplage: return new SolveurAStar(etatDepart, 1, true, parent, true);
        // Régime d'essai (§6.0, 2026-07-28) : A* macro + PLONGEON dès qu'un état
        // bat le record de caisses posées et dépasse le seuil de remplissage.
        case AstarMacroPlongeon: return new SolveurAStar(etatDepart, 1, true, parent, false, true);
        case AstarMacroCouplagePlongeon: return new SolveurAStar(etatDepart, 1, true, parent, true, true);
        // ORDRE DYNAMIQUE (§6.2, 2026-07-31) : le flag vit dans le Game, pas dans le
        // solveur — posé ici sur l'état de départ, il se propage par copie à toute la
        // recherche (et au plongeon, qui copie les mêmes états).
        case AstarMacroCouplagePlongeonOrdre: {
            Game depart(etatDepart);
            depart.setOrdreDynamique(true);
            return new SolveurAStar(depart, 1, true, parent, true, true);
        }
        // ORDRE-LOOK (§6.2, 2026-08-08) : le drapeau vit dans le Game, comme
        // `ordreDynamique` — posé ici sur l'état de départ, il RECALCULE `ordreButs`
        // et se propage ensuite par copie à toute la recherche.
        case AstarMacroCouplagePlongeonLook: {
            Game depart(etatDepart);
            depart.setOrdreLookahead(true);
            return new SolveurAStar(depart, 1, true, parent, true, true);
        }
        // LOI DE L'ORDRE (§6.2, 2026-08-03 — restaurée ISOLÉE le 2026-08-19, cf.
        // solveur.h) : une caisse ne peut pas se tenir sur une case morte vue du
        // but actif. Le PRUNE (caseMorteLoi) vit dans le solveur, comme avant ;
        // l'ORDRE, lui, doit être armé sur l'état de départ (`setOrdreAlignement`,
        // game.h) — sans lui, `ordreButs` reste celui par défaut, qui met (2,5)
        // avant (1,5) sur le niveau 6 et que `caseMorteLoi` condamne alors à
        // coup sûr (`AUCUNE`, mesuré le 2026-08-19).
        case AstarMacroCouplagePlongeonLoi: {
            Game depart(etatDepart);
            depart.setOrdreAlignement(true);
            return new SolveurAStar(depart, 1, true, parent, true, true, true);
        }
        // RELÉGATION (§6.4 forme (a) : dé-prioriser, jamais élaguer). La pénalité
        // est un BUDGET À BALAYER et non une constante à figer (méthode
        // CORRAL_BUDGET, §6.2) : le harnais la fixe par RELEG_F, le solveur ne fait
        // que la recevoir. ⚠️ Elle n'est PAS lue ici par qgetenv — un interrupteur
        // d'environnement DANS le solveur fait diverger l'app du bench en silence
        // (§7, le cas CORRAL_DETECT). Défaut 2 = un recul de retard, le mou étant
        // toujours pair (§3).
        case AstarMacroCouplagePlongeonRelegue:
            return new SolveurAStar(etatDepart, 1, true, parent, true, true, false, penaliteRelegation);
        case AstarMacroCouplagePlongeonLoiRelegue: {
            Game depart(etatDepart);
            depart.setOrdreAlignement(true);
            return new SolveurAStar(depart, 1, true, parent, true, true, true, penaliteRelegation);
        }
    }
    return nullptr;
}

// Le solveur ne raisonne qu'en poussées ; c'est ici, une seule fois, qu'on
// redescend au niveau des coups pour l'UI. On remonte jusqu'à la racine pour
// retrouver la suite de poussées, puis on la REJOUE depuis l'état de départ :
// chaque poussée n'est jouable qu'à sa place dans la séquence, et le trajet de
// marche qui y mène dépend de la position des caisses à cet instant précis.
QList<Game::EDirection> Solveur::reconstruire(int idx) {
    QList<quint32> chaine;
    // ⚠️ Le sentinelle de racine est ArbreNoeuds::RACINE, plus -1 : les index de
    // noeuds sont non signés depuis le passage en tableaux parallèles (§6.5).
    for (quint32 i = (quint32)idx; i != ArbreNoeuds::RACINE; i = noeuds.parent(i)) {
        chaine.prepend(i);
    }

    QList<Game::EDirection> chemin;
    Game g(depart);

    for (quint32 i : chaine) {
        if (noeuds.parent(i) == ArbreNoeuds::RACINE) continue;   // la racine n'est précédée d'aucune poussée

        const Game::EDirection dir = (Game::EDirection)noeuds.dir(i);
        const quint16 idxCaisse = noeuds.idxCaisse(i);

        const QPoint appui(idxCaisse % g.getLargeur() + appuis[dir].dx,
                           idxCaisse / g.getLargeur() + appuis[dir].dy);

        chemin += AStar(&g).getChemin(g.getPlayerPoint(), appui);

        g.pousse(idxCaisse, dir);
        chemin.append(dir);
    }

    return chemin;
}
