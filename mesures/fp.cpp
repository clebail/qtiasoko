// Harnais FP — le juge des tests de deadlock (§6.1).
//
//   fp <numNiveau> [variante] [astar|macro]      (défaut : variante 4, macro)
//
// La question, la seule qui compte pour un élagage : le test invente-t-il des
// morts ? On résout le niveau (avec LIVRAISON=0, donc SANS le test), puis on
// rejoue la solution coup par coup et on interroge butNonLivrable(variante) sur
// CHAQUE état traversé. Tous ces états sont solubles par construction — une
// solution y passe. Donc :
//
//        toute détection sur ce chemin est un FAUX POSITIF PROUVÉ.
//
// C'est ce que l'échantillonnage de `mort` ne pouvait pas voir : lui classait des
// états quelconques par sous-solve borné ; ici on part d'états dont la solubilité
// est certaine.
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <cstdio>
#include <cstdlib>
#include "level.h"
#include "game.h"
#include "solveur.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (argc < 2) { fprintf(stderr, "usage: fp <niveau> [variante] [astar|macro]\n"); return 2; }
    const int num      = QString(argv[1]).toInt();
    const int variante = (argc > 2) ? QString(argv[2]).toInt() : 4;
    const QString md   = (argc > 3) ? argv[3] : "macro";

    // Le solveur doit chercher SANS le test, sinon on jugerait le test avec
    // lui-même : c'est sa solution qu'on lui oppose.
    if (qgetenv("LIVRAISON") != "0") {
        fprintf(stderr, "fp: relancer avec LIVRAISON=0 (le solveur doit ignorer le test)\n");
        return 2;
    }

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game game(level, num);

    Solveur* s = Solveur::creer(md == "astar" ? Solveur::Astar : Solveur::AstarMacro, game);

    QObject::connect(s, &Solveur::solutionTrouvee,
                     [num, variante, game](QList<Game::EDirection> chemin, qint64) {
        Game g(game);
        int coup = 0, poussees = 0, faux = 0, premier = -1;

        if (g.butNonLivrable(variante)) { faux++; premier = 0; }   // l'état de départ !
        for (Game::EDirection d : chemin) {
            const int avant = g.getNbDepCaisse();
            g.deplace(d);
            coup++;
            if (g.getNbDepCaisse() == avant) continue;   // simple marche
            poussees++;
            if (g.butNonLivrable(variante)) {
                faux++;
                if (premier < 0) premier = coup;
            }
        }

        printf("niveau %d, variante %d : %d poussees sur le chemin gagnant, %s"
               " — %d FAUX POSITIFS%s\n",
               num, variante, poussees, g.isGagne() ? "rejeu OK" : "REJEU PERDU",
               faux, premier >= 0 ? QString(" (1er au coup %1)").arg(premier).toLocal8Bit().constData() : "");
        fflush(stdout);
        QCoreApplication::quit();
    });

    QObject::connect(s, &Solveur::aucuneSolution, [num]() {
        printf("niveau %d : AUCUNE solution — rien a juger\n", num);
        fflush(stdout);
        QCoreApplication::quit();
    });

    s->start();
    return app.exec();
}
