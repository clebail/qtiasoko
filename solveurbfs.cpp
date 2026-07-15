#include <QQueue>
#include <QtDebug>
#include <unordered_set>
#include <utility>
#include "cle.h"
#include "solveurbfs.h"

SolveurBFS::SolveurBFS(const Game& etatDepart, QObject* parent) : Solveur(etatDepart, parent) {
}

void SolveurBFS::run() {
    QQueue<QPair<Game, int>> file;
    qint64 compteur = 0;

    // Clés rangées bout à bout dans l'arène, l'ensemble des vus n'en portant que
    // des références de 4 octets (cf. cle.h). C'était son poste mémoire : un
    // malloc et un en-tête QArrayData par QByteArray, pour 22 o utiles.
    Arene arene(depart.tailleCle());
    std::unordered_set<Cle,CleHash,CleEq> vus(1024, CleHash{&arene}, CleEq{&arene});

    noeuds.clear();
    noeuds.append(Noeud{-1, 0, 0});   // racine : aucune poussée ne la précède (idxCaisse/dir jamais lus)

    file.enqueue({depart, 0});
    depart.getEtat(arene.reserve());
    vus.insert(Cle{arene.dernier()});

    int maxRangees = 0;
    while (file.size()) {
        auto [g, idx] = file.dequeue();
        compteur++;
        if (compteur % 1000 == 0) {
            qDebug() << "SolveurBFS:" << compteur << "etats depiles, file =" << file.size() << ", vus =" << vus.size();
        }

        const int rangees = g.nbCaissesSurBut();
        if (rangees > maxRangees) {
            maxRangees = rangees;
            emit nouveauMaxCaisses(g, rangees);   // diagnostic état-max (§10)
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
                        // Clé écrite directement en fin d'arène. insert() dit s'il
                        // s'agit d'un nouvel état ; si non, on reprend la clé —
                        // elle est déjà dans l'arène, sous son offset d'origine.
                        e.getEtat(arene.reserve());

                        if (vus.insert(Cle{arene.dernier()}).second) {
                            noeuds.append(Noeud{idx, (quint16)i, (quint8)d});
                            file.enqueue({std::move(e), noeuds.size() - 1});
                        } else {
                            arene.annule();
                        }
                    }
                }
            }
        }
    }

    qDebug() << "SolveurBFS: aucune solution," << compteur << "etats explores.";
    emit aucuneSolution();
}
