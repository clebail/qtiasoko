#ifndef JUGEMACRO_H
#define JUGEMACRO_H

// LE JUGE DE LA MACRO HUMAINE (2026-08-23) — le solveur produirait-il CETTE macro,
// à CET état, et à quelle place ?
//
// POURQUOI IL EXISTE. Le journal hybride confrontait le solveur aux seules poussées
// choisies À LA MAIN (`⚠ ECARTE`, cf. mainwindow.h). Sur la partie gagnée du
// level0200 — 363 coups, 133 macros, 7 poussées manuelles — cela laissait 95 % de la
// partie hors de tout contrôle, alors que le trou diagnostiqué (plan.md §6.0,
// 2026-08-22) est justement dans la GÉNÉRATION DES MACROS : « l'espace engendré par
// le régime macro ne contient pas de solution pour cette zone, alors qu'une solution
// existe ».
//
// EXEMPLAIRE UNIQUE, et c'est la raison de ce header (§7 : une règle écrite à deux
// endroits diverge). Il est appelé par l'UI (mainwindow.cpp, sur le clic qui lance
// une macro) ET par le harnais mesures/rejeu (qui dépouille les journaux DÉJÀ en
// banque, sans rejouer une partie à la main). Deux répliques du contrat auraient
// fini par ne plus juger la même chose — c'est exactement le piège du demi-tour de
// la macro (§7, 2026-08-07), où `avanceVersBut` et `getCaissesDeplacable`
// exprimaient la même règle et divergeaient.
//
// CE QU'IL REJOUE. Les deux passes de `SolveurAStar::tenteMacro` puis l'enfilage
// (solveurastar.cpp), à l'identique :
//   1. le RÉGIME DU COUPLAGE — la passe 0 ne tente que `caisseAssignee(but)` ; si SA
//      descente aboutit, la boucle casse et les autres caisses ne sont JAMAIS
//      tentées. Une macro que l'écran cercle en vert peut donc n'exister dans aucun
//      état de l'arbre du solveur ;
//   2. le corral unitaire, puis le corral-N (même CORRAL_BUDGET) ;
//   3. la clé du comparateur — f croissant, puis g DÉCROISSANT, puis guidage
//      croissant.
//
// CE QU'IL NE REJOUE PAS, et il faut le savoir en lisant un verdict :
//   - `loiTropTot`, propre au régime `loi` (réfuté, §6.0 point 5) ;
//   - la dédup `meilleurG` / `ferme`, qui demande l'historique d'un run entier. Un
//     `rang` dit donc « le solveur enfilerait ceci ici », pas « il le développerait ».
//
// ⚠️ `abouties` compte, comme `macrosOk` dans le solveur, les descentes qui
// ABOUTISSENT et non les enfants réellement enfilés : l'incrément y suit `enfiler()`,
// qui a pu élaguer en silence. L'engagement est reproduit avec cette nuance — une
// passe 0 dont l'unique enfant meurt au corral coupe quand même la passe de repli.
//
// ⚠️ Le `g` n'est PAS commun aux frères, contrairement aux poussées simples : une
// macro enfile à `g + (nombre de poussées)`. C'est donc bien Δf, et jamais Δh, qui
// se transporte à la file.

#include <QVector>
#include <QHash>
#include <QByteArray>
#include "game.h"
#include "solveurastar.h"   // corralActif() / CORRAL_BUDGET — le juge doit élaguer comme le solveur

struct VerdictMacro {
    enum Type {
        // Le couplage assigne ce but à une AUTRE caisse dont la macro aboutit : le
        // solveur s'y engage et ne génère jamais celle-ci. LE désaccord de
        // génération — la ligne qui dit quelque chose de neuf sur l'angle mort.
        HorsPasseCouplage,
        // Générée, puis élaguée. Sur une partie GAGNÉE, faux positif PROUVÉ (le
        // raisonnement de mesures/fp, étendu aux macros et aux niveaux non résolus).
        Ecarte,
        // L'appelant affirme que la macro est jouable et le juge ne la retrouve pas :
        // les deux lectures ont divergé. À dire, jamais à avaler.
        Introuvable,
        // Générée et retenue : voici sa place parmi les macros que le solveur enfile.
        Retenue,
    };

    Type type = Introuvable;
    int  nbEnfilees = 0;        // macros retenues à cet état (toutes causes d'élagage déduites)

    const char* cause = nullptr; // Ecarte : quel étage a coupé
    int voulue = -1;             // HorsPasseCouplage : la caisse que le couplage préfère
    bool estCouplage = false;    // la macro jugée EST celle du couplage

    // Retenue seulement.
    int rang = 0, exAequo = 0;
    int poussees = 0, g = 0, h = 0, f = 0;
    int bestCaisse = -1, bestPoussees = 0, bestF = 0;
    int df = 0;                  // f(jouée) − f(meilleure) : le Δf qui part à la file
};

// 'avant' = l'état d'où part la macro, 'idxCaisse' = la case où la caisse se tient
// ALORS, 'but' = l'index du but visé (celui de butActif(), que l'appelant a déjà lu).
inline VerdictMacro jugeMacro(const Game& avant, int idxCaisse, int but) {
    VerdictMacro v;
    if (but < 0) return v;

    const QVector<bool>   zone    = avant.getZoneJoueur();
    const QVector<quint8> caisses = avant.getCaissesDeplacable(zone);

    // Le régime de référence (coupl-plongeon, et tous ceux qui en dérivent) arme
    // macroCouplage : on rejoue donc les DEUX passes. Un régime sans couplage
    // (AstarMacro nu) n'en a qu'une — c'est exactement ce que rend voulue < 0.
    const int voulue = avant.caisseAssignee(but);
    v.voulue = voulue;
    v.estCouplage = (voulue >= 0 && idxCaisse == voulue);

    struct Enfant { int caisse; int poussees; int g; int f; int h; qint64 guidage; };
    QVector<Enfant> enfants;
    int iJoueur = -1;
    bool tentee = false;
    int  aboutiesPasse0 = 0;

    // Cache d'enclos LOCAL à l'appel : il n'évite que de rejuger deux fois le même
    // corral entre frères. Le faire vivre d'un appel à l'autre mesurerait un cache
    // d'historique différent de celui d'un run réel (même choix qu'en mesureRangCoup).
    QVector<bool> zoneEnfant, visiteCorral;
    QHash<QByteArray, Game::VerdictEnclos> cacheEnclos;

    const int gCur = avant.getNbDepCaisse();

    for (int passe = 0; passe < (voulue >= 0 ? 2 : 1); passe++) {
        int abouties = 0;
        for (int i = 0; i < caisses.size(); i++) {
            if (caisses[i] == 0) continue;
            if (voulue >= 0) {
                if (passe == 0 && i != voulue) continue;
                if (passe == 1 && i == voulue) continue;
            }
            // Écarter AVANT de copier, comme le solveur : près d'une tentative sur
            // deux n'avance même pas d'un pas.
            if (!avant.macroPeutDemarrer(i, but, zone)) continue;

            Game e(avant);
            QVector<QPair<int,int>> poussees;
            if (!e.macroVersButBacktrack(i, but, poussees) || e.isPerdu()) continue;

            abouties++;                       // cf. l'avertissement en tête de fichier
            const bool estLeCoup = (i == idxCaisse);
            if (estLeCoup) tentee = true;

            // Case de REPOS de la caisse déplacée : destination de la DERNIÈRE
            // poussée de la chaîne, lue sur l'état APRÈS la macro. Les deux étages
            // du corral en partent, dans cet ordre (l'unitaire d'abord, il est O(1)).
            const int arrivee = poussees.isEmpty() ? -1
                : e.caseApres(poussees.last().first, (Game::EDirection)poussees.last().second);
            if (corralActif() && arrivee >= 0) {
                if (e.corralUnitaireMort(arrivee)) {
                    if (estLeCoup) { v.type = VerdictMacro::Ecarte; v.cause = "corral unitaire"; }
                    continue;
                }
                e.getZoneJoueur(zoneEnfant);
                const Game::EnclosInfo inf =
                    e.detecteEnclosArrivee(arrivee, zoneEnfant, visiteCorral,
                                           &cacheEnclos, CORRAL_BUDGET);
                if (inf.dursMorts > 0) {
                    if (estLeCoup) { v.type = VerdictMacro::Ecarte; v.cause = "corral-N (mort PROUVEE)"; }
                    continue;
                }
            }

            qint64 guidage = 0;
            const int h  = e.getHeuristique(&guidage);
            const int gE = gCur + poussees.size();
            enfants.append({i, (int)poussees.size(), gE, gE + h, h, guidage});
            if (estLeCoup) iJoueur = enfants.size() - 1;
        }
        if (passe == 0) aboutiesPasse0 = abouties;
        // La passe 0 a produit un enfant : le solveur s'ENGAGE, pas de repli.
        if (abouties > 0) break;
    }

    v.nbEnfilees = enfants.size();

    if (!tentee && voulue >= 0 && idxCaisse != voulue && aboutiesPasse0 > 0) {
        v.type = VerdictMacro::HorsPasseCouplage;
        return v;
    }
    if (v.type == VerdictMacro::Ecarte) return v;
    if (iJoueur < 0) { v.type = VerdictMacro::Introuvable; return v; }

    // Clé du comparateur (solveurastar.cpp) : f croissant, puis g DÉCROISSANT, puis
    // guidage croissant. Ici les trois mordent — deux macros de longueurs
    // différentes n'enfilent pas au même g.
    auto meilleurQue = [](const Enfant& a, const Enfant& b) {
        if (a.f != b.f) return a.f < b.f;
        if (a.g != b.g) return a.g > b.g;
        return a.guidage < b.guidage;
    };

    const Enfant& joueur = enfants[iJoueur];
    int rang = 1, exAequo = -1, iBest = 0;   // exAequo part à -1 : la macro jugée ne se compte pas
    for (int i = 0; i < enfants.size(); i++) {
        if (meilleurQue(enfants[i], joueur))       rang++;
        else if (!meilleurQue(joueur, enfants[i])) exAequo++;
        if (meilleurQue(enfants[i], enfants[iBest])) iBest = i;
    }
    const Enfant& best = enfants[iBest];

    v.type = VerdictMacro::Retenue;
    v.rang = rang;  v.exAequo = exAequo;
    v.poussees = joueur.poussees; v.g = joueur.g; v.h = joueur.h; v.f = joueur.f;
    v.bestCaisse = best.caisse; v.bestPoussees = best.poussees; v.bestF = best.f;
    v.df = joueur.f - best.f;
    return v;
}

#endif // JUGEMACRO_H
