#ifndef PLAYER_H
#define PLAYER_H

#include "sprite.h"

// Le perso, dans ses 10 poses : trois de dos (il monte), trois de face (il
// descend), deux de profil dans chaque sens. Plus haut que large, donc posé sur
// le bas de sa case au lieu de la remplir.
class Player : public Sprite
{
public:
    Player();
    Sprite* clone() const override { return new Player(*this); }

    // Pose à afficher pour une direction et un rang dans le cycle de marche.
    // 'direction' suit Game::EDirection (0 haut, 1 droite, 2 bas, 3 gauche) —
    // l'entier plutôt que le type pour ne pas tirer game.h dans les sprites.
    // 'pas' s'incrémente à chaque coup joué et peut croître sans fin.
    static int frame(int direction, int pas);
protected:
    int getNbImage() const override;
    QRect getRect(int idx) const override;
    EAncrage getAncrage() const override { return aPose; }
};

#endif // PLAYER_H
