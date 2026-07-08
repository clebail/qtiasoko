#ifndef GAME_H
#define GAME_H

#include <QByteArray>
#include <QPainter>
#include "level.h"
#include "sprite.h"

#define NB_DIRECTION                4
#define NB_COIN_TO_CHECK            2
#define NB_MUR_TO_CHECK             2

class Game {
    // Accès aux membres privés pour les tests unitaires (tests/tst_getetat.cpp).
    friend class TestGetEtat;

public:
    typedef enum { dHaut, dDroite, dBas, dGauche} EDirection;
    typedef struct _SDirection {
        int dx, dy;
    }SDirection;

    typedef struct _SPlayerDirection {
        SDirection direction;
        int playerDirection;
    }SPlayerDirection;

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
    QVector<quint8> getCaissesDeplacable() const;
    bool isLibre(const QPoint& p) const;
private:
    int largeur = 0;
    int hauteur = 0;
    int size = 0;
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
    void checkDefaite();
    short getMinIdx() const;
    bool isLibre(int idx) const;
    QVector<bool> getZoneJoueur() const;
};

#endif // GAME_H
