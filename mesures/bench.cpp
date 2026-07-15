// Harnais headless : résout un niveau et imprime le canari.
//
//   bench <numNiveau> [astar|macro|pondere|bfs]   (défaut : astar optimal)
//
// Sortie : "OK <niveau> etats=<n> poussees=<p>" — les deux chiffres qui doivent
// rester identiques à l'octet près après l'étape 11 (adressage ouvert).
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <cstdio>
#include "level.h"
#include "game.h"
#include "solveur.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (argc < 2) { fprintf(stderr, "usage: bench <niveau> [astar|macro|pondere|bfs]\n"); return 2; }
    const int num    = QString(argv[1]).toInt();
    const QString md = (argc > 2) ? argv[2] : "astar";
    const Solveur::EType type =
        (md == "macro")   ? Solveur::AstarMacro   :
        (md == "pondere" || md == "2") ? Solveur::AstarPondere :
        (md == "bfs")     ? Solveur::Bfs          : Solveur::Astar;

    Level level;
    level.load(QString("%1/level%2.xsb")
               .arg(LEVELS_DIR)
               .arg(num, 4, 10, QChar('0')));

    Game game(level, num);

    Solveur* s = Solveur::creer(type, game);

    QObject::connect(s, &Solveur::solutionTrouvee,
                     [num, game](QList<Game::EDirection> chemin, qint64 etats) {
        // Le signal ne porte le chemin qu'en COUPS (marche + poussées mêlées).
        // Le canari, lui, est en POUSSÉES : on rejoue la solution coup par coup
        // sur une copie de l'état de départ. Ça donne le compte exact — et ça
        // vérifie du même geste que le chemin reconstruit gagne réellement la
        // partie, ce qu'aucun compteur d'états ne dirait.
        Game g(game);
        for (Game::EDirection d : chemin) g.deplace(d);

        printf("%s %d etats=%lld poussees=%d coups=%d\n",
               g.isGagne() ? "OK" : "PERDU-REJEU",
               num, (long long)etats, g.getNbDepCaisse(), (int)chemin.size());
        fflush(stdout);
        QCoreApplication::quit();
    });

    QObject::connect(s, &Solveur::aucuneSolution, [num]() {
        printf("AUCUNE %d\n", num);
        fflush(stdout);
        QCoreApplication::quit();
    });

    s->start();
    return app.exec();
}
