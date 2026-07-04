#include "sprite.h"

Sprite::Sprite() {
}

Sprite::~Sprite() {

}

QImage Sprite::getImage(int idx) const {
    const QPoint origine = getOrigine(idx);
    return QImage(":/sprites.png").copy(QRect(origine, QPoint(origine.x() + SPRITE_WIDTH - 1, origine.y() + SPRITE_HEIGHT - 1)));
}
