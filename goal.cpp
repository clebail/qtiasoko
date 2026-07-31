#include "goal.h"

Goal::Goal() {
}

int Goal::getNbImage() const {
    return 2;
}

// idx 0 = BLEU (le but actif, celui que vise la goal macro), idx 1 = SABLE (tous
// les autres). Les deux disques sont voisins sur la même rangée de la planche —
// même taille, même ancrage, donc seul le rect change. Le sable se distingue du
// bleu sans crier : sur un plateau à 32 buts, marquer les 31 autres en couleur
// vive rendrait l'actif invisible, ce qui est exactement l'inverse du but.
QRect Goal::getRect(int idx) const {
    return idx == 1 ? QRect(32, 384, 32, 32)     // EndPoint_Beige.png
                    : QRect(128, 384, 32, 32);   // EndPoint_Blue.png
}
