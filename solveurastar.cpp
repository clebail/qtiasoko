#include <QtDebug>
#include <QHash>
#include <algorithm>
#include <climits>
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
    return a.g < b.g;
}

SolveurAStar::SolveurAStar(const Game &etatDepart, int poids, QObject *parent)
    : Solveur(etatDepart, parent), poids(poids) {
}


void SolveurAStar::run() {
    std::vector<SElement> file;
    QHash<QByteArray,int> meilleurG;
    qint64 compteur = 0;

    // 'noeuds' appartient à la classe de base et survit d'une résolution à
    // l'autre : sans ce reset, la racine ne serait pas à l'indice 0 et le premier
    // enfant deviendrait son propre parent — reconstruire() boucherait à l'infini.
    noeuds.clear();
    noeuds.append(Noeud{-1, -1, Game::dHaut});   // racine : aucune poussée ne la précède

    const QByteArray cleDepart = depart.getEtat();
    meilleurG.insert(cleDepart, 0);

    file.push_back({poids * depart.getHeuristique(), 0, 0, cleDepart});
    std::push_heap(file.begin(), file.end(), compare);

    // État de travail RÉUTILISÉ d'un dépilement à l'autre : appliqueEtat()
    // réécrit intégralement le plateau, donc pas besoin d'un Game neuf à chaque
    // tour. Surtout, on part d'une copie de 'depart' pour hériter de casesMortes
    // et distanceButs (QVector en partage implicite → copie quasi gratuite) sans
    // jamais relancer calculCaseMorte(), qui est un flood-fill complet.
    Game etat(depart);

    while(file.size()) {
        // pop_heap n'enlève rien : il amène le meilleur élément en DERNIÈRE
        // position et réordonne le reste. C'est pop_back() qui le retire — et
        // entre les deux, on peut le VOLER (std::move) au lieu de le copier.
        std::pop_heap(file.begin(), file.end(), compare);
        SElement cur = std::move(file.back());
        file.pop_back();

        // Entrée périmée : un meilleur chemin vers ce même état a été trouvé
        // APRÈS qu'on ait enfilé celle-ci. On la jette sans la compter.
        if(cur.g > meilleurG.value(cur.cle, INT_MAX)) continue;

        compteur++;
        if (compteur % 1000 == 0) {
            qDebug() << "SolveurAStar(w=" << poids << "):" << compteur << "etats depiles, file =" << file.size()
                     << ", vus =" << meilleurG.size() << ", f =" << cur.f;
        }

        // Le Game n'était pas dans la file : on le reconstruit depuis la clé.
        etat.appliqueEtat(cur.cle);

        if(etat.isGagne()) {
            qDebug() << "SolveurAStar: solution trouvee apres" << compteur << "etats explores,"
                     << cur.g << "poussees.";
            emit solutionTrouvee(reconstruire(cur.idxNoeud), compteur);
            return;
        }

        QVector<bool> zone = etat.getZoneJoueur();
        QVector<quint8> caisses = etat.getCaissesDeplacable(zone);
        for(int i = 0; i < caisses.size(); i++) {
            quint8 dirPoussePossible = caisses[i];

            for (int d = 0; d < NB_DIRECTION; d++) {
                quint8 mask = 1 << d;
                if (dirPoussePossible & mask) {
                    Game e(etat);
                    e.pousse(i, (Game::EDirection)d);

                    if(!e.isPerdu()) {
                        auto cle = e.getEtat();
                        int gE = cur.g + 1;   // une poussée = un pas, toujours

                        // Ce test commande l'ENFILAGE, pas seulement l'insertion
                        // dans meilleurG : on n'enfile que si l'état est inconnu,
                        // ou atteint par un chemin strictement meilleur. Le BFS
                        // pouvait se contenter d'un QSet parce qu'une FIFO
                        // découvre les états dans l'ordre du coût ; A* dépile par
                        // f, pas par g, et n'a pas cette garantie.
                        if(gE >= meilleurG.value(cle, INT_MAX)) continue;
                        meilleurG.insert(cle, gE);

                        noeuds.append(Noeud{cur.idxNoeud, i, (Game::EDirection)d});

                        file.push_back({gE + poids * e.getHeuristique(), gE, noeuds.size()-1, cle});
                        std::push_heap(file.begin(), file.end(), compare);
                    }
                }
            }
        }
    }

    qDebug() << "SolveurAStar: aucune solution," << compteur << "etats explores.";
    emit aucuneSolution();
}
