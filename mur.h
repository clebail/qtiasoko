#ifndef MUR_H
#define MUR_H

#include "sprite.h"

class Mur : public Sprite {
public:
    Mur();
    Sprite* clone() const override { return new Mur(*this); }
protected:
    int getNbImage() const override;
    QPoint getOrigine(int idx) const override;
};

#endif // MUR_H
