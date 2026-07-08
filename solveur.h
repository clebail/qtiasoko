#ifndef SOLVEUR_H
#define SOLVEUR_H

#include <QThread>
#include <QVector>
#include "game.h"

// Tourne dans son propre thread : solve() explore potentiellement un très
// grand nombre d'états, hors de question de bloquer le thread GUI le temps
// du calcul. Le résultat sort par signal plutôt que par valeur de retour.
class Solveur : public QThread {
    Q_OBJECT

public:
    explicit Solveur(const Game &etatDepart, QObject *parent = nullptr);

signals:
    void solutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores);
    void aucuneSolution();

protected:
    void run() override;

private:
    // Un noeud par état enfilé : 'parent' pointe vers son index dans
    // 'noeuds' (-1 pour la racine), 'coups' est la marche + poussée qui y
    // mène depuis ce parent. Permet de reconstruire le chemin complet une
    // fois un état gagnant trouvé, sans le porter dans la file elle-même.
    struct Noeud {
        int parent;
        QList<Game::EDirection> coups;
    };

    Game depart;
    QVector<Noeud> noeuds;

    QList<Game::EDirection> reconstruire(int idx);
};

#endif // SOLVEUR_H
