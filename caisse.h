#ifndef CAISSE_H
#define CAISSE_H

#include "sprite.h"

class Caisse : public Sprite {
public:
    Caisse();
    Sprite* clone() const override { return new Caisse(*this); }
protected:
    int getNbImage() const override;
    QPoint getOrigine(int idx) const override;
};

#endif // CAISSE_H
