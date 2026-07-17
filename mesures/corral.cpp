// Harnais : QUELLE PROPORTION de ce que le solveur explore est déjà MORTE d'un
// corral — et surtout, À QUELLE PROFONDEUR ?
//
//   corral <numNiveau> [nbDepilements]      (défaut : 200000)
//
// Pourquoi cet outil (plan §12, et la séance du 2026-07-17) :
//  - `mou` (§9.1) répond « 0 deadlock non détecté » — mais il n'a JAMAIS
//    échantillonné le 11, parce qu'il a besoin de RÉSOUDRE depuis chaque état
//    échantillonné. Sur un niveau qu'on ne sait pas résoudre, il est muet. C'est
//    le trou exact dans lequel ce résultat se cache.
//  - Ici, pas d'oracle coûteux : le test corral est un flood-fill, il coûte une
//    milliseconde par état. On peut donc mesurer là où `mou` ne peut pas.
//
// LA PROFONDEUR EST LE CHIFFRE QUI COMPTE (remarque de l'utilisateur, et elle est
// juste) : un deadlock détecté à 11 caisses rangées élague une BRINDILLE — le
// solveur a déjà payé tout le travail pour y arriver. Détecté à 2, il élague un
// CONTINENT. Un ratio global ne dirait rien ; la distribution par profondeur, si.
//
// ⚠️ Ce test n'est PAS sûr et n'a pas à l'être : il ne sert qu'à COMPTER, jamais
// à élaguer. Le trou connu (§12) — pousser une caisse DANS la poche libère sa
// case et le joueur la suit à l'intérieur — le rend optimiste : il peut déclarer
// mort un état qui ne l'est pas. Il donne donc une borne HAUTE. Si même cette
// borne haute est petite, le chantier corral est mort ; si elle est grosse, il
// faudra écrire la vraie règle et payer le prix de la rigueur.
#include <QCoreApplication>
#include <QString>
#include <QVector>
#include <QVarLengthArray>
#include <cstdio>
#include <vector>
#include <utility>
#include "level.h"
#include "game.h"
#include "solveur.h"

extern std::vector<std::pair<QByteArray,int>>& etatsDeveloppes();
extern int& limiteDepilements();

// Un état est-il mort d'un corral ? Règle instruite le 2026-07-17 sur trois
// positions réelles (niveaux 8 et 11) :
//   une caisse hors but dont CHAQUE poussée est soit impossible à vie (destination
//   murée, appui muré, ou appui dans une poche), soit aboutit dans une poche SANS
//   but → elle ne reverra jamais un but.
// Poche = composante connexe de cases libres que le joueur n'atteint pas.
static bool corralMort(const Game& g) {
    const int L = g.getLargeur(), H = g.getHauteur(), N = L * H;
    const QVector<bool> zone = g.getZoneJoueur();
    auto mur    = [&](int i) { return g.getCase(i) == Level::tcMur; };
    auto caisse = [&](int i) { return g.getCase(i) == Level::tcCaisse || g.getCase(i) == Level::tcGoalCaisse; };
    auto libre  = [&](int i) { return g.getCase(i) == Level::tcNone  || g.getCase(i) == Level::tcGoal
                                   || g.getCase(i) == Level::tcPlayer || g.getCase(i) == Level::tcGoalPlayer; };
    static const int dx[4] = {0, 0, -1, 1}, dy[4] = {-1, 1, 0, 0};

    QVector<bool> vu(N, false);
    for (int s = 0; s < N; s++) {
        if (vu[s] || zone[s] || !libre(s)) continue;

        // Flood-fill de la poche + repérage d'un but dedans.
        QVector<int> poche, pile;
        QVector<bool> dans(N, false);
        pile.append(s); vu[s] = true; dans[s] = true;
        bool aBut = false;
        while (!pile.isEmpty()) {
            const int i = pile.takeLast();
            poche.append(i);
            if (g.getCase(i) == Level::tcGoal) aBut = true;
            const int x = i % L, y = i / L;
            for (int d = 0; d < 4; d++) {
                const int nx = x + dx[d], ny = y + dy[d];
                if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
                const int n = nx + ny * L;
                if (!vu[n] && !zone[n] && libre(n)) { vu[n] = true; dans[n] = true; pile.append(n); }
            }
        }
        if (aBut) continue;   // la poche peut ranger : on ne conclut rien.

        // Caisses hors but au bord de cette poche.
        QVector<bool> bord(N, false);
        for (int i : poche) {
            const int x = i % L, y = i / L;
            for (int d = 0; d < 4; d++) {
                const int nx = x + dx[d], ny = y + dy[d];
                if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
                const int n = nx + ny * L;
                if (g.getCase(n) == Level::tcCaisse) bord[n] = true;
            }
        }

        for (int b = 0; b < N; b++) {
            if (!bord[b]) continue;
            const int bx = b % L, by = b / L;
            bool condamnee = true;
            for (int d = 0; d < 4 && condamnee; d++) {
                const int destX = bx + dx[d], destY = by + dy[d];
                const int appX  = bx - dx[d], appY  = by - dy[d];
                if (destX < 0 || destX >= L || destY < 0 || destY >= H) continue;   // murée de fait
                if (appX  < 0 || appX  >= L || appY  < 0 || appY  >= H) continue;
                const int dest = destX + destY * L, app = appX + appY * L;
                if (mur(dest)) continue;            // destination murée : impossible à vie
                if (mur(app))  continue;            // appui muré        : impossible à vie
                if (dans[app]) continue;            // appui DANS la poche : impossible à vie
                if (dans[dest]) continue;           // pousse dans une poche SANS but : perdue
                // Reste : une caisse bloque (elle peut partir), ou une vraie issue.
                // Dans les deux cas on NE conclut PAS — c'est ce qui garde le test
                // du côté de la sous-détection sur ce point précis.
                condamnee = false;
            }
            if (condamnee) return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: corral <niveau> [nbDepilements]\n"); return 2; }
    const int num   = QString(argv[1]).toInt();
    const int cap   = (argc > 2) ? QString(argv[2]).toInt() : 200000;

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game game(level, num);

    limiteDepilements() = cap;
    Solveur* s = Solveur::creer(Solveur::AstarMacro, game);

    auto analyse = [num, game]() {
        const auto& etats = etatsDeveloppes();
        Game g(game);                       // hérite des tables par COW : pas de recalcul
        const int nbButs = game.getNbButs();

        QVector<int> total(nbButs + 1, 0), morts(nbButs + 1, 0);
        int nTotal = 0, nMorts = 0;
        // ⚠️ La version QByteArray de getEtat() encode en GROS-BOUTISTE (« endianness
        // fixe et constante », §1.1) : octet de poids fort d'abord. Un
        // reinterpret_cast<quint16*> la relit en petit-boutiste sur ARM/x86 et
        // transforme la case 40 en case 10240 — appliqueEtat écrit alors hors du
        // plateau, piétine le tas, et le crash tombe des dizaines d'états plus loin.
        // Il FAUT décoder à la main.
        QVarLengthArray<quint16, 32> cle(game.tailleCle());
        for (const auto& e : etats) {
            const unsigned char* o = reinterpret_cast<const unsigned char*>(e.first.constData());
            for (int i = 0; i < cle.size(); ++i)
                cle[i] = (quint16)((o[2 * i] << 8) | o[2 * i + 1]);

            const int rangees = g.appliqueEtat(cle.data());
            total[rangees]++; nTotal++;
            if (corralMort(g)) { morts[rangees]++; nMorts++; }
        }

        printf("\n===== NIVEAU %d — CORRAL SUR %d ETATS REELLEMENT DEPILES =====\n", num, nTotal);
        printf("  morts d'un corral : %d / %d  (%.1f %%)\n\n", nMorts, nTotal, nTotal ? 100.0 * nMorts / nTotal : 0.0);
        printf("  %-9s %10s %10s %8s   %s\n", "rangees", "depiles", "morts", "%", "");
        for (int k = 0; k <= nbButs; k++) {
            if (!total[k]) continue;
            const double p = 100.0 * morts[k] / total[k];
            QByteArray bar(int(p / 5), '#');
            printf("  %2d/%-6d %10d %10d %7.1f%%   %s\n", k, nbButs, total[k], morts[k], p, bar.constData());
        }
        printf("\n  Lecture : les lignes du HAUT (peu de caisses rangees) sont celles qui\n"
               "  comptent — c'est la que le solveur passe son temps, et un elagage y\n"
               "  coupe un continent. Un corral qui n'apparait qu'en bas de tableau ne\n"
               "  vaut presque rien, quelle que soit sa frequence.\n");
        fflush(stdout);
        QCoreApplication::quit();
    };

    QObject::connect(s, &Solveur::aucuneSolution, analyse);
    QObject::connect(s, &Solveur::solutionTrouvee, [analyse](QList<Game::EDirection>, qint64) { analyse(); });

    s->start();
    return app.exec();
}
