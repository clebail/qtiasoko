// Harnais headless : résout un niveau et imprime le canari.
//
//   bench <numNiveau> [astar|macro|pondere|couplage|bfs] [record]
//
// Sortie : "OK <niveau> etats=<n> poussees=<p>" — les deux chiffres qui doivent
// rester identiques à l'octet près après l'étape 11 (adressage ouvert).
//
// MODE 'record' (§6.0, chantier du PLONGEON-SUR-RECORD) : écrit en .xsb CHAQUE
// état qui bat le max de caisses posées, au fil du solve. C'est la matière du
// chantier : un record est-il COMPLÉTABLE, et à quel prix ? On répond en
// re-résolvant la fixture produite, sans toucher au solveur. Marche aussi sur un
// niveau qu'on ne finit pas (11) — les records tombent bien avant la solution.
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <cstdio>
#include "level.h"
#include "game.h"
#include "solveur.h"

// Plateau au format .xsb — même convention de caractères que l'export de l'app
// (MainWindow::onExportXsb), pour que les fixtures produites ici se rechargent
// telles quelles dans l'app comme dans le bench.
static bool ecritXsb(const Game& g, const QString& chemin) {
    QFile f(chemin);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    const int L = g.getLargeur(), H = g.getHauteur();
    QTextStream out(&f);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            switch (g.getCase(x + y * L)) {
                case Level::tcMur:        out << '#'; break;
                case Level::tcCaisse:     out << '$'; break;
                case Level::tcGoalCaisse: out << '*'; break;
                case Level::tcGoal:       out << '.'; break;
                case Level::tcPlayer:     out << '@'; break;
                case Level::tcGoalPlayer: out << '+'; break;
                default:                  out << ' '; break;
            }
        }
        out << "\n";
    }
    return true;
}

// Une caisse occupe-t-elle cette case ? (posée ou non)
static bool caisseIci(const Game& g, int i) {
    const Level::ETypeCase c = g.getCase(i);
    return c == Level::tcCaisse || c == Level::tcGoalCaisse;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (argc < 2) { fprintf(stderr, "usage: bench <niveau|fichier.xsb> [astar|macro|pondere|bfs] [record]\n"); return 2; }
    // Un CHEMIN .xsb est accepté à la place d'un numéro : c'est ce qui rend
    // exploitables les fixtures du mode 'record' (et tout plateau exporté depuis
    // l'app) sans avoir à les renommer en levelNNNN.xsb ni à les poser dans le
    // répertoire des sources. Le numéro n'est alors que cosmétique (numNiveau).
    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num    = parChemin ? 0 : arg1.toInt();
    const QString md = (argc > 2) ? argv[2] : "astar";
    const Solveur::EType type =
        (md == "macro")   ? Solveur::AstarMacro   :
        (md == "couplage")? Solveur::AstarMacroCouplage :
        (md == "plongeon")? Solveur::AstarMacroPlongeon :
        (md == "coupl-plongeon")? Solveur::AstarMacroCouplagePlongeon :
        (md == "coins")   ? Solveur::AstarMacroCouplagePlongeonCoins :
        (md == "loi")     ? Solveur::AstarMacroCouplagePlongeonLoi :
        (md == "ordre-loi")? Solveur::AstarMacroCouplagePlongeonOrdreLoi :
        (md == "ordre-dyn")? Solveur::AstarMacroCouplagePlongeonOrdre :
        (md == "pondere" || md == "2") ? Solveur::AstarPondere :
        (md == "bfs")     ? Solveur::Bfs          : Solveur::Astar;

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "bench: niveau introuvable (%s)\n", qPrintable(arg1)); return 2; }

    Game game(level, num);

    // Mode TEST du gate full-scan sur un état chargé : la détection corral-N
    // flague-t-elle un enclos DUR dans CE plateau ? Usage : bench <niv> gate
    if (md == "gate") {
        printf("gate %d : gateEnclosMort() = %s\n", num,
               game.gateEnclosMort() ? "DUR TROUVE (mode3 pruneraient)" : "aucun dur");
        return 0;
    }

    // Mode TEST du mini-solveur d'enclos : prend TOUTES les caisses du niveau comme
    // frontière et lance sousSolveEnclos. Valide sur les fixtures (0196 → MORT,
    // 0195 → vivant). Usage : bench <niv> enclos [budget]
    if (md == "enclos") {
        const int budget = (argc > 3) ? QString(argv[3]).toInt() : 100000;
        QVarLengthArray<int, 32> boxes;
        const int L = game.getLargeur(), H = game.getHauteur();
        for (int i = 0; i < L * H; i++) {
            auto c = game.getCase(i);
            if (c == Level::tcCaisse || c == Level::tcGoalCaisse) boxes.append(i);
        }
        const int v = game.sousSolveEnclos(boxes, budget);
        printf("enclos %d : %d caisses, budget %d -> %s\n", num, boxes.size(), budget,
               v == 0 ? "MORT" : (v == 1 ? "vivant" : "inconnu"));
        return 0;
    }

    Solveur* s = Solveur::creer(type, game);

    // MODE 'record' : un .xsb par record de caisses posées. Connexion DIRECTE (pas
    // de contexte donné à connect) : le slot s'exécute dans le thread du solveur,
    // qui attend l'écriture — pas de course sur l'état, et le fichier est cohérent
    // avec le chemin reconstruit à cet instant.
    if (argc > 3 && QString(argv[3]) == "record") {
        QObject::connect(s, &Solveur::nouveauMaxCaisses,
                         [num, game](Game etatMax, int nbRangees, QList<Game::EDirection> chemin) {
            // Le signal ne porte le chemin qu'en COUPS (marche + poussées mêlées) :
            // on le rejoue sur une copie du départ pour compter les poussées — et
            // du même geste on VÉRIFIE que ce chemin mène bien à cet état. Sans ce
            // contrôle, une fixture fausse ne se verrait pas (c'est exactement le
            // bug big-endian qui a faussé `mou` pendant des semaines, cf. §5).
            Game g(game);
            for (Game::EDirection d : chemin) g.deplace(d);
            int ecarts = 0;
            for (int i = 0; i < g.getLargeur() * g.getHauteur(); i++)
                if (caisseIci(g, i) != caisseIci(etatMax, i)) ecarts++;

            const QString nom = QString("record_niv%1_r%2_g%3.xsb")
                                    .arg(num, 2, 10, QChar('0'))
                                    .arg(nbRangees, 2, 10, QChar('0'))
                                    .arg(g.getNbDepCaisse(), 4, 10, QChar('0'));
            const bool ok = ecritXsb(etatMax, nom);

            // Le CHEMIN, à côté du .xsb : le `.xsb` ne donne que l'état d'arrivée,
            // or ce qu'on veut annoter c'est la trajectoire qui y mène. Une lettre
            // par coup (H/D/B/G, ordre de EDirection), marche et poussées mêlées —
            // le rejeu les sépare tout seul, et le format reste diffable.
            QFile fc(QString(nom).replace(".xsb", ".chemin"));
            if (fc.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QByteArray t;
                for (Game::EDirection d : chemin) t += "HDBG"[(int)d];
                fc.write(t); fc.write("\n");
            }

            // stderr, comme la jauge de progression : les deux flux s'entrelacent,
            // donc le dernier point de jauge DATE le record en dépilements — sans
            // avoir à toucher à la signature du signal.
            fprintf(stderr, "[record] rangees %d/%d  %d coups / %d poussees  -> %s%s%s\n",
                    nbRangees, etatMax.getNbButs(), (int)chemin.size(), g.getNbDepCaisse(),
                    ok ? nom.toLocal8Bit().constData() : "(ECRITURE IMPOSSIBLE)",
                    ecarts ? "  ⚠️ CHEMIN INCOHERENT, " : "",
                    ecarts ? QString("%1 caisses mal placees").arg(ecarts).toLocal8Bit().constData() : "");
            fflush(stderr);
        });
    }

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
