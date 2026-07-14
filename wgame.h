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
    // Nombre de fois qu'une CAISSE a été poussée SUR chaque case depuis le début
    // de la partie (cumulé : une caisse qui repasse incrémente à nouveau). Vide =
    // rien à afficher. Le compteur est tenu par MainWindow, pas par Game : le
    // solveur clone Game des millions de fois, un QVector par état ferait exploser
    // la mémoire.
    void setPassages(const QVector<int>& p);
    static QString formaterMillier(qint64 n);
protected:
    virtual void paintEvent(QPaintEvent *);
private:
    const Game *game = nullptr;
    qint64 etatsExplores = 0;
    QVector<int> passages;
    Sprite *sprites[NB_SPRITE];

    void initSprites();
    void freeSprites();
};

#endif // WGAME_H
