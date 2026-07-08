#include <math.h>
#include <limits.h>
#include "astar.h"

typedef struct _Direction {
    int dx;
    int dy;
    Game::EDirection direction;

    _Direction(int dx, int dy, Game::EDirection direction) {
        this->dx = dx;
        this->dy = dy;
        this->direction = direction;
    }
}Direction;

static Direction directions[NB_DIRECTION] = { Direction(0, -1, Game::dHaut), Direction(1, 0, Game::dDroite), Direction(0, 1, Game::dBas), Direction(-1, 0, Game::dGauche) };

AStar::AStar(const Game *game) {
    this->game = game;
}

QList<Game::EDirection> AStar::getChemin(const QPoint& start, const QPoint& goal) {
    int sx = start.x();
    int sy = start.y();
    int gx = goal.x();
    int gy = goal.y();

    if (sx == gx && sy == gy) {
        return QList<Game::EDirection>();
    }

    auto startKey = key(sx, sy);

    scores.insert(startKey, 0);
    openList.append(Node(sx, sy, 0, heuristique(start, goal)));

    while(openList.size() > 0) {
        // Sélection A* : le nœud de f = cout + heuristique minimal.
        int best = 0;
        for(int i = 1; i < openList.size(); i++) {
            if(openList[i].cout + openList[i].heuristique <
               openList[best].cout + openList[best].heuristique) {
                best = i;
            }
        }
        Node u = openList.takeAt(best);
        auto uKey = key(u.x, u.y);

        if (u.cout > scores.value(uKey, INT_MAX)) {
            continue;
        }

        if(u.x == gx && u.y == gy) {
            return reconstuire(goal);
        }

        for(int i=0;i<NB_DIRECTION;i++) {
            Direction dir = directions[i];
            int nx = u.x + dir.dx;
            int ny = u.y + dir.dy;
            QPoint p = QPoint(nx, ny);

            if (!game->isLibre(p) && p != goal) {
                continue;   // la case but est autorisée même si occupée (ex: la queue)
            }

            int tentative = u.cout + 1;
            auto vKey = key(nx, ny);

            if (!scores.contains(vKey) || tentative < scores.value(vKey)) {
                scores.insert(vKey, tentative);
                cameFrom.insert(vKey, QPair<QPoint, Game::EDirection>(QPoint(u.x, u.y), dir.direction));
                openList.append(Node(nx, ny, tentative, heuristique(p, goal)));
            }
        }
    }

    return QList<Game::EDirection>();
}

double AStar::heuristique(const QPoint& start, const QPoint& goal) const
{
    return abs(start.x() - goal.x()) + abs(start.y() - goal.y());
}

QString AStar::key(int x, int y) const {
    return QString("%1-%2").arg(x).arg(y);
}

QList<Game::EDirection> AStar::reconstuire(const QPoint& goal) const {
    QList<Game::EDirection> chemin;
    auto courant = key(goal.x(), goal.y());
    auto prov = cameFrom.value(courant);

    while (cameFrom.contains(courant)) {
        chemin.insert(0, prov.second);
        courant = key(prov.first.x(), prov.first.y());
        prov = cameFrom.value(courant);
    }

    return chemin;
}
