#include <QtDebug>
#include <climits>
#include <utility>
#include "game.h"

static const Game::SPlayerDirection playerDirections[NB_DIRECTION] = {{{0, -1}, 0}, {{1, 0}, 2}, {{0, 1}, 0}, {{-1, 0}, 1}};
static const Game::SDirection directions[NB_DIRECTION] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};

// L'opposé de directions[], sous ses deux lectures — c'est la même table, et la
// dupliquer localement ferait diverger les copies au premier changement :
//  - case d'appui : où le joueur doit se tenir pour pousser dans la direction d ;
//  - sens du tirage : l'inverse d'une poussée, pour le flood-fill à rebours
//    depuis les buts (calculCaseMorte).
static const Game::SDirection opposees[NB_DIRECTION] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};

Game::Game() {
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
                if (c.typeCase == Level::tcGoalPlayer) {
                    goals.append(idx);
                }
            } else if(c.typeCase == Level::tcGoal || c.typeCase == Level::tcGoalCaisse) {
                goals.append(idx);
            }
        }
    }

    calculCaseMorte();
}

Game::Game(const Game& other)
    : largeur(other.largeur), hauteur(other.hauteur), size(other.size),
      playerPoint(other.playerPoint), playerDirection(other.playerDirection),
      nbDep(other.nbDep), nbDepCaisse(other.nbDepCaisse), numNiveau(other.numNiveau),
    gagne(other.gagne), perdu(other.perdu), goals(other.goals), casesMortes(other.casesMortes), distanceButs(other.distanceButs)
{
    if (other.cases) {
        cases = new Level::ETypeCase[size];
        for (int i = 0; i < size; ++i)
            cases[i] = other.cases[i];
    }
}

Game& Game::operator=(const Game& other) {
    if (this == &other) return *this;
    delete[] cases;
    largeur = other.largeur;
    hauteur = other.hauteur;
    size = other.size;
    playerPoint = other.playerPoint;
    playerDirection = other.playerDirection;
    nbDep = other.nbDep;
    nbDepCaisse = other.nbDepCaisse;
    numNiveau = other.numNiveau;
    gagne = other.gagne;
    perdu = other.perdu;
    goals = other.goals;
    casesMortes = other.casesMortes;
    distanceButs = other.distanceButs;

    if (other.cases) {
        cases = new Level::ETypeCase[size];
        for (int i = 0; i < size; ++i)
            cases[i] = other.cases[i];
    } else {
        cases = nullptr;
    }

    return *this;
}

Game::Game(Game&& other) noexcept
    : largeur(other.largeur), hauteur(other.hauteur), size(other.size),
      playerPoint(other.playerPoint), playerDirection(other.playerDirection),
      cases(other.cases),
      nbDep(other.nbDep), nbDepCaisse(other.nbDepCaisse), numNiveau(other.numNiveau),
      gagne(other.gagne), perdu(other.perdu),
      goals(std::move(other.goals)), casesMortes(std::move(other.casesMortes)),
      distanceButs(std::move(other.distanceButs))
{
    other.cases = nullptr;   // sinon les deux destructeurs libéreraient le même tableau
}

Game& Game::operator=(Game&& other) noexcept {
    if (this == &other) return *this;
    delete[] cases;
    largeur = other.largeur;
    hauteur = other.hauteur;
    size = other.size;
    playerPoint = other.playerPoint;
    playerDirection = other.playerDirection;
    nbDep = other.nbDep;
    nbDepCaisse = other.nbDepCaisse;
    numNiveau = other.numNiveau;
    gagne = other.gagne;
    perdu = other.perdu;
    goals = std::move(other.goals);
    casesMortes = std::move(other.casesMortes);
    distanceButs = std::move(other.distanceButs);

    cases = other.cases;

    other.cases = nullptr;

    return *this;
}

Game::~Game() {
    delete[] cases;
}

bool Game::isLoaded() const {
    return cases != nullptr;
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
    gagne = true;
}

void Game::checkDefaite() {
    // Ne teste que les tcCaisse : une caisse gelée SUR un but est parfaitement
    // légitime, c'est un morceau de la solution.
    QVector<bool> enCours(size, false);

    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;

            if (cases[idx] == Level::tcCaisse) {
                if(casesMortes[idx] || caisseGelee(idx, enCours)) {
                    perdu = true;
                    return;
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

QByteArray Game::getEtat(const QVector<bool>& zone) const {
    QByteArray etat;
    const short idxPalyer= getMinIdx(zone);

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

short Game::getMinIdx(const QVector<bool>& zone) const {
    short result = (short)SHRT_MAX;

    for (int i = 0; i < size; ++i) {
        if (zone[i] && i < result) {
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

QVector<quint8> Game::getCaissesDeplacable(const QVector<bool>& zone) const {
    QVector<quint8> result(size, 0);
    const int idxPlayer = playerPoint.x() + playerPoint.y() * largeur;

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
                    int xPousse = x + opposees[d].dx;
                    int yPousse = y + opposees[d].dy;
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

void Game::calculCaseMorte()  {
    QList<int> file(goals);
    distanceButs = QVector<int>(size, -1);

    for (int g : goals) {
        distanceButs[g] = 0;
    }

    while(file.size()) {
        int idx = file.takeFirst();
        for (int d=0;d<NB_DIRECTION;d++) {
            int x = idx % largeur;
            int y = idx / largeur;
            int xD = x + opposees[d].dx;
            int yD = y + opposees[d].dy;
            int xP = x + 2 * opposees[d].dx;
            int yP = y + 2 * opposees[d].dy;
            int idxD = xD + yD * largeur;
            int idxP = xP + yP * largeur;

            if (cases[idxD] != Level::tcMur && cases[idxP] != Level::tcMur) {
                if (distanceButs[idxD] == -1) {
                    distanceButs[idxD] = distanceButs[idx] + 1;
                    file.append((idxD));
                }
            }
        }
    }

    casesMortes = QVector<bool>(size, false);
    for(int y=0;y<hauteur;y++) {
        for(int x=0;x<largeur;x++) {
            int idx = x + y * largeur;

            if(cases[idx] != Level::tcMur && distanceButs[idx] == -1) {
                casesMortes[idx] = true;
            }
        }
    }
}

int Game::getHeuristique() const {
    int h = 0;

    for (int i = 0; i < size; ++i) {
        if (cases[i] == Level::tcCaisse || cases[i] == Level::tcGoalCaisse) {
            h += distanceButs[i];
        }
    }

    return h;
}

void Game::appliqueEtat(const QByteArray& cle) {
    // 1. Plateau à nu. Mapping LOCAL, case par case : surtout pas de
    //    goals.contains(i), qui serait une recherche linéaire dans une QList —
    //    donc O(n²) sur l'ensemble du plateau.
    for (int i = 0; i < size; ++i) {
        switch (cases[i]) {
            case Level::tcCaisse:
            case Level::tcPlayer:     cases[i] = Level::tcNone; break;
            case Level::tcGoalCaisse:
            case Level::tcGoalPlayer: cases[i] = Level::tcGoal; break;
            default: break;   // mur, case vide, but : inchangés
        }
    }

    // 2. La clé = [id des N caisses triés] + [id canonique de la zone du joueur],
    //    un short big-endian par case (cf. getEtat()). Le dernier est le joueur.
    const int nb = cle.size() / 2;
    for (int k = 0; k < nb; ++k) {
        const int idx = (static_cast<quint8>(cle[2 * k]) << 8) | static_cast<quint8>(cle[2 * k + 1]);

        if (k < nb - 1) {
            cases[idx] = (cases[idx] == Level::tcGoal) ? Level::tcGoalCaisse : Level::tcCaisse;
        } else {
            cases[idx] = (cases[idx] == Level::tcGoal) ? Level::tcGoalPlayer : Level::tcPlayer;
            playerPoint = QPoint(idx % largeur, idx / largeur);
        }
    }

    nbDep = 0;
    nbDepCaisse = 0;
    perdu = false;
    gagne = false;

    // Indispensable : sans ça 'gagne' resterait celui du modèle (faux), et le
    // solveur ne reconnaîtrait JAMAIS l'état gagnant qu'il vient de reconstruire.
    checkVictoire();
}

bool Game::pousse(int idxCaisse, EDirection dir) {
    const int x = idxCaisse % largeur;
    const int y = idxCaisse / largeur;
    int idxPlayer = playerPoint.x() + playerPoint.y() * largeur;

    cases[idxPlayer] = cases[idxPlayer] == Level::tcGoalPlayer ? Level::tcGoal : Level::tcNone;

    playerPoint = QPoint(x + opposees[(int)dir].dx, y + opposees[(int)dir].dy);
    idxPlayer = playerPoint.x() + playerPoint.y() * largeur;

    cases[idxPlayer] = cases[idxPlayer] == Level::tcGoal ? Level::tcGoalPlayer : Level::tcPlayer;

    return move(dir);
}

// Test de gel (« freeze deadlock »). Distingue « bloquée maintenant » de
// « bloquée pour toujours » : une caisse voisine ne bloque durablement que si
// elle est ELLE-MÊME gelée — sinon elle peut s'en aller et tout libérer.
//
// 'enCours' est la garde de récursion : la caisse en cours d'examen y est
// marquée, et compte alors comme un mur pour ses voisines. Sans ça, A interroge
// B qui réinterroge A, indéfiniment. C'est aussi ce qui rend le raisonnement
// juste : on demande « B serait-elle gelée si A ne bougeait pas ? », ce qui est
// exactement la question posée.
bool Game::caisseGelee(int idxCaisse, QVector<bool>& enCours) const {
    if (enCours[idxCaisse]) return true;   // traitée en mur par l'appelant

    enCours[idxCaisse] = true;
    // Les deux axes, dans l'ordre des 'directions' : {haut, droite, bas, gauche}
    // → axe vertical = (dHaut, dBas), axe horizontal = (dDroite, dGauche).
    const bool gel = bloqueeSurAxe(idxCaisse, dDroite, dGauche, enCours)
                  && bloqueeSurAxe(idxCaisse, dHaut,   dBas,    enCours);
    enCours[idxCaisse] = false;

    return gel;
}

// Une caisse ne peut se déplacer sur un axe que si les DEUX cases de cet axe
// sont libres : l'une pour la destination, l'autre pour que le joueur s'y tienne.
// Donc un seul côté bloqué suffit à bloquer tout l'axe.
bool Game::bloqueeSurAxe(int idxCaisse, EDirection dirA, EDirection dirB, QVector<bool>& enCours) const {
    const int x = idxCaisse % largeur;
    const int y = idxCaisse / largeur;
    const int a = (x + directions[dirA].dx) + (y + directions[dirA].dy) * largeur;
    const int b = (x + directions[dirB].dx) + (y + directions[dirB].dy) * largeur;

    // 1. Un mur d'un côté (ou une caisse que l'appelant traite en mur).
    if (cases[a] == Level::tcMur || enCours[a]) return true;
    if (cases[b] == Level::tcMur || enCours[b]) return true;

    // 2. Les deux destinations sont des cases mortes : pousser sur cet axe mène
    //    à un deadlock de toute façon, l'axe est donc inutilisable.
    if (casesMortes[a] && casesMortes[b]) return true;

    // 3. Une caisse voisine ELLE-MÊME gelée. C'est la récursion, et c'est ce qui
    //    évite le faux positif : « il y a une caisse à côté » ne suffit pas.
    if (estCaisse(a) && caisseGelee(a, enCours)) return true;
    if (estCaisse(b) && caisseGelee(b, enCours)) return true;

    return false;
}

bool Game::estCaisse(int idx) const {
    return cases[idx] == Level::tcCaisse || cases[idx] == Level::tcGoalCaisse;
}
