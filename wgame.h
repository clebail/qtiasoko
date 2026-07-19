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
    void showPassage(bool show);
    void setDuree(double duree);
    // Taille naturelle du plateau (sprites fixes 32 px) + une bande d'une case tout
    // autour pour les règles de numéros x/y. minimumSizeHint est la clé : le
    // QScrollArea (widgetResizable) agrandit le widget pour remplir la vue quand
    // elle est plus grande, mais jamais sous ce minimum → barres de défilement dès
    // que le niveau dépasse en hauteur OU en largeur. Aucun zoom.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    virtual void paintEvent(QPaintEvent *) override;
private:
    const Game *game = nullptr;
    qint64 etatsExplores = 0;
    QVector<int> passages;
    Sprite *sprites[NB_SPRITE];
    bool show = false;
    double duree = 0;

    void initSprites();
    void freeSprites();
};

#endif // WGAME_H
