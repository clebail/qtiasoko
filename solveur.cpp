#include "solveur.h"
#include "solveurbfs.h"

const Game::SDirection Solveur::directions[NB_DIRECTION] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};

Solveur::Solveur(const Game& etatDepart, QObject* parent) : QThread(parent), depart(etatDepart) {
    qRegisterMetaType<QList<Game::EDirection>>("QList<Game::EDirection>");
}

QVector<Solveur::SType> Solveur::types() {
    return {
        {Bfs, "BFS"}
    };
}

Solveur* Solveur::creer(EType type, const Game& etatDepart, QObject* parent) {
    switch (type) {
        case Bfs: return new SolveurBFS(etatDepart, parent);
    }
    return nullptr;
}

QList<Game::EDirection> Solveur::reconstruire(int idx) {
    QList<Game::EDirection> chemin;

    while (idx != -1) {
        chemin = noeuds[idx].coups + chemin;
        idx = noeuds[idx].parent;
    }

    return chemin;
}
