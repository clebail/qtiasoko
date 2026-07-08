#ifndef WGAME_H
#define WGAME_H

#include <QWidget>
#include "game.h"
#include "sprite.h"

class WGame : public QWidget
{
    Q_OBJECT

public:
    explicit WGame(QWidget *parent = nullptr);
    ~WGame();

    void setGame(const Game *game);
    void setEtatsExplores(qint64 n);
    static QString formaterMillier(qint64 n);
protected:
    virtual void paintEvent(QPaintEvent *);
private:
    const Game *game = nullptr;
    qint64 etatsExplores = 0;
    Sprite *sprites[NB_SPRITE];

    void initSprites();
    void freeSprites();
};

#endif // WGAME_H
