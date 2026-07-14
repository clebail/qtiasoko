// Anatomie des poussees NON PRODUCTIVES : quelle caisse est ecartee, et pourquoi ?
//
//   congestion <niveau> <solo0> <solo1> ...
//   ex: congestion 17 120 121 122 123 124 125
//
// On sait (§9.7) que h = somme des trajets des caisses resolues SEULES, et que le
// mou vaut exactement 2 x (nombre de poussees non productives). Une poussee est
// NON PRODUCTIVE quand elle ne fait pas descendre h — elle eloigne CETTE caisse de
// SON but.
//
// ⚠️ Le critere est PAR CAISSE, pas par case : une caisse ecartee atterrit souvent
// sur le trajet d'une VOISINE, donc sans jamais sortir du « reseau » (l'union des
// trajets). Une carte de trafic agregee ne peut pas le voir — elle a perdu
// l'identite des caisses. (Mesure : niveau 17, 12 de mou pour seulement 2 passages
// hors reseau.)
//
// Pour chaque poussee non productive, on imprime :
//   - QUELLE caisse bouge (identite suivie depuis le depart)
//   - si elle QUITTE son propre trajet solo
//   - QUELLES autres caisses ont leur trajet solo qui passe par la case liberee
//     -> celles qu'on est peut-etre en train de debloquer.
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <QSet>
#include <algorithm>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

// Rejoue une solution et rend la suite des cases occupees par LA caisse (niveaux
// a une seule caisse).
static QSet<int> trajetSolo(int num, int L) {
    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game depart(level, num);

    QSet<int> cases;
    for (int i = 0; i < depart.getLargeur() * depart.getHauteur(); i++)
        if (depart.getCase(i) == Level::tcCaisse || depart.getCase(i) == Level::tcGoalCaisse)
            cases.insert(i);

    Solveur* s = Solveur::creer(Solveur::Bfs, depart);   // BFS : optimal, sans macro
    QObject::connect(s, &Solveur::solutionTrouvee, s,
                     [&cases, depart, L](QList<Game::EDirection> coups, qint64) {
        Game g(depart);
        for (Game::EDirection d : coups) {
            const int avant = g.getNbDepCaisse();
            const QPoint pj = g.getPlayerPoint();
            g.deplace(d);
            if (g.getNbDepCaisse() == avant) continue;
            const QPoint np = g.getPlayerPoint();
            const QPoint c  = np + (np - pj);
            cases.insert(c.x() + c.y() * L);
        }
    }, Qt::DirectConnection);
    s->start(); s->wait(); delete s;
    return cases;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 3) { fprintf(stderr, "usage: congestion <niveau> <solo...>\n"); return 2; }

    const int num = QString(argv[1]).toInt();

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game depart(level, num);
    const int L = depart.getLargeur(), H = depart.getHauteur();

    // Trajets solos, dans l'ordre de balayage (meme convention que derive.py).
    std::vector<QSet<int>> trajets;
    for (int a = 2; a < argc; a++)
        trajets.push_back(trajetSolo(QString(argv[a]).toInt(), L));

    // Identite des caisses : ordre de balayage au depart.
    std::vector<int> pos;   // pos[k] = case de la caisse k
    for (int i = 0; i < L * H; i++)
        if (depart.getCase(i) == Level::tcCaisse || depart.getCase(i) == Level::tcGoalCaisse)
            pos.push_back(i);

    const int n = (int)pos.size();
    if ((int)trajets.size() != n) {
        fprintf(stderr, "!! %d caisses mais %d trajets solos\n", n, (int)trajets.size());
        return 1;
    }

    printf("===== NIVEAU %d : %d caisses =====\n", num, n);
    printf("h(depart) = %d\n\n", depart.getHeuristique());

    Solveur* s = Solveur::creer(Solveur::Astar, depart);   // A* OPTIMAL

    QObject::connect(s, &Solveur::solutionTrouvee, s,
                     [&](QList<Game::EDirection> coups, qint64) {
        Game g(depart);
        std::vector<int> p = pos;
        int hPrec = g.getHeuristique();
        int poussee = 0, nonProd = 0;

        for (Game::EDirection d : coups) {
            const int avant = g.getNbDepCaisse();
            const QPoint pj = g.getPlayerPoint();
            g.deplace(d);
            if (g.getNbDepCaisse() == avant) continue;

            poussee++;
            const QPoint np = g.getPlayerPoint();
            const QPoint pc = np + (np - pj);
            const int depuis = pj.x() + (np.x() - pj.x()) + (pj.y() + (np.y() - pj.y())) * L;
            const int vers   = pc.x() + pc.y() * L;

            // Quelle caisse ? Celle qui etait sur 'depuis'.
            int k = -1;
            for (int i = 0; i < n; i++) if (p[i] == depuis) { k = i; break; }
            if (k < 0) { printf("!! caisse introuvable a la poussee %d\n", poussee); continue; }
            p[k] = vers;

            const int h = g.getHeuristique();
            const int gain = hPrec - h;
            hPrec = h;

            if (gain > 0) continue;      // productive : h a baisse

            nonProd++;
            printf("-- poussee %3d : NON PRODUCTIVE (h %+d)\n", poussee, -gain);
            printf("   caisse %d : (%d,%d) -> (%d,%d)\n", k,
                   depuis % L, depuis / L, vers % L, vers / L);
            printf("   quitte son propre trajet solo ? %s\n",
                   trajets[k].contains(vers) ? "NON (elle y reste)" : "OUI");

            // Qui passe par la case qu'elle vient de LIBERER ?
            QString bloquees;
            for (int i = 0; i < n; i++) {
                if (i == k) continue;
                if (trajets[i].contains(depuis))
                    bloquees += QString("%1 ").arg(i);
            }
            printf("   la case liberee (%d,%d) est sur le trajet des caisses : %s\n",
                   depuis % L, depuis / L,
                   bloquees.isEmpty() ? "(aucune)" : qPrintable(bloquees));

            // Et la case ou elle atterrit : sur le trajet de qui ?
            QString sur;
            for (int i = 0; i < n; i++) {
                if (i == k) continue;
                if (trajets[i].contains(vers)) sur += QString("%1 ").arg(i);
            }
            printf("   elle atterrit sur le trajet des caisses         : %s\n\n",
                   sur.isEmpty() ? "(aucune — hors reseau)" : qPrintable(sur));
        }

        printf("=== %d poussees, dont %d NON PRODUCTIVES  ->  mou attendu = %d ===\n",
               poussee, nonProd, 2 * nonProd);
        fflush(stdout);
        QCoreApplication::quit();
    });

    s->start();
    return app.exec();
}
