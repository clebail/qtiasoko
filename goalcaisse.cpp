#include "goalcaisse.h"

GoalCaisse::GoalCaisse() {
}

int GoalCaisse::getNbImage() const {
    return 1;
}

QPoint GoalCaisse::getOrigine(int) const {
    return QPoint(SPRITE_WIDTH, SPRITE_HEIGHT);
}
