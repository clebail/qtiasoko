#ifndef MUR_H
#define MUR_H

#include "sprite.h"

// Wall_Brown. Tuile pleine et opaque : rien n'est dessiné dessous.
class Mur : public Sprite {
public:
    Mur();
    Sprite* clone() const override { return new Mur(*this); }
protected:
    int getNbImage() const override;
    QRect getRect(int idx) const override;
};

#endif // MUR_H
