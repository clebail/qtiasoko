#ifndef SPRITE_H
#define SPRITE_H

#include <QPixmap>
#include <QRect>

// ---------------------------------------------------------------------------
// LE SEUL RÉGLAGE DE TAILLE
//
// Côté d'une case à l'écran. La planche (sprites.png, cf. sprites.xml) est
// dessinée en 64 : 64 = taille native, sans redimensionnement.
//
// Mettre 32 divise TOUT par deux d'un coup — tuiles, buts, perso, règles de
// numéros, marges — et rien d'autre n'est à toucher. C'est une réduction de
// facteur entier, donc nette. À faire si le plateau devient trop grand pour
// l'écran : le plus haut niveau fait 34 cases, soit 2176 px en 64 contre 1088
// en 32.
// ---------------------------------------------------------------------------
#define SPRITE_WIDTH            32
#define SPRITE_HEIGHT           32

// Côté d'une tuile pleine DANS la planche. C'est l'unité de référence de
// l'échelle : les sous-sprites qui ne font pas 64x64 (le but en 32x32, le perso
// en 37x59) sont réduits du même facteur, donc gardent leurs proportions les
// uns par rapport aux autres. Ne décrit pas l'affichage — ne pas y toucher.
#define TUILE_SOURCE            64

class QPainter;

class Sprite {
public:
    // Où le sous-sprite se pose dans sa case. Toutes les sous-textures ne font
    // pas la taille d'une case : le but est un petit disque, le perso est plus
    // haut que large.
    typedef enum {
        aRemplit,   // tuile pleine case : sol, mur, caisse
        aCentre,    // centré dans la case : le but
        aPose       // centré en x, posé sur le bas de la case : le perso
    } EAncrage;

    Sprite();
    virtual ~Sprite() = 0;
    virtual Sprite* clone() const = 0;

    // Dessine la frame idx dans la case dont le coin haut-gauche est en 'coin'.
    // 'coin' est un QPointF : pendant une animation, le perso et la caisse
    // poussée sont à une position interpolée, entre deux cases.
    void dessine(QPainter& p, const QPointF& coin, int idx = 0) const;

    virtual int getNbImage() const = 0;
protected:
    // Sous-texture dans la planche, en coordonnées natives (celles de
    // sprites.xml, à recopier telles quelles).
    virtual QRect getRect(int idx) const = 0;
    virtual EAncrage getAncrage() const { return aRemplit; }
private:
    // La planche, décodée UNE fois pour toute l'application. L'ancienne version
    // construisait un QImage(":/sprites.png") dans getImage(), donc décodait le
    // PNG entier une fois par case et par rafraîchissement. Invisible en 32 px
    // sur un plateau figé, intenable à 60 images/s.
    static const QPixmap& planche();
};

#endif // SPRITE_H
