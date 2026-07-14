// Mesure la DIVERGENCE entre l'état optimal et l'état que h désignerait.
//
//   diverge <niveau>
//
// Une fois la solution optimale connue (A* optimal), on la rejoue coup par coup.
// À chaque état du chemin optimal on génère TOUS les enfants, on les classe par
// h croissante, et on regarde à quel RANG se trouve le bon — celui qui est
// réellement sur le chemin.
//
// Pourquoi le rang selon h, et pas selon f : tous les enfants d'un même état ont
// le même g (une poussée = un pas, toujours). Trier par f = g + h revient donc
// exactement à trier par h. Le choix LOCAL ne dépend que de h ; g ne départage
// rien.
//
// Deux verdicts en sortent :
//  - h comme CLASSEUR : si le bon enfant est presque toujours en rang 1-2, un
//    faisceau étroit suffirait (§5.1 du plan, jamais testé).
//  - h comme BORNE : le long d'un chemin optimal, le coût restant réel vaut
//    exactement C* - g. Le mou de h se calcule donc au chiffre près à chaque
//    profondeur : mou(i) = (C* - i) - h(s_i). On voit OÙ h ment, pas seulement
//    de combien au départ (la « tension » du §7.1 ne regardait que h(depart)).
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <QByteArray>
#include <QVector>
#include <algorithm>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

struct Etape {
    int profondeur;    // i = nombre de poussées déjà faites
    int h;             // h(s_i)
    int hStar;         // C* - i : le coût restant RÉEL (exact sur un chemin optimal)
    int rang;          // rang du bon enfant parmi les enfants classés par h (1 = premier)
    int nbEnfants;     // facteur de branchement
    int exAequo;       // enfants de même h que le bon (le rang est ambigu à ce point)
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: diverge <niveau>\n"); return 2; }
    const int num = QString(argv[1]).toInt();

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game depart(level, num);

    Solveur* s = Solveur::creer(Solveur::Astar, depart);

    QObject::connect(s, &Solveur::aucuneSolution, []() {
        printf("AUCUNE SOLUTION\n"); QCoreApplication::quit();
    });

    QObject::connect(s, &Solveur::solutionTrouvee,
                     [num, depart](QList<Game::EDirection> coups, qint64 etats) {
        // 1. Rejouer la solution pour extraire la SUITE DES ÉTATS du chemin
        //    optimal. Le signal ne porte que des coups (marche + poussées
        //    mêlées) ; un coup est une poussée si et seulement s'il fait monter
        //    nbDepCaisse.
        std::vector<Game> chemin;
        Game g(depart);
        chemin.push_back(g);

        for (Game::EDirection d : coups) {
            const int avant = g.getNbDepCaisse();
            g.deplace(d);
            if (g.getNbDepCaisse() > avant) chemin.push_back(g);   // c'était une poussée
        }

        if (!g.isGagne()) { printf("REJEU PERDU\n"); QCoreApplication::quit(); return; }

        const int cStar = (int)chemin.size() - 1;   // nombre de poussées optimal

        // 2. À chaque état du chemin, classer les enfants par h et situer le bon.
        std::vector<Etape> etapes;

        for (int i = 0; i < cStar; i++) {
            const Game& cur = chemin[i];
            const QByteArray cleBonne = chemin[i + 1].getEtat();

            std::vector<int> hEnfants;
            int hBon = -1;

            QVector<bool> zone = cur.getZoneJoueur();
            QVector<quint8> caisses = cur.getCaissesDeplacable(zone);

            for (int c = 0; c < caisses.size(); c++) {
                for (int d = 0; d < NB_DIRECTION; d++) {
                    if (!(caisses[c] & (1 << d))) continue;

                    Game e(cur);
                    e.pousse(c, (Game::EDirection)d);
                    if (e.isPerdu()) continue;   // élagué par le solveur : pas un candidat

                    const int he = e.getHeuristique();
                    hEnfants.push_back(he);
                    if (e.getEtat() == cleBonne) hBon = he;
                }
            }

            if (hBon < 0) { printf("BUG: enfant optimal introuvable a i=%d\n", i); continue; }

            // Rang = nombre d'enfants STRICTEMENT meilleurs, +1. Les ex aequo sont
            // comptés à part : à h égal, rien dans h ne permet de choisir — c'est
            // du départage arbitraire, pas de la discrimination.
            int mieux = 0, egaux = 0;
            for (int he : hEnfants) {
                if (he < hBon) mieux++;
                else if (he == hBon) egaux++;
            }

            etapes.push_back({i, cur.getHeuristique(), cStar - i,
                              mieux + 1, (int)hEnfants.size(), egaux - 1});
        }

        // 3. Synthèse.
        printf("\n===== NIVEAU %d — C* = %d poussees, %lld etats developpes =====\n",
               num, cStar, (long long)etats);

        printf("\n-- h comme CLASSEUR : rang du bon enfant (1 = h le designe en premier) --\n");
        int rang1 = 0, rang12 = 0, rang123 = 0, sommeRang = 0, pireRang = 0;
        int sommeEnfants = 0, sommeExAequo = 0;
        for (const Etape& e : etapes) {
            if (e.rang == 1) rang1++;
            if (e.rang <= 2) rang12++;
            if (e.rang <= 3) rang123++;
            sommeRang += e.rang;
            pireRang = std::max(pireRang, e.rang);
            sommeEnfants += e.nbEnfants;
            sommeExAequo += e.exAequo;
        }
        const int n = (int)etapes.size();
        printf("  rang 1      : %3d / %3d  (%.0f %%)\n", rang1, n, 100.0 * rang1 / n);
        printf("  rang <= 2   : %3d / %3d  (%.0f %%)\n", rang12, n, 100.0 * rang12 / n);
        printf("  rang <= 3   : %3d / %3d  (%.0f %%)\n", rang123, n, 100.0 * rang123 / n);
        printf("  rang moyen  : %.2f     rang pire : %d\n", (double)sommeRang / n, pireRang);
        printf("  enfants/etat: %.1f     ex aequo avec le bon : %.1f\n",
               (double)sommeEnfants / n, (double)sommeExAequo / n);

        printf("\n-- h comme BORNE : mou = (C* - i) - h, le long du chemin optimal --\n");
        printf("  %5s %6s %7s %6s\n", "i", "h", "reste", "mou");
        int sommeMou = 0, pireMou = 0;
        for (const Etape& e : etapes) {
            const int mou = e.hStar - e.h;
            sommeMou += mou;
            pireMou = std::max(pireMou, mou);
            // n'imprimer qu'un echantillon : debut, fin, et un point sur 10
            if (e.profondeur < 5 || e.profondeur % 10 == 0 || e.profondeur > cStar - 3)
                printf("  %5d %6d %7d %6d\n", e.profondeur, e.h, e.hStar, mou);
        }
        printf("  mou moyen : %.1f    mou max : %d    tension depart : %.0f %%\n",
               (double)sommeMou / n, pireMou,
               100.0 * etapes[0].h / etapes[0].hStar);

        fflush(stdout);
        QCoreApplication::quit();
    });

    s->start();
    return app.exec();
}
