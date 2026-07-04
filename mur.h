#ifndef MUR_H
#define MUR_H

#include "sprite.h"

class Mur : public Sprite {
public:
    Mur();
    Sprite* clone() const override { return new Mur(*this); }
protected:
    virtual int getNbImage() const;
    virtual QPoint getOrigine(int idx) const;
};

#endif // MUR_H
