#include "mur.h"

Mur::Mur() {

}

int Mur::getNbImage() const {
    return 1;
}

QPoint Mur::getOrigine(int) const {
    return QPoint(0, 0);
}
