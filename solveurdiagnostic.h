#ifndef SOLVEURDIAGNOSTIC_H
#define SOLVEURDIAGNOSTIC_H

// Tout ce qui OBSERVE le solveur sans jamais influencer une décision d'élagage :
// compteurs runtime, impression sur stderr, histogrammes de harnais. Séparé de
// solveurastar.cpp le 2026-08-18 (idée utilisateur) — le solveur ne devrait
// contenir que ce qui DÉCIDE, pas comment on le lit depuis l'extérieur.
//
// ⚠️ Deux familles, pas une seule :
//   - StatsCorral/imprimeMemoire tournent en PRODUCTION, sans #ifdef :
//     plusieurs runs de plusieurs heures (niveaux 25/29/31) ont été TUÉS sans
//     jamais imprimer de bilan de fin — seul ce qui part EN CONTINU avec la
//     jauge se relève. Ce n'est donc pas du confort de mesure, c'est la seule
//     fenêtre d'observation sur un run qu'on ne peut pas laisser finir.
//   - StatsDeltaF/imprimeHistoF/etatsDeveloppes sont sous #ifdef et ne compilent
//     QUE dans les harnais de mesures/ — absents du binaire produit.
#include <QByteArray>
#include <QtGlobal>
#include <vector>
#include <utility>
#include "cle.h"

class Arene;
class TableG;

// Répartition mémoire des quatre postes du solveur (arène, table de meilleur g,
// arbre de noeuds, file ouverte), cf. plan.md §6.5. 'fileOctets' est calculé par
// l'appelant (capacité de la file × sizeof(SElement)) : ce fichier ne connaît
// pas le type SElement, propre à solveurastar.h.
void imprimeMemoire(const char* quand, const Arene& arene, const TableG& meilleurG,
                    size_t noeudsOctets, size_t fileOctets, size_t etatsVus);

// Stats du corral-N, agrégées sur tout le solve. Runtime, jamais de #ifdef :
// la fraction de durs prouvés morts PRÉDIT le gain sur un niveau neuf (plan.md
// §6.1), on veut la lire sans recompiler.
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
StatsCorral& statsCorral();
void imprimeStatsCorral();

#ifdef INSTRUM_SONDE
// Défini ici, déclaré dans cle.h (TableG en a besoin structurellement pour
// incrémenter ses compteurs de sondage à chaque cherche()/insere()).
#endif

#ifdef DUMP_DEV
// Les états RÉELLEMENT dépilés — et non l'ensemble {f <= C*}, ~25x plus gros :
// A* s'arrête dès qu'il atteint le but. Un seul thread solveur à la fois, pas
// de verrou. Utilisé par les harnais qui rejouent une trace (mou, mort, corral).
std::vector<std::pair<QByteArray,int>>& etatsDeveloppes();
// Plafond de dépilements, pour instrumenter un niveau qu'on NE SAIT PAS résoudre.
// 0 = pas de plafond.
int& limiteDepilements();
#endif

#ifdef INSTRUM_F
#include <vector>
// cStar = g de l'état gagnant = le coût optimal. Histogramme des f au
// dépilement (harnais `bench ... record`, cf. plan.md §3) : tout état développé
// avec f < C* a un mou PROUVÉ (mou(s) >= C* - f(s)), gratuit, sans sous-solve.
void imprimeHistoF(const std::vector<qint64>& histoF, int cStar, qint64 total);
#endif

#ifdef INSTRUM_DELTAF
#include <vector>
// Instrumentation hors-ligne du Δf DES ENFANTS ENFILÉS (harnais mesures/deltaf,
// cf. plan.md §1). Question posée : à f égal, le comparateur préfère le g le
// plus GRAND, donc une goal macro de N poussées enfilée à f CONSTANT (Δh = -N)
// passe en tête. Mais Δf = N + poids·Δh, et rien ne garantit Δh = -N — d'où la
// distribution qu'on mesure ici, macro contre poussée simple.
struct StatsDeltaF {
    static const int DECALAGE = 64;    // Δf peut être négatif (h non cohérente)
    static const int TAILLE   = 256;

    std::vector<qint64> histoMacro  = std::vector<qint64>(TAILLE, 0);
    std::vector<qint64> histoSimple = std::vector<qint64>(TAILLE, 0);
    qint64 horsBornes = 0;

    // Croisement (longueur de chaîne N) × (Δf).
    std::vector<qint64> parLongueur    = std::vector<qint64>(64, 0);  // enfants macro par N
    std::vector<qint64> parLongueurNul = std::vector<qint64>(64, 0);  // ... dont Δf == 0
    std::vector<qint64> parLongueurNeg = std::vector<qint64>(64, 0);  // ... dont Δf < 0

    qint64 nMacro = 0, nSimple = 0;
    qint64 sommeDeltaMacro = 0, sommeDeltaSimple = 0;
    qint64 sommeLongMacro = 0;

    // Décomposition de Δh sur les enfants de macro relégués (Δf > 0), cf.
    // solveurastar.cpp (enfiler) pour le calcul de dhCaisses/dhJoueur.
    qint64 nReleg = 0;
    qint64 sommeDhCaisses = 0, sommeDhJoueur = 0, sommeLongReleg = 0;
    qint64 relegPurJoueur = 0;         // ... dont dhCaisses == -N (macro parfaite)
    qint64 relegCouplage = 0;          // ... dont dhCaisses  > -N (couplage remanié)
    qint64 dhJoueurNonNul = 0, dhJoueurPositif = 0, dhJoueurNegatif = 0;
};
StatsDeltaF& statsDeltaF();
#endif

#endif // SOLVEURDIAGNOSTIC_H
