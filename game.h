#ifndef GAME_H
#define GAME_H

#include <QByteArray>
#include <QPainter>
#include "level.h"
#include "sprite.h"

#define NB_DIRECTION                4

class Game {
public:
    typedef enum { dHaut, dDroite, dBas, dGauche} EDirection;
    typedef struct _SDirection {
        int dx, dy;
        int pd;
    }SDirection;

    Game();
    Game(const Level& level, int numNiveau = 1);
    Game(const Game& other);
    Game& operator=(const Game& other);
    ~Game();
    bool isLoaded() const;
    void draw(QPainter *painter) const;
    bool haut();
    bool droite();
    bool bas();
    bool gauche();
    int getNbDep() const { return nbDep; }
    int getNbDepCaisse() const { return nbDepCaisse; }
    int getNumNiveau() const { return numNiveau; }
    bool isGagne() const { return gagne; }
    bool isPerdu() const { return perdu; }
    QByteArray getEtat() const;
private:
    int largeur = 0;
    int hauteur = 0;
    QPoint playerPoint;
    int playerDirection = 0;
    Level::ETypeCase *cases = nullptr;
    int nbDep = 0;
    int nbDepCaisse = 0;
    int numNiveau = 1;
    bool gagne = false;
    bool perdu = false;
    Sprite *sprites[NB_SPRITE];

    void initSprites();
    void freeSprites();
    void cloneSprites(const Game& other);
    bool move(EDirection dir);
    bool moveCaisse(Level::ETypeCase *cases, QPoint playerPoint, QPoint caissePoint, SDirection direction);
    void checkVictoire();
};

#endif // GAME_H
