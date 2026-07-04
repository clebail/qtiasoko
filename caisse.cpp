#include "caisse.h"

Caisse::Caisse() {
}

int Caisse::getNbImage() const {
    return 1;
}

QPoint Caisse::getOrigine(int) const {
    return QPoint(SPRITE_WIDTH, 0);
}

