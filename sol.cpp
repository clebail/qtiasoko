#include "sol.h"

Sol::Sol() {
}

int Sol::getNbImage() const {
    return 1;
}

QRect Sol::getRect(int) const {
    return QRect(64, 128, 64, 64);   // GroundGravel_Sand.png
}

SolHors::SolHors() {
}

int SolHors::getNbImage() const {
    return 1;
}

QRect SolHors::getRect(int) const {
    return QRect(128, 0, 64, 64);    // Ground_Sand.png
}
