#ifndef SOLVEUR_H
#define SOLVEUR_H

#include <QAtomicInt>
#include <QThread>
#include <QVector>
#include <vector>
#include <new>
#include <cstdio>
#include "game.h"

// ── CROISSANCE DOUCE DES GROS VECTEURS (§6.5, chantier noeuds/file) ──────────
// `std::vector` double sa capacité. Pour les deux vecteurs qui pèsent en gigaoctets
// — `noeuds` et la file d'A* — ce facteur 2 coûte DEUX fois, et les deux ont été
// mesurés sur le 29 à 235,7 M états vus :
//   1. LE VIDE PERMANENT. La file portait 78,5 M éléments dans une capacité de
//      134,2 M : **1 264 Mo alloués et jamais écrits**, soit 10 % du solveur. Sur un
//      cycle de doublement la capacité vaut en moyenne 1,5x le contenu — 33 % de
//      vide. À 1,25 elle vaut 1,125x, soit 11 %.
//   2. LA POINTE. Une réallocation détient l'ancien tableau ET le nouveau : 3 + 6 =
//      9 Go le temps d'un doublement de la file, contre 3 + 3,75 = 6,75 à 1,25.
//      C'est exactement la pointe qui a tué des runs sur `TableG` (§6.5).
//
// Ce qu'on paie : le trafic total de recopie vaut N/(k-1) éléments, donc 4N à 1,25
// contre N à 2. Sur une file de 1,9 Go utile cela fait ~7,5 Go de memcpy sur TOUT le
// run — de l'ordre de la seconde, contre des heures de recherche. Le compte est sans
// appel dans ce sens-là.
//
// ⚠️ Et surtout : **ça ne change RIEN à la trajectoire**. Ni l'ordre de dépilement, ni
// les états, ni les poussées — seule la capacité allouée bouge. Contrairement à un
// tie-break ou à un élagage, le canari ne peut PAS bouger ; s'il bouge, c'est un bug,
// pas un arbitrage.
// ⚠️ `reserve` d'une valeur explicite est honorée à l'octet par libstdc++, là où
// `push_back` seul re-double. Il faut donc appeler reserve AVANT chaque push qui
// remplirait la capacité, pas se reposer sur la croissance implicite.
template <typename V>
inline void reserveDouce(V& v) {
    if (v.size() == v.capacity())
        v.reserve(v.capacity() + v.capacity() / 4 + 16);
}

// Base abstraite des solveurs. Tourne dans son propre thread : une résolution
// explore potentiellement un très grand nombre d'états, hors de question de
// bloquer le thread GUI le temps du calcul. Le résultat sort par signal plutôt
// que par valeur de retour.
//
// Une sous-classe n'a qu'à implémenter run() : elle hérite de la machinerie
// commune (état de départ, arbre des noeuds, reconstruction du chemin) et se
// contente de choisir sa stratégie d'exploration.
class Solveur : public QThread {
    Q_OBJECT

public:
    // Stratégies disponibles. L'UI peuple son select à partir de types() et
    // instancie via creer() : ajouter un solveur = une entrée ici, une ligne
    // dans types() et un cas dans creer(), rien à toucher côté MainWindow.
    // 'Astar' et non 'AStar' : la classe de pathfinding d'astar.h porte déjà ce
    // nom, et un énumérateur homonyme le masquerait dans toute la portée de
    // Solveur — reconstruire() ne pourrait plus construire un AStar.
    enum EType {
        Bfs,
        Astar,
        AstarPondere,
        AstarMacro,
        // Même chose, mais la macro pousse en PRIORITÉ la caisse que le couplage
        // hongrois destine au but actif (cf. plan.md §6.3, 2026-07-24). Régime
        // ALTERNATIF — AstarMacro reste le défaut ; les deux se comparent sur le
        // même binaire, sans variable d'environnement : sur le 12, 100 %
        // des enfants de macro partaient à f+2 parce que la caisse posée volait
        // son but à une autre. Repli sur les autres caisses si celle-là ne passe pas.
        AstarMacroCouplage,
        // A* macro + PLONGEON SUR RECORD (§6.0, 2026-07-28). Dès qu'un état bat le
        // max de caisses posées ET qu'il dépasse le seuil de remplissage, on tente
        // de le COMPLÉTER par une recherche gloutonne bornée avant de reprendre
        // l'A* normal. Renonce à l'optimalité — mais mesuré : l'optimum exact sur
        // 7 des 8 niveaux testés, +2 poussées sur le 4, pour ×33 d'états sur le 4
        // et ×4,2 sur le 9.
        AstarMacroPlongeon,
        // Les DEUX régimes d'essai ensemble : la macro vise la caisse du couplage,
        // ET on plonge sur chaque record. Créé pour le 11 (2026-07-28) — c'est le
        // couplage qui l'amène à 11/14 (le macro seul plafonne à 8/14), et le 11/14
        // est prouvé complétable (fixture level0194, 9 états macro). Le plongeon a
        // donc une cible réelle, avec un budget colossal vu le travail déjà consenti.
        AstarMacroCouplagePlongeon,
        // Idem + ORDRE DYNAMIQUE (§6.2, chantier 2026-07-31). `butActif()` ne rend plus
        // le premier but non rempli de l'ordre statique, mais le premier qui soit encore
        // LIVRABLE depuis l'état courant. Motivation : trois niveaux (13, 18, 22) ont un
        // ordre MURÉ calculé au chargement, et le 18 prouve qu'aucun budget de recherche
        // n'y changera rien — aucun ordre sain complet n'existe dans ce modèle statique.
        // Décider le but suivant DEPUIS L'ÉTAT fait disparaître la question. Régime
        // d'ESSAI, jamais le défaut : le canari reste sur les régimes existants.
        AstarMacroCouplagePlongeonOrdre,
        // ORDRE-LOOK (§6.2, 2026-08-08) : au rang 0 du calcul de l'ordre, et parmi
        // les buts de la SALLE que la règle existante a élue, préférer celui qui
        // laisse le plus de candidats SÛRS au rang suivant. Régime SÉPARÉ, jamais le
        // défaut — il fait tomber le 12 sans injection (2 097 523 états, l'ordre
        // régénéré est exactement l'ordre humain de juillet) mais fait DÉCROCHER le
        // 32, qui est résolu. Tout le reste est inchangé : 26 ordres sur 35 sont
        // bit-à-bit identiques, donc le canari est préservé par construction.
        AstarMacroCouplagePlongeonLook
    };

    struct SType {
        EType type;
        QString libelle;
    };

    static QVector<SType> types();
    static Solveur* creer(EType type, const Game& etatDepart, QObject* parent = nullptr);

    explicit Solveur(const Game& etatDepart, QObject* parent = nullptr);

    // Demande l'arrêt de la recherche, depuis le thread GUI. COOPÉRATIF : on ne
    // tue pas le thread (il tient des conteneurs de plusieurs centaines de Mo,
    // un terminate() les fuirait et laisserait l'arène à moitié écrite), on pose
    // un drapeau que la boucle de run() consulte à chaque dépilement. Le thread
    // sort alors de lui-même, en émettant rechercheArretee().
    void demanderArret() { arret.storeRelaxed(1); }

signals:
    void solutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores);
    void aucuneSolution();
    // La recherche s'est interrompue sur demande (cf. demanderArret()), sans
    // conclure. Distinct d'aucuneSolution() : rien n'est prouvé sur le niveau.
    void rechercheArretee(qint64 etatsExplores);
    // Émis quand la recherche bat son record de caisses rangées (§10, diagnostic) :
    // porte une copie de l'état atteint et le nombre de caisses posées. L'UI peut
    // l'afficher pour voir OÙ le solveur se coince (ex. niveau 4 plafonné à 17/20).
    //
    // 'chemin' porte en plus la suite de coups qui MÈNE à cet état, pour pouvoir le
    // rejouer pas à pas — c'est le seul moyen de voir COMMENT un run qui n'aboutit
    // pas en est arrivé là (sur un run gagnant, solutionTrouvee suffit). Reconstruit
    // à chaque record seulement : rare (au plus une fois par but), donc le coût des
    // AStar de marche de reconstruire() est négligeable.
    void nouveauMaxCaisses(Game etatMax, int nbRangees, QList<Game::EDirection> chemin);

public:
    // Offset de la case d'APPUI relative à la caisse — l'opposé du vecteur de
    // déplacement, pas le vecteur lui-même : pour pousser vers 'd', le joueur se
    // tient derrière la caisse.
    //
    // PUBLIC depuis le mode hybride (2026-08-01) : l'UI descend elle aussi des
    // poussées en coups de marche (MainWindow::joueMacro, même recette que
    // reconstruire()). Exemplaire unique — recopier cette table ailleurs, c'est
    // se garantir qu'un jour les deux ne diront plus la même chose.
    static const Game::SDirection appuis[NB_DIRECTION];

protected:
    void run() override = 0;

    // À consulter en tête de la boucle d'exploration. Lecture 'relaxed' : le
    // drapeau n'ordonne aucune autre donnée entre les deux threads, on ne veut
    // pas payer une barrière mémoire par état dépilé.
    bool arretDemande() const { return arret.loadRelaxed() != 0; }

    // Un noeud par état enfilé : 'parent' pointe vers son index dans 'noeuds'
    // (-1 pour la racine), et (idxCaisse, dir) est la POUSSÉE qui y mène depuis
    // ce parent.
    //
    // On n'y stocke délibérément PAS le trajet de marche : le calculer pour
    // chaque enfant généré revenait à lancer un AStar complet sur la grille
    // avant même de savoir si l'enfant était un doublon — et l'immense majorité
    // le sont. Ce trajet ne sert qu'à l'affichage, jamais à l'identité d'un
    // état : reconstruire() le recalcule donc une seule fois, le long de la
    // solution, en rejouant les poussées depuis 'depart'.
    // ⚠️ 'idxCaisse' est un index de CASE sur la grille (reconstruire() en tire
    // x = idx % largeur, y = idx / largeur), PAS le rang de la caisse parmi les
    // N. Les grilles vont jusqu'à 20x16 = 320 cases : un quint8 y déborderait en
    // silence (la case 300 deviendrait la 44) et corromprait le rejeu sans que
    // le nombre d'états ni le nombre de poussées ne bougent d'un chiffre.
    //
    // ── DEUX TABLEAUX PARALLÈLES, 6 OCTETS (§6.5, chantier noeuds) ────────────
    // C'était `struct Noeud { qint32 parent; quint16 idxCaisse; quint8 dir; }`,
    // soit 7 octets utiles repadés à 8 par l'alignement du qint32. Même forme et
    // même remède que TableG le 2026-08-11 : séparer les champs supprime le
    // padding, parce qu'un tableau de quint16 n'a pas à s'aligner sur 4.
    //   parent (4 o) + (case << 2 | dir) (2 o) = **6 octets au lieu de 8, −25 %**.
    // `noeuds` pèse 28 % du solveur sur le 26 et 1,36 entrée par état vu (la goal
    // macro pose un noeud PAR POUSSÉE de sa chaîne, pour que reconstruire() la
    // rejoue) : c'est donc ~7 % du total.
    //
    // Bénéfice second, comme pour TableG : la remontée de reconstruire() ne lit
    // QUE 'parents', donc 16 parents par ligne de cache au lieu de 8 noeuds.
    // Et la POINTE de réallocation baisse : deux vecteurs de 4 et 2 octets qui
    // doublent chacun de leur côté demandent au pire 1,5x4 + 2 = 8 n octets, là
    // où un seul vecteur de 8 en demandait 12 n.
    //
    // ⚠️ `dir` ne prend que 2 bits (NB_DIRECTION = 4) et `idxCaisse` 9 (320 cases
    // au plus), soit 11 bits sur les 16 : la garde ci-dessous est là parce que le
    // §7 collectionne les troncatures muettes — un débordement ne planterait pas,
    // il rejouerait la mauvaise case et le nombre d'états ne bougerait pas d'un
    // chiffre.
    // ⚠️ La racine n'a pas de parent. Le sentinelle est RACINE et non -1 : les
    // index sont désormais non signés (un qint32 plafonnerait à 2,1 G noeuds, et
    // le plus gros run du projet en a déjà vu 640 M).
    class ArbreNoeuds {
    public:
        static const quint32 RACINE = 0xFFFFFFFFu;

        void clear() { parents.clear(); caisseDir.clear(); }
        size_t size() const { return parents.size(); }
        // Octets RÉELLEMENT alloués : c'est la CAPACITÉ qui pèse, pas l'occupation
        // (un vecteur à moitié plein coûte son tableau entier). Utilisé par le
        // relevé [MEM] de solveurastar.cpp.
        size_t octets() const {
            return parents.capacity() * sizeof(quint32) + caisseDir.capacity() * sizeof(quint16);
        }
        void resize(size_t n) { parents.resize(n); caisseDir.resize(n); }

        // Ajoute un noeud et rend son index. 'parent' = RACINE pour la racine.
        quint32 ajoute(quint32 parent, int idxCaisse, int dir) {
            Q_ASSERT_X(idxCaisse >= 0 && idxCaisse < 16384, "ArbreNoeuds::ajoute",
                       "idxCaisse deborde les 14 bits (§6.5)");
            Q_ASSERT_X(dir >= 0 && dir < 4, "ArbreNoeuds::ajoute",
                       "dir deborde les 2 bits (§6.5)");
            try {
                reserveDouce(parents);
                reserveDouce(caisseDir);
                parents.push_back(parent);
                caisseDir.push_back((quint16)((idxCaisse << 2) | dir));
            } catch (const std::bad_alloc&) {
                // Même diagnostic que l'arène : nommer le conteneur qui refuse.
                // Trois hypothèses fausses ont précédé la bonne le 2026-08-13
                // faute de cette ligne (§6.5).
                fprintf(stderr, "[BADALLOC] NOEUDS : %zu entrees (%.0f Mo) REFUSE\n",
                        parents.size(), octets() / 1048576.0);
                fflush(stderr);
                throw;
            }
            return (quint32)(parents.size() - 1);
        }

        quint32 parent(quint32 i) const   { return parents[i]; }
        quint16 idxCaisse(quint32 i) const { return (quint16)(caisseDir[i] >> 2); }
        quint8  dir(quint32 i) const       { return (quint8)(caisseDir[i] & 3); }

    private:
        // ⚠️ std::vector et NON QVector (§6.5, 2026-08-13). QVector plafonne à 2 Go —
        // limite des tailles en `int` de Qt 5 — et refuse au-delà QUELLE QUE SOIT la
        // mémoire libre. Mesuré en isolation : à 2 048 Mo QVector échoue là où
        // std::vector passe, et à 4 096 Mo aussi.
        // C'est ce plafond, et non la RAM ni une pointe de réhachage, qui tuait le
        // niveau 29 sur `std::bad_alloc` avec 11 Go encore libres — trois runs de
        // suite, au même dépilement puisque la recherche est déterministe.
        std::vector<quint32> parents;
        std::vector<quint16> caisseDir;   // idxCaisse << 2 | dir
    };

    Game depart;
    ArbreNoeuds noeuds;

    QList<Game::EDirection> reconstruire(int idx);

private:
    QAtomicInt arret{0};   // posé par le thread GUI, lu par le thread solveur
};

#endif // SOLVEUR_H
