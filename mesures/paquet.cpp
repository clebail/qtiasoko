// Harnais : QUELLE FRÉQUENCE a le motif « PAQUET DE CAISSES NON LIVRABLE », et
// surtout À QUELLE PROFONDEUR — et COMBIEN il coûte à détecter.
//
//   paquet <niveau> [nbDepilements] [budget] [mode]
//        défauts :        200000      2000    coupl-plongeon
//
// LE MOTIF (campagne de critique du chemin du solveur, 2026-08-03). Il vient de
// l'utilisateur, qui a annoté les chemins de record des niveaux 14, 15 et 16 et
// désigné cinq états « morts à coup sûr ». Les cinq ont été confirmés par une
// recherche complète — dont deux uniquement grâce à sa méthode de RÉDUCTION :
// « ne garde que les caisses du coin, tu verras 2 poussées max ».
//
//   Un groupe de caisses 8-connexe, hors but, dont on retire TOUTES les autres
//   caisses du plateau en gardant TOUS les buts, ne peut pas être entièrement
//   posé sur des buts  ⇒  l'état réel est MORT.
//
// POURQUOI C'EST SOUND. Retirer des caisses ne fait qu'augmenter la liberté du
// joueur et des caisses restantes ; garder tous les buts leur laisse toutes leurs
// destinations. `Game::checkVictoire` étant « aucune caisse hors but » (et non
// « tous les buts remplis »), l'instance relâchée est bien un sur-ensemble de ce
// que l'état réel autorise. Si elle est insoluble, l'état réel l'est.
//
// CE N'EST PAS LE CORRAL, et c'est le point. Mesuré sur l'état #1 du niveau 16 :
// la zone du joueur fait 25 cases et atteint x=10 — la poche haut-gauche n'est
// PAS scellée, il n'y a aucun enclos à trouver. Le corral cherche une région
// close ; ici les caisses sont simplement coincées entre elles. De plus le corral
// est incrémental, indexé sur la case d'arrivée de la caisse qui vient de bouger
// (`detecteEnclosArrivee`) : une mort née de la COMBINAISON de caisses posées à
// des moments différents ne lui repasse jamais sous les yeux.
//
// ⚠️ CE QUE CET OUTIL NE FAIT PAS : élaguer. Il COMPTE. La fréquence seule ne
// décide de rien — le §6.1 (2026-07-31) a établi que le prédicteur qui survit est
// « états de recherche ÉPARGNÉS ÷ états de sous-solve DÉPENSÉS », et que la
// « fraction de durs morts » ne prédit rien. On imprime donc la DÉPENSE avec la
// fréquence, sans quoi le chiffre serait joli et inutilisable.
//
// ⚠️ Les états échantillonnés sont ceux que le solveur a réellement DÉPILÉS : ils
// ont donc déjà passé `checkDefaite`, le corral unitaire, la pince et le corral-N.
// Ce qu'on mesure est exactement ce que l'élagage actuel LAISSE PASSER.
#include <QCoreApplication>
#include <QString>
#include <QVector>
#include <QVarLengthArray>
#include <QHash>
#include <cstdio>
#include <vector>
#include <utility>
#include <algorithm>
#include "level.h"
#include "game.h"
#include "solveur.h"

extern std::vector<std::pair<QByteArray,int>>& etatsDeveloppes();
extern int& limiteDepilements();

// Groupes de caisses HORS BUT mutuellement adjacentes en 8-connexité. Le 8 et non
// le 4 : deux caisses en diagonale se bloquent mutuellement autant que côte à côte
// (c'est le motif du gel), et le paquet du niveau 14 qui a produit la preuve est
// diagonal en deux endroits.
static QVector<QVarLengthArray<int, 32>> paquets(const Game& g) {
    const int L = g.getLargeur(), H = g.getHauteur(), N = L * H;
    QVector<int> caisses;
    for (int i = 0; i < N; i++)
        if (g.getCase(i) == Level::tcCaisse) caisses.append(i);   // tcCaisse = hors but

    QVector<bool> pris(N, false);
    QVector<QVarLengthArray<int, 32>> out;
    for (int s : caisses) {
        if (pris[s]) continue;
        QVarLengthArray<int, 32> grp;
        QVector<int> pile; pile.append(s); pris[s] = true;
        while (!pile.isEmpty()) {
            const int i = pile.takeLast();
            grp.append(i);
            const int x = i % L, y = i / L;
            for (int dx = -1; dx <= 1; dx++)
                for (int dy = -1; dy <= 1; dy++) {
                    if (!dx && !dy) continue;
                    const int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
                    const int n = nx + ny * L;
                    if (!pris[n] && g.getCase(n) == Level::tcCaisse) { pris[n] = true; pile.append(n); }
                }
        }
        std::sort(grp.begin(), grp.end());
        out.append(grp);
    }
    return out;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        fprintf(stderr, "usage: paquet <niveau> [nbDepilements] [budget] [mode]\n");
        return 2;
    }
    // MODE JUGE : `paquet <fichier.xsb> [budget]` teste UN plateau et rend son
    // verdict. Sert au juge `fp` (§1) : rejoué sur les états d'une partie GAGNANTE,
    // tous solubles par construction, toute détection est un faux positif PROUVÉ.
    // C'est le contrôle sans lequel une fréquence de 71 % ne vaut rien — le projet
    // s'est fait avoir trois fois sur ce terrain (gel naïf, `h` qui soustrait,
    // caisses=murs), et deux fois le test avait l'air excellent avant le juge.
    const QString arg1 = argv[1];
    if (arg1.endsWith(".xsb")) {
        const int bud = (argc > 2) ? QString(argv[2]).toInt() : 2000;
        Level lv; lv.load(arg1);
        Game gj(lv, 0);
        int pire = 1;      // 1 vivant, -1 inconnu, 0 mort
        for (const auto& p : paquets(gj)) {
            if (p.isEmpty()) continue;
            const int v = gj.sousSolveEnclos(p, bud);
            if (v == 0) { pire = 0; break; }
            if (v == -1) pire = -1;
        }
        printf("%s %s\n", pire == 0 ? "MORT" : (pire == 1 ? "vivant" : "inconnu"),
               qPrintable(arg1));
        return 0;
    }

    const int num    = QString(argv[1]).toInt();
    const int cap    = (argc > 2) ? QString(argv[2]).toInt() : 200000;
    const int budget = (argc > 3) ? QString(argv[3]).toInt() : 2000;
    const QString md = (argc > 4) ? argv[4] : "coupl-plongeon";
    const Solveur::EType type =
        (md == "macro")          ? Solveur::AstarMacro :
        (md == "plongeon")       ? Solveur::AstarMacroPlongeon :
        (md == "coupl-plongeon") ? Solveur::AstarMacroCouplagePlongeon :
        (md == "couplage")       ? Solveur::AstarMacroCouplage :
                                   Solveur::Astar;

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game game(level, num);

    limiteDepilements() = cap;
    Solveur* s = Solveur::creer(type, game);

    auto analyse = [num, game, budget]() {
        const auto& etats = etatsDeveloppes();
        Game g(game);                       // hérite des tables par COW
        const int nbButs = game.getNbButs();

        QVector<int> total(nbButs + 1, 0), morts(nbButs + 1, 0);
        int nTotal = 0, nMorts = 0;
        long long depense = 0, appels = 0, inconnus = 0;
        QVector<int> tailleTueuse(33, 0);
        // Mémoïsation par paquet trié : le même paquet revient des milliers de fois
        // (c'est ce qui rend le corral-N abordable, ×37 à ×223 d'amortissement).
        QHash<QByteArray, int> cache;
        long long hits = 0;

        QVarLengthArray<quint16, 32> cle(game.tailleCle());
        for (const auto& e : etats) {
            // ⚠️ getEtat() encode en GROS-BOUTISTE : un reinterpret_cast le relit à
            // l'envers et appliqueEtat écrit hors plateau (§5, le bug qui a faussé
            // `mou` pendant des semaines). Décodage à la main, obligatoire.
            const unsigned char* o = reinterpret_cast<const unsigned char*>(e.first.constData());
            for (int i = 0; i < cle.size(); ++i)
                cle[i] = (quint16)((o[2 * i] << 8) | o[2 * i + 1]);

            const int rangees = g.appliqueEtat(cle.data());
            total[rangees]++; nTotal++;

            bool mort = false; int tueur = 0;
            for (const auto& p : paquets(g)) {
                if (p.isEmpty()) continue;
                QByteArray k(reinterpret_cast<const char*>(p.constData()), p.size() * (int)sizeof(int));
                int v;
                auto it = cache.constFind(k);
                if (it != cache.constEnd()) { v = *it; hits++; }
                else {
                    int dev = 0;
                    v = g.sousSolveEnclos(p, budget, &dev);
                    depense += dev; appels++;
                    cache.insert(k, v);
                }
                if (v == -1) inconnus++;
                if (v == 0) { mort = true; tueur = p.size(); break; }   // un seul suffit
            }
            if (mort) { morts[rangees]++; nMorts++; if (tueur < 33) tailleTueuse[tueur]++; }
        }

        printf("\n===== NIVEAU %d — PAQUET NON LIVRABLE, SUR %d ETATS REELLEMENT DEPILES =====\n", num, nTotal);
        printf("  budget de sous-solve : %d etats par paquet\n\n", budget);
        printf("  MORTS non detectes par l'elagage actuel : %d / %d  (%.1f %%)\n",
               nMorts, nTotal, nTotal ? 100.0 * nMorts / nTotal : 0.0);
        printf("  DEPENSE : %lld etats de sous-solve pour %lld appels (%lld cache-hits, amortissement %.1fx)\n",
               depense, appels, hits, appels ? (double)(appels + hits) / appels : 0.0);
        printf("  cout par etat juge : %.1f etats de sous-solve   |  verdicts INCONNUS (budget) : %lld\n\n",
               nTotal ? (double)depense / nTotal : 0.0, inconnus);

        printf("  %-9s %10s %10s %8s\n", "rangees", "depiles", "morts", "%");
        for (int k = 0; k <= nbButs; k++) {
            if (!total[k]) continue;
            const double p = 100.0 * morts[k] / total[k];
            QByteArray bar(int(p / 5), '#');
            printf("  %2d/%-6d %10d %10d %7.1f%%   %s\n", k, nbButs, total[k], morts[k], p, bar.constData());
        }
        printf("\n  taille du paquet qui tue : ");
        for (int k = 1; k < 33; k++) if (tailleTueuse[k]) printf("%d caisses:%d  ", k, tailleTueuse[k]);
        printf("\n\n  Lecture : la PROFONDEUR decide. Un mort trouve a 2 caisses rangees coupe un\n"
               "  continent ; a 11, une brindille — le solveur a deja paye pour y arriver.\n"
               "  Et la frequence ne suffit pas : le predicteur qui survit (plan 6.1, 2026-07-31)\n"
               "  est ETATS EPARGNES / ETATS DE SOUS-SOLVE DEPENSES, d'ou la ligne DEPENSE.\n");
        fflush(stdout);
        QCoreApplication::quit();
    };

    QObject::connect(s, &Solveur::aucuneSolution, analyse);
    QObject::connect(s, &Solveur::solutionTrouvee, [analyse](QList<Game::EDirection>, qint64) { analyse(); });
    QObject::connect(s, &Solveur::rechercheArretee, [analyse](qint64) { analyse(); });

    s->start();
    return app.exec();
}
