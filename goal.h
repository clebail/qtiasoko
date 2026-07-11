#ifndef GOAL_H
#define GOAL_H

#include "sprite.h"

class Goal : public Sprite {
public:
    Goal();
    Sprite* clone() const override { return new Goal(*this); }
protected:
    int getNbImage() const override;
    QPoint getOrigine(int idx) const override;
};

#endif // GOAL_H
