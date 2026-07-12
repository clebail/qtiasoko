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
        {AstarPondere, "A* pondéré (rapide, approché)"}
    };
}

Solveur* Solveur::creer(EType type, const Game& etatDepart, QObject* parent) {
    switch (type) {
        case Bfs:          return new SolveurBFS(etatDepart, parent);
        case Astar:        return new SolveurAStar(etatDepart, 1, parent);
        // w=2 mesuré comme le meilleur compromis : w=3 et w=5 explorent PLUS
        // d'états que w=2 (une h trop gonflée fait perdre le fil au lieu de guider).
        case AstarPondere: return new SolveurAStar(etatDepart, 2, parent);
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

        const QPoint appui(n.idxCaisse % g.getLargeur() + appuis[n.dir].dx,
                           n.idxCaisse / g.getLargeur() + appuis[n.dir].dy);

        chemin += AStar(&g).getChemin(g.getPlayerPoint(), appui);
        chemin.append(n.dir);

        g.pousse(n.idxCaisse, n.dir);
    }

    return chemin;
}
