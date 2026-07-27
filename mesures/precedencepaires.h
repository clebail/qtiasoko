#ifndef PRECEDENCEPAIRES_H
#define PRECEDENCEPAIRES_H

// PRÉCÉDENCE PAR PAIRES — deux buts occupés SIMULTANÉMENT affament un but vide.
//
// D'OÙ ÇA VIENT (2026-07-30, niveau 13). `Game::precedenceGlobale()` teste un seul
// bloqueur à la fois : « G doit précéder B si, B traité comme occupé, plus aucune
// caisse n'atteint G ». Sur le 13 elle rend 4 arêtes et AUCUNE ne concerne (14,6),
// alors que ce but est le point de murage réel — parce qu'il est enclavé entre les
// murs (13,6) et (15,6), donc ses seules approches sont verticales, et il faut
// occuper les DEUX appuis (14,4) et (14,8) pour le condamner. Aucun but seul n'y
// suffit ; la paire, oui. La précédence simple est structurellement aveugle à ce
// motif — pas imprécise, aveugle.
//
// LA RÈGLE, exactement celle du §6.2 avec deux obstacles au lieu d'un :
//
//     {B1,B2} affame G si, B1 ET B2 traités comme occupés, plus aucune caisse
//     n'atteint G.
//
// Même relaxation OPTIMISTE que l'originale (joueur supposé capable d'atteindre
// n'importe quel appui, autres caisses ignorées comme obstacles) : « inatteignable »
// est donc une PREUVE, « atteignable » ne promet rien.
//
// ❌❌ RÉFUTÉ COMME ÉLAGAGE — MESURÉ LE 2026-07-30, NE PAS LE RESSORTIR.
//
// Le soupçon était écrit d'avance : l'arête dit « si B1 et B2 RESTENT occupés, G est
// perdu », or une caisse peut ressortir d'un but — le projet y tient explicitement
// (§4 : le découpage une-caisse-à-la-fois est réfuté parce qu'il « interdit le parking
// temporaire et le ressortir-d'un-but »), et les records du 13 le font réellement
// ((14,9) est posée dans r04–r08 et plus du tout dans r09).
//
// Le juge `fp -3` (rejeu de solutions GAGNANTES, donc toute détection est un faux
// positif PROUVÉ) l'a chiffré, macro, niveaux résolus :
//
//     niveau  0   1   2   3   4    5   6   7    9   17
//     FP      0   1   2   4   10   6   4   14   6   1
//
// 9 sur 10 en faute ; seul le 0 est propre, et il fait 4 poussées. Ce n'est pas une
// imprécision réglable, c'est l'hypothèse de la règle qui est fausse.
//
// La version renforcée « exiger B1 et B2 IMMOBILES » ne sauve rien : le point fixe de
// gel s'effondre précisément sur le plateau qui motivait la piste (dans r09, la caisse
// (16,5) peut sortir vers (15,5), ce qui décoince toute la cascade). Et le repli
// habituel « dé-prioriser au lieu de couper » (§6.4a) ne mord pas non plus, le §3
// donnant la raison : un guidage ne touche pas la masse `f < C*`, qui est justement ce
// qui bloque le 13.
//
// CE QUI RESTE VRAI, et c'est tout : le motif DÉCRIT correctement pourquoi les records
// r08 et r09 du 13 sont morts (`{(14,4),(14,8)} → (14,6)`, confirmé par A\* pur, espace
// épuisé en 29,0 M et 5,2 M états). Outil de LECTURE d'un état précis, jamais un test.

#include <QVector>
#include "game.h"
#include "level.h"

namespace PrecedencePaires {

struct Arete { int G, B1, B2; };   // cases (index plateau) ; B2 = -1 : arête SIMPLE

// BFS de TIRAGE à rebours depuis G : on remonte les poussées (la caisse était en
// G−d, le joueur en G−2d) jusqu'à tomber sur une caisse réelle. `bloque`/`bloque2`
// sont infranchissables — ni passage de caisse, ni appui du joueur.
//
// EXEMPLAIRE UNIQUE, partagé par la version simple (bloque2 = -1) et la version
// paire : les deux ne peuvent pas diverger. Principe déjà appliqué à `avanceVersBut`
// (§6.3) — un test dupliqué finit toujours par mentir.
inline bool atteintUneCaisse(const Game& g, int G, int bloque, int bloque2 = -1)
{
    static const int DX[4] = { 0, 1, 0, -1 };
    static const int DY[4] = { -1, 0, 1, 0 };
    const int L = g.getLargeur(), H = g.getHauteur();

    QVector<bool> vu(L * H, false);
    QVector<int> file;
    file.append(G);
    vu[G] = true;

    for (int t = 0; t < file.size(); t++) {
        const int c = file[t];
        if (c != G) {   // G lui-même n'est pas un point de DÉPART valable
            const Level::ETypeCase tc = g.getCase(c);
            if (tc == Level::tcCaisse || tc == Level::tcGoalCaisse) return true;
        }
        const int cx = c % L, cy = c / L;
        for (int d = 0; d < 4; d++) {
            const int px = cx - DX[d],     py = cy - DY[d];       // caisse avant
            const int ax = cx - 2 * DX[d], ay = cy - 2 * DY[d];   // appui joueur
            if (px < 0 || px >= L || py < 0 || py >= H) continue;
            if (ax < 0 || ax >= L || ay < 0 || ay >= H) continue;
            const int pp = px + py * L, aa = ax + ay * L;
            if (g.getCase(pp) == Level::tcMur || g.getCase(aa) == Level::tcMur) continue;
            if (pp == bloque  || aa == bloque)  continue;
            if (pp == bloque2 || aa == bloque2) continue;
            if (!vu[pp]) { vu[pp] = true; file.append(pp); }
        }
    }
    return false;
}

// Arêtes VIOLÉES par l'état courant : un but VIDE dont toutes les routes sont
// fermées par des buts actuellement OCCUPÉS.
//
// On ne balaie que les bloqueurs réellement occupés — inutile de calculer les arêtes
// dont les deux extrémités sont vides, elles ne peuvent rien violer. Coût :
// (buts vides) × (occupés + paires d'occupés) BFS, très en dessous du O(buts³) d'une
// table exhaustive.
//
// `simples` (optionnel) reçoit les violations à UN seul bloqueur — objet plus faible
// et déjà connu (§6.2), tenu à part pour ne pas gonfler le compte des paires. Une
// paire dont un membre viole déjà seul est SUBSUMÉE et n'est pas comptée.
inline int violations(const Game& g,
                      QVector<Arete>* paires = nullptr,
                      QVector<Arete>* simples = nullptr,
                      int* butsInatteignables = nullptr)
{
    const int nb = g.getNbButs();
    QVector<int> vides, occupes;
    for (int b = 0; b < nb; b++) {
        const int c = g.getCaseBut(b);
        if (g.getCase(c) == Level::tcGoalCaisse) occupes.append(c);
        else                                     vides.append(c);
    }

    int nPaires = 0;
    for (int G : vides) {
        // Un but que plus aucune caisse n'atteint SANS le moindre obstacle : l'état
        // est déjà mort, mais par le test symétrique de `staticDeadlock` (§6.1),
        // mesuré strictement neutre. Ce n'est pas notre objet — compté à part.
        if (!atteintUneCaisse(g, G, -1)) {
            if (butsInatteignables) (*butsInatteignables)++;
            continue;
        }

        QVector<int> bloqueursSimples;
        for (int B : occupes) {
            if (!atteintUneCaisse(g, G, B)) {
                bloqueursSimples.append(B);
                if (simples) simples->append({ G, B, -1 });
            }
        }

        for (int i = 0; i < occupes.size(); i++) {
            if (bloqueursSimples.contains(occupes[i])) continue;   // subsumée
            for (int j = i + 1; j < occupes.size(); j++) {
                if (bloqueursSimples.contains(occupes[j])) continue;
                if (atteintUneCaisse(g, G, occupes[i], occupes[j])) continue;
                nPaires++;
                if (paires) paires->append({ G, occupes[i], occupes[j] });
            }
        }
    }
    return nPaires;
}

}   // namespace PrecedencePaires

#endif // PRECEDENCEPAIRES_H
