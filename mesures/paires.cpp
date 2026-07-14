// D'ou viennent les poussees que h ne voit pas ?
//
//   paires <niveau>
//
// h calcule chaque caisse SEULE (distanceParBut, joueur-aware) puis apparie
// caisses et buts (hongrois). Elle relaxe donc entierement l'interaction
// CAISSE <-> CAISSE. Mesure : sur le niveau 2, le vrai cout est exactement 2 de
// plus que h, uniformement.
//
// Hypothese a tester : ces 2 poussees sont une interaction de PAIRE (deux
// caisses qui doivent se croiser, ou l'une qu'il faut parquer hors de son but
// pour laisser passer l'autre). Si oui, une h fondee sur les paires les
// recupererait.
//
// Protocole, pour chaque paire (i, j) de caisses :
//   cout2(i,j) = cout OPTIMAL de ranger i et j SEULES (autres caisses retirees,
//                tous les buts disponibles) -> resolution A* exacte
//   h2(i,j)    = ce que h predit sur ce meme sous-probleme
//   delta(i,j) = cout2 - h2 >= 0 : le supplement du a l'interaction de la paire
//
// Retirer les autres caisses ne fait qu'OTER des obstacles, donc cout2 est bien
// une borne inferieure du cout reel de ces deux caisses dans le vrai niveau :
// la mesure est admissible par construction (meme argument qu'au §8.5 — et
// surtout PAS "les autres caisses = murs", qui surestimerait et casserait tout).
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

static int resoudre(const Game& etat) {
    Solveur* s = Solveur::creer(Solveur::Astar, etat);
    int cout = -1;
    QObject::connect(s, &Solveur::solutionTrouvee, s,
                     [&cout, etat](QList<Game::EDirection> coups, qint64) {
        Game g(etat);
        int p = 0;
        for (Game::EDirection d : coups) {
            const int avant = g.getNbDepCaisse();
            g.deplace(d);
            if (g.getNbDepCaisse() > avant) p++;
        }
        cout = g.isGagne() ? p : -1;
    }, Qt::DirectConnection);
    s->start();
    s->wait();
    delete s;
    return cout;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: paires <niveau>\n"); return 2; }
    const int num = QString(argv[1]).toInt();

    const QString chemin = QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0'));

    // 1. Lire la grille brute.
    QStringList lignes;
    {
        QFile f(chemin);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { fprintf(stderr, "lecture KO\n"); return 1; }
        QTextStream in(&f);
        while (!in.atEnd()) lignes << in.readLine();
    }

    // Position de chaque caisse ('$' hors but, '*' sur but) et de chaque but
    // ('.', '*', '+').
    std::vector<QPoint> caisses, buts;
    for (int y = 0; y < lignes.size(); y++)
        for (int x = 0; x < lignes[y].size(); x++) {
            const QChar c = lignes[y][x];
            if (c == '$' || c == '*') caisses.push_back(QPoint(x, y));
            if (c == '.' || c == '*' || c == '+') buts.push_back(QPoint(x, y));
        }

    // 2. Reference : le niveau complet.
    Level plein;
    plein.load(chemin);
    Game gPlein(plein, num);
    const int hPlein = gPlein.getHeuristique();
    const int cStar  = resoudre(gPlein);

    printf("===== NIVEAU %d : %d caisses =====\n", num, (int)caisses.size());
    printf("h(depart) = %d   C* = %d   MOU = %d\n\n", hPlein, cStar, cStar - hPlein);

    // 3. Chaque paire, resolue exactement.
    const int n = (int)caisses.size();
    std::vector<std::vector<int>> delta(n, std::vector<int>(n, 0));

    int deltaMax = 0, nbPairesEnInteraction = 0;

    const int nb = (int)buts.size();

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            // Pour chaque PAIRE DE BUTS : un sous-probleme CARRE (2 caisses,
            // 2 buts). Indispensable — avec tous les buts conserves, n != nbButs
            // declenche le repli de getHeuristique() (game.cpp:483) et h2 n'est
            // plus le couplage mais l'ancienne borne : on mesurerait l'ecart
            // entre le couplage et son repli, pas l'interaction des caisses.
            //
            // On retient l'assignation de buts qui MINIMISE le cout : c'est la
            // seule qui soit une borne inferieure valide (on ignore quels buts
            // la vraie solution attribuera a ces deux caisses).
            int meilleurCout = -1, h2Retenu = 0;

            for (int a = 0; a < nb; a++) {
                for (int b = a + 1; b < nb; b++) {
                    QStringList g = lignes;

                    // Retirer les autres caisses : '$' -> sol, '*' -> but nu.
                    // On OTE des obstacles, on n'en ajoute jamais (cf. §8.5 :
                    // « caisses = murs » surestimerait et casserait tout).
                    for (int k = 0; k < n; k++) {
                        if (k == i || k == j) continue;
                        const QPoint& p = caisses[k];
                        g[p.y()][p.x()] = (lignes[p.y()][p.x()] == '*') ? '.' : ' ';
                    }
                    // Retirer les autres buts : '.' -> sol. Un but portant une
                    // caisse gardee ('*') ou le joueur ('+') se degrade en
                    // caisse/joueur nus.
                    for (int k = 0; k < nb; k++) {
                        if (k == a || k == b) continue;
                        const QPoint& p = buts[k];
                        const QChar c = g[p.y()][p.x()];
                        if      (c == '.') g[p.y()][p.x()] = ' ';
                        else if (c == '*') g[p.y()][p.x()] = '$';
                        else if (c == '+') g[p.y()][p.x()] = '@';
                    }

                    const QString tmp = QString("%1/p_%2_%3_%4_%5.xsb")
                                            .arg(SCRATCH).arg(i).arg(j).arg(a).arg(b);
                    {
                        QFile f(tmp);
                        f.open(QIODevice::WriteOnly | QIODevice::Text);
                        QTextStream out(&f);
                        for (const QString& l : g) out << l << "\n";
                    }

                    Level lv;
                    lv.load(tmp);
                    Game gp(lv, num);
                    QFile::remove(tmp);

                    const int cout2 = resoudre(gp);
                    if (cout2 < 0) continue;              // paire (caisses,buts) insoluble

                    if (meilleurCout < 0 || cout2 < meilleurCout) {
                        meilleurCout = cout2;
                        h2Retenu     = gp.getHeuristique();
                    }
                }
            }

            if (meilleurCout < 0) { printf("  paire (%2d,%2d) : insoluble sur TOUS les buts\n", i, j); continue; }

            const int d = meilleurCout - h2Retenu;   // interaction pure de la paire
            delta[i][j] = delta[j][i] = d;
            if (d > 0) nbPairesEnInteraction++;
            deltaMax = std::max(deltaMax, d);
        }
    }

    // 4. Synthese.
    printf("-- supplement d'interaction par paire (cout2 - h2) --\n");
    printf("      ");
    for (int j = 0; j < n; j++) printf("%3d", j);
    printf("\n");
    for (int i = 0; i < n; i++) {
        printf("  %2d :", i);
        for (int j = 0; j < n; j++) {
            if (i == j) printf("  .");
            else printf("%3d", delta[i][j]);
        }
        printf("\n");
    }

    printf("\n-- verdict --\n");
    printf("  MOU a combler (C* - h)            : %d\n", cStar - hPlein);
    printf("  interaction de paire MAX          : %d\n", deltaMax);
    printf("  paires en interaction (delta > 0) : %d / %d\n",
           nbPairesEnInteraction, n * (n - 1) / 2);
    printf("\n");
    if (deltaMax >= cStar - hPlein)
        printf("  => l'interaction de paire SUFFIT a expliquer le mou.\n");
    else
        printf("  => l'interaction de paire NE SUFFIT PAS (%d < %d) : le mou vient\n"
               "     d'ailleurs (interaction a 3+ caisses, ou cout de manoeuvre).\n",
               deltaMax, cStar - hPlein);

    // ⚠️ On n'additionne PAS les deltas de paires disjointes. Teste au niveau 0 :
    // h serait passee de 4 a 5 pour un C* de 4 — SURESTIMATION, donc perte
    // silencieuse de l'optimalite. La raison : deux paires disjointes en CAISSES
    // peuvent viser les MEMES buts, et leurs couts se recouvrent. Une heuristique
    // additive par paires devrait partitionner les BUTS aussi, pas seulement les
    // caisses. Meme famille de faute qu'au §8.5.
    printf("\n  (pas de somme sur paires disjointes : elle SURESTIME — cf. niveau 0,\n"
           "   h=5 pour C*=4. Les buts sont partages entre paires.)\n");

    fflush(stdout);
    return 0;
}
