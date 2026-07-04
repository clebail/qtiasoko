#include "goal.h"

Goal::Goal() {
}

int Goal::getNbImage() const {
    return 1;
}

QPoint Goal::getOrigine(int) const {
    return QPoint(0, SPRITE_HEIGHT);
}

