// Potentiel des MACRO-POUSSEES (fusion des poussees en couloir).
//
//   tunnels <niveau>
//
// Une caisse poussee dans la direction d atterrit sur la case c. Si c est un
// COULOIR selon d — les deux cases perpendiculaires a d sont des murs — alors la
// caisse ne peut plus etre poussee que le long de l'axe d : le joueur ne peut pas
// la prendre de cote, il n'y a pas la place. Et si c n'est pas un but, s'y
// ARRETER n'apporte rien : aucun chemin optimal n'a de raison de laisser une
// caisse au milieu d'un couloir.
//
// De tels etats intermediaires n'existent donc QUE pour permettre des
// entrelacements : « j'avance la caisse A de 3 cases, je vais bouger B, je
// reviens finir A ». Ils sont tous distincts (les caisses ne sont pas aux memes
// endroits) et tous optimaux — la deduplication ne les attrape pas. C'est
// exactement la multiplicite combinatoire qui fait exploser A*.
//
// Fusionner ces poussees en une seule arete (macro-poussee) les supprime SANS
// perdre aucune solution.
//
// On mesure ici, le long de la solution OPTIMALE, la part des poussees qui
// atterrissent dans un tel couloir. C'est le potentiel, avant d'ecrire une ligne.
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: tunnels <niveau>\n"); return 2; }
    const int num = QString(argv[1]).toInt();

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game depart(level, num);

    Solveur* s = Solveur::creer(Solveur::Astar, depart);

    QObject::connect(s, &Solveur::solutionTrouvee, s,
                     [depart, num](QList<Game::EDirection> coups, qint64 etats) {
        const int L = depart.getLargeur(), H = depart.getHauteur();

        auto estMur = [&](int x, int y) {
            if (x < 0 || y < 0 || x >= L || y >= H) return true;
            return depart.getCase(x + y * L) == Level::tcMur;
        };
        auto estBut = [&](int x, int y) {
            const Level::ETypeCase c = depart.getCase(x + y * L);
            return c == Level::tcGoal || c == Level::tcGoalCaisse || c == Level::tcGoalPlayer;
        };

        // Combien de cases du niveau sont des couloirs ? (potentiel structurel)
        int libres = 0, couloirs = 0;
        for (int y = 0; y < H; y++)
            for (int x = 0; x < L; x++) {
                if (estMur(x, y)) continue;
                libres++;
                const bool tunnelH = estMur(x, y - 1) && estMur(x, y + 1);   // couloir horizontal
                const bool tunnelV = estMur(x - 1, y) && estMur(x + 1, y);   // couloir vertical
                if ((tunnelH || tunnelV) && !estBut(x, y)) couloirs++;
            }

        // Le long de la solution : chaque poussee atterrit-elle dans un couloir
        // selon SA direction ?
        Game g(depart);
        int poussees = 0, fusionnables = 0;

        for (Game::EDirection d : coups) {
            const int avant = g.getNbDepCaisse();
            const QPoint pj = g.getPlayerPoint();
            g.deplace(d);
            if (g.getNbDepCaisse() == avant) continue;

            poussees++;

            // Le joueur a avance d'une case : la caisse est juste devant lui.
            const QPoint np = g.getPlayerPoint();
            const int dx = np.x() - pj.x(), dy = np.y() - pj.y();
            const int cx = np.x() + dx, cy = np.y() + dy;   // ou la caisse a atterri

            // Couloir SELON LA DIRECTION de poussee : les cotes perpendiculaires
            // sont des murs. La caisse ne peut plus qu'avancer ou reculer.
            const bool enCouloir = (dx != 0) ? (estMur(cx, cy - 1) && estMur(cx, cy + 1))
                                             : (estMur(cx - 1, cy) && estMur(cx + 1, cy));

            // Un but est une destination legitime : s'y arreter a un sens.
            if (enCouloir && !estBut(cx, cy)) fusionnables++;
        }

        printf("===== NIVEAU %2d =====\n", num);
        printf("  cases libres            : %d   dont couloirs (hors buts) : %d  (%.0f %%)\n",
               libres, couloirs, 100.0 * couloirs / libres);
        printf("  poussees (C*)           : %d   etats developpes : %lld\n",
               poussees, (long long)etats);
        printf("  poussees FUSIONNABLES   : %d  (%.0f %%)  <- etats intermediaires supprimables\n",
               fusionnables, 100.0 * fusionnables / poussees);
        printf("  gagne : %s\n", g.isGagne() ? "OUI" : "NON");
        fflush(stdout);
        QCoreApplication::quit();
    });

    s->start();
    return app.exec();
}
