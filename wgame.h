#ifndef WGAME_H
#define WGAME_H

#include <QWidget>
#include "game.h"

class WGame : public QWidget
{
    Q_OBJECT

public:
    explicit WGame(QWidget *parent = nullptr);
    ~WGame();

    void setGame(const Game *game);
protected:
    virtual void paintEvent(QPaintEvent *);
private:
    const Game *game = nullptr;
};

#endif // WGAME_H
