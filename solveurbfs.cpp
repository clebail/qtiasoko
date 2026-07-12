#include <QQueue>
#include <QSet>
#include <QtDebug>
#include <utility>
#include "solveurbfs.h"

SolveurBFS::SolveurBFS(const Game& etatDepart, QObject* parent) : Solveur(etatDepart, parent) {
}

void SolveurBFS::run() {
    QQueue<QPair<Game, int>> file;
    QSet<QByteArray> vus;
    qint64 compteur = 0;

    noeuds.clear();
    noeuds.append(Noeud{-1, -1, Game::dHaut});   // racine : aucune poussée ne la précède

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
                    // Poussée par téléportation : aucun trajet de marche calculé
                    // ici. Il ne servirait qu'à l'affichage, et la plupart de ces
                    // enfants vont être jetés comme doublons. reconstruire() s'en
                    // charge, une seule fois, sur la solution retenue.
                    Game e(g);
                    e.pousse(i, (Game::EDirection)d);

                    if (!e.isPerdu()) {
                        auto key = e.getEtat();

                        if (!vus.contains(key)) {
                            vus.insert(key);
                            noeuds.append(Noeud{idx, i, (Game::EDirection)d});
                            file.enqueue({std::move(e), noeuds.size() - 1});
                        }
                    }
                }
            }
        }
    }

    qDebug() << "SolveurBFS: aucune solution," << compteur << "etats explores.";
    emit aucuneSolution();
}
