#include <QtDebug>
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

SolveurAStar::SolveurAStar(const Game &etatDepart, int poids, QObject *parent)
    : Solveur(etatDepart, parent), poids(poids) {
}

#ifdef DUMP_DEV
// Uniquement pour l'instrumentation hors-ligne (harnais de mesure). Un seul
// thread solveur tourne à la fois, pas de verrou.
std::vector<std::pair<QByteArray,int>>& etatsDeveloppes() {
    static std::vector<std::pair<QByteArray,int>> v;
    return v;
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

    while(file.size()) {
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
        if (compteur % 1000 == 0) {
            qDebug() << "SolveurAStar(w=" << poids << "):" << compteur << "etats depiles, file =" << file.size()
                     << ", vus =" << meilleurG.size() << ", f =" << cur.f;
        }

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
        etat.appliqueEtat(arene.lit(cur.cle.offset));

#ifdef DUMP_DEV
        // Les états RÉELLEMENT dépilés — et non l'ensemble {f <= C*}, qui est
        // ~25x plus gros : A* s'arrête dès qu'il atteint le but et n'en visite
        // qu'une fraction. Échantillonner {f <= C*} au lieu de ceci fausse toute
        // mesure portant sur « ce que le solveur explore vraiment ».
        etatsDeveloppes().push_back({etat.getEtat(), cur.g});
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

                    if(e.pousse(i, (Game::EDirection)d) && !e.isPerdu()) {
                        // La clé s'écrit directement en fin d'arène — aucune
                        // allocation. Si l'enfant se révèle être un doublon, on
                        // la reprend par annule() : elle y figure déjà.
                        e.getEtat(arene.reserve());
                        Cle cle{arene.dernier()};

                        int gE = cur.g + 1;   // une poussée = une arête de coût 1

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
                        TableG::Slot* slot = meilleurG.cherche(cle);
                        if (slot) {
                            if (gE >= slot->g) {
                                arene.annule();
                                continue;
                            }
                            slot->g = gE;
                            // L'état est déjà dans l'arène : on réutilise SA clé et
                            // on rend celle qu'on vient d'écrire, sinon chaque
                            // réenfilage d'un état connu la stockerait à nouveau.
                            cle = slot->cle;
                            arene.annule();
                        } else {
                            meilleurG.insere(cle, gE);
                        }

                        noeuds.append(Noeud{cur.idxNoeud, (quint16)i, (quint8)d});

                        qint64 score;
                        const int hE = e.getHeuristique(&score);
                        file.push_back({gE + poids * hE, gE, noeuds.size()-1, cle, score});
                        std::push_heap(file.begin(), file.end(), compare);
                    }
                }
            }
        }
    }

    qDebug() << "SolveurAStar: aucune solution," << compteur << "etats explores.";
    emit aucuneSolution();
}
