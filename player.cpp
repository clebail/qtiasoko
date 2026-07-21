#include "player.h"

// Les 10 poses de la planche (coordonnées de sprites.xml), groupées par
// direction. Le perso nous tourne le dos quand il monte et nous fait face quand
// il descend ; de profil, la visière de la casquette donne le sens.
static const QRect poses[] = {
    // de dos — le perso monte
    {384,   0, 37, 60},   // 0  Character7   appui
    {362, 188, 37, 60},   // 1  Character8   pas gauche
    {362, 128, 37, 60},   // 2  Character9   pas droit
    // de profil, vers la droite
    {320, 245, 42, 59},   // 3  Character2   appui
    {320, 128, 42, 58},   // 4  Character3   pas
    // de face — le perso descend
    {362, 248, 37, 59},   // 5  Character4   appui
    {320, 362, 37, 59},   // 6  Character5   pas gauche
    {357, 362, 37, 59},   // 7  Character6   pas droit
    // de profil, vers la gauche
    {320, 186, 42, 59},   // 8  Character1   appui
    {320, 304, 42, 58},   // 9  Character10  pas
};

// Cycle de marche par direction, dans l'ordre de Game::EDirection. Les vues de
// face et de dos ont deux pas ET une pose d'appui : on repasse par l'appui entre
// les deux, sinon le perso sautille d'un pied sur l'autre sans jamais poser. De
// profil il n'y a que deux poses, elles alternent.
static const int cycleHaut[]   = {0, 1, 0, 2};
static const int cycleDroite[] = {3, 4};
static const int cycleBas[]    = {5, 6, 5, 7};
static const int cycleGauche[] = {8, 9};

static const struct { const int* pas; int n; } cycles[] = {
    {cycleHaut, 4}, {cycleDroite, 2}, {cycleBas, 4}, {cycleGauche, 2}
};

Player::Player() {

}

int Player::getNbImage() const {
    return int(sizeof(poses) / sizeof(poses[0]));
}

QRect Player::getRect(int idx) const {
    if (idx < 0 || idx >= getNbImage()) idx = 5;   // de face, à l'appui
    return poses[idx];
}

int Player::frame(int direction, int pas) {
    const int nbDir = int(sizeof(cycles) / sizeof(cycles[0]));
    if (direction < 0 || direction >= nbDir) direction = 2;   // dBas

    const int n = cycles[direction].n;
    return cycles[direction].pas[((pas % n) + n) % n];        // 'pas' peut être négatif
}
