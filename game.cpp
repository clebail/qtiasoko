#include <QtDebug>
#include <climits>
#include "game.h"
#include "player.h"
#include "mur.h"
#include "caisse.h"
#include "goal.h"
#include "goalcaisse.h"

static const Game::SPlayerDirection playerDirections[NB_DIRECTION] = {{{0, -1}, 0}, {{1, 0}, 2}, {{0, 1}, 0}, {{-1, 0}, 1}};
static const Game::SDirection directions[NB_DIRECTION] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};

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
    size = largeur * hauteur;

    cases = new Level::ETypeCase[size];
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
    : largeur(other.largeur), hauteur(other.hauteur), size(other.size),
      playerPoint(other.playerPoint), playerDirection(other.playerDirection),
      numNiveau(other.numNiveau)
{
    if (other.cases) {
        cases = new Level::ETypeCase[size];
        for (int i = 0; i < size; ++i)
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
    size = other.size;
    playerPoint = other.playerPoint;
    playerDirection = other.playerDirection;
    numNiveau = other.numNiveau;

    if (other.cases) {
        cases = new Level::ETypeCase[size];
        for (int i = 0; i < size; ++i)
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
    for (int i = 0; i < size; ++i) {
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
    if (isLibre(idxNew)) {
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

    if (!isLibre(idxCaisseNew))
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
    const QVector<bool> visite = getZoneJoueur();
    short result = (short)SHRT_MAX;

    for (int i = 0; i < size; ++i) {
        if (visite[i] && i < result) {
            result = i;
        }
    }

    return result;
}

QVector<bool> Game::getZoneJoueur() const {
    QList<short> file;
    QVector<bool> visite(size, false);
    short idx = playerPoint.x() + playerPoint.y() * largeur;

    file.append(idx);
    visite[idx] = true;

    while(file.size()) {
        short vHaut, vDroite, vBas, vGauche;

        idx = file.takeFirst();

        vHaut = idx - largeur;
        if(vHaut >= 0 && isLibre(vHaut) && !visite[vHaut]) {
            file.append(vHaut);
            visite[vHaut] = true;
        }

        vDroite = idx + 1;
        if((idx % largeur) != largeur -1  && isLibre(vDroite) && !visite[vDroite]) {
            file.append(vDroite);
            visite[vDroite] = true;
        }

        vBas = idx + largeur;
        if(vBas < largeur * hauteur && isLibre(vBas) && !visite[vBas]) {
            file.append(vBas);
            visite[vBas] = true;
        }

        vGauche = idx - 1;
        if(idx % largeur != 0 && isLibre(vGauche) && !visite[vGauche]) {
            file.append(vGauche);
            visite[vGauche] = true;
        }
    }

    return visite;
}

bool Game::isLibre(const QPoint& p) const {
    return isLibre(p.x() + p.y() * largeur);
}

bool Game::isLibre(int idx) const {
    // Pas de test de bornes : la bordure du niveau est toujours en murs.
    return cases[idx] == Level::tcNone || cases[idx] == Level::tcGoal;
}

QVector<quint8> Game::getCaissesDeplacable() const {
    QVector<quint8> result(size, 0);
    const SDirection offsetsPousse[NB_DIRECTION] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};
    const int idxPlayer = playerPoint.x() + playerPoint.y() * largeur;
    // Zone atteignable calculée une seule fois pour tout le plateau : un
    // lookup O(1) par direction remplace une recherche AStar dédiée par
    // (caisse, direction). Le joueur y figure toujours (case de départ du
    // flood-fill), ce qui couvre aussi le cas où il est déjà en position.
    const QVector<bool> zone = getZoneJoueur();

    for(int y = 0; y < hauteur; y++) {
        for(int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;
            quint8 mask = 0;

            if (cases[idx] != Level::tcCaisse && cases[idx] != Level::tcGoalCaisse) continue;

            for(int d = 0; d < NB_DIRECTION; d++) {
                int idxDestination = (x + directions[d].dx) + (y + directions[d].dy) * largeur;

                // Le joueur libère sa propre case en marchant vers le point de
                // poussée avant de pousser : elle compte comme libre même si
                // elle est actuellement occupée par lui.
                if(isLibre(idxDestination) || idxDestination == idxPlayer) {
                    int xPousse = x + offsetsPousse[d].dx;
                    int yPousse = y + offsetsPousse[d].dy;
                    int idxPousse = xPousse + yPousse * largeur;

                    if(zone[idxPousse]) {
                        mask |= (1 << d);
                    }
                }
            }
            result[idx] = mask;
        }
    }

    return result;
}
