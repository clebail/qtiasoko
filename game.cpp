#include <QtDebug>
#include <climits>
#include "game.h"
#include "player.h"
#include "mur.h"
#include "caisse.h"
#include "goal.h"
#include "goalcaisse.h"

static const Game::SPlayerDirection playerDirections[NB_DIRECTION] = {{{0, -1}, 0}, {{1, 0}, 2}, {{0, 1}, 0}, {{-1, 0}, 1}};

void Game::initSprites() {
    sprites[0] = nullptr;
    sprites[1] = new Mur();
    sprites[2] = new Player();
    sprites[3] = new Caisse();
    sprites[4] = new Goal();
    sprites[5] = new GoalCaisse();
    sprites[6] = sprites[2]; // GoalPlayer réutilise le sprite Player
}

void Game::freeSprites() {
    sprites[6] = nullptr; // alias, ne pas double-free
    for (int i = 1; i < NB_SPRITE; ++i) {
        delete sprites[i];
        sprites[i] = nullptr;
    }
}

void Game::cloneSprites(const Game& other) {
    sprites[0] = nullptr;
    for (int i = 1; i < NB_SPRITE - 1; ++i)
        sprites[i] = other.sprites[i] ? other.sprites[i]->clone() : nullptr;
    sprites[6] = sprites[2]; // alias GoalPlayer → Player
}

Game::Game() {
    sprites[0] = nullptr;
    for (int i = 1; i < NB_SPRITE; ++i)
        sprites[i] = nullptr;
}

Game::Game(const Level& level, int numNiveau) : numNiveau(numNiveau) {
    largeur = level.getLargeur();
    hauteur = level.getHauteur();

    cases = new Level::ETypeCase[largeur * hauteur];
    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;
            Level::SCase c = level.getCases().at(idx);
            cases[idx] = c.typeCase;
            if (c.typeCase == Level::tcPlayer || c.typeCase == Level::tcGoalPlayer) {
                playerPoint = QPoint(x, y);
            }
        }
    }
    initSprites();
}

Game::Game(const Game& other)
    : largeur(other.largeur), hauteur(other.hauteur),
      playerPoint(other.playerPoint), playerDirection(other.playerDirection),
      numNiveau(other.numNiveau)
{
    if (other.cases) {
        cases = new Level::ETypeCase[largeur * hauteur];
        for (int i = 0; i < largeur * hauteur; ++i)
            cases[i] = other.cases[i];
    }
    cloneSprites(other);
}

Game& Game::operator=(const Game& other) {
    if (this == &other) return *this;
    delete[] cases;
    freeSprites();
    largeur = other.largeur;
    hauteur = other.hauteur;
    playerPoint = other.playerPoint;
    playerDirection = other.playerDirection;
    numNiveau = other.numNiveau;

    if (other.cases) {
        cases = new Level::ETypeCase[largeur * hauteur];
        for (int i = 0; i < largeur * hauteur; ++i)
            cases[i] = other.cases[i];
    } else {
        cases = nullptr;
    }

    cloneSprites(other);
    return *this;
}

Game::~Game() {
    delete[] cases;
    freeSprites();
}

bool Game::isLoaded() const {
    return cases != nullptr;
}

void Game::draw(QPainter *painter) const {
    QSize pSize = painter->window().size();
    int margX = (pSize.width() - largeur * SPRITE_WIDTH) / 2;
    int margY = (pSize.height() - hauteur * SPRITE_HEIGHT) / 2;

    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;
            Sprite *s = sprites[(int)cases[idx]];
            int idxS = cases[idx] == Level::tcPlayer || cases[idx] == Level::tcGoalPlayer ? playerDirection : 0;

            if (s) {
                painter->drawImage(QPoint(margX + x * SPRITE_WIDTH, margY + y * SPRITE_HEIGHT), s->getImage(idxS));
            }

        }
    }
}

bool Game::haut() {
    return move(dHaut);
}

bool Game::droite() {
    return move(dDroite);
}

bool Game::bas() {
    return move(dBas);
}

bool Game::gauche() {
    return move(dGauche);
}

void Game::checkVictoire() {
    for (int i = 0; i < largeur * hauteur; ++i) {
        if (cases[i] == Level::tcCaisse) return;
    }
    qDebug() << "Victoire";
    gagne = true;
}

void Game::checkDefaite() {
    const SDirection coins[NB_DIRECTION][NB_COIN_TO_CHECK] = {{{0, -1}, {1, 0}}, {{1, 0}, {0, 1}}, {{0, 1}, {-1, 0}}, {{-1, 0}, {0, -1}}};
    const SDirection adjacents[NB_DIRECTION] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
    const SDirection adjacentsMur[NB_DIRECTION][NB_MUR_TO_CHECK] = {{{1, 0}, {-1, 0}}, {{0, -1}, {0, 1}}, {{1, 0}, {-1, 0}}, {{0, -1}, {0, 1}}};

    // Test des corners deadlocks (caisse coincée dans un coin)
    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;

            if (cases[idx] == Level::tcCaisse) {
                for(int d = 0; d < NB_DIRECTION; d++) {
                    bool bloque = true;
                    for(int c = 0; c < NB_COIN_TO_CHECK ; c++) {
                        int idxC = (x + coins[d][c].dx) + (y + coins[d][c].dy) * largeur;

                        bloque &= cases[idxC] == Level::tcMur;
                    }

                    if (bloque) {
                        perdu = true;
                        return;
                    }
                }
            };
        }
    }

    // Test des adjacents deadlocks (2 caisses adjacentes collées à un mur)
    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;

            if (cases[idx] == Level::tcCaisse) {
                for(int d = 0; d < NB_DIRECTION; d++) {
                    int xC = x + adjacents[d].dx;
                    int yC = y + adjacents[d].dy;
                    int idxC = xC + yC * largeur;

                    if (cases[idxC] == Level::tcCaisse || cases[idxC] == Level::tcGoalCaisse) {
                        for(int m = 0; m < NB_MUR_TO_CHECK; m++) {
                            int idxM1 = (x + adjacentsMur[d][m].dx) + (y + adjacentsMur[d][m].dy) * largeur;
                            int idxM2 = (xC + adjacentsMur[d][m].dx) + (yC + adjacentsMur[d][m].dy) * largeur;

                            if ((cases[idxM1] == Level::tcMur || cases[idxM1] == Level::tcCaisse || cases[idxM1] == Level::tcGoalCaisse) && (cases[idxM2] == Level::tcMur || cases[idxM2] == Level::tcCaisse || cases[idxM2] == Level::tcGoalCaisse)) {
                                perdu = true;
                                return;
                            }
                        }
                    }
                }
            };
        }
    }
}


bool Game::move(EDirection dir) {
    if (gagne || perdu) return false;
    SPlayerDirection pDirection = playerDirections[(int)dir];
    QPoint playerPointNew(playerPoint.x() + pDirection.direction.dx, playerPoint.y() + pDirection.direction.dy);

    // Pas de test de bornes : la bordure du niveau est toujours en murs, le
    // joueur est donc toujours intérieur et playerPointNew reste dans la grille.
    int idx    = playerPoint.x()    + playerPoint.y()    * largeur;
    int idxNew = playerPointNew.x() + playerPointNew.y() * largeur;

    // Déplacement vers case vide ou goal
    if (cases[idxNew] == Level::tcNone || cases[idxNew] == Level::tcGoal) {
        cases[idxNew] = cases[idxNew] == Level::tcGoal ? Level::tcGoalPlayer : Level::tcPlayer;
        cases[idx]    = cases[idx]    == Level::tcPlayer ? Level::tcNone : Level::tcGoal;
        playerPoint     = playerPointNew;
        playerDirection = pDirection.playerDirection;
        nbDep++;
        return true;
    }

    // Poussée de caisse
    if (cases[idxNew] == Level::tcCaisse || cases[idxNew] == Level::tcGoalCaisse)
        if(moveCaisse(cases, playerPoint, playerPointNew, pDirection.direction)) {
            playerPoint = playerPointNew;
            playerDirection = pDirection.playerDirection;
            nbDep++;
            nbDepCaisse++;
            checkVictoire();
            if(!gagne) {
                checkDefaite();
            }
            return true;
        }

    return false;
}

bool Game::moveCaisse(Level::ETypeCase *cases, QPoint playerPoint, QPoint caissePoint, SDirection direction) {
    QPoint caissePointNew(caissePoint.x() + direction.dx, caissePoint.y() + direction.dy);

    // Idem : une caisse est toujours intérieure, caissePointNew reste dans la grille.
    int idxCaisse    = caissePoint.x()    + caissePoint.y()    * largeur;
    int idxCaisseNew = caissePointNew.x() + caissePointNew.y() * largeur;
    int idxPlayer    = playerPoint.x()    + playerPoint.y()    * largeur;

    if (cases[idxCaisseNew] != Level::tcNone && cases[idxCaisseNew] != Level::tcGoal)
        return false;

    cases[idxCaisseNew] = cases[idxCaisseNew] == Level::tcGoal ? Level::tcGoalCaisse : Level::tcCaisse;
    cases[idxCaisse]    = cases[idxCaisse]    == Level::tcGoalCaisse ? Level::tcGoalPlayer : Level::tcPlayer;
    cases[idxPlayer]    = cases[idxPlayer]    == Level::tcPlayer ? Level::tcNone : Level::tcGoal;

    return true;
}

QByteArray Game::getEtat() const {
    QByteArray etat;
    const short idxPalyer= getMinIdx();

    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;
            if(cases[idx] == Level::tcCaisse || cases[idx] == Level::tcGoalCaisse) {
                etat += (unsigned char)(((short)idx) >> 8);
                etat += (unsigned char)(((short)idx) & 0x00FF);
            }
        }
    }

    etat += (unsigned char)(((short)idxPalyer) >> 8);
    etat += (unsigned char)(((short)idxPalyer) & 0x00FF);

    return etat;
}

short Game::getMinIdx() const {
    QList<short> file;
    QVector<bool> visite(largeur*hauteur, false);
    short idx = playerPoint.x() + playerPoint.y() * largeur;
    short result = (short)SHRT_MAX;

    file.append(idx);
    visite[idx] = true;

    while(file.size()) {
        short vHaut, vDroite, vBas, vGauche;

        idx = file.takeFirst();
        if(idx < result) {
            result = idx;
        }

        vHaut = idx - largeur;
        if(vHaut >= 0 && (cases[vHaut] == Level::tcNone || cases[vHaut] == Level::tcGoal) && !visite[vHaut]) {
            file.append(vHaut);
            visite[vHaut] = true;
        }

        vDroite = idx + 1;
        if((idx % largeur) != largeur -1  && (cases[vDroite] == Level::tcNone || cases[vDroite] == Level::tcGoal) && !visite[vDroite]) {
            file.append(vDroite);
            visite[vDroite] = true;
        }

        vBas = idx + largeur;
        if(vBas < largeur * hauteur && (cases[vBas] == Level::tcNone || cases[vBas] == Level::tcGoal) && !visite[vBas]) {
            file.append(vBas);
            visite[vBas] = true;
        }

        vGauche = idx - 1;
        if(idx % largeur != 0 && (cases[vGauche] == Level::tcNone || cases[vGauche] == Level::tcGoal) && !visite[vGauche]) {
            file.append(vGauche);
            visite[vGauche] = true;
        }
    }

    return result;
}
