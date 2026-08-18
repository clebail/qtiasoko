// Harnais FP — le juge des tests de deadlock (§6.1).
//
//   fp <numNiveau> [variante] [astar|macro]      (défaut : variante -1, macro)
//
// La question, la seule qui compte pour un élagage : le test invente-t-il des
// morts ? On résout le niveau, puis on rejoue la solution coup par coup et on
// interroge le test sur CHAQUE état traversé. Tous ces états sont solubles par
// construction — une solution y passe. Donc :
//
//        toute détection sur ce chemin est un FAUX POSITIF PROUVÉ.
//
// C'est ce que l'échantillonnage de `mort` ne pouvait pas voir : lui classait des
// états quelconques par sous-solve borné ; ici on part d'états dont la solubilité
// est certaine.
//
// ⚠️ Le test « but orphelin » (variantes >= 0, `Game::butNonLivrable`) a été
// retiré entièrement le 2026-08-18 : c'est LUI qui avait produit son propre
// verdict, via cet outil — cinq variantes en faux positif prouvé, la sixième
// sans capture. Seuls les tests négatifs restent (corral, gate, précédence).
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <cstdio>
#include <cstdlib>
#include "level.h"
#include "game.h"
#include "solveur.h"
#include "precedencepaires.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (argc < 2) { fprintf(stderr, "usage: fp <niveau> [-1=corral|-2=gate|-3=precedence] [astar|macro]\n"); return 2; }
    const int num      = QString(argv[1]).toInt();
    const int variante = (argc > 2) ? QString(argv[2]).toInt() : -1;
    const QString md   = (argc > 3) ? argv[3] : "macro";

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game game(level, num);

    Solveur* s = Solveur::creer(md == "astar" ? Solveur::Astar : Solveur::AstarMacro, game);

    QObject::connect(s, &Solveur::solutionTrouvee,
                     [num, variante, game](QList<Game::EDirection> chemin, qint64) {
        Game g(game);
        int coup = 0, poussees = 0, faux = 0, premier = -1;

        // Même verdict binaire pour les trois tests : toute détection sur un
        // chemin gagnant est un faux positif prouvé.
        auto detecte = [variante](const Game& e) {
            // -3 : PRÉCÉDENCE PAR PAIRES (mesures/precedencepaires.h). Objet de
            // diagnostic, pas encore un élagage — c'est précisément ce juge qui doit
            // dire s'il pourrait le devenir un jour.
            if (variante == -3) return PrecedencePaires::violations(e) > 0;
            if (variante == -2) return e.gateEnclosMort();     // gate corral-N (item B)
            return e.corralUnitaireMort();                     // corral unitaire + pince (défaut)
        };

        if (detecte(g)) { faux++; premier = 0; }   // l'état de départ !
        for (Game::EDirection d : chemin) {
            const int avant = g.getNbDepCaisse();
            g.deplace(d);
            coup++;
            if (g.getNbDepCaisse() == avant) continue;   // simple marche
            poussees++;
            if (detecte(g)) {
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
