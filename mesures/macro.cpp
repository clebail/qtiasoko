// Harnais : POURQUOI la goal macro échoue-t-elle ?
//
//   macro <numNiveau> [secondes]        (défaut : 60 s)
//
// Pourquoi cet outil (2026-07-21). macroVersBut() descend le champ
// `distanceParBut` en prenant, à chaque pas, la PREMIÈRE direction décroissante
// dans l'ordre de l'énumération (dHaut, dDroite, dBas, dGauche) — et elle ne
// revient JAMAIS dessus. Or il existe en général PLUSIEURS descentes optimales
// (amener une caisse en diagonale admet autant de chemins que d'entrelacements
// des poussées verticales et horizontales). La macro peut donc échouer alors
// qu'un chemin de MÊME COÛT passait.
//
// Quand elle échoue, le solveur retombe sur les poussées simples et perd tout
// l'élagage de la macro — c'est le symptôme observé sur le 11 et le 8 (la macro
// ne s'engage pas, `rangees 0` en régime courant, la file qui monte).
//
// LE CHIFFRE QUI DÉCIDE : la part des échecs survenus APRÈS un « fork » (un pas
// où au moins deux descentes optimales s'offraient). C'est la borne HAUTE de ce
// qu'un backtracking, ou un simple changement d'ordre d'essai, pourrait
// récupérer. Si elle est petite, les caisses sont vraiment bloquées et il n'y a
// rien à gratter ici. Si elle est grosse, c'est un gain de pur temps, sans
// toucher ni à l'optimalité ni au canari (macroVersBut ne fait que PROPOSER des
// enfants — un échec ne peut pas produire de fausse solution).
#include <QCoreApplication>
#include <QString>
#include <QTimer>
#include <cstdio>
#include "level.h"
#include "game.h"
#include "solveur.h"

static void imprime(int niveau) {
    const StatsMacro& s = statsMacro();
    const qint64 echecs = s.echecRegion + s.echecDistance + s.echecBloque + s.echecPousse;
    const double pc = s.tentatives ? 100.0 / s.tentatives : 0.0;

    printf("\n== GOAL MACRO, niveau %d ==\n", niveau);
    printf("  tentatives      %10lld\n", (long long)s.tentatives);
    printf("  succes          %10lld  (%.1f %%)\n", (long long)s.succes, s.succes * pc);
    printf("  echecs          %10lld  (%.1f %%)\n", (long long)echecs, echecs * pc);
    printf("     bloque (aucune poussee n'avance) %10lld  (%.1f %%)\n",
           (long long)s.echecBloque, s.echecBloque * pc);
    printf("     pousse refusee (deadlock)        %10lld  (%.1f %%)\n",
           (long long)s.echecPousse, s.echecPousse * pc);
    printf("     but inatteignable (d < 0)        %10lld  (%.1f %%)\n",
           (long long)s.echecDistance, s.echecDistance * pc);
    printf("     region joueur invalide           %10lld  (%.1f %%)\n",
           (long long)s.echecRegion, s.echecRegion * pc);

    printf("\n  -- LE CHIFFRE QUI DECIDE --\n");
    printf("  echecs APRES un choix arbitraire  %10lld  (%.1f %% des echecs)\n",
           (long long)s.echecAvecFork, echecs ? 100.0 * s.echecAvecFork / echecs : 0.0);
    printf("     ^ borne HAUTE de ce qu'un backtracking pourrait recuperer\n");
    printf("  succes ayant traverse un fork     %10lld  (%.1f %% des succes)\n",
           (long long)s.succesAvecFork, s.succes ? 100.0 * s.succesAvecFork / s.succes : 0.0);
    printf("  pas offrant >= 2 descentes        %10lld  sur %lld pas joues (%.1f %%)\n",
           (long long)s.forksTotal, (long long)s.pasTotal,
           s.pasTotal ? 100.0 * s.forksTotal / s.pasTotal : 0.0);
    printf("  distance restante moyenne au blocage  %.2f poussees\n",
           echecs ? (double)s.resteAuBlocage / echecs : 0.0);

    printf("\n  -- OU l'echec survient (pas deja joues) --\n");
    for (size_t i = 0; i < s.histoEchecPas.size(); i++)
        if (s.histoEchecPas[i])
            printf("     pas %3zu : %10lld  (%.1f %%)\n", i, (long long)s.histoEchecPas[i],
                   echecs ? 100.0 * s.histoEchecPas[i] / echecs : 0.0);
    printf("     ^ pas 0 = la caisse ne demarre meme pas ; tardif = travail gaspille\n");

    printf("\n  -- longueur des chaines REUSSIES --\n");
    for (size_t i = 0; i < s.histoSuccesLong.size(); i++)
        if (s.histoSuccesLong[i])
            printf("     %3zu poussees : %10lld\n", i, (long long)s.histoSuccesLong[i]);

    if (s.btTentatives) {
        printf("\n  -- BACKTRACK_MACRO (prototype 2026-07-23) --\n");
        printf("  tentatives      %10lld\n", (long long)s.btTentatives);
        printf("  succes          %10lld  (%.1f %%)\n",
               (long long)s.btSucces, 100.0 * s.btSucces / s.btTentatives);
        printf("     dont RECUPERES par backtracking (essais > 1) %10lld  (%.1f %% des succes)\n",
               (long long)s.btSuccesApresBacktrack,
               s.btSucces ? 100.0 * s.btSuccesApresBacktrack / s.btSucces : 0.0);
        printf("  essais moyens / tentative   %.2f\n", (double)s.btEssaisTotal / s.btTentatives);
        printf("  essais max sur une tentative %10lld\n", (long long)s.btEssaisMax);
    }
    fflush(stdout);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (argc < 2) { fprintf(stderr, "usage: macro <niveau> [secondes]\n"); return 2; }
    const int num  = QString(argv[1]).toInt();
    const int secs = (argc > 2) ? QString(argv[2]).toInt() : 60;

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game game(level, num);

    Solveur* s = Solveur::creer(Solveur::AstarMacro, game);

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

    // Budget de temps : les niveaux visés (8, 11) ne se resolvent jamais.
    QTimer::singleShot(secs * 1000, [s]() { s->demanderArret(); });

    s->start();
    return app.exec();
}
