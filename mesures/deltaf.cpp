// Harnais : la goal macro PROMEUT-elle vraiment ses enfants dans la file ?
//
//   deltaf <numNiveau> [secondes] [macro|astar]     (défaut : 60 s, macro)
//
// Pourquoi cet outil (2026-07-24). Le comparateur d'A* (solveurastar.cpp) classe
// par f croissant, puis — à f égal — par g DÉCROISSANT : le plus profond d'abord.
// Une goal macro qui pose une caisse en N poussées enfile un enfant à g+N. Si h a
// baissé de N en échange, f est inchangé et cet enfant double tous les états de
// même f atteints par des poussées simples : « ranger une caisse » revient alors à
// passer en tête. C'est le mécanisme qui explique le niveau 1.
//
// Mais il n'est garanti par rien :
//     Δf = N + poids · Δh
// Il suffit qu'UNE poussée de la chaîne ne soit pas productive au sens du couplage
// hongrois (écarter une caisse, contourner, revenir) pour que Δh > −N, donc Δf > 0.
// L'enfant part alors sur un palier SUPÉRIEUR — et au lieu d'être promu, il est
// relégué DERRIÈRE toute la masse restante du palier courant. Sur un niveau dominé
// par le mou (f < C*, cf. plan §3), cette masse se compte en millions.
//
// LE CHIFFRE QUI DÉCIDE : la part des enfants de macro à Δf == 0. Si elle est
// proche de 100 %, le mécanisme de promotion fonctionne et il n'y a rien à voir
// ici. Si elle est basse, la macro se saborde elle-même — elle produit les enfants
// les plus rangés du solve et les envoie au fond de la file.
//
// Δf < 0 est possible et n'est PAS un bug : h (couplage joueur-aware) n'est pas
// tenue d'être cohérente. C'est même le cas favorable — l'enfant remonte d'un
// palier. On le compte à part.
#include <QCoreApplication>
#include <QString>
#include <QTimer>
#include <cstdio>
#include "level.h"
#include "game.h"
#include "solveur.h"
#include "solveurastar.h"

static void imprimeHisto(const char* titre, const std::vector<qint64>& h, qint64 n) {
    if (!n) { printf("\n  -- %s : aucun --\n", titre); return; }
    printf("\n  -- %s (%lld enfants enfiles) --\n", titre, (long long)n);
    for (size_t i = 0; i < h.size(); i++) {
        if (!h[i]) continue;
        const int df = (int)i - StatsDeltaF::DECALAGE;
        printf("     df = %+4d : %12lld  (%5.2f %%)%s\n", df, (long long)h[i],
               100.0 * h[i] / n,
               df == 0 ? "   <- reste sur le palier : PROMU par le tie-break g" :
               df <  0 ? "   <- remonte d'un palier" :
                         "   <- RELEGUE sur un palier superieur");
    }
}

static void imprime(int niveau) {
    const StatsDeltaF& s = statsDeltaF();

    printf("\n== DELTA f DES ENFANTS ENFILES, niveau %d ==\n", niveau);
    printf("   df = f(enfant) - f(parent) = N + poids * dh,  N = longueur de la chaine\n");

    imprimeHisto("GOAL MACRO", s.histoMacro, s.nMacro);
    imprimeHisto("POUSSEES SIMPLES", s.histoSimple, s.nSimple);
    if (s.horsBornes)
        printf("\n     (%lld enfants hors bornes d'histogramme)\n", (long long)s.horsBornes);

    qint64 macroNul = s.nMacro ? s.histoMacro[StatsDeltaF::DECALAGE] : 0;
    qint64 macroNeg = 0, simpleNul = s.nSimple ? s.histoSimple[StatsDeltaF::DECALAGE] : 0;
    qint64 simpleNeg = 0;
    for (int i = 0; i < StatsDeltaF::DECALAGE; i++) { macroNeg += s.histoMacro[i]; simpleNeg += s.histoSimple[i]; }

    printf("\n  -- LE CHIFFRE QUI DECIDE --\n");
    if (s.nMacro) {
        printf("  enfants de MACRO a df == 0 (promus)   %12lld  (%.2f %%)\n",
               (long long)macroNul, 100.0 * macroNul / s.nMacro);
        printf("                       a df <  0        %12lld  (%.2f %%)\n",
               (long long)macroNeg, 100.0 * macroNeg / s.nMacro);
        printf("                       a df >  0 (RELEGUES) %12lld  (%.2f %%)\n",
               (long long)(s.nMacro - macroNul - macroNeg),
               100.0 * (s.nMacro - macroNul - macroNeg) / s.nMacro);
        printf("  df moyen (macro)    %+.3f     longueur de chaine moyenne  %.2f\n",
               (double)s.sommeDeltaMacro / s.nMacro, (double)s.sommeLongMacro / s.nMacro);
    }
    if (s.nSimple) {
        printf("  enfants SIMPLES a df == 0             %12lld  (%.2f %%)\n",
               (long long)simpleNul, 100.0 * simpleNul / s.nSimple);
        printf("                  a df <  0             %12lld  (%.2f %%)\n",
               (long long)simpleNeg, 100.0 * simpleNeg / s.nSimple);
        printf("  df moyen (simple)   %+.3f\n", (double)s.sommeDeltaSimple / s.nSimple);
    }

    if (s.nReleg) {
        printf("\n  -- POURQUOI les enfants de macro sont RELEGUES (df > 0) --\n");
        printf("  h est joueur-aware : df = N + dh, et dh se decompose en\n");
        printf("     dh(caisses) = h(enfant, joueur PARENT) - h(parent)   <- le travail reel\n");
        printf("     dh(joueur)  = h(enfant, joueur enfant) - h(enfant, joueur parent)\n");
        printf("  enfants relegues            %12lld   (N moyen %.2f)\n",
               (long long)s.nReleg, (double)s.sommeLongReleg / s.nReleg);
        printf("  dh(caisses) moyen  %+.3f      <- vaut -N si la macro a fait son travail\n",
               (double)s.sommeDhCaisses / s.nReleg);
        printf("  dh(joueur)  moyen  %+.3f      <- ce que le joueur mal place coute\n",
               (double)s.sommeDhJoueur / s.nReleg);
        // Une moyenne nulle ne prouve rien : elle peut masquer des valeurs opposees.
        printf("     dont dh(joueur) != 0 : %lld (%.2f %%)   [+ : %lld, - : %lld]\n",
               (long long)s.dhJoueurNonNul, 100.0 * s.dhJoueurNonNul / s.nReleg,
               (long long)s.dhJoueurPositif, (long long)s.dhJoueurNegatif);
        printf("  dont dh(caisses) == -N (macro PARFAITE, c'est le JOUEUR qui coute) %lld (%.1f %%)\n",
               (long long)s.relegPurJoueur, 100.0 * s.relegPurJoueur / s.nReleg);
        printf("  dont dh(caisses)  > -N (le COUPLAGE se rearrange)                  %lld (%.1f %%)\n",
               (long long)s.relegCouplage, 100.0 * s.relegCouplage / s.nReleg);
    }

    // Les Δf > 0 viennent-ils des chaines longues (une poussee non productive
    // quelque part dedans) ou sont-ils uniformes ?
    printf("\n  -- CROISEMENT longueur de chaine N x df (macro) --\n");
    printf("     %4s %12s %10s %10s %10s\n", "N", "enfants", "df==0", "df<0", "df>0");
    for (size_t n = 0; n < s.parLongueur.size(); n++) {
        if (!s.parLongueur[n]) continue;
        const qint64 t = s.parLongueur[n], z = s.parLongueurNul[n], g = s.parLongueurNeg[n];
        printf("     %4zu %12lld %9.1f%% %9.1f%% %9.1f%%\n", n, (long long)t,
               100.0 * z / t, 100.0 * g / t, 100.0 * (t - z - g) / t);
    }
    printf("     ^ si df==0 s'effondre quand N monte : ce sont les chaines longues qui derapent\n");
    fflush(stdout);
}

// SELF-TEST du parametre 'posJoueur' de getHeuristique.
//
// La decomposition de dh ci-dessus ne vaut RIEN si ce parametre est inoperant :
// on lirait « dh(joueur) == 0 partout » sur une mesure qui ne mesure rien. On
// recalcule donc h de l'etat de depart depuis CHAQUE case non-mur et on compte
// les valeurs distinctes. h etant joueur-aware (distanceParBut est indexee par
// regions[joueur][caisse]), il doit en sortir plus d'une des lors qu'une caisse
// coupe le plateau. Si le compte vaut 1, ce n'est PAS forcement un bug : sur ce
// niveau, aucune caisse n'est un point de coupure — mais alors « joueur-aware »
// n'y change rien, et il faut le dire au lieu de conclure.
static void selfTest(const Game& g) {
    const int size = g.getLargeur() * g.getHauteur();
    QList<int> vues;
    int nonMur = 0;
    for (int i = 0; i < size; i++) {
        const Level::ETypeCase t = g.getCase(i);
        if (t == Level::tcMur || t == Level::tcNone) continue;
        nonMur++;
        const int h = g.getHeuristique(nullptr, i);
        if (!vues.contains(h)) vues.append(h);
    }
    printf("[self-test posJoueur] %d cases jouables, %d valeurs de h distinctes -> %s\n",
           nonMur, (int)vues.size(),
           vues.size() > 1 ? "le parametre AGIT (h est bien joueur-aware ici)"
                           : "h ne depend PAS du joueur sur ce niveau (aucune caisse coupante)");
    fflush(stdout);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (argc < 2) { fprintf(stderr, "usage: deltaf <niveau> [secondes] [macro|astar]\n"); return 2; }
    const int num  = QString(argv[1]).toInt();
    const int secs = (argc > 2) ? QString(argv[2]).toInt() : 60;
    const QString md = (argc > 3) ? argv[3] : "macro";

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game game(level, num);

    selfTest(game);

    Solveur* s = Solveur::creer(md == "astar" ? Solveur::Astar : Solveur::AstarMacro, game);

    QObject::connect(s, &Solveur::solutionTrouvee, [num](QList<Game::EDirection>, qint64 etats) {
        printf("RESOLU %d etats=%lld\n", num, (long long)etats);
        imprime(num);
        QCoreApplication::quit();
    });
    QObject::connect(s, &Solveur::rechercheArretee, [num](qint64 etats) {
        printf("ARRETE %d etats=%lld\n", num, (long long)etats);
        imprime(num);
        QCoreApplication::quit();
    });
    QObject::connect(s, &Solveur::aucuneSolution, [num]() {
        printf("AUCUNE %d\n", num);
        imprime(num);
        QCoreApplication::quit();
    });

    // Budget de temps : les cibles (11, 12) ne se resolvent jamais.
    QTimer::singleShot(secs * 1000, [s]() { s->demanderArret(); });

    s->start();
    return app.exec();
}
