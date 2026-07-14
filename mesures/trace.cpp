// Affiche le cheminement de la solution optimale, poussee par poussee.
//
//   trace <niveau> [pasAffichage]
//
// Pour chaque poussee : la grille, quelle caisse a bouge, et si la poussee est
// PRODUCTIVE (h diminue : la caisse se rapproche d'un but) ou NON PRODUCTIVE
// (h stagne ou monte : on deplace une caisse pour degager le passage — c'est le
// « cout de manoeuvre » que h ne voit pas).
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

static void dessine(const Game& g, int largeur, int hauteur) {
    for (int y = 0; y < hauteur; y++) {
        printf("  ");
        for (int x = 0; x < largeur; x++) {
            switch (g.getCase(x + y * largeur)) {
                case Level::tcMur:        printf("#"); break;
                case Level::tcCaisse:     printf("$"); break;
                case Level::tcGoalCaisse: printf("*"); break;
                case Level::tcGoal:       printf("."); break;
                case Level::tcPlayer:     printf("@"); break;
                case Level::tcGoalPlayer: printf("+"); break;
                default:                  printf(" "); break;
            }
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: trace <niveau> [pas]\n"); return 2; }
    const int num = QString(argv[1]).toInt();
    const int pas = (argc > 2) ? QString(argv[2]).toInt() : 1;

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game depart(level, num);

    Solveur* s = Solveur::creer(Solveur::Astar, depart);

    QObject::connect(s, &Solveur::solutionTrouvee, s,
                     [depart, num, pas](QList<Game::EDirection> coups, qint64) {
        const int L = depart.getLargeur(), H = depart.getHauteur();

        Game g(depart);
        int poussee = 0;
        int hPrec = g.getHeuristique();

        printf("===== NIVEAU %d — depart, h = %d =====\n", num, hPrec);
        dessine(g, L, H);

        int nonProductives = 0;

        for (Game::EDirection d : coups) {
            const int avant = g.getNbDepCaisse();
            g.deplace(d);
            if (g.getNbDepCaisse() == avant) continue;   // simple marche

            poussee++;
            const int h = g.getHeuristique();
            const int gain = hPrec - h;          // 1 = productive, <=0 = manoeuvre
            if (gain <= 0) nonProductives++;

            if (poussee % pas == 0 || gain <= 0) {
                printf("\n-- poussee %3d   h = %3d  (%+d) %s\n",
                       poussee, h, -gain,
                       gain <= 0 ? "  <<<< NON PRODUCTIVE (manoeuvre)" : "");
                dessine(g, L, H);
            }
            hPrec = h;
        }

        printf("\n===== BILAN niveau %d =====\n", num);
        printf("  poussees          : %d\n", poussee);
        printf("  non productives   : %d  (%.0f %%)\n",
               nonProductives, 100.0 * nonProductives / poussee);
        printf("  gagne             : %s\n", g.isGagne() ? "OUI" : "NON");
        fflush(stdout);
        QCoreApplication::quit();
    });

    s->start();
    return app.exec();
}
