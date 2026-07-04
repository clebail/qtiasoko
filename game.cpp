#include <QtDebug>
#include "game.h"
#include "player.h"
#include "mur.h"
#include "caisse.h"
#include "goal.h"
#include "goalcaisse.h"

static const Game::SDirection directions[NB_DIRECTION] = {{0, -1, 0}, {1, 0, 2}, {0, 1, 0}, {-1, 0, 1}};

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


bool Game::move(EDirection dir) {
    if (gagne || perdu) return false;
    SDirection direction = directions[(int)dir];
    QPoint playerPointNew(playerPoint.x() + direction.dx, playerPoint.y() + direction.dy);

    if (playerPointNew.x() < 0 || playerPointNew.x() >= largeur ||
        playerPointNew.y() < 0 || playerPointNew.y() >= hauteur) {
        return false;
    }

    int idx    = playerPoint.x()    + playerPoint.y()    * largeur;
    int idxNew = playerPointNew.x() + playerPointNew.y() * largeur;

    // Déplacement vers case vide ou goal
    if (cases[idxNew] == Level::tcNone || cases[idxNew] == Level::tcGoal) {
        cases[idxNew] = cases[idxNew] == Level::tcGoal ? Level::tcGoalPlayer : Level::tcPlayer;
        cases[idx]    = cases[idx]    == Level::tcPlayer ? Level::tcNone : Level::tcGoal;
        playerPoint     = playerPointNew;
        playerDirection = direction.pd;
        nbDep++;
        return true;
    }

    // Poussée de caisse
    if (cases[idxNew] == Level::tcCaisse || cases[idxNew] == Level::tcGoalCaisse)
        if(moveCaisse(cases, playerPoint, playerPointNew, direction)) {
            playerPoint = playerPointNew;
            playerDirection = direction.pd;
            nbDep++;
            nbDepCaisse++;
            checkVictoire();
            return true;
        }

    return false;
}

bool Game::moveCaisse(Level::ETypeCase *cases, QPoint playerPoint, QPoint caissePoint, SDirection direction) {
    QPoint caissePointNew(caissePoint.x() + direction.dx, caissePoint.y() + direction.dy);

    if (caissePointNew.x() < 0 || caissePointNew.x() >= largeur ||
        caissePointNew.y() < 0 || caissePointNew.y() >= hauteur)
        return false;

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
    return QByteArray();
}
