#include "goalcaisse.h"

GoalCaisse::GoalCaisse() {
}

int GoalCaisse::getNbImage() const {
    return 1;
}

QRect GoalCaisse::getRect(int) const {
    return QRect(192, 192, 64, 64);   // Crate_Blue.png
}
