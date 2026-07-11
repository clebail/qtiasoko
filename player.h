#ifndef PLAYER_H
#define PLAYER_H

#include "sprite.h"

class Player : public Sprite
{
public:
    Player();
    Sprite* clone() const override { return new Player(*this); }
protected:
    int getNbImage() const override;
    QPoint getOrigine(int idx) const override;
};

#endif // PLAYER_H
