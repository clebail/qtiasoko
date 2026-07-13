#include <QtDebug>
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
    return a.g < b.g;
}

SolveurAStar::SolveurAStar(const Game &etatDepart, int poids, QObject *parent)
    : Solveur(etatDepart, parent), poids(poids) {
}


void SolveurAStar::run() {
    std::vector<SElement> file;
    qint64 compteur = 0;

    // Toutes les clés du solve vivent ici, bout à bout (cf. cle.h). Les
    // conteneurs ci-dessous n'en portent que des références de 4 octets.
    //
    // std::unordered_map et non QHash : hacher ou comparer une clé exige de LIRE
    // l'arène, or QHash passe par un qHash(T)/operator== globaux, à qui on n'a
    // aucun moyen de la transmettre. Les foncteurs de la STL, eux, sont des
    // objets — ils la portent.
    Arene arene(depart.tailleCle());
    std::unordered_map<Cle,int,CleHash,CleEq> meilleurG(1024, CleHash{&arene}, CleEq{&arene});

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
    noeuds.append(Noeud{-1, -1, Game::dHaut});   // racine : aucune poussée ne la précède

    depart.getEtat(arene.reserve());
    const Cle cleDepart{arene.dernier()};
    meilleurG.emplace(cleDepart, 0);

    file.push_back({poids * depart.getHeuristique(), 0, 0, cleDepart});
    std::push_heap(file.begin(), file.end(), compare);

    // État de travail RÉUTILISÉ d'un dépilement à l'autre : appliqueEtat()
    // réécrit intégralement le plateau, donc pas besoin d'un Game neuf à chaque
    // tour. Surtout, on part d'une copie de 'depart' pour hériter de casesMortes
    // et distancePoussee (QVector en partage implicite → copie quasi gratuite)
    // sans jamais relancer calculDistancePoussee(), qui est en O(size²).
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
        auto itCur = meilleurG.find(cur.cle);
        if(itCur != meilleurG.end() && cur.g > itCur->second) continue;

        if (interditRedeveloppement) {
            if (ferme.count(cur.cle)) continue;
            ferme.insert(cur.cle);
        }

        compteur++;
        if (compteur % 1000 == 0) {
            qDebug() << "SolveurAStar(w=" << poids << "):" << compteur << "etats depiles, file =" << file.size()
                     << ", vus =" << meilleurG.size() << ", f =" << cur.f;
        }

        // Le Game n'était pas dans la file : on le reconstruit depuis la clé.
        etat.appliqueEtat(arene.lit(cur.cle.offset));

        if(etat.isGagne()) {
            qDebug() << "SolveurAStar: solution trouvee apres" << compteur << "etats explores,"
                     << cur.g << "poussees.";
            qDebug() << "  arene =" << arene.nbCles() << "cles,  meilleurG =" << meilleurG.size()
                     << ",  noeuds =" << noeuds.size() << ",  file =" << file.size()
                     << ",  capacite file =" << file.capacity();
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
                        // La clé s'écrit directement en fin d'arène — aucune
                        // allocation. Si l'enfant se révèle être un doublon, on
                        // la reprend par annule() : elle y figure déjà.
                        e.getEtat(arene.reserve());
                        Cle cle{arene.dernier()};

                        int gE = cur.g + 1;   // une poussée = un pas, toujours

                        // Déjà développé : inutile de le ré-enfiler, on le
                        // jetterait au dépilement — et la file gonflerait pour rien.
                        if (interditRedeveloppement && ferme.count(cle)) {
                            arene.annule();
                            continue;
                        }

                        // Ce test commande l'ENFILAGE, pas seulement l'insertion
                        // dans meilleurG : on n'enfile que si l'état est inconnu,
                        // ou atteint par un chemin strictement meilleur. Le BFS
                        // pouvait se contenter d'un ensemble de vus parce qu'une
                        // FIFO découvre les états dans l'ordre du coût ; A* dépile
                        // par f, pas par g, et n'a pas cette garantie.
                        auto it = meilleurG.find(cle);
                        if (it != meilleurG.end()) {
                            if (gE >= it->second) {
                                arene.annule();
                                continue;
                            }
                            it->second = gE;
                            // L'état est déjà dans l'arène : on réutilise SA clé et
                            // on rend celle qu'on vient d'écrire, sinon chaque
                            // réenfilage d'un état connu la stockerait à nouveau.
                            cle = it->first;
                            arene.annule();
                        } else {
                            meilleurG.emplace(cle, gE);
                        }

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
