// Carte de TRAFIC : par combien de caisses chaque case doit-elle être empruntée ?
//
//   trafic <niveau>
//
// Un « couloir » n'est peut-être pas une propriété géométrique (des murs de chaque
// côté) mais une propriété de TRAFIC : les cases par lesquelles beaucoup de caisses
// doivent transiter pour rejoindre les buts.
//
// Protocole : chaque caisse est résolue SEULE (les autres retirées, tous les buts
// disponibles — c'est exactement ce que sont les niveaux 010x), et on trace les cases
// que la caisse traverse. Le compte par case donne la carte.
//
// Pourquoi ça compte : la macro-poussée (§9.5) a cassé l'optimalité du niveau 2 sur
// la case (5,4) — le seul passage vers la salle des buts, donc la case de trafic
// MAXIMAL. La solution optimale a besoin d'y GARER une caisse au milieu, pour
// organiser l'entrelacement. Si l'hypothèse tient, le critère de fusion devient :
// fusionner sur les couloirs à faible trafic, JAMAIS sur les artères.
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QList>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: trafic <niveau>\n"); return 2; }
    const int num = QString(argv[1]).toInt();

    const QString chemin = QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0'));

    QStringList lignes;
    { QFile f(chemin); f.open(QIODevice::ReadOnly | QIODevice::Text); QTextStream in(&f);
      while (!in.atEnd()) lignes << in.readLine(); }

    std::vector<QPoint> caisses;
    for (int y = 0; y < lignes.size(); y++)
        for (int x = 0; x < lignes[y].size(); x++)
            if (lignes[y][x] == '$' || lignes[y][x] == '*') caisses.push_back(QPoint(x, y));

    Level plein; plein.load(chemin);
    Game gPlein(plein, num);
    const int L = gPlein.getLargeur(), H = gPlein.getHauteur();

    std::vector<int> trafic(L * H, 0);
    const int n = (int)caisses.size();

    for (int k = 0; k < n; k++) {
        // Sous-niveau : la caisse k SEULE, tous les buts. On ÔTE des obstacles,
        // jamais on n'en ajoute (cf. §8.5).
        QStringList g = lignes;
        for (int j = 0; j < n; j++) {
            if (j == k) continue;
            const QPoint& p = caisses[j];
            g[p.y()][p.x()] = (lignes[p.y()][p.x()] == '*') ? '.' : ' ';
        }

        const QString tmp = QString("%1/trafic_%2.xsb").arg(SCRATCH).arg(k);
        { QFile f(tmp); f.open(QIODevice::WriteOnly | QIODevice::Text);
          QTextStream out(&f); for (const QString& l : g) out << l << "\n"; }

        Level lv; lv.load(tmp);
        Game depart(lv, num);
        QFile::remove(tmp);

        Solveur* s = Solveur::creer(Solveur::Astar, depart);
        std::vector<int> vues;
        QObject::connect(s, &Solveur::solutionTrouvee, s,
                         [&vues, depart, L](QList<Game::EDirection> coups, qint64) {
            // Tracer les cases OCCUPÉES par la caisse au fil de la solution.
            Game g(depart);
            auto caseCaisse = [&g, L]() {
                for (int i = 0; i < g.getLargeur() * g.getHauteur(); i++)
                    if (g.getCase(i) == Level::tcCaisse || g.getCase(i) == Level::tcGoalCaisse)
                        return i;
                return -1;
            };
            vues.push_back(caseCaisse());
            for (Game::EDirection d : coups) {
                const int avant = g.getNbDepCaisse();
                g.deplace(d);
                if (g.getNbDepCaisse() > avant) vues.push_back(caseCaisse());
            }
        }, Qt::DirectConnection);
        s->start(); s->wait(); delete s;

        if (vues.empty()) { printf("caisse %d : INSOLUBLE SEULE\n", k); continue; }
        for (int i : vues) if (i >= 0) trafic[i]++;
    }

    // Carte.
    printf("===== NIVEAU %d — TRAFIC (nb de caisses qui traversent la case) =====\n", num);
    printf("  '.' = but   '#' = mur   chiffre = nb de caisses   '+' = 10 et plus\n\n");
    for (int y = 0; y < H; y++) {
        printf("  ");
        for (int x = 0; x < L; x++) {
            const int i = x + y * L;
            const Level::ETypeCase c = gPlein.getCase(i);
            if (c == Level::tcMur) { printf("#"); continue; }
            if (trafic[i] == 0) {
                const bool but = (c == Level::tcGoal || c == Level::tcGoalCaisse || c == Level::tcGoalPlayer);
                printf("%c", but ? '.' : ' ');
                continue;
            }
            if (trafic[i] >= 10) printf("+");
            else                 printf("%d", trafic[i]);
        }
        printf("\n");
    }

    // Croisement avec le critère GÉOMÉTRIQUE de la macro.
    printf("\n-- cases a fort trafic (>= 3 caisses) --\n");
    for (int y = 0; y < H; y++)
        for (int x = 0; x < L; x++) {
            const int i = x + y * L;
            if (trafic[i] >= 3)
                printf("   (%2d,%2d)  trafic = %2d  %s\n", x, y, trafic[i],
                       (gPlein.getCase(i) == Level::tcGoal ||
                        gPlein.getCase(i) == Level::tcGoalCaisse) ? "[BUT]" : "");
        }

    fflush(stdout);
    return 0;
}
