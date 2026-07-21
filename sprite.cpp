#include <QPainter>
#include "sprite.h"

Sprite::Sprite() {
}

Sprite::~Sprite() {

}

const QPixmap& Sprite::planche() {
    static const QPixmap p(":/sprites.png");
    return p;
}

void Sprite::dessine(QPainter& p, const QPointF& coin, int idx) const {
    const QRect src = getRect(idx);

    // Un seul facteur, appliqué en x comme en y : les sprites ne se déforment
    // pas quand on change SPRITE_WIDTH. (Les cases sont carrées,
    // SPRITE_WIDTH == SPRITE_HEIGHT.)
    const qreal k = qreal(SPRITE_WIDTH) / TUILE_SOURCE;
    const qreal w = src.width()  * k;
    const qreal h = src.height() * k;

    QPointF pos = coin;
    switch (getAncrage()) {
        case aCentre: pos += QPointF((SPRITE_WIDTH - w) / 2.0, (SPRITE_HEIGHT - h) / 2.0); break;
        case aPose:   pos += QPointF((SPRITE_WIDTH - w) / 2.0,  SPRITE_HEIGHT - h);        break;
        case aRemplit: break;
    }

    p.drawPixmap(QRectF(pos, QSizeF(w, h)), planche(), src);
}
