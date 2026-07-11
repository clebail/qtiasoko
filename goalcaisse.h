#ifndef GOALCAISSE_H
#define GOALCAISSE_H

#include "sprite.h"

class GoalCaisse : public Sprite {
public:
    GoalCaisse();
    Sprite* clone() const override { return new GoalCaisse(*this); }
protected:
    int getNbImage() const override;
    QPoint getOrigine(int idx) const override;
};

#endif // GOALCAISSE_H
