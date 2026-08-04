#include <QtDebug>
#include <QSet>
#include <cstdio>
#include <algorithm>
#include <climits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "solveurastar.h"

// Tas-MIN sur f. push_heap/pop_heap construisent un tas-MAX au sens du
// comparateur : comp(a, b) répond « a passe APRÈS b ». En rendant vrai quand
// a.f > b.f, le « plus grand » devient celui de plus petit f — c'est lui qui se
// retrouve à la racine, donc dépilé en premier.
//
// À f égal, on préfère le g le plus GRAND : l'état le plus profond est le plus
// proche du but, ce qui fait plonger A* vers la solution au lieu de balayer tout
// un palier.
static bool compare(const SolveurAStar::SElement& a, const SolveurAStar::SElement& b) {
    if (a.f != b.f) return a.f > b.f;
    if (a.g != b.g) return a.g < b.g;
    // Départage (§10.2) : à f et g égaux, l'état au score de guidage le plus PETIT
    // passe devant (rangement dans l'ordre canonique des buts). Pur ordre de visite,
    // sans effet sur l'optimalité — attaque la multiplicité des entrelacements (§9.4).
    return a.guidage > b.guidage;
}

SolveurAStar::SolveurAStar(const Game &etatDepart, int poids, bool macro, QObject *parent,
                           bool macroCouplage, bool plongeon, bool ordreCoins)
    : Solveur(etatDepart, parent), poids(poids), macro(macro), macroCouplage(macroCouplage),
      ordreCoins(ordreCoins), plongeon(plongeon) {
}

// ── BUTS EN COIN (régime 'ordreCoins', cf. solveurastar.h) ────────────────────
// Deux tables statiques, calculées UNE fois par solve depuis l'API publique de
// Game (murs + ordreButs) : rien à ajouter dans Game, rien à maintenir en double.
//   rangDeCase[cell]  : rang de remplissage du but occupant cette case, sinon -1
//   coinDeCase[cell]  : ce but est-il TERMINAL (aucune poussée sortante) ?
// « Terminal » se lit sur les murs seuls : pour sortir une caisse d'une case il
// faut la case d'arrivée ET la case d'appui libres de mur, sur le même axe.
void SolveurAStar::construitTablesCoins(const Game& g) {
    const int L = g.getLargeur(), H = g.getHauteur(), N = L * H;
    rangDeCase.fill(-1, N);
    coinDeCase.fill(false, N);
    const QVector<int>& ordre = g.getOrdreButs();
    static const int dxx[4] = {1, -1, 0, 0}, dyy[4] = {0, 0, 1, -1};
    for (int k = 0; k < ordre.size(); k++) {
        const int cell = g.getCaseBut(ordre[k]);
        rangDeCase[cell] = k;
        const int x = cell % L, y = cell / L;
        bool sortie = false;
        for (int d = 0; d < 4 && !sortie; d++) {
            const int ax = x + dxx[d], ay = y + dyy[d];    // arrivée de la caisse
            const int bx = x - dxx[d], by = y - dyy[d];    // appui du joueur
            if (ax < 0 || ax >= L || ay < 0 || ay >= H) continue;
            if (bx < 0 || bx >= L || by < 0 || by >= H) continue;
            if (g.getCase(ax + ay * L) != Level::tcMur && g.getCase(bx + by * L) != Level::tcMur)
                sortie = true;
        }
        coinDeCase[cell] = !sortie;
    }
    int n = 0; for (bool b : coinDeCase) if (b) n++;
    fprintf(stderr, "[COINS] regime ordreCoins ACTIF — %d buts en coin sur %d.\n",
            n, (int)ordre.size());
    fflush(stderr);
}

// Le test lui-même, aux DEUX points d'enfilage (le plongeon en est un — leçon du
// 2026-08-03). Vrai = la poussée dépose une caisse sur un but en coin qui n'est
// pas encore à son tour → on coupe.
bool SolveurAStar::coinTropTot(const Game& e, int arrivee) const {
    if (!ordreCoins || arrivee < 0) return false;
    if (arrivee >= rangDeCase.size() || rangDeCase[arrivee] < 0) return false;
    if (!coinDeCase[arrivee]) return false;
    const int actif = e.butActif();
    if (actif < 0) return false;                       // plus de but : état gagnant
    const int cellActif = e.getCaseBut(actif);
    return rangDeCase[arrivee] > rangDeCase[cellActif];
}

#ifdef DUMP_DEV
// Uniquement pour l'instrumentation hors-ligne (harnais de mesure). Un seul
// thread solveur tourne à la fois, pas de verrou.
std::vector<std::pair<QByteArray,int>>& etatsDeveloppes() {
    static std::vector<std::pair<QByteArray,int>> v;
    return v;
}
// Plafond de dépilements, pour instrumenter un niveau qu'on NE SAIT PAS résoudre
// (le 11 : `mou` ne peut rien y mesurer, il attend une solution qui n'arrive pas).
// 0 = pas de plafond → comportement de `mou` strictement inchangé.
int& limiteDepilements() {
    static int n = 0;
    return n;
}
#endif

#ifdef INSTRUM_F
// cStar = g de l'état gagnant = le coût optimal.
static void imprimeHistoF(const std::vector<qint64>& histoF, int cStar, qint64 total) {
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


// LIVRAISON=5 : le test de livraison s'applique aux états ENFILÉS (cf. game.h).
// Interrupteur de mesure, à retirer avec le verdict.
static const bool livraisonSurEnfants = (qgetenv("LIVRAISON").toInt() == 5);

// corralActif() : déclaré dans solveurastar.h, même raison que CORRAL_BUDGET.

// CORRAL_BUDGET : déclaré dans solveurastar.h depuis le 2026-08-01 — le mode
// hybride rejoue l'enfilage dans l'UI et doit passer le MÊME budget, sinon les
// deux régimes divergeraient en silence (§7).

// Stats du corral-N, agrégées sur tout le solve puis imprimées sur stderr en fin
// de run(). Runtime, pas de #ifdef : la fraction de durs prouvés morts est ce qui
// PRÉDIT le gain sur un niveau neuf (§6.1), on veut la lire sans recompiler. Coût
// nul devant le flood-fill de l'enfilage. Un seul solve par process (bench).
struct StatsCorral {
    qint64 enfilages = 0;
    qint64 avecCandidat = 0, totCandidats = 0;   // portail brut
    qint64 avecDur = 0, totDurs = 0;             // après gate Hall + non-rouvrable
    qint64 totCells = 0, totFrontiere = 0, totButsVides = 0;   // sur les DURS
    qint64 dursMorts = 0, dursVivants = 0, dursInconnus = 0;   // verdict strip + A*
    qint64 cacheHits = 0, solveStates = 0;
    qint64 enfilagesPrunes = 0;                  // enfilages coupés par une mort prouvée
    // Étage 0 « clé du cache sans le joueur » (CACHE_JOUEUR=1, cf. game.cpp).
    qint64 hitsTestes = 0, hitsZoneDiff = 0;
    qint64 diffMort = 0, diffVivant = 0, diffInconnu = 0;
    qint64 etage1[9] = {0,0,0,0,0,0,0,0,0};      // croisement cache × recalcul
    qint64 etage1States = 0;
    qint64 arbitre[3] = {0,0,0};                 // MORT→inconnu rejugés à budget large
    qint64 arbitreStates = 0;
};
static StatsCorral& statsCorral() { static StatsCorral s; return s; }

// Stats du PAQUET NON LIVRABLE (chantier). Mêmes raisons que StatsCorral :
// runtime, imprimées en fin de run(), lues sans recompiler.
struct StatsPaquet {
    qint64 enfilages = 0, testes = 0;
    qint64 morts = 0, vivants = 0, inconnus = 0;
    qint64 cacheHits = 0, solveStates = 0, prunes = 0;
};
static StatsPaquet& statsPaquet() { static StatsPaquet s; return s; }

// Le paquet 8-connexe de caisses HORS BUT contenant 'depart'. Rendu TRIÉ : c'est
// la clé de mémoïsation, et deux ordres différents casseraient le cache en silence.
// Vide si 'depart' ne porte pas une caisse hors but (caisse posée sur un but : elle
// est légitimement immobile, cf. checkDefaite qui ne teste que les tcCaisse).
static void paquetHorsBut(const Game& g, int depart, QVarLengthArray<int, 32>& out) {
    out.clear();
    if (g.getCase(depart) != Level::tcCaisse) return;
    const int L = g.getLargeur(), H = g.getHauteur();
    QVarLengthArray<int, 32> pile;
    pile.append(depart); out.append(depart);
    while (!pile.isEmpty()) {
        const int i = pile.last(); pile.removeLast();
        const int x = i % L, y = i / L;
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++) {
                if (!dx && !dy) continue;
                const int nx = x + dx, ny = y + dy;
                if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
                const int n = nx + ny * L;
                if (g.getCase(n) != Level::tcCaisse) continue;
                if (out.contains(n)) continue;      // out reste petit (≤ nbCaisses)
                out.append(n); pile.append(n);
            }
    }
    std::sort(out.begin(), out.end());
}
static void imprimeStatsPaquet() {
    const StatsPaquet& s = statsPaquet();
    if (!s.enfilages) return;
    fprintf(stderr,
        "[PAQUET] ⚠️ ELAGAGE DE CHANTIER ACTIF (PAQUET=1) — ce run n'est PAS le defaut.\n"
        "   enfilages=%lld  testes=%lld (%.1f%%)  |  MORTS=%lld  vivants=%lld  inconnus=%lld\n"
        "   PRUNES=%lld  |  cache-hits=%lld (amortissement %.1fx)  etats de sous-solve=%lld\n",
        (long long)s.enfilages, (long long)s.testes,
        100.0 * (double)s.testes / (double)s.enfilages,
        (long long)s.morts, (long long)s.vivants, (long long)s.inconnus,
        (long long)s.prunes, (long long)s.cacheHits,
        (s.testes - s.cacheHits) ? (double)s.testes / (double)(s.testes - s.cacheHits) : 0.0,
        (long long)s.solveStates);
    fflush(stderr);
}

static void imprimeStatsCorral() {
    imprimeStatsPaquet();
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
    // ÉTAGE 0 (plan.md §6.1) — le verdict caché a été PROUVÉ pour une position de
    // joueur donnée ; combien de fois est-il transféré à une zone DIFFÉRENTE ?
    //   diffMort    → prunes potentiellement INJUSTIFIÉS (le faux positif redouté)
    //   diffVivant  } → prunes potentiellement MANQUÉS : c'est le GAIN possible,
    //   diffInconnu }   et l'inconnu est le plus suspect (verdict par défaut, vide)
    // Le désaccord de verdict lui-même n'est PAS mesuré ici : « zone différente » ne
    // veut pas dire « verdict différent ». C'est l'étage 1 (recalcul du sous-solve
    // sur les seules collisions) qui tranche, et lui seul.
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
            // ⚠️ LECTURE, ET ELLE EST ASYMÉTRIQUE — « inconnu » n'est PAS « vivant » :
            // c'est « budget épuisé sans conclure », donc ça ne prouve RIEN.
            //   MORT → vivant  : le SEUL faux positif prouvé (on a prune un état que
            //                    le sous-solve résout depuis la vraie position).
            //   MORT → inconnu : NON TRANCHÉ. Le prune repose sur un verdict qu'on ne
            //                    sait pas reproduire ici, mais rien ne dit qu'il est
            //                    faux — il faut un budget plus large pour conclure.
            //   * → MORT       : prune MANQUÉ, et celui-là est PROUVÉ (l'exhaustion
            //                    sous budget est une preuve, cf. sousSolveEnclos).
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


// PLONGEON SUR RECORD (§6.0) — le budget est une FRACTION DU TRAVAIL DÉJÀ FAIT,
// et c'est tout le réglage. Ni seuil de remplissage, ni budget fixe.
//
// POURQUOI PAS UN SEUIL EN % DE BUTS REMPLIS (essayé le 2026-07-28, abandonné) :
// aucune valeur ne convient. 80 % gagne sur le 4 (×32,6) et le 9 (×4,1) ; 66 %
// gagne sur le 8 (×3,7) et le 3 (×2,5) mais DÉGRADE le 2 (+2 poussées) et le 5
// (+8). Et le 8 a tranché la question : son record complétable le plus précoce est
// à 28 % du plateau (5/18, finissable en 1 133 états) — alors qu'au MÊME
// pourcentage, le 9 a des records MORTS qui coûtent 59 771 états à réfuter. Le
// pourcentage ne distingue donc pas les deux cas : ce n'est pas la bonne variable.
//
// CE QUI LES DISTINGUE, c'est le travail déjà consenti. Le 8 atteint son 5/18
// après 158 000 dépilements, le 9 ses records morts après ~1 000. D'où la règle :
//
//     budget du plongeon = (états déjà développés) / PLONGEON_DIVISEUR
//
// Plus on a ramé, plus il est rationnel de parier. Les conséquences tombent toutes
// seules, sans réglage par plateau :
//   - le 2 (412 états au total) n'accorde jamais assez de budget pour qu'un
//     plongeon aboutisse → il ne plonge JAMAIS et garde son optimum, par
//     construction et non par un seuil ;
//   - les plongeons ruineux du 9 sont étouffés : à ce stade le budget vaut ~10 ;
//   - le 8 à 158 000 dépilements dispose de 1 580, de quoi payer ses 1 133.
// Le paramètre restant porte sur le COMPORTEMENT OBSERVÉ du solveur, pas sur une
// propriété du plateau devinée — il a donc une chance de tenir sur un niveau
// jamais vu, ce qu'aucun pourcentage calé sur 8 plateaux ne peut promettre.
//
// LE DIVISEUR, BALAYÉ puis FIGÉ le 2026-07-28 (méthode CORRAL_BUDGET : on ne fige
// qu'après avoir mesuré les deux bords). Plage sûre mesurée : **[1/20, 1/100]**,
// bornée par deux mécanismes opposés —
//   - EN HAUT (budget trop généreux) : à 1/15 le niveau 2 dérive à 133 poussées,
//     et à 1/10 à 139. On plonge trop tôt, depuis un chemin qui a déjà dévié.
//   - EN BAS (budget trop maigre) : à 1/500 le 4 REPERD tout (67 159 états au lieu
//     de 2 115) — il lui faut 19 états à ~2 000 dépilements, donc un diviseur ≤ 105 ;
//     et le 8 retombe à 1 174 706 (il lui faut 340 états à 158 000, donc ≤ 464).
// **1/50 est au centre** : ×2,5 de marge avant le bord haut, ×10 avant le bord bas.
// Le premier réglage retenu (1/100) était à la limite basse et laissait **×6,9 sur
// le 8** (159 484 contre 22 991) — d'où la règle : ne jamais figer sans balayer.
static const int PLONGEON_DIVISEUR = 50;

int SolveurAStar::plonge(const Game& etatDepart, int gDepart, int idxNoeudDepart,
                         QHash<QByteArray,Game::VerdictEnclos>& cacheEnclos,
                         QHash<QByteArray,int>& cachePaquet, int budget, qint64* etatsOut) {
    // Un échec ne doit RIEN laisser derrière lui : on rend 'noeuds' à sa taille
    // d'avant. Sans ça, chaque plongeon raté enflerait définitivement l'arbre de
    // reconstruction de la recherche principale.
    const int noeudsAvant = noeuds.size();

    // Les états vivent dans un vecteur qui ne fait que croître ; le tas ne porte
    // que (h, index), donc aucun Game n'est recopié pendant les push_heap.
    std::vector<Game> etats;
    std::vector<int> gs, idxN;
    std::vector<std::pair<int,int>> tas;

    // Tas-MIN sur h SEUL — c'est toute la différence avec la recherche principale :
    // on ignore g, donc on fonce vers le but au lieu de développer les paliers.
    // À h égal, le plus PROFOND d'abord (même raison : plonger, pas balayer).
    auto apres = [&gs](const std::pair<int,int>& a, const std::pair<int,int>& b) {
        if (a.first != b.first) return a.first > b.first;
        return gs[a.second] < gs[b.second];
    };

    QSet<QByteArray> vus;
    QVector<bool> zone, zoneEnfant, visiteCorral;
    qint64 developpes = 0;
    int idxGagnant = -1;

    etats.push_back(etatDepart);
    gs.push_back(gDepart);
    idxN.push_back(idxNoeudDepart);
    vus.insert(etatDepart.getEtat());
    tas.push_back({etatDepart.getHeuristique(nullptr), 0});

    // Ajoute un enfant : mêmes élagages que l'enfilage principal (corral unitaire
    // puis corral-N, sur le même cache mémoïsé — un enclos déjà jugé ne se rejuge
    // pas), même chaînage des noeuds pour que reconstruire() rejoue la macro.
    auto ajoute = [&](Game& c, int gC, const QVector<QPair<int,int>>& chaine, int parent) {
        const int arrivee = chaine.isEmpty() ? -1
            : c.caseApres(chaine.last().first, (Game::EDirection)chaine.last().second);
        if (corralActif() && arrivee >= 0) {
            if (c.corralUnitaireMort(arrivee)) return;
            c.getZoneJoueur(zoneEnfant);
            const Game::EnclosInfo inf = c.detecteEnclosArrivee(arrivee, zoneEnfant, visiteCorral,
                                                                &cacheEnclos, CORRAL_BUDGET);
            if (inf.dursMorts > 0) return;
        }
        if (coinTropTot(c, arrivee)) return;
        // PAQUET NON LIVRABLE — même test qu'à l'enfilage principal, même cache.
        // Sans lui le plongeon explorait des états que la recherche refusait.
        if (paquetActif() && arrivee >= 0) {
            StatsPaquet& sp = statsPaquet();
            sp.enfilages++;
            QVarLengthArray<int, 32> grp;
            paquetHorsBut(c, arrivee, grp);
            if (grp.size() >= 2) {
                sp.testes++;
                const QByteArray k(reinterpret_cast<const char*>(grp.constData()),
                                   grp.size() * (int)sizeof(int));
                auto it = cachePaquet.constFind(k);
                int v;
                if (it != cachePaquet.constEnd()) { v = *it; sp.cacheHits++; }
                else {
                    int dev = 0;
                    v = c.sousSolveEnclos(grp, paquetBudget(), &dev);
                    sp.solveStates += dev;
                    cachePaquet.insert(k, v);
                }
                if (v == 0)  { sp.morts++; sp.prunes++; return; }
                if (v == -1) sp.inconnus++; else sp.vivants++;
            }
        }
        const QByteArray cle = c.getEtat();
        if (vus.contains(cle)) return;
        vus.insert(cle);

        int p = parent;
        for (const auto& q : chaine) {
            noeuds.append(Noeud{p, (quint16)q.first, (quint8)q.second});
            p = noeuds.size() - 1;
        }
        etats.push_back(c);
        gs.push_back(gC);
        idxN.push_back(p);
        tas.push_back({c.getHeuristique(nullptr), (int)etats.size() - 1});
        std::push_heap(tas.begin(), tas.end(), apres);
    };

    while (!tas.empty() && developpes < budget) {
        if (arretDemande()) break;

        std::pop_heap(tas.begin(), tas.end(), apres);
        const int i = tas.back().second;
        tas.pop_back();
        developpes++;

        // COPIE et non référence : 'etats' grossit dans la boucle ci-dessous, et
        // une réallocation invaliderait une référence dans son dos.
        Game e = etats[i];
        if (e.isGagne()) { idxGagnant = idxN[i]; break; }

        e.getZoneJoueur(zone);
        const QVector<quint8> caisses = e.getCaissesDeplacable(zone);

        // Même régime d'engagement que la recherche principale : si le but actif
        // est atteignable, on ne génère QUE les macros qui l'y envoient.
        int macrosOk = 0;
        const int but = e.butActif();
        if (but >= 0) {
            for (int c = 0; c < caisses.size(); c++) {
                if (caisses[c] == 0) continue;
                if (!e.macroPeutDemarrer(c, but, zone)) continue;
                Game f(e);
                QVector<QPair<int,int>> poussees;
                if (f.macroVersButBacktrack(c, but, poussees) && !f.isPerdu()) {
                    ajoute(f, gs[i] + poussees.size(), poussees, idxN[i]);
                    macrosOk++;
                }
            }
        }
        if (macrosOk == 0) {
            for (int c = 0; c < caisses.size(); c++) {
                const quint8 dirs = caisses[c];
                for (int d = 0; d < NB_DIRECTION; d++) {
                    if (!(dirs & (1 << d))) continue;
                    Game f(e);
                    if (f.pousse(c, (Game::EDirection)d) && !f.isPerdu())
                        ajoute(f, gs[i] + 1, {{c, d}}, idxN[i]);
                }
            }
        }
    }

    if (etatsOut) *etatsOut = developpes;
    if (idxGagnant < 0) noeuds.resize(noeudsAvant);
    return idxGagnant;
}

void SolveurAStar::run() {
    std::vector<SElement> file;
    qint64 compteur = 0;
#ifdef INSTRUM_F
    std::vector<qint64> histoF;
#endif

    // Toutes les clés du solve vivent ici, bout à bout (cf. cle.h). Les
    // conteneurs ci-dessous n'en portent que des références de 4 octets.
    //
    // meilleurG est à ADRESSAGE OUVERT (TableG, cf. cle.h) et non un
    // std::unordered_map : la map chaînée payait ~40 o d'infrastructure par
    // entrée (noeud alloué un par un + seau) pour 8 o utiles, ce qui en faisait
    // le premier poste mémoire du solveur — ~800 Mo sur le niveau 3.
    Arene arene(depart.tailleCle());
    TableG meilleurG(&arene);

    // Ensemble des états DÉJÀ DÉVELOPPÉS. Uniquement en mode pondéré.
    //
    // Depuis que h tient compte de l'accessibilité du joueur, elle n'est plus
    // COHÉRENTE : une poussée déplace le joueur, or la contribution de TOUTES les
    // autres caisses dépend de sa position (elle se lit dans leur région). h peut
    // donc sauter de plusieurs unités en une seule poussée, alors que le coût, lui,
    // n'augmente que de 1. Elle reste admissible (jamais de surestimation), mais la
    // garantie « premier dépilement = g optimal » tombe.
    //
    // Conséquence : un état est développé, puis redécouvert par un meilleur chemin,
    // ré-enfilé, redéveloppé. Mesuré sur le niveau 17 : 4 264 544 dépilements pour
    // 1 659 245 états distincts — 2,6x de travail en pur re-développement.
    //
    // En mode OPTIMAL (poids == 1), ce re-développement n'est pas du gaspillage :
    // c'est LUI qui rétablit l'optimalité face à une h incohérente. On le garde.
    //
    // En mode PONDÉRÉ, l'optimalité est déjà abandonnée. On interdit donc le
    // re-développement : la solution reste bornée par w * C*, et on récupère le
    // facteur 2,6.
    const bool interditRedeveloppement = (poids > 1);
    std::unordered_set<Cle,CleHash,CleEq> ferme(1024, CleHash{&arene}, CleEq{&arene});

    // 'noeuds' appartient à la classe de base et survit d'une résolution à
    // l'autre : sans ce reset, la racine ne serait pas à l'indice 0 et le premier
    // enfant deviendrait son propre parent — reconstruire() boucherait à l'infini.
    noeuds.clear();
    noeuds.append(Noeud{-1, 0, 0});   // racine : aucune poussée ne la précède (idxCaisse/dir jamais lus)

    depart.getEtat(arene.reserve());
    const Cle cleDepart{arene.dernier()};
    meilleurG.insere(cleDepart, 0);

    qint64 scoreDepart;
    const int hDepart = depart.getHeuristique(&scoreDepart);
    file.push_back({poids * hDepart, 0, 0, cleDepart, scoreDepart});
    std::push_heap(file.begin(), file.end(), compare);

    // État de travail RÉUTILISÉ d'un dépilement à l'autre : appliqueEtat()
    // réécrit intégralement le plateau, donc pas besoin d'un Game neuf à chaque
    // tour. Surtout, on part d'une copie de 'depart' pour hériter de casesMortes
    // et distancePoussee (QVector en partage implicite → copie quasi gratuite)
    // sans jamais relancer calculDistancePoussee(), qui est en O(size²).
    Game etat(depart);
    int fileAvant = 0;   // taille de la file au dernier affichage (tendance)
    int maxRangees = 0;  // plus grand nombre de caisses rangées atteint (jauge de blocage)
    int plongeons = 0;   // tentatives de plongeon (régime plongeon seulement)
    qint64 etatsPlongeon = 0;   // ... et ce qu'elles ont coûté, réussies ou non

    // Tampons de flood-fill, hissés HORS de la boucle : réutilisés d'un état à
    // l'autre, ils ne réallouent plus (cf. getZoneJoueur(QVector<bool>&)). Ne
    // JAMAIS en garder une copie ailleurs, sinon le fill() détache et réalloue.
    QVector<bool> zone;        // zone de l'état développé
    QVector<bool> zoneEnfant;  // zone de l'enfant qu'on enfile
    QVector<bool> visiteCorral; // tampon de la détection d'enclos (corral-N)
    // Mémoïsation du sous-solve d'enclos, par frontière triée : le verdict ne
    // dépend QUE d'elle (tout le reste — murs, buts, cases mortes — est statique),
    // et le même corral revient des centaines de fois. C'est ce qui rend le coût
    // soutenable malgré 10-21 % d'enfilages qui déclenchent une preuve. Local au
    // run : rien à réinitialiser d'un solve à l'autre.
    if (ordreCoins) construitTablesCoins(etat);
    QHash<QByteArray,Game::VerdictEnclos> cacheEnclos;
    // Mémoïsation du paquet non livrable (chantier PAQUET=1). Même argument que
    // cacheEnclos : le verdict ne dépend que du paquet trié. Mesuré ×223 à ×1040
    // d'amortissement par l'outil `paquet`.
    QHash<QByteArray,int> cachePaquet;

    while(file.size()) {
        // Arrêt demandé depuis l'UI : on sort AVANT de dépiler, de sorte que le
        // compteur affiché soit bien le nombre d'états réellement développés.
        // Tout meurt avec la pile de run() (arène, file, tables) — rien à
        // libérer à la main.
        if (arretDemande()) {
            qDebug() << "SolveurAStar: arret demande apres" << compteur << "etats explores.";
            emit rechercheArretee(compteur);
            return;
        }

        // pop_heap n'enlève rien : il amène le meilleur élément en DERNIÈRE
        // position et réordonne le reste. C'est pop_back() qui le retire — et
        // entre les deux, on peut le VOLER (std::move) au lieu de le copier.
        std::pop_heap(file.begin(), file.end(), compare);
        SElement cur = std::move(file.back());
        file.pop_back();

        // Entrée périmée : un meilleur chemin vers ce même état a été trouvé
        // APRÈS qu'on ait enfilé celle-ci. On la jette sans la compter.
        const TableG::Slot* slotCur = meilleurG.cherche(cur.cle);
        if(slotCur && cur.g > slotCur->g) continue;

        if (interditRedeveloppement) {
            if (ferme.count(cur.cle)) continue;
            ferme.insert(cur.cle);
        }

        compteur++;

#ifdef INSTRUM_F
        // Combien de mou reste-t-il à gratter dans h ? Pour TOUT état développé,
        // le meilleur chemin qui le traverse coûte au moins l'optimum :
        //     g(s) + h*(s) >= C*   =>   h*(s) >= C* - g(s)
        // donc le mou de h sur cet état vaut au minimum
        //     mou(s) = h*(s) - h(s) >= C* - f(s).
        // Autrement dit : tout état développé avec f < C* a un mou PROUVÉ, connu
        // gratuitement, sans jamais résoudre depuis lui. L'histogramme des f au
        // dépilement borne donc par le bas ce qu'une h plus serrée pourrait
        // élaguer. (C* n'est connu qu'à la fin : on garde les f et on conclut là.)
        if ((size_t)cur.f >= histoF.size()) histoF.resize(cur.f + 1, 0);
        histoF[cur.f]++;
#endif

        // Le Game n'était pas dans la file : on le reconstruit depuis la clé.
        // appliqueEtat renvoie gratuitement le nombre de caisses déjà rangées.
        const int rangees = etat.appliqueEtat(arene.lit(cur.cle.offset));
        if (rangees > maxRangees) {
            maxRangees = rangees;
            // Copie figée pour l'UI (§10) + le chemin qui y mène, pour le rejeu pas
            // à pas d'un run qui n'aboutit pas.
            emit nouveauMaxCaisses(etat, rangees, reconstruire(cur.idxNoeud));

            // PLONGEON SUR RECORD (§6.0) — régime d'essai. A* optimal ne « fonce »
            // jamais : il doit vider toute la masse f < C* avant de descendre, même
            // quand il tient déjà un état complétable en 13 coups (mesuré sur le 4 :
            // 16/20 caisses posées dès 2 000 dépilements, puis 65 000 états pour
            // finir). On tente donc de le compléter tout de suite, gloutonnement.
            // Budget = fraction du travail déjà fait (cf. ci-dessus). Nul au
            // démarrage : on ne plonge pas tant qu'on n'a rien investi.
            const int budgetPlongeon = plongeon ? (int)(compteur / PLONGEON_DIVISEUR) : 0;
            if (budgetPlongeon > 0) {
                qint64 devPlongeon = 0;
                const int idxGagnant = plonge(etat, cur.g, cur.idxNoeud, cacheEnclos, cachePaquet,
                                              budgetPlongeon, &devPlongeon);
                // Ces états sont RÉELLEMENT développés : les compter, sinon le
                // compteur du régime plongeon ne serait pas comparable au défaut.
                compteur += devPlongeon;
                plongeons++;
                etatsPlongeon += devPlongeon;

                // UNE LIGNE PAR TENTATIVE, réussie ou non, sur stderr comme la
                // jauge : sur un run long, les échecs sont la seule façon de voir
                // ce que le plongeon coûte AVANT la fin — et un run qu'on arrête à
                // la main n'imprime jamais son bilan. (Manque constaté sur le 11 le
                // 2026-07-28 : 14 minutes sans savoir s'il avait seulement tenté.)
                qDebug().nospace()
                    << "[plongeon " << plongeons << "] record " << rangees << "/"
                    << etat.getNbButs() << " a " << compteur << " depiles"
                    << " | budget " << budgetPlongeon
                    << " -> " << (idxGagnant >= 0 ? "REUSSI" : "echec")
                    << " en " << devPlongeon << " etats"
                    << " | cumul plongeons " << etatsPlongeon
                    << " (" << (compteur ? 100.0 * (double)etatsPlongeon / (double)compteur : 0.0)
                    << " % du travail)";

                if (idxGagnant >= 0) {
                    qDebug() << "SolveurAStar: PLONGEON reussi depuis" << rangees << "/"
                             << etat.getNbButs() << "caisses posees, apres" << devPlongeon
                             << "etats de plongeon (budget" << budgetPlongeon << ","
                             << plongeons << "plongeons au total).";
                    qDebug() << "SolveurAStar: solution trouvee apres" << compteur
                             << "etats explores.";
                    imprimeStatsCorral();
                    emit solutionTrouvee(reconstruire(idxGagnant), compteur);
                    return;
                }
            }
        }

        if (compteur % 1000 == 0) {
            // Diagnostic : TENDANCE de la file (Δ depuis le dernier point), reste
            // ESTIMÉ h = f - g (descend vers 0 = fin proche), et CAISSES RANGÉES
            // (courant + MAX atteint / nbButs). Voir plan §10 (jauges de convergence).
            const int dfile = (int)file.size() - fileAvant;
            fileAvant = (int)file.size();
            const char* tend = dfile > 100 ? "MONTE" : (dfile < -100 ? "DESCEND" : "stagne");
            qDebug().nospace()
                << "w" << poids << " | " << compteur << " depiles"
                << " | file " << file.size() << " (" << (dfile >= 0 ? "+" : "") << dfile << " " << tend << ")"
                << " | vus " << meilleurG.size()
                << " | f " << cur.f << " h(reste) " << (cur.f - cur.g)
                << " | rangees " << rangees << " (max " << maxRangees << ")/" << etat.getNbButs();
            // ⚠️ Les stats de chantier partent AVEC la jauge et pas seulement en fin
            // de run() : un run tué (c'est le cas de tous les non-résolus, donc de
            // toutes les cibles) n'en rendait aucune. Même trou que [CORRAL-N] et que
            // le profilage du §6.6 — seul ce qui part en continu se relève.
            if (paquetActif()) {
                const StatsPaquet& p = statsPaquet();
                const qint64 calculs = p.testes - p.cacheHits;   // vrais sous-solves
                fprintf(stderr, "[PAQUET] testes=%lld MORTS=%lld PRUNES=%lld inconnus=%lld"
                                " | PAQUETS DISTINCTS=%lld (amorti %.1fx, %.0f etats/calcul)"
                                " | sous-solve=%lld etats (%.1f par depilement)\n",
                        (long long)p.testes, (long long)p.morts, (long long)p.prunes,
                        (long long)p.inconnus, (long long)calculs,
                        calculs ? (double)p.testes / (double)calculs : 0.0,
                        calculs ? (double)p.solveStates / (double)calculs : 0.0,
                        (long long)p.solveStates,
                        compteur ? (double)p.solveStates / (double)compteur : 0.0);
                fflush(stderr);
            }
        }

#ifdef DUMP_DEV
        // Les états RÉELLEMENT dépilés — et non l'ensemble {f <= C*}, qui est
        // ~25x plus gros : A* s'arrête dès qu'il atteint le but et n'en visite
        // qu'une fraction. Échantillonner {f <= C*} au lieu de ceci fausse toute
        // mesure portant sur « ce que le solveur explore vraiment ».
        etatsDeveloppes().push_back({etat.getEtat(), cur.g});
        if (limiteDepilements() && (int)etatsDeveloppes().size() >= limiteDepilements()) {
            qDebug() << "SolveurAStar: PLAFOND d'instrumentation atteint apres"
                     << compteur << "depilements — arret volontaire, ce n'est PAS un echec.";
            emit aucuneSolution();
            return;
        }
#endif

        if(etat.isGagne()) {
            qDebug() << "SolveurAStar: solution trouvee apres" << compteur << "etats explores,"
                     << cur.g << "poussees.";
            qDebug() << "  arene =" << arene.nbCles() << "cles,  meilleurG =" << meilleurG.size()
                     << ",  noeuds =" << noeuds.size() << ",  file =" << file.size()
                     << ",  capacite file =" << file.capacity();
#ifdef INSTRUM_F
            imprimeHistoF(histoF, cur.g, compteur);
#endif
            if (plongeons)
                qDebug() << "SolveurAStar:" << plongeons << "plongeons TENTES, tous rates,"
                         << etatsPlongeon << "etats depenses.";
            imprimeStatsCorral();
            emit solutionTrouvee(reconstruire(cur.idxNoeud), compteur);
            return;
        }

        // Enfile l'état 'e' (déjà obtenu, non perdu), atteint depuis 'cur' par la
        // suite de poussées 'chaine' ((case caisse, dir)), de coût total gE. Gère
        // la clé en arène, la dédup meilleurG/ferme, la chaîne de noeuds (un par
        // poussée, pour que reconstruire() rejoue une macro à l'identique) et le
        // push_heap. Partagé entre poussées simples et goal macro.
        auto enfiler = [&](Game& e, int gE, const QVector<QPair<int,int>>& chaine,
                           [[maybe_unused]] bool estMacro) {
            // Deadlock de LIVRAISON (§6.1) : un but vide qu'aucune caisse ne peut
            // plus atteindre. Testé ICI et pas dans checkDefaite — sur un état
            // intermédiaire de goal macro il ferait avorter la macro (mesuré :
            // niveaux 3 et 5 perdus). Ici, la macro va au bout et c'est son
            // RÉSULTAT qu'on juge.
            if (livraisonSurEnfants && e.butNonLivrable(4)) return;
            // Case de REPOS de la caisse déplacée : destination de la DERNIÈRE
            // poussée de 'chaine'. Les deux étages du corral en partent — leurs
            // formes incrémentales reposent sur le même argument : une transition
            // (poussée simple ou goal macro) ne déplace qu'UNE caisse, vers une
            // seule case, donc elle seule a pu sceller quelque chose.
            const int arrivee = chaine.isEmpty() ? -1
                : e.caseApres(chaine.last().first, (Game::EDirection)chaine.last().second);
            // Corral unitaire (§6.1 item 4) : élague l'enfant dont une case scellée
            // prouve l'immobilité définitive d'une caisse hors but. Élagage PROUVÉ
            // (0 faux positif au juge fp sur les 11 résolus), promu en défaut le
            // 2026-07-27 : canari intact, ×6,6 sur le 4 et ×6,8 sur le 7, coût
            // négligeable là où le motif est absent (cf. plan.md, USok).
            // Testé à l'ENFILAGE et pas dans checkDefaite : marquer 'perdu' sur un
            // état intermédiaire de goal macro ferait avorter la macro entière
            // (mesuré, plan §6.1 — niveaux 3 et 5 perdus). Ici la macro va au bout,
            // c'est son RÉSULTAT qu'on juge. Forme incrémentale (équivalence prouvée
            // au balayage complet, cf. game.h) : seule la case de repos de la caisse
            // déplacée peut avoir nouvellement scellé une voisine.
            if (corralActif() && arrivee >= 0) {
                if (e.corralUnitaireMort(arrivee)) return;
            }
            // getEtat(cle) referait le flood-fill en interne, dans un QVector
            // neuf — un par enfant enfilé. Le tampon évite l'allocation.
            e.getZoneJoueur(zoneEnfant);
            // CORRAL-N (§6.1 item B) — promu en défaut le 2026-07-28. Trois étages :
            //   1. DÉTECTION incrémentale des enclos au contact de 'arrivee' (les
            //      seuls à pouvoir venir de se sceller) ;
            //   2. GATE structurel (Hall + non-rouvrable) : un FILTRE, pas une
            //      preuve — il ne prune JAMAIS, il décide seulement quoi soumettre
            //      à l'étage 3. Mesuré faux positif s'il tranchait lui-même (juge
            //      fp, variante -2) ; il ramène 40 % → 10-21 % des enfilages ;
            //   3. PREUVE : strip (retirer les caisses non frontière = relaxation
            //      valide, moins d'obstacles = joueur plus libre) puis BFS de
            //      poussées borné. MORT ssi l'espace est ÉPUISÉ sous budget — le
            //      strip relaxe, l'exhaustion prouve. Mémoïsée par frontière triée
            //      (le verdict n'en dépend que d'elle) : amortissement ×37 à ×223.
            // C'est le seul levier du projet qui attaque la masse f < C* (§3) :
            // ×9,9 sur le 4, ×7,7 sur le 17, ×3,4 sur le 9. Sound par construction
            // (on ne prune que sur une mort PROUVÉE), canari intact sur les 11
            // résolus. La zone du joueur vient d'être calculée : réutilisée gratis.
            if (corralActif() && arrivee >= 0) {
                const Game::EnclosInfo inf = e.detecteEnclosArrivee(arrivee, zoneEnfant,
                                                                    visiteCorral, &cacheEnclos,
                                                                    CORRAL_BUDGET);
                StatsCorral& sd = statsCorral();
                sd.enfilages++;
                if (inf.candidats > 0) { sd.avecCandidat++; sd.totCandidats += inf.candidats; }
                if (inf.durs > 0) {
                    sd.avecDur++;
                    sd.totDurs       += inf.durs;
                    sd.totCells      += inf.cells;
                    sd.totFrontiere  += inf.frontiere;
                    sd.totButsVides  += inf.butsVides;
                }
                sd.dursMorts    += inf.dursMorts;
                sd.dursVivants  += inf.dursVivants;
                sd.dursInconnus += inf.dursInconnus;
                sd.cacheHits    += inf.cacheHits;
                sd.solveStates  += inf.solveStates;
                // Étage 0 (CACHE_JOUEUR=1) — nuls hors instrumentation. Comme les
                // stats ci-dessus, seule la recherche PRINCIPALE est comptée : le
                // plongeon partage le cache mais n'alimente aucun compteur (choix
                // existant), et l'étage 0 se mesure de toute façon à budget de temps.
                sd.hitsTestes   += inf.hitsTestes;
                sd.hitsZoneDiff += inf.hitsZoneDiff;
                sd.diffMort     += inf.diffMort;
                sd.diffVivant   += inf.diffVivant;
                sd.diffInconnu  += inf.diffInconnu;
                for (int i = 0; i < 9; i++) sd.etage1[i] += inf.etage1[i];
                sd.etage1States += inf.etage1States;
                for (int i = 0; i < 3; i++) sd.arbitre[i] += inf.arbitre[i];
                sd.arbitreStates += inf.arbitreStates;
                // PRUNE : une mort PROUVÉE (strip + exhaustion) est sound → on coupe.
                if (inf.dursMorts > 0) { sd.enfilagesPrunes++; return; }
            }
            if (coinTropTot(e, arrivee)) return;
            // PAQUET NON LIVRABLE (chantier, PAQUET=1 — cf. solveurastar.h).
            // Vient APRÈS le corral : ce qu'il attrape est précisément ce que le
            // corral laisse passer, et le mesurer derrière lui donne le surplus.
            if (paquetActif() && arrivee >= 0) {
                StatsPaquet& sp = statsPaquet();
                sp.enfilages++;
                QVarLengthArray<int, 32> grp;
                paquetHorsBut(e, arrivee, grp);
                // Un paquet d'UNE caisse est déjà couvert par casesMortes et le
                // corral unitaire : le tester ne ferait que payer un sous-solve
                // pour un verdict connu.
                if (grp.size() >= 2) {
                    sp.testes++;
                    const QByteArray k(reinterpret_cast<const char*>(grp.constData()),
                                       grp.size() * (int)sizeof(int));
                    auto it = cachePaquet.constFind(k);
                    int v;
                    if (it != cachePaquet.constEnd()) { v = *it; sp.cacheHits++; }
                    else {
                        int dev = 0;
                        v = e.sousSolveEnclos(grp, paquetBudget(), &dev);
                        sp.solveStates += dev;
                        cachePaquet.insert(k, v);
                    }
                    if (v == 0)  { sp.morts++; sp.prunes++; return; }
                    if (v == -1) sp.inconnus++; else sp.vivants++;
                }
            }
            e.getEtat(arene.reserve(), zoneEnfant);
            Cle cle{arene.dernier()};
            if (interditRedeveloppement && ferme.count(cle)) { arene.annule(); return; }
            TableG::Slot* slot = meilleurG.cherche(cle);
            if (slot) {
                if (gE >= slot->g) { arene.annule(); return; }
                slot->g = gE;
                cle = slot->cle;
                arene.annule();
            } else {
                meilleurG.insere(cle, gE);
            }
            int parent = cur.idxNoeud;
            for (const auto& p : chaine) {
                noeuds.append(Noeud{parent, (quint16)p.first, (quint8)p.second});
                parent = noeuds.size() - 1;
            }
            qint64 score;
            const int hE = e.getHeuristique(&score);
            const int fE = gE + poids * hE;
#ifdef INSTRUM_DELTAF
            {
                StatsDeltaF& sd = statsDeltaF();
                const int df = fE - cur.f;
                const int i  = df + StatsDeltaF::DECALAGE;
                if (i < 0 || i >= StatsDeltaF::TAILLE) sd.horsBornes++;
                else (estMacro ? sd.histoMacro : sd.histoSimple)[i]++;

                if (estMacro) {
                    sd.nMacro++;
                    sd.sommeDeltaMacro += df;
                    const int n = chaine.size();
                    sd.sommeLongMacro += n;
                    if (n < (int)sd.parLongueur.size()) {
                        sd.parLongueur[n]++;
                        if (df == 0) sd.parLongueurNul[n]++;
                        else if (df < 0) sd.parLongueurNeg[n]++;
                    }
                    // Pourquoi cet enfant est-il relégué ? On refait h sur
                    // l'enfant avec le joueur du PARENT ('etat' n'a pas bougé :
                    // les enfants sont des copies).
                    if (df > 0) {
                        const QPoint pp = etat.getPlayerPoint();
                        const int jParent = pp.x() + pp.y() * etat.getLargeur();
                        const int hFige   = e.getHeuristique(nullptr, jParent);
                        const int hParent = (cur.f - cur.g) / poids;
                        const int dhCaisses = hFige - hParent;
                        const int dhJoueur  = hE - hFige;
                        sd.nReleg++;
                        sd.sommeDhCaisses += dhCaisses;
                        sd.sommeDhJoueur  += dhJoueur;
                        sd.sommeLongReleg += n;
                        if (dhCaisses <= -n) sd.relegPurJoueur++;
                        else                 sd.relegCouplage++;
                        if (dhJoueur) {
                            sd.dhJoueurNonNul++;
                            if (dhJoueur > 0) sd.dhJoueurPositif++;
                            else              sd.dhJoueurNegatif++;
                        }
                    }
                } else {
                    sd.nSimple++;
                    sd.sommeDeltaSimple += df;
                }
            }
#endif
            file.push_back({fE, gE, parent, cle, score});
            std::push_heap(file.begin(), file.end(), compare);
        };

        etat.getZoneJoueur(zone);
        QVector<quint8> caisses = etat.getCaissesDeplacable(zone);

        // GOAL MACRO (§10.5) — régime d'ENGAGEMENT : si le but actif (le plus
        // profond non rempli) peut être atteint par au moins une caisse, on ne
        // génère QUE les macros qui l'y envoient (une branche par caisse capable),
        // et rien d'autre. On abandonne ainsi toutes les façons de bouger ces
        // caisses autrement — c'est ce qui coupe la combinatoire. Repli sur les
        // poussées simples si aucune macro n'aboutit (caisse coincée par la
        // congestion : la recherche doit d'abord démêler).
        int macrosOk = 0;
        if (macro) {
            const int but = etat.butActif();
            if (but >= 0) {
                // Régime d'essai « but du couplage » (§6.3, 2026-07-24). On tente
                // d'abord la SEULE caisse que le couplage destine à ce but : elle
                // seule fait baisser h de N, donc elle seule produit un enfant à f
                // constant, promu en tête par le tie-break « g le plus grand ».
                // Toute autre caisse lui vole son but, le couplage se réarrange, et
                // l'enfant part sur le palier f+2 (mesuré : 100 % des macros du
                // niveau 12). Si elle ne passe pas, on rejoue la passe complète —
                // ce régime ne RETIRE donc aucune branche, il en PRÉFÈRE une.
                const int voulue = macroCouplage ? etat.caisseAssignee(but) : -1;
                for (int passe = 0; passe < (voulue >= 0 ? 2 : 1); passe++) {
                for (int i = 0; i < caisses.size(); i++) {
                    if (caisses[i] == 0) continue;   // pas de caisse poussable ici
                    if (voulue >= 0) {
                        // passe 0 : la caisse du couplage seule. passe 1 (repli,
                        // seulement si la passe 0 n'a rien donné) : toutes les autres.
                        if (passe == 0 && i != voulue) continue;
                        if (passe == 1 && i == voulue) continue;
                    }
                    // Écarter AVANT de copier : près d'une tentative sur deux
                    // n'avance même pas d'un pas (48,5 % au niveau 11), et la
                    // copie du plateau était payée pour rien.
                    if (!etat.macroPeutDemarrer(i, but, zone)) continue;
                    Game e(etat);
                    QVector<QPair<int,int>> poussees;
                    // Backtracke sur les forks (game.h) au lieu de s'arrêter à la
                    // première descente arbitraire — promu par défaut le 2026-07-23
                    // (§6.3) : canari intact, gain net sur 5/9 (le 9 ne finissait
                    // même pas sans), coût nul en l'absence de fork. 'zone' n'est
                    // PAS réutilisée ici (contrairement à l'ancien macroVersBut) :
                    // le prototype recalcule son propre premier flood-fill — perte
                    // de perf connue, pas encore corrigée (cf. plan.md §6.3).
                    if (e.macroVersButBacktrack(i, but, poussees) && !e.isPerdu()) {
                        enfiler(e, cur.g + poussees.size(), poussees, true);
                        macrosOk++;
                    }
                }
                // La caisse du couplage a produit un enfant : on s'y ENGAGE, pas
                // de passe de repli. C'est ce qui coupe la combinatoire.
                if (macrosOk > 0) break;
                }
            }
        }

        if (macrosOk == 0) {
            for(int i = 0; i < caisses.size(); i++) {
                quint8 dirPoussePossible = caisses[i];
                for (int d = 0; d < NB_DIRECTION; d++) {
                    if (dirPoussePossible & (1 << d)) {
                        Game e(etat);
                        if(e.pousse(i, (Game::EDirection)d) && !e.isPerdu())
                            enfiler(e, cur.g + 1, {{i, d}}, false);
                    }
                }
            }
        }
    }

    qDebug() << "SolveurAStar: aucune solution," << compteur << "etats explores.";
    imprimeStatsCorral();
    emit aucuneSolution();
}
