#ifndef GOAL_H
#define GOAL_H

#include "sprite.h"

class Goal : public Sprite {
public:
    Goal();
    Sprite* clone() const override { return new Goal(*this); }
protected:
    virtual int getNbImage() const;
    virtual QPoint getOrigine(int idx) const;
};

#endif // GOAL_H
