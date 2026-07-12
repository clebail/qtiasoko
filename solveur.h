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
    enum EType {
        Bfs
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
    struct Noeud {
        int parent;
        int idxCaisse;
        Game::EDirection dir;
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
