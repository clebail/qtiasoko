#include "caisse.h"

Caisse::Caisse() {
}

int Caisse::getNbImage() const {
    return 1;
}

QRect Caisse::getRect(int) const {
    return QRect(128, 320, 64, 64);   // Crate_Red.png
}
