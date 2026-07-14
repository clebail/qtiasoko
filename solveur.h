#ifndef SOLVEUR_H
#define SOLVEUR_H

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
        AstarPondere
    };

    struct SType {
        EType type;
        QString libelle;
    };

    static QVector<SType> types();
    static Solveur* creer(EType type, const Game& etatDepart, QObject* parent = nullptr);

    explicit Solveur(const Game& etatDepart, QObject* parent = nullptr);

signals:
    void solutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores);
    void aucuneSolution();

protected:
    void run() override = 0;

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

    // Offset de la case d'APPUI relative à la caisse — l'opposé du vecteur de
    // déplacement, pas le vecteur lui-même : pour pousser vers 'd', le joueur se
    // tient derrière la caisse.
    static const Game::SDirection appuis[NB_DIRECTION];

    Game depart;
    QVector<Noeud> noeuds;

    QList<Game::EDirection> reconstruire(int idx);
};

#endif // SOLVEUR_H
