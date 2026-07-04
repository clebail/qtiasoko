#include "player.h"

Player::Player() {

}

int Player::getNbImage() const {
    return 3;
}

QPoint Player::getOrigine(int idx) const {
    int x = (idx % 2) * SPRITE_WIDTH;
    int y = (idx / 2) * SPRITE_HEIGHT + (2 * SPRITE_HEIGHT);

    return QPoint(x, y);
}
