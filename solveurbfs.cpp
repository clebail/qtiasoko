#include <QQueue>
#include <QSet>
#include <QtDebug>
#include "solveurbfs.h"
#include "astar.h"

SolveurBFS::SolveurBFS(const Game& etatDepart, QObject* parent) : Solveur(etatDepart, parent) {
}

void SolveurBFS::run() {
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
            qDebug() << "SolveurBFS:" << compteur << "etats depiles, file =" << file.size() << ", vus =" << vus.size();
        }

        if(g.isGagne()) {
            qDebug() << "SolveurBFS: solution trouvee apres" << compteur << "etats explores.";
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

    qDebug() << "SolveurBFS: aucune solution," << compteur << "etats explores.";
    emit aucuneSolution();
}
