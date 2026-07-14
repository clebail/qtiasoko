// Carte des PASSAGES de caisse, pour un niveau ou pour la SOMME de plusieurs.
//
//   passages 17            -> carte du niveau 17 (les 6 caisses ensemble)
//   passages 120 121 ...   -> SOMME des cartes (chaque caisse resolue SEULE)
//
// Meme comptage que l'app (bouton « Export passages ») : une case vaut le nombre
// de fois qu'une caisse l'a OCCUPEE. La case de depart compte 1 (la caisse y est
// deja), et une caisse qui repasse incremente a nouveau.
//
// Comparer les deux repond a : les couloirs sont-ils une propriete du NIVEAU, ou
// naissent-ils de ce que les caisses se genent ?
//
// Verifie sur le niveau 17 : les couloirs sont IDENTIQUES (ecart nul), et tout
// l'ecart (12 = le mou exact) est concentre dans la zone de DEPART, ou les caisses
// se demelent. h est donc exacte sur les trajets ; le deficit est le demelage.
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <algorithm>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 3) {
        fprintf(stderr, "usage: passages <bfs|astar> <niveau> [niveau...]\n");
        return 2;
    }

    // BFS pour les sous-niveaux a 1 caisse : optimal en poussees, aucun doute sur
    // le trajet mesure.
    // A* optimal pour le niveau complet : le BFS n'y terminerait pas.
    const bool bfs = (QString(argv[1]) == "bfs");
    const Solveur::EType type = bfs ? Solveur::Bfs : Solveur::Astar;

    std::vector<int> cumul;
    int L = 0, H = 0;
    Game modele;
    int totalPoussees = 0;

    for (int a = 2; a < argc; a++) {
        const int num = QString(argv[a]).toInt();

        Level level;
        level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
        Game depart(level, num);

        if (cumul.empty()) {
            L = depart.getLargeur(); H = depart.getHauteur();
            cumul.assign((size_t)L * H, 0);
            modele = depart;
        }

        // La caisse occupe deja sa case de depart.
        for (int i = 0; i < L * H; i++)
            if (depart.getCase(i) == Level::tcCaisse || depart.getCase(i) == Level::tcGoalCaisse)
                cumul[i]++;

        Solveur* s = Solveur::creer(type, depart);
        int poussees = 0;
        QObject::connect(s, &Solveur::solutionTrouvee, s,
                         [&cumul, &poussees, depart, L](QList<Game::EDirection> coups, qint64) {
            Game g(depart);
            for (Game::EDirection d : coups) {
                const int avant = g.getNbDepCaisse();
                const QPoint pj = g.getPlayerPoint();
                g.deplace(d);
                if (g.getNbDepCaisse() == avant) continue;

                const QPoint np = g.getPlayerPoint();
                const QPoint caisse = np + (np - pj);   // la caisse est devant le joueur
                cumul[caisse.x() + caisse.y() * L]++;
                poussees++;
            }
        }, Qt::DirectConnection);
        s->start(); s->wait(); delete s;

        if (argc > 3) printf("  niveau %-4d : %3d poussees\n", num, poussees);
        totalPoussees += poussees;
    }

    printf("\ntotal : %d poussees\n\n", totalPoussees);

    int pic = 0, somme = 0;
    for (int i = 0; i < L * H; i++) { pic = std::max(pic, cumul[i]); somme += cumul[i]; }

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            const int i = x + y * L;
            if (modele.getCase(i) == Level::tcMur) { printf("###"); continue; }
            if (cumul[i] == 0)                     { printf("  ."); continue; }
            printf("%3d", cumul[i]);
        }
        printf("\n");
    }

    printf("\ntotal des passages : %d   maximum sur une case : %d\n", somme, pic);
    return 0;
}
