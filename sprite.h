#ifndef SPRITE_H
#define SPRITE_H

#include <QImage>

#define SPRITE_WIDTH            32
#define SPRITE_HEIGHT           32

class Sprite {
public:
    Sprite();
    virtual ~Sprite() = 0;
    virtual Sprite* clone() const = 0;
    QImage getImage(int idx) const;
protected:
    virtual int getNbImage() const = 0;
    virtual QPoint getOrigine(int idx) const = 0;
};

#endif // SPRITE_H
