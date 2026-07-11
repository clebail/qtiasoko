#ifndef SOLVEURBFS_H
#define SOLVEURBFS_H

#include "solveur.h"

// Parcours en largeur sur les états caisses+zone : optimal en nombre de
// poussées (chaque arête de la file est une marche suivie d'une poussée), au
// prix d'une exploration exhaustive palier par palier — la file explose sur
// les niveaux à beaucoup de caisses.
class SolveurBFS : public Solveur {
    Q_OBJECT

public:
    explicit SolveurBFS(const Game& etatDepart, QObject* parent = nullptr);

protected:
    void run() override;
};

#endif // SOLVEURBFS_H
