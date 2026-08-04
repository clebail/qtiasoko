#ifndef SOLVEUR_H
#define SOLVEUR_H

#include <QAtomicInt>
#include <QThread>
#include <QVector>
#include "game.h"

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
        // Régime d'essai (§6.2, 2026-08-03 — idée utilisateur) : ordre strict sur
        // les seuls buts EN COIN. Cf. solveurastar.h pour le fondement et la limite.
        AstarMacroCouplagePlongeonCoins,
        // Régime d'essai (§6.2, 2026-08-03) : LA LOI DE L'ORDRE — cases mortes
        // recalculées but par but, exemptées par alignement. C'est la formulation
        // qui a survécu au juge là où celle des coins ci-dessus perd le niveau 6.
        // Règle dans game.h, câblage dans solveurastar.h.
        AstarMacroCouplagePlongeonLoi,
        // ❌ RÉGIME RÉFUTÉ LE JOUR MÊME DE SA CRÉATION (2026-08-04) — conservé pour
        // qu'il ne soit pas reproposé. Les deux moitiés du travail du jour ensemble :
        // ordre dynamique + contrainte de porte (QUAND remplir) ET loi de l'ordre +
        // gel hors tour (OÙ ne pas gaspiller ses caisses pendant ce temps).
        //
        // Canari : **les niveaux 5, 6 et 17 passent de résolus à `AUCUNE`**, le 2
        // dérive de 131 à 141 poussées et le 7 de 88 à 92. Chacune des deux moitiés
        // prise SEULE est saine (la loi seule ne perd que le 6, pour une raison
        // documentée ; l'ordre dynamique seul résout les 8).
        //
        // La cause, mesurée : sur le 17, **233 prunes dont 2 seulement de gel** ; sur
        // le 5, 6 565 dont 9. C'est la LOI qui coupe. Et c'est logique après coup —
        // ses cases mortes sont indexées par le but ACTIF, or la loi n'a jamais été
        // validée que contre l'ordre STATIQUE (le gabarit du 16 a été dessiné avec ces
        // rangs-là, `juge_loi.py` a jugé avec eux). L'ordre dynamique rechoisit depuis
        // l'état courant et peut revenir en arrière : une case légitimement utilisée
        // comme garage devient morte dès que le but actif change. **Le « 0 faux
        // positif » de la loi ne se transporte pas à un ordre qui bouge.**
        AstarMacroCouplagePlongeonOrdreLoi
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
    // 8 octets, pas 12 : il y a un Noeud par état DÉCOUVERT, et le niveau 3 en
    // découvre 21,5 M — chaque octet s'y paie en centaines de mégaoctets.
    //
    // ⚠️ 'idxCaisse' est un index de CASE sur la grille (reconstruire() en tire
    // x = idx % largeur, y = idx / largeur), PAS le rang de la caisse parmi les
    // N. Les grilles vont jusqu'à 20x16 = 320 cases : un quint8 y déborderait en
    // silence (la case 300 deviendrait la 44) et corromprait le rejeu sans que
    // le nombre d'états ni le nombre de poussées ne bougent d'un chiffre. D'où
    // le quint16 — qui, avec le padding, tient dans les mêmes 8 octets.
    struct Noeud {
        qint32 parent;
        quint16 idxCaisse;
        quint8 dir;
    };

    Game depart;
    QVector<Noeud> noeuds;

    QList<Game::EDirection> reconstruire(int idx);

private:
    QAtomicInt arret{0};   // posé par le thread GUI, lu par le thread solveur
};

#endif // SOLVEUR_H
