#ifndef ASTAR_H
#define ASTAR_H

#include <QMap>
#include "game.h"

class AStar {
public:
    AStar(const Game *game);
    QList<Game::EDirection> getChemin(const QPoint& start, const QPoint& goal);
private:
    typedef struct _Node {
        int x;
        int y;
        int cout;
        int heuristique;

        _Node(int x, int y, int cout, int heuristique) {
            this->x = x;
            this->y = y;
            this->cout = cout;
            this->heuristique = heuristique;
        }

        int compare(const struct _Node& n1, const struct _Node& n2) {
            if (n1.heuristique < n2.heuristique) return 1;
            if (n1.heuristique == n2.heuristique) return 0;
            return -1;
        }
    }Node;

    const Game *game;
    QList<Node> closedList;
    QList<Node> openList;
    QMap<QString, int> scores;
    QMap<QString, QPair<QPoint, Game::EDirection> > cameFrom;

    double heuristique(const QPoint& start, const QPoint& goal) const;
    QString key(int x, int y) const;
    QList<Game::EDirection> reconstuire(const QPoint& goal) const;
};

#endif // ASTAR_H
