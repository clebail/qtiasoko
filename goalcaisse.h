#ifndef GOALCAISSE_H
#define GOALCAISSE_H

#include "sprite.h"

class GoalCaisse : public Sprite {
public:
    GoalCaisse();
    Sprite* clone() const override { return new GoalCaisse(*this); }
protected:
    virtual int getNbImage() const;
    virtual QPoint getOrigine(int idx) const;
};

#endif // GOALCAISSE_H
