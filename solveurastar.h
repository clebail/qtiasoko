#ifndef SOLVEURASTAR_H
#define SOLVEURASTAR_H

#include "cle.h"
#include "solveur.h"

#ifdef INSTRUM_DELTAF
#include <vector>
// Instrumentation hors-ligne du Δf DES ENFANTS ENFILÉS (harnais mesures/deltaf).
// Ne compile que dans les harnais : le code produit ne définit jamais INSTRUM_DELTAF.
//
// Question posée. À f égal, le comparateur préfère le g le plus GRAND
// (solveurastar.cpp). Une goal macro de N poussées enfile donc un enfant à g+N :
// s'il reste à f constant (h a baissé de N), il double tous les états de même f
// et « ranger une caisse » revient à passer en tête. Mais RIEN ne le garantit :
//     Δf = N + poids · Δh
// et si une seule poussée de la chaîne n'est pas productive au sens du couplage
// hongrois, Δh > −N, donc Δf > 0 — l'enfant tombe sur un palier SUPÉRIEUR et se
// retrouve RELÉGUÉ derrière toute la masse restante, au lieu d'être promu.
// C'est la distribution de ce Δf qu'on mesure ici, macro contre poussée simple.
//
// Compté au moment du push_heap, donc sur les enfants RÉELLEMENT enfilés (un
// enfant rejeté par la dédup ne concourt pas au classement).
struct StatsDeltaF {
    static const int DECALAGE = 64;    // Δf peut être négatif (h non cohérente)
    static const int TAILLE   = 256;

    std::vector<qint64> histoMacro  = std::vector<qint64>(TAILLE, 0);
    std::vector<qint64> histoSimple = std::vector<qint64>(TAILLE, 0);
    qint64 horsBornes = 0;

    // Croisement (longueur de chaîne N) × (Δf) : c'est lui qui dit si les Δf > 0
    // viennent des chaînes longues (une poussée non productive quelque part) ou
    // s'ils sont uniformes.
    std::vector<qint64> parLongueur    = std::vector<qint64>(64, 0);  // enfants macro par N
    std::vector<qint64> parLongueurNul = std::vector<qint64>(64, 0);  // ... dont Δf == 0
    std::vector<qint64> parLongueurNeg = std::vector<qint64>(64, 0);  // ... dont Δf < 0

    qint64 nMacro = 0, nSimple = 0;
    qint64 sommeDeltaMacro = 0, sommeDeltaSimple = 0;
    qint64 sommeLongMacro = 0;

    // DÉCOMPOSITION de Δh, sur les enfants de macro relégués (Δf > 0). h est
    // joueur-aware : à caisses identiques, déplacer le joueur change h. On
    // recalcule donc h(enfant) avec la position du PARENT :
    //     dhCaisses = h(enfant, joueur parent) - h(parent)   <- le travail réel
    //     dhJoueur  = h(enfant, joueur enfant) - h(enfant, joueur parent)
    // Si dhCaisses == -N, la macro a bien fait son travail et c'est le JOUEUR
    // laissé du mauvais côté qui mange le gain. Sinon, c'est le couplage qui se
    // réarrange (la caisse posée n'était pas celle que le couplage destinait à
    // ce but).
    qint64 nReleg = 0;                 // enfants de macro à Δf > 0
    qint64 sommeDhCaisses = 0, sommeDhJoueur = 0, sommeLongReleg = 0;
    qint64 relegPurJoueur = 0;         // ... dont dhCaisses == -N (macro parfaite)
    qint64 relegCouplage = 0;          // ... dont dhCaisses  > -N (couplage remanié)
    // Une MOYENNE nulle de dhJoueur pourrait masquer des valeurs opposées : on
    // compte donc séparément les cas non nuls, dans chaque sens.
    qint64 dhJoueurNonNul = 0, dhJoueurPositif = 0, dhJoueurNegatif = 0;
};
StatsDeltaF& statsDeltaF();
#endif

// A* sur les poussées : f = g + poids * h.
//
// poids = 1 : A* classique. h est admissible ET cohérente, donc la solution est
//             OPTIMALE en nombre de poussées. Mais l'élagage est quasi nul (−3 à
//             −20 % d'états seulement) : une poussée utile fait g+1 et h−1, donc
//             f ne bouge pas, et A* doit développer tout état de f <= C*. Une
//             heuristique admissible ne peut pas élaguer ce qui n'est pas mauvais.
//
// poids > 1 : h est gonflée, donc plus admissible — l'optimalité est PERDUE, et
//             la cohérence avec elle (un état peut être re-développé après avoir
//             été atteint par un meilleur chemin ; c'est normal et géré par
//             'meilleurG'). En échange, la recherche plonge vers la solution au
//             lieu de balayer les paliers : mesuré ×30 en temps sur le niveau 1
//             pour +6 % de poussées.
class SolveurAStar : public Solveur
{
    Q_OBJECT

public:
    // NE PORTE PAS de Game. Un Game complet pèse ~700 o (72 o d'objet + le
    // tableau 'cases'), et la file ouverte d'A* compte des millions d'entrées :
    // c'était LE poste mémoire (3,4 Go sur 4,8 Go pour le niveau 2). La clé
    // détermine entièrement l'état — on reconstruit le Game au dépilement avec
    // Game::appliqueEtat().
    //
    // Et la clé elle-même n'est plus un QByteArray mais une simple référence
    // dans l'arène (4 o, cf. cle.h) : le QByteArray coûtait un malloc et un
    // en-tête QArrayData par clé, pour 22 o utiles. SElement tient maintenant en
    // 16 octets, entièrement POD — donc memcpy-able quand le tas se réalloue.
    typedef struct _SElement {
        int f;
        int g;
        int idxNoeud;
        Cle cle;
        qint64 guidage;   // départage lexicographique (§10.2) : plus PETIT = préféré
    } SElement;

    // 'macro' active la goal macro (§10.5) : rapide, optimal sur les niveaux à
    // faible congestion, approché sur les gros (le trajet solo peut y différer du
    // réel). 'false' = A* pur (optimal garanti, mais lent/inabouti sur les gros).
    //
    // 'macroCouplage' (régime d'essai, plan.md §6.3) : la macro tente D'ABORD la
    // caisse que le couplage hongrois destine au but actif. Pousser celle-là fait
    // baisser h d'exactement N, donc l'enfant reste à f CONSTANT et le tie-break
    // « g le plus grand » le fait passer en tête ; pousser une autre caisse lui
    // fait voler son but, le couplage se réarrange et h ne baisse pas (l'enfant
    // part alors DERRIÈRE tout le palier). Sans effet si la caisse assignée ne
    // peut pas faire la macro : on retombe sur les autres candidates.
    explicit SolveurAStar(const Game& etatDepart, int poids = 1, bool macro = false,
                          QObject* parent = nullptr, bool macroCouplage = false);

protected:
    void run() override;

private:
    const int poids;
    const bool macro;
    const bool macroCouplage;
};

#endif // SOLVEURASTAR_H
