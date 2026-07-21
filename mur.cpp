#include "mur.h"

Mur::Mur() {

}

int Mur::getNbImage() const {
    return 1;
}

QRect Mur::getRect(int) const {
    return QRect(0, 320, 64, 64);   // Wall_Brown.png
}
