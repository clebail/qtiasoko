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
        {AstarMacroCouplagePlongeonCoins, "A* macro — couplage + plongeon + buts en coin dans l'ordre (essai)"},
        {AstarMacroCouplagePlongeonLoi, "A* macro — couplage + plongeon + loi de l'ordre (essai)"},
        {AstarMacroCouplagePlongeonOrdreLoi, "A* macro — couplage + plongeon + ordre dynamique + loi (essai)"}
    };
}

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
        // BUTS EN COIN DANS L'ORDRE (§6.2, 2026-08-03) : une caisse ne peut pas se
        // poser sur un but en coin dont le rang dépasse l'actif. Régime d'ESSAI :
        // la règle repose sur la justesse de l'ordre, pas sur la géométrie.
        case AstarMacroCouplagePlongeonCoins:
            return new SolveurAStar(etatDepart, 1, true, parent, true, true, true);
        // LOI DE L'ORDRE (§6.2, 2026-08-03) : une caisse ne peut pas se tenir sur une
        // case morte vue du BUT ACTIF. Régime d'ESSAI, même raison que ci-dessus —
        // la règle repose sur la justesse de l'ordre, pas sur la géométrie.
        case AstarMacroCouplagePlongeonLoi:
            return new SolveurAStar(etatDepart, 1, true, parent, true, true, false, true);
        // LES DEUX MOITIÉS ENSEMBLE (§6.2, 2026-08-04) : l'ordre dynamique porte la
        // contrainte de PORTE (dans butActif), le drapeau porte la LOI et le GEL (à
        // l'enfilage). Aucun run ne les avait jamais eues toutes les deux.
        case AstarMacroCouplagePlongeonOrdreLoi: {
            Game depart(etatDepart);
            depart.setOrdreDynamique(true);
            return new SolveurAStar(depart, 1, true, parent, true, true, false, true);
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
    QList<int> chaine;
    for (int i = idx; i != -1; i = noeuds[i].parent) {
        chaine.prepend(i);
    }

    QList<Game::EDirection> chemin;
    Game g(depart);

    for (int i : chaine) {
        const Noeud& n = noeuds[i];
        if (n.parent == -1) continue;   // la racine n'est précédée d'aucune poussée

        const Game::EDirection dir = (Game::EDirection)n.dir;

        const QPoint appui(n.idxCaisse % g.getLargeur() + appuis[dir].dx,
                           n.idxCaisse / g.getLargeur() + appuis[dir].dy);

        chemin += AStar(&g).getChemin(g.getPlayerPoint(), appui);

        g.pousse(n.idxCaisse, dir);
        chemin.append(dir);
    }

    return chemin;
}
