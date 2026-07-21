#ifndef GOAL_H
#define GOAL_H

#include "sprite.h"

// EndPoint_Blue : un but vide. C'est un petit disque de 32x32, soit le quart de
// la surface d'une case — il se centre sur le sol au lieu de le remplacer.
class Goal : public Sprite {
public:
    Goal();
    Sprite* clone() const override { return new Goal(*this); }
protected:
    int getNbImage() const override;
    QRect getRect(int idx) const override;
    EAncrage getAncrage() const override { return aCentre; }
};

#endif // GOAL_H
