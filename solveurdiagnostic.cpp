#include "solveurdiagnostic.h"
#include <cstdio>

#ifdef INSTRUM_SONDE
StatsSonde& statsSonde() { static StatsSonde s; return s; }
#endif

// MUR MÉMOIRE — cf. plan.md §6.5 pour l'historique complet (le mur a été réel
// jusqu'au 2026-08-17 ; trois gains successifs — arène empaquetée, TableG et
// noeuds en tableaux parallèles — l'ont fait reculer sans le supprimer : la
// mémoire reste un PLAFOND, pas un problème résolu).
//
// ⚠️ On mesure la CAPACITÉ, pas l'occupation : un vecteur à moitié plein coûte
// son tableau entier, et c'est le coût réel qui arrête le solveur.
void imprimeMemoire(const char* quand, const Arene& arene, const TableG& meilleurG,
                    size_t noeudsOctets, size_t fileOctets, size_t etatsVus) {
    const double MO = 1024.0 * 1024.0;
    const size_t oArene   = arene.octets();
    const size_t oTable   = meilleurG.capacite() * TableG::octetsParCellule();
    const size_t oNoeuds  = noeudsOctets;
    const size_t oFile    = fileOctets;
    const size_t total    = oArene + oTable + oNoeuds + oFile;
    if (total == 0) return;
    fprintf(stderr,
            "[MEM %s] total %.0f Mo | arene %.0f (%.0f%%) | tableG %.0f (%.0f%%) | "
            "noeuds %.0f (%.0f%%) | file %.0f (%.0f%%) | %.1f o/etat vu | %zu cles de %d shorts\n",
            quand, total / MO,
            oArene / MO, 100.0 * oArene / total,
            oTable / MO, 100.0 * oTable / total,
            oNoeuds / MO, 100.0 * oNoeuds / total,
            oFile / MO, 100.0 * oFile / total,
            etatsVus ? (double)total / etatsVus : 0.0,
            arene.nbCles(), arene.getTaille());
    fprintf(stderr, "[MEM %s] tableG charge %.1f %% (%zu entrees / %zu cellules, %.0f Mo vides)\n",
            quand, meilleurG.capacite() ? 100.0 * meilleurG.size() / meilleurG.capacite() : 0.0,
            meilleurG.size(), meilleurG.capacite(),
            (meilleurG.capacite() - meilleurG.size()) * TableG::octetsParCellule() / MO);
#ifdef INSTRUM_SONDE
    // DELTA depuis le point précédent, pas le cumul : le cumul moyenne toutes
    // les charges traversées depuis le début et noie le coût d'une sonde À
    // CETTE charge-là — c'est ce couple (charge, sondes) qui tranche entre
    // serrer la table et garder le chemin chaud rapide.
    {
        static unsigned long long cA = 0, cS = 0, iA = 0, iS = 0;
        const StatsSonde& st = statsSonde();
        const unsigned long long dcA = st.chercheAppels - cA, dcS = st.chercheSondes - cS;
        const unsigned long long diA = st.insereAppels  - iA, diS = st.insereSondes  - iS;
        cA = st.chercheAppels; cS = st.chercheSondes; iA = st.insereAppels; iS = st.insereSondes;
        fprintf(stderr, "[SONDE %s] charge %.1f %% -> cherche %.2f sondes (%llu) | insere %.2f (%llu)\n",
                quand,
                meilleurG.capacite() ? 100.0 * meilleurG.size() / meilleurG.capacite() : 0.0,
                dcA ? (double)dcS / dcA : 0.0, dcA,
                diA ? (double)diS / diA : 0.0, diA);
    }
#endif
    fflush(stderr);
}

static StatsCorral s_statsCorral;
StatsCorral& statsCorral() { return s_statsCorral; }

// Lecture des deux étages : cf. plan.md §6.1 pour la preuve de correction du
// corral-N (gate + strip + BFS borné). Ici, juste la LECTURE des compteurs.
//   diffMort/diffVivant/diffInconnu : le verdict caché (étage 0, clé sans le
//     joueur) a été PROUVÉ pour une position donnée ; transféré à une zone
//     DIFFÉRENTE, il ne veut rien dire tant que l'étage 1 ne le recalcule pas.
//   etage1[MORT->vivant] : le SEUL faux positif prouvé (un état prune que le
//     sous-solve résout depuis la vraie position).
//   etage1[MORT->inconnu] : non tranché (budget épuisé, pas une preuve).
//   etage1[*->MORT] : prune MANQUÉ et PROUVÉ (l'exhaustion sous budget prouve).
void imprimeStatsCorral() {
    const StatsCorral& s = statsCorral();
    if (!s.enfilages) return;   // corral coupé (CORRAL=0), ou run sans enfilage
    fprintf(stderr,
        "[CORRAL-N] enfilages=%lld\n"
        "   portail BRUT  : %lld enfilages avec candidat (%.3f%%), %lld candidats\n"
        "   apres GATE    : %lld enfilages avec DUR      (%.3f%%), %lld durs\n"
        "   taille moy. d'un DUR : cells=%.1f  frontiere=%.1f  buts=%.1f\n",
        (long long)s.enfilages,
        (long long)s.avecCandidat, 100.0 * (double)s.avecCandidat / (double)s.enfilages,
        (long long)s.totCandidats,
        (long long)s.avecDur, 100.0 * (double)s.avecDur / (double)s.enfilages,
        (long long)s.totDurs,
        s.totDurs ? (double)s.totCells / (double)s.totDurs : 0.0,
        s.totDurs ? (double)s.totFrontiere / (double)s.totDurs : 0.0,
        s.totDurs ? (double)s.totButsVides / (double)s.totDurs : 0.0);
    const qint64 juges = s.dursMorts + s.dursVivants + s.dursInconnus;
    if (juges) {
        fprintf(stderr,
            "   STRIP+A* : durs juges=%lld  MORTS=%lld (%.1f%%)  vivants=%lld  inconnus=%lld\n"
            "              configs distinctes solvees=%lld  cache-hits=%lld (amortissement %.1fx)\n"
            "              etats de sous-solve=%lld (moy %.0f/config)  enfilages PRUNES=%lld\n",
            (long long)juges, (long long)s.dursMorts,
            juges ? 100.0 * (double)s.dursMorts / (double)juges : 0.0,
            (long long)s.dursVivants, (long long)s.dursInconnus,
            (long long)(juges - s.cacheHits), (long long)s.cacheHits,
            (juges - s.cacheHits) ? (double)juges / (double)(juges - s.cacheHits) : 0.0,
            (long long)s.solveStates,
            (juges - s.cacheHits) ? (double)s.solveStates / (double)(juges - s.cacheHits) : 0.0,
            (long long)s.enfilagesPrunes);
    }
    if (s.hitsTestes) {
        fprintf(stderr,
            "   [ETAGE 0 cle-joueur] hits testes=%lld  zone DIFFERENTE=%lld (%.2f%%)\n"
            "              dont verdict cache : MORT=%lld  vivant=%lld  inconnu=%lld\n",
            (long long)s.hitsTestes, (long long)s.hitsZoneDiff,
            100.0 * (double)s.hitsZoneDiff / (double)s.hitsTestes,
            (long long)s.diffMort, (long long)s.diffVivant, (long long)s.diffInconnu);
        const qint64 n1 = s.etage1[0]+s.etage1[1]+s.etage1[2]+s.etage1[3]+s.etage1[4]
                        + s.etage1[5]+s.etage1[6]+s.etage1[7]+s.etage1[8];
        if (n1) {
            static const char* nom[3] = {"MORT   ", "vivant ", "inconnu"};
            fprintf(stderr, "   [ETAGE 1 recalcul] %lld collisions rejugees pour la VRAIE position\n"
                            "              cache \\ vrai :     MORT    vivant   inconnu\n", (long long)n1);
            for (int c = 0; c < 3; c++)
                fprintf(stderr, "                 %s : %8lld %8lld %8lld\n", nom[c],
                        (long long)s.etage1[3*c], (long long)s.etage1[3*c+1], (long long)s.etage1[3*c+2]);
            fprintf(stderr, "              => FP PROUVES (MORT->vivant)=%lld | non tranches (MORT->inconnu)=%lld\n"
                            "                 prunes MANQUES PROUVES (->MORT)=%lld   (etats de recalcul=%lld)\n",
                    (long long)s.etage1[1], (long long)s.etage1[2],
                    (long long)(s.etage1[3] + s.etage1[6]),
                    (long long)s.etage1States);
            if (s.arbitre[0] + s.arbitre[1] + s.arbitre[2]) {
                fprintf(stderr, "   [ARBITRAGE budget large] %lld cas MORT->inconnu rejuges :\n"
                                "              MORT (transfert LEGITIME)=%lld | vivant (FAUX POSITIF PROUVE)=%lld"
                                " | toujours inconnu=%lld   (etats=%lld)\n",
                        (long long)(s.arbitre[0] + s.arbitre[1] + s.arbitre[2]),
                        (long long)s.arbitre[0], (long long)s.arbitre[1], (long long)s.arbitre[2],
                        (long long)s.arbitreStates);
            }
        }
    }
    fflush(stderr);
}

#ifdef DUMP_DEV
std::vector<std::pair<QByteArray,int>>& etatsDeveloppes() {
    static std::vector<std::pair<QByteArray,int>> v;
    return v;
}
int& limiteDepilements() {
    static int n = 0;
    return n;
}
#endif

#ifdef INSTRUM_F
void imprimeHistoF(const std::vector<qint64>& histoF, int cStar, qint64 total) {
    qint64 sousCStar = 0, aCStar = 0;
    qint64 mouProuve = 0;   // somme des (C* - f), le mou minimal garanti

    for (size_t f = 0; f < histoF.size(); ++f) {
        if (!histoF[f]) continue;
        if ((int)f < cStar) { sousCStar += histoF[f]; mouProuve += histoF[f] * (cStar - (int)f); }
        else if ((int)f == cStar) aCStar += histoF[f];
    }

    printf("\n-- HISTOGRAMME DES f AU DEPILEMENT (C* = %d) --\n", cStar);
    for (size_t f = 0; f < histoF.size(); ++f)
        if (histoF[f])
            printf("   f = %3zu %s : %10lld  (%.1f %%)\n", f,
                   (int)f == cStar ? "=C*" : "<C*",
                   (long long)histoF[f], 100.0 * histoF[f] / total);

    printf("   ----\n");
    printf("   f <  C* : %10lld  (%.1f %%)  <- mou PROUVE, elaguables par une h plus serree\n",
           (long long)sousCStar, 100.0 * sousCStar / total);
    printf("   f == C* : %10lld  (%.1f %%)  <- a la limite : f seul ne peut PAS les distinguer\n",
           (long long)aCStar, 100.0 * aCStar / total);
    printf("   mou moyen prouve sur les f < C* : %.2f poussees\n",
           sousCStar ? (double)mouProuve / sousCStar : 0.0);
    fflush(stdout);
}
#endif

#ifdef INSTRUM_DELTAF
StatsDeltaF& statsDeltaF() {
    static StatsDeltaF s;
    return s;
}
#endif
