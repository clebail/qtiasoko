#ifndef CAISSE_H
#define CAISSE_H

#include "sprite.h"

// Crate_Red : une caisse qui n'est PAS sur un but. Coins arrondis transparents,
// donc le sol doit être dessiné dessous.
class Caisse : public Sprite {
public:
    Caisse();
    Sprite* clone() const override { return new Caisse(*this); }
protected:
    int getNbImage() const override;
    QRect getRect(int idx) const override;
};

#endif // CAISSE_H
