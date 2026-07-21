#include "goal.h"

Goal::Goal() {
}

int Goal::getNbImage() const {
    return 1;
}

QRect Goal::getRect(int) const {
    return QRect(128, 384, 32, 32);   // EndPoint_Blue.png
}
