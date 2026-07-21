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

SolveurAStar::SolveurAStar(const Game &etatDepart, int poids, bool macro, QObject *parent)
    : Solveur(etatDepart, parent), poids(poids), macro(macro) {
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


// LIVRAISON=5 : le test de livraison s'applique aux états ENFILÉS (cf. game.h).
// Interrupteur de mesure, à retirer avec le verdict.
static const bool livraisonSurEnfants = (qgetenv("LIVRAISON").toInt() == 5);

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

    // Tampons de flood-fill, hissés HORS de la boucle : réutilisés d'un état à
    // l'autre, ils ne réallouent plus (cf. getZoneJoueur(QVector<bool>&)). Ne
    // JAMAIS en garder une copie ailleurs, sinon le fill() détache et réalloue.
    QVector<bool> zone;        // zone de l'état développé
    QVector<bool> zoneEnfant;  // zone de l'enfant qu'on enfile

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
            emit nouveauMaxCaisses(etat, rangees);   // copie figée pour l'UI (§10)
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
            emit solutionTrouvee(reconstruire(cur.idxNoeud), compteur);
            return;
        }

        // Enfile l'état 'e' (déjà obtenu, non perdu), atteint depuis 'cur' par la
        // suite de poussées 'chaine' ((case caisse, dir)), de coût total gE. Gère
        // la clé en arène, la dédup meilleurG/ferme, la chaîne de noeuds (un par
        // poussée, pour que reconstruire() rejoue une macro à l'identique) et le
        // push_heap. Partagé entre poussées simples et goal macro.
        auto enfiler = [&](Game& e, int gE, const QVector<QPair<int,int>>& chaine) {
            // Deadlock de LIVRAISON (§6.1) : un but vide qu'aucune caisse ne peut
            // plus atteindre. Testé ICI et pas dans checkDefaite — sur un état
            // intermédiaire de goal macro il ferait avorter la macro (mesuré :
            // niveaux 3 et 5 perdus). Ici, la macro va au bout et c'est son
            // RÉSULTAT qu'on juge.
            if (livraisonSurEnfants && e.butNonLivrable(4)) return;
            // getEtat(cle) referait le flood-fill en interne, dans un QVector
            // neuf — un par enfant enfilé. Le tampon évite l'allocation.
            e.getZoneJoueur(zoneEnfant);
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
            file.push_back({gE + poids * hE, gE, parent, cle, score});
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
                for (int i = 0; i < caisses.size(); i++) {
                    if (caisses[i] == 0) continue;   // pas de caisse poussable ici
                    // Écarter AVANT de copier : près d'une tentative sur deux
                    // n'avance même pas d'un pas (48,5 % au niveau 11), et la
                    // copie du plateau était payée pour rien.
                    if (!etat.macroPeutDemarrer(i, but, zone)) continue;
                    Game e(etat);
                    QVector<QPair<int,int>> poussees;
                    // 'zone' est celle d'etat, et e en est une copie non encore
                    // modifiée : elle vaut donc pour le premier pas de la macro.
                    if (e.macroVersBut(i, but, poussees, &zone) && !e.isPerdu()) {
                        enfiler(e, cur.g + poussees.size(), poussees);
                        macrosOk++;
                    }
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
                            enfiler(e, cur.g + 1, {{i, d}});
                    }
                }
            }
        }
    }

    qDebug() << "SolveurAStar: aucune solution," << compteur << "etats explores.";
    emit aucuneSolution();
}
