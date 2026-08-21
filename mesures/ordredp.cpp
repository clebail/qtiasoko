// ordredp — LE GOAL-ORDERING EST-IL UN PROBLÈME DE BUDGET, OU UNE IMPOSSIBILITÉ
// STRUCTURELLE ? Programmation dynamique par sous-ensembles (schéma de calcul de
// Held-Karp — TSP/chemin hamiltonien —, simplifié : pas besoin de retenir « le
// dernier visité », seulement « quels buts sont posés »), en remplacement de la
// recherche à budget arbitraire de `Game::ordreParPrecedence` (game.cpp).
//
//   ordredp <niveau|fichier.xsb>
//
// LE CONSTAT (2026-08-21, idée utilisateur). `Game::butMureLocalement(h, bloque)`
// ne dépend QUE de l'ensemble des buts déjà posés (`bloque`), JAMAIS de l'ordre
// dans lequel on les a posés — la position de départ du joueur est fixe, la
// géométrie aussi. Donc « existe-t-il un ordre complet sans murage » est une pure
// question d'ACCESSIBILITÉ dans le graphe des 2^n sous-ensembles (n = nbButs) :
// mémoïser sur le bitmask évite de refaire dix mille fois le même sous-arbre que
// la recherche à budget de `ordreParPrecedence` refait aujourd'hui — et surtout,
// ça permet de PROUVER l'UNSAT (tout le treillis atteignable épuisé) au lieu du
// verdict ambigu « budget épuisé » de la recherche bornée.
//
// PRÉCÉDENCE utilisée comme contrainte DURE : la seule PROUVÉE — celle de
// `Game::precedenceGlobale` (privée ; recalculée ici via
// `PrecedencePaires::atteintUneCaisse`, déjà exemplaire unique avec `ordre.cpp`,
// §7 : une règle à deux endroits diverge). La précédence par ALIGNEMENT (indice,
// pas preuve, cf. game.h) est délibérément IGNORÉE ici : elle biaise VERS un
// ordre particulier, elle ne prouve rien sur l'EXISTENCE d'un ordre sûr — la
// question posée ici est le plafond THÉORIQUE, pas ce qu'une heuristique trouve.
//
// ⚠️ Coût O(2^n × n × flood-fill). Pour n > ~24 (le niveau 10 en a 32) c'est hors
// de portée tel quel — outil de diagnostic sur les niveaux à faible nombre de
// buts, pas un remplacement du solveur pour tous. Aucune décomposition par
// composante connexe ici : première version, pour trancher SAT/UNSAT sur les cas
// qui font actuellement saturer le budget de 500 (game.cpp §6.0).

#include <QCoreApplication>
#include <QVector>
#include <cstdio>
#include <QElapsedTimer>
#include "game.h"
#include "precedencepaires.h"
#include "level.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: ordredp <niveau|fichier.xsb>\n"); return 2; }

    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "ordredp: niveau introuvable (%s)\n", qPrintable(arg1)); return 2; }

    Game game(level, num);
    const int nb = game.getNbButs();
    const int L = game.getLargeur(), H = game.getHauteur(), size = L * H;
    (void)H;

    if (nb > 28) {
        fprintf(stderr, "ordredp: %d buts -- 2^%d hors de portee (limite ~24). Abandon.\n", nb, nb);
        return 2;
    }

    auto xy = [&](int c) { return QString("(%1,%2)").arg(c % L).arg(c / L); };

    // ── Précédence PROUVÉE, exactement le calcul privé de Game::precedenceGlobale
    //    (game.cpp:1734), rejoué ici via l'exemplaire unique PrecedencePaires ──
    QVector<QVector<int>> requis(nb);   // requis[b] = but-index devant être posés AVANT b
    for (int g = 0; g < nb; g++) {
        const int G = game.getCaseBut(g);
        if (!PrecedencePaires::atteintUneCaisse(game, G, -1)) continue;  // plateau douteux
        for (int b = 0; b < nb; b++) {
            if (b == g) continue;
            const int B = game.getCaseBut(b);
            if (!PrecedencePaires::atteintUneCaisse(game, G, B)) requis[b].append(g);
        }
    }

    // ── CONTRAINTE MANUELLE (2026-08-21, hypothèse utilisateur sur le niveau 22) ──
    // `--avant x,y,x,y,...  --apres x,y,x,y,...` : force TOUS les buts de --avant à
    // précéder TOUS ceux de --apres. Ce n'est PAS une preuve comme `requis` ci-dessus
    // — c'est une hypothèse humaine (« ce corridor doit se vider avant que la salle ne
    // se referme ») qu'on teste en la rendant contrainte dure, pour voir si un ordre
    // sain existe SOUS CETTE hypothèse. Si oui : témoin à rejouer. Si non (UNSAT) :
    // l'hypothèse contredit la précédence prouvée, donc elle est fausse telle quelle.
    auto listeCoords = [&](const QString& s) {
        QVector<int> idx;
        const QStringList parts = s.split(',', Qt::SkipEmptyParts);
        for (int i = 0; i + 1 < parts.size(); i += 2) {
            const int x = parts[i].toInt(), y = parts[i + 1].toInt();
            const int cell = x + y * L;
            int trouve = -1;
            for (int b = 0; b < nb; b++) if (game.getCaseBut(b) == cell) { trouve = b; break; }
            if (trouve < 0) { fprintf(stderr, "ordredp: (%d,%d) n'est pas un but\n", x, y); continue; }
            idx.append(trouve);
        }
        return idx;
    };
    QVector<int> avantIdx, apresIdx;
    for (int i = 1; i < argc; i++) {
        if (QString(argv[i]) == "--avant" && i + 1 < argc) avantIdx = listeCoords(argv[++i]);
        if (QString(argv[i]) == "--apres" && i + 1 < argc) apresIdx = listeCoords(argv[++i]);
    }
    if (!avantIdx.isEmpty() && !apresIdx.isEmpty()) {
        fprintf(stderr, "[ordredp] contrainte manuelle : %d but(s) avant %d but(s)\n",
                (int)avantIdx.size(), (int)apresIdx.size());
        for (int a : avantIdx)
            for (int b : apresIdx)
                if (a != b && !requis[b].contains(a)) requis[b].append(a);
    }

    const quint32 nEtats = 1u << nb;
    const quint32 FULL = nEtats - 1u;

    QVector<bool> visited(nEtats, false);
    QVector<qint8> choixFait(nEtats, -1);     // quel but a été AJOUTÉ pour atteindre cet état
    QVector<quint32> parentDe(nEtats, 0);
    QVector<quint32> file;
    file.reserve(nEtats > (1u << 20) ? (1u << 20) : nEtats);

    visited[0] = true;
    file.append(0);
    bool trouve = (FULL == 0);
    QVector<bool> bloque(size, false);

    // PROGRESSION (2026-08-21, diagnostic niveau 22) : sans ça, un run qui n'a pas
    // fini au bout de plusieurs minutes est indiscernable d'un run qui explose --
    // même piège que le "budget epuise" ambigu de ordreParPrecedence qu'on cherche
    // justement a remplacer par une preuve. Chronometre, pas au compte d'iterations
    // (le cout par etat varie avec le nombre de candidats encore surs).
    QElapsedTimer chrono; chrono.start();
    qint64 dernierLog = 0;

    for (int t = 0; t < file.size() && !trouve; t++) {
        const quint32 S = file[t];

        if (chrono.elapsed() - dernierLog >= 2000) {
            dernierLog = chrono.elapsed();
            fprintf(stderr, "[ordredp] ... %d etats visites, %d en file d'attente, %.1fs ecoulees\n",
                    (int)file.size(), (int)(file.size() - t), chrono.elapsed() / 1000.0);
        }

        for (int b = 0; b < nb; b++) {
            if (S & (1u << b)) continue;

            bool pret = true;
            for (int r : requis[b]) if (!(S & (1u << r))) { pret = false; break; }
            if (!pret) continue;

            const quint32 Sp = S | (1u << b);
            if (visited[Sp]) continue;

            bloque.fill(false);
            for (int g = 0; g < nb; g++) if (Sp & (1u << g)) bloque[game.getCaseBut(g)] = true;

            bool ok = true;
            for (int h = 0; h < nb && ok; h++) {
                if (Sp & (1u << h)) continue;
                if (game.butMureLocalement(h, bloque)) ok = false;
            }
            if (!ok) continue;

            visited[Sp] = true;
            choixFait[Sp] = (qint8)b;
            parentDe[Sp] = S;
            file.append(Sp);
            if (Sp == FULL) { trouve = true; break; }
        }
    }

    fprintf(stderr, "[ordredp] %s -- %d buts, %d etats de l'espace atteignable explores (sur %u au plafond)\n",
            qPrintable(arg1), nb, (int)file.size(), nEtats);

    if (trouve) {
        printf("SAT -- un ordre sans murage existe (preuve constructive, precedence PROUVEE respectee) :\n");
        QVector<int> ordreTrouve;
        quint32 S = FULL;
        while (S != 0) {
            const int b = choixFait[S];
            ordreTrouve.prepend(b);
            S = parentDe[S];
        }
        for (int k = 0; k < ordreTrouve.size(); k++) {
            const int b = ordreTrouve[k];
            printf("  %2d. but %s\n", k, qPrintable(xy(game.getCaseBut(b))));
        }
    } else {
        printf("UNSAT PROUVE -- tout l'espace des sous-ensembles ATTEIGNABLE (%d etats) est epuise, "
               "aucun ne mene a l'etat plein. Aucun ordre total ne peut a la fois respecter la "
               "precedence prouvee et n'ecraser aucune approche : ce n'est PAS un manque de budget.\n",
               (int)file.size());
    }
    return 0;
}
