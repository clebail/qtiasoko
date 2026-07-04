#ifndef PLAYER_H
#define PLAYER_H

#include "sprite.h"

class Player : public Sprite
{
public:
    Player();
    Sprite* clone() const override { return new Player(*this); }
protected:
    virtual int getNbImage() const;
    virtual QPoint getOrigine(int idx) const;
};

#endif // PLAYER_H
