#ifndef SOLVEURASTAR_H
#define SOLVEURASTAR_H

#include "solveur.h"

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
    // (~22 o) détermine entièrement l'état — on reconstruit le Game au
    // dépilement avec Game::appliqueEtat(). ~700 o → ~50 o.
    typedef struct _SElement {
        int f;
        int g;
        int idxNoeud;
        QByteArray cle;
    } SElement;

    explicit SolveurAStar(const Game& etatDepart, int poids = 1, QObject* parent = nullptr);

protected:
    void run() override;

private:
    const int poids;
};

#endif // SOLVEURASTAR_H
