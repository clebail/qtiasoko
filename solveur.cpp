#include <QQueue>
#include <QSet>
#include <QtDebug>
#include "solveur.h"
#include "astar.h"

static const Game::SDirection directions[NB_DIRECTION] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};

Solveur::Solveur(const Game& etatDepart, QObject *parent) : QThread(parent), depart(etatDepart) {
    qRegisterMetaType<QList<Game::EDirection>>("QList<Game::EDirection>");
}

void Solveur::run() {
    QQueue<QPair<Game, int>> file;
    QSet<QByteArray> vus;
    qint64 compteur = 0;

    noeuds.clear();
    noeuds.append(Noeud{-1, {}});

    file.enqueue({depart, 0});
    vus.insert(depart.getEtat());

    while (file.size()) {
        auto [g, idx] = file.dequeue();
        compteur++;
        if (compteur % 1000 == 0) {
            qDebug() << "Solveur:" << compteur << "etats depiles, file =" << file.size() << ", vus =" << vus.size();
        }

        if(g.isGagne()) {
            qDebug() << "Solveur: solution trouvee apres" << compteur << "etats explores.";
            emit solutionTrouvee(reconstruire(idx), compteur);
            return;
        }

        QVector<bool> zone = g.getZoneJoueur();
        QVector<quint8> caisses = g.getCaissesDeplacable(zone);
        for(int i = 0; i < caisses.size(); i++) {
            quint8 dirPoussePossible = caisses[i];

            for (int d = 0; d < NB_DIRECTION; d++) {
                quint8 mask = 1 << d;
                if (dirPoussePossible & mask) {
                    Game e(g);
                    int x = (i % e.getLargeur()) + directions[d].dx;
                    int y = (i / e.getLargeur()) + directions[d].dy;
                    QPoint p(x, y);

                    QList<Game::EDirection> coups = AStar(&e).getChemin(e.getPlayerPoint(), p);
                    for (Game::EDirection dir : coups) {
                        e.deplace(dir);
                    }
                    e.deplace((Game::EDirection)d);
                    coups.append((Game::EDirection)d);

                    if (!e.isPerdu()) {
                        auto key = e.getEtat();

                        if (!vus.contains(key)) {
                            vus.insert(key);
                            noeuds.append(Noeud{idx, coups});
                            file.enqueue({e, noeuds.size() - 1});
                        }
                    }
                }
            }
        }
    }

    qDebug() << "Solveur: aucune solution," << compteur << "etats explores.";
    emit aucuneSolution();
}

QList<Game::EDirection> Solveur::reconstruire(int idx) {
    QList<Game::EDirection> chemin;

    while (idx != -1) {
        chemin = noeuds[idx].coups + chemin;
        idx = noeuds[idx].parent;
    }

    return chemin;
}
