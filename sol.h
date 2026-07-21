#ifndef SOL_H
#define SOL_H

#include "sprite.h"

// Les deux sols. Ils ne diffèrent que par la texture, et n'existent que l'un
// par rapport à l'autre : c'est leur contraste qui dessine le contour du
// plateau, là où le fichier .xsb ne distingue pas « sol vide » de « rien »
// (les deux sont un espace). WGame tranche par un remplissage depuis le joueur.

// GroundGravel_Sand : le sol praticable, à l'intérieur du plateau.
class Sol : public Sprite {
public:
    Sol();
    Sprite* clone() const override { return new Sol(*this); }
protected:
    int getNbImage() const override;
    QRect getRect(int idx) const override;
};

// Ground_Sand : ce qui est hors du plateau. Uni, là où le sol intérieur est
// grenu — le joueur voit d'un coup d'œil où il peut aller.
class SolHors : public Sprite {
public:
    SolHors();
    Sprite* clone() const override { return new SolHors(*this); }
protected:
    int getNbImage() const override;
    QRect getRect(int idx) const override;
};

#endif // SOL_H
