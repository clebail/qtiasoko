#ifndef GOALCAISSE_H
#define GOALCAISSE_H

#include "sprite.h"

// Crate_Blue : une caisse POSÉE sur un but. Elle couvre le but, qui n'est donc
// pas dessiné dessous — mais le sol l'est (coins arrondis transparents).
class GoalCaisse : public Sprite {
public:
    GoalCaisse();
    Sprite* clone() const override { return new GoalCaisse(*this); }
protected:
    int getNbImage() const override;
    QRect getRect(int idx) const override;
};

#endif // GOALCAISSE_H
