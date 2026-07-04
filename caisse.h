#ifndef CAISSE_H
#define CAISSE_H

#include "sprite.h"

class Caisse : public Sprite {
public:
    Caisse();
    Sprite* clone() const override { return new Caisse(*this); }
protected:
    virtual int getNbImage() const;
    virtual QPoint getOrigine(int idx) const;
};

#endif // CAISSE_H
