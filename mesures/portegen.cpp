// portegen <niv|plateau.xsb> — LE PORTE GÉNÉRALISÉ (§6.0, 2026-08-18).
//
// Validation en dur du prédicat `Game::porteGeneraliseeCoupe` (game.cpp) contre
// le mineur `mesures/stock.py cut`, qui l'a établi hors-solveur sur les parties
// gagnées. Contrairement à `porte` (statique, calculé une fois au chargement,
// obstacles = murs + LA caisse + LE but testés seuls), celui-ci interroge
// l'état COURANT du plateau donné : toutes les caisses qui y sont posées
// comptent comme obstacles. D'où l'usage sur un `.xsb` de MILIEU DE PARTIE
// (comme `bench`/`ordre`/`pas0`), pas seulement sur un niveau au départ — un
// niveau au départ n'a presque jamais rien à couper (aucune caisse encore
// déplacée).
//
// Pour chaque caisse NON livrée (case, pas rang) et chaque but NON rempli,
// teste si occuper ce but à la place de cette caisse coupe l'accès du joueur à
// une AUTRE caisse non livrée ou un AUTRE but non rempli. Imprime les paires
// coupées ; silencieux sur les autres (comme `porte`, le signal est rare).
#include <QVector>
#include <cstdio>
#include "game.h"
#include "level.h"

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: portegen <niv|plateau.xsb>\n"); return 2; }
    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "portegen: plateau introuvable (%s)\n", argv[1]); return 2; }
    Game g(level, num);

    const int L = g.getLargeur();
    printf("=== %s — PORTE GENERALISE (etat courant, %d but(s)) ===\n",
           parChemin ? qPrintable(arg1) : qPrintable(QString("niveau %1").arg(num)),
           g.getNbButs());

    int caisses = 0, testes = 0, coupes = 0;
    for (int c = 0; c < L * g.getHauteur(); c++) {
        if (g.getCase(c) != Level::tcCaisse) continue;   // non livree seulement
        caisses++;
        const int cx = c % L, cy = c / L;
        for (int b = 0; b < g.getNbButs(); b++) {
            const int gb = g.getCaseBut(b);
            if (g.getCase(gb) == Level::tcGoalCaisse) continue;   // deja rempli
            testes++;
            if (g.porteGeneraliseeCoupe(c, b)) {
                coupes++;
                printf("  caisse (%2d,%2d) -> but (%2d,%2d) : COUPE (accès perdu ailleurs)\n",
                       cx, cy, gb % L, gb / L);
            }
        }
    }
    printf("  --- %d caisse(s) non livree(s), %d paire(s) testee(s), %d coupe(s)\n",
           caisses, testes, coupes);
    return 0;
}
