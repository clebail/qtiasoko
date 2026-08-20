// jugeloi — LA LOI DE L'ORDRE ÉLAGUE-T-ELLE À TORT ?
//
// Même protocole que `fp` et que `fpporte.py` : on rejoue une partie GAGNANTE et on
// interroge le prédicat sur chacun de ses états. Ces états sont solubles PAR
// CONSTRUCTION — la partie finit gagnée — donc **toute détection est un faux positif
// prouvé**. C'est le seul juge possible pour un élagage qui n'est pas un théorème :
// `caseMorteLoi` est une exigence d'ORDRE, pas une preuve de géométrie, et rien ne
// garantit qu'elle ne coupe pas la seule branche gagnante (game.h le dit d'elle-même).
//
// POURQUOI IL FALLAIT L'ÉCRIRE. Jusqu'ici la loi était mesurée `PRUNES=0` partout
// sauf sur le niveau 6 : inerte, donc rien à juger. Le 2026-08-20 elle mord enfin
// ailleurs — `bench 18 loi` rend **1 101 923 prunes sur 28,9 M enfilages (3,82 %)** et
// progresse MOINS que le témoin (max 6/11 contre 8/11). « Moins loin à budget égal »
// n'est pas une preuve d'erreur ; ce juge-ci en est une.
//
// ⚠️ Un prédécesseur, `juge_loi.py`, a existé le 2026-08-03 et a été PERDU avec le
// scratchpad de sa session (§1). D'où ce rapatriement dans `mesures/` le jour même.
//
// ⚠️ EXEMPLAIRE UNIQUE DU CONTRAT : on ne réécrit pas la règle, on rejoue celle de
// `SolveurAStar::loiTropTot` (solveurastar.cpp) à l'identique — même but actif, même
// case d'arrivée, même balayage complet déclenché par une pose sur but. Une règle
// écrite à deux endroits diverge (§7) ; ici la copie est inévitable puisque
// `loiTropTot` est privée au solveur, alors on la duplique EN LA CITANT, et le moindre
// écart entre les deux se verra sur les compteurs (`enfilages`, `balayages`).
//
// USAGE :
//   jugeloi <niv> [align] < poussees.txt
// où poussees.txt donne une poussée par ligne : "<case> <dir>", `case` étant l'INDEX
// de case (x + y*largeur) de la caisse à pousser et `dir` l'ordre de EDirection
// (0=Haut 1=Droite 2=Bas 3=Gauche). `align` arme `setOrdreAlignement`, c'est-à-dire
// l'ordre que le régime `loi` calcule réellement.
//
// ⚠️ Le fichier de poussées se produit depuis le journal hybride (mesures/taches.py
// sait déjà en extraire les parties et les valider par rejeu) : c'est la SEULE source
// de parties gagnées sur les niveaux non résolus.

#include <cstdio>
#include <cstdlib>
#include <QString>
#include <QVector>
#include "level.h"
#include "game.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: jugeloi <niv> [align] < poussees.txt\n");
        return 2;
    }
    const int num = QString(argv[1]).toInt();
    bool align = false;
    for (int i = 2; i < argc; i++) if (QString(argv[i]) == "align") align = true;

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "jugeloi: niveau %d introuvable\n", num); return 2; }

    Game game(level, num);
    // L'ordre décide de TOUT ici : `caseMorteLoi` juge « vu du but actif », et le but
    // actif est le premier non rempli de `ordreButs`. Juger la loi sous un autre ordre
    // que celui du régime testé ne dirait rien du régime.
    if (align) game.setOrdreAlignement(true);

    const int L = game.getLargeur();
    auto xy = [&](int c) { return QString("(%1,%2)").arg(c % L).arg(c / L); };

    qint64 enfilages = 0, balayages = 0, fp = 0;
    int    coup = 0;
    char   ligne[256];

    while (fgets(ligne, sizeof(ligne), stdin)) {
        int cell = -1, dir = -1;
        if (sscanf(ligne, "%d %d", &cell, &dir) != 2) continue;
        coup++;

        const int arrivee = game.caseApres(cell, (Game::EDirection)dir);
        if (!game.pousse(cell, (Game::EDirection)dir)) {
            // Le rejeu diverge : mieux vaut refuser bruyamment que juger un autre
            // plateau que celui de la partie (§7, le `.xsb.txt` qui résout le 0).
            fprintf(stderr, "jugeloi: poussee %d ILLEGALE (case %s dir %d) — rejeu abandonne\n",
                    coup, qPrintable(xy(cell)), dir);
            return 3;
        }

        // ── Le contrat de SolveurAStar::loiTropTot, rejoué à l'identique ──────────
        const int actif = game.butActif();
        if (actif < 0) break;                     // plus de but : état gagnant
        enfilages++;

        if (arrivee >= 0 && game.caseMorteLoi(actif, arrivee)) {
            fp++;
            printf("FP coup %3d : caisse arrivee en %-8s condamnee par le but actif %s\n",
                   coup, qPrintable(xy(arrivee)), qPrintable(xy(game.getCaseBut(actif))));
            continue;
        }
        if (arrivee >= 0 && game.getCase(arrivee) == Level::tcGoalCaisse) {
            balayages++;
            const int N = game.getLargeur() * game.getHauteur();
            for (int c = 0; c < N; c++) {
                const Level::ETypeCase t = game.getCase(c);
                if (t != Level::tcCaisse && t != Level::tcGoalCaisse) continue;
                if (game.caseMorteLoi(actif, c)) {
                    fp++;
                    printf("FP coup %3d : caisse en %-8s condamnee au balayage, but actif %s\n",
                           coup, qPrintable(xy(c)), qPrintable(xy(game.getCaseBut(actif))));
                    break;
                }
            }
        }
    }

    printf("\n=== niveau %d — ordre %s ===\n", num, align ? "PAR ALIGNEMENT (regime loi)" : "par defaut");
    printf("poussees rejouees : %d | enfilages juges : %lld | balayages complets : %lld\n",
           coup, (long long)enfilages, (long long)balayages);
    if (fp == 0)
        printf("0 FAUX POSITIF — la loi ne coupe AUCUN etat de cette partie gagnante.\n"
               "  ⚠️ Elle n'est pas prouvee sure pour autant : une seule partie ne visite\n"
               "  qu'un chemin. C'est une non-refutation, pas une preuve (§6.6).\n");
    else
        printf("%lld FAUX POSITIFS PROUVES — la loi condamne des etats d'une partie GAGNEE.\n"
               "  L'elagage n'est donc pas sur : il peut retirer la seule branche gagnante.\n",
               (long long)fp);
    return fp ? 1 : 0;
}
