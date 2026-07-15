#include <QtDebug>
#include <QVarLengthArray>
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

            if (c.typeCase == Level::tcCaisse || c.typeCase == Level::tcGoalCaisse) {
                nbCaisses++;
            }
        }
    }

    calculDistancePoussee();
    calculCaseMorte();
}

Game::Game(const Game& other)
    : largeur(other.largeur), hauteur(other.hauteur), size(other.size),
      playerPoint(other.playerPoint), playerDirection(other.playerDirection),
      nbDep(other.nbDep), nbDepCaisse(other.nbDepCaisse), numNiveau(other.numNiveau),
      nbCaisses(other.nbCaisses),
    gagne(other.gagne), perdu(other.perdu), goals(other.goals), casesMortes(other.casesMortes),
    regions(other.regions), nbRegions(other.nbRegions), distancePoussee(other.distancePoussee),
    distanceParBut(other.distanceParBut), nbButs(other.nbButs),
    maxRegions(other.maxRegions), ordreButs(other.ordreButs)
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
    nbCaisses = other.nbCaisses;
    gagne = other.gagne;
    perdu = other.perdu;
    goals = other.goals;
    casesMortes = other.casesMortes;
    maxRegions = other.maxRegions;
    regions = other.regions;
    nbRegions = other.nbRegions;
    distancePoussee = other.distancePoussee;
    distanceParBut = other.distanceParBut;
    nbButs = other.nbButs;
    ordreButs = other.ordreButs;

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
      nbCaisses(other.nbCaisses),
      gagne(other.gagne), perdu(other.perdu),
      goals(std::move(other.goals)), casesMortes(std::move(other.casesMortes)),
      regions(std::move(other.regions)), nbRegions(std::move(other.nbRegions)),
      distancePoussee(std::move(other.distancePoussee)),
      distanceParBut(std::move(other.distanceParBut)), nbButs(other.nbButs),
      maxRegions(other.maxRegions), ordreButs(std::move(other.ordreButs))
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
    nbCaisses = other.nbCaisses;
    gagne = other.gagne;
    perdu = other.perdu;
    goals = std::move(other.goals);
    casesMortes = std::move(other.casesMortes);
    maxRegions = other.maxRegions;
    regions = std::move(other.regions);
    nbRegions = std::move(other.nbRegions);
    distancePoussee = std::move(other.distancePoussee);
    distanceParBut = std::move(other.distanceParBut);
    nbButs = other.nbButs;
    ordreButs = std::move(other.ordreButs);

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
    const int idxJoueur = playerPoint.x() + playerPoint.y() * largeur;

    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;

            if (cases[idx] == Level::tcCaisse) {
                // Deadlock DYNAMIQUE : cette caisse ne peut plus atteindre aucun
                // but avec le joueur de CE côté-ci. casesMortes ne peut pas le
                // voir — elle n'est vraie que si la caisse est perdue pour TOUTES
                // les régions. Sain pour la même raison que l'admissibilité de h :
                // dans le vrai jeu le joueur est encore plus contraint (les autres
                // caisses le gênent), donc une caisse déjà condamnée seule l'est
                // a fortiori avec les autres.
                //
                // Ce test n'est pas qu'un bonus d'élagage : sans lui,
                // getHeuristique() ajouterait ce -1 et se mettrait à SOUSTRAIRE.
                // .at() et NON operator[] : checkDefaite() n'est pas const, donc
                // l'operator[] non-const de QVector appelle detach(). Ces vecteurs
                // sont partagés par COW entre tous les clones du solveur (refcount
                // > 1), si bien que chaque lecture en faisait une COPIE PROFONDE —
                // 97 Ko pour 'regions', à chaque poussée, des millions de fois.
                // .at() est const et ne détache jamais.
                const qint16 r = regions.at(idxJoueur * size + idx);

                if(casesMortes.at(idx)
                   || distancePoussee.at(idx * maxRegions + r) == -1
                   || caisseGelee(idx, enCours)) {
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

void Game::getEtat(quint16* cle, const QVector<bool>& zone) const {
    int n = 0;

    // Balayage y/x croissant : les caisses sortent triées par id de case, ce qui
    // canonicalise le fait qu'elles sont indistinguables.
    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            int idx = x + y * largeur;
            if(cases[idx] == Level::tcCaisse || cases[idx] == Level::tcGoalCaisse) {
                cle[n++] = (quint16)idx;
            }
        }
    }

    // Le joueur en dernier, sur la case CANONIQUE de sa zone. La longueur est
    // donc toujours nbCaisses + 1 = tailleCle() : pas de délimiteur, et l'arène
    // peut ranger les clés bout à bout (cf. cle.h).
    cle[n] = (quint16)getMinIdx(zone);
}

QByteArray Game::getEtat(const QVector<bool>& zone) const {
    QVarLengthArray<quint16, 32> cle(tailleCle());
    getEtat(cle.data(), zone);

    QByteArray etat;
    for (int i = 0; i < cle.size(); ++i) {
        etat += (unsigned char)(cle[i] >> 8);
        etat += (unsigned char)(cle[i] & 0x00FF);
    }

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
    casesMortes = QVector<bool>(size, false);
    for (int b = 0; b < size; b++) {
        if (cases[b] == Level::tcMur) continue;
        bool jamais = true;
        for (int r = 0; r < nbRegions[b]; r++)
            if (distancePoussee[b * maxRegions + r] != -1) jamais = false;
        casesMortes[b] = jamais;
    }
}

// Coût d'une paire caisse->but inatteignable dans la matrice du couplage. GRAND
// mais FINI (§7.2) : le hongrois ADDITIONNE des coûts, INT_MAX déborderait. Avec
// n <= ~30 caisses, n * INF_COUPLAGE reste très loin de la limite d'un int.
static const int INF_COUPLAGE = 1000000;

// Affectation de coût minimal (hongrois, méthode des potentiels, O(n^3)) sur une
// matrice n x n donnée à plat en ligne-major. Renvoie la somme minimale.
// Implémentation classique 1-indexée (u/v potentiels, p affectation, way chemin).
//
// Si 'affectation' n'est pas nul, il reçoit affectation[but] = caisse (0-indexés) :
// l'identité « quelle caisse va à quel but », dont le guidage (§10.2) a besoin.
static int hongrois(const int* cout, int n, int* affectation = nullptr) {
    // Qt 5.15 : QVarLengthArray n'a pas de constructeur de remplissage, on initialise
    // à la main. Prealloc = 32 -> pas d'allocation tas tant que n < 32.
    QVarLengthArray<int, 32> u(n + 1), v(n + 1), p(n + 1), way(n + 1);
    for (int k = 0; k <= n; k++) { u[k] = 0; v[k] = 0; p[k] = 0; way[k] = 0; }

    QVarLengthArray<int, 32>  minv(n + 1);
    QVarLengthArray<bool, 32> used(n + 1);

    for (int i = 1; i <= n; i++) {
        p[0] = i;
        int j0 = 0;
        for (int k = 0; k <= n; k++) { minv[k] = INT_MAX; used[k] = false; }

        do {
            used[j0] = true;
            const int i0 = p[j0];
            int delta = INT_MAX, j1 = -1;

            for (int j = 1; j <= n; j++) {
                if (used[j]) continue;
                const int cur = cout[(i0 - 1) * n + (j - 1)] - u[i0] - v[j];
                if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                if (minv[j] < delta) { delta = minv[j]; j1 = j; }
            }

            for (int j = 0; j <= n; j++) {
                if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                else         { minv[j] -= delta; }
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            const int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }

    int total = 0;
    for (int j = 1; j <= n; j++) {
        total += cout[(p[j] - 1) * n + (j - 1)];   // caisse p[j] affectée au but j
        if (affectation) affectation[j - 1] = p[j] - 1;   // but j-1 (0-indexé) -> caisse p[j]-1
    }
    return total;
}

int Game::getHeuristique(qint64* scoreGuidage) const {
    if (scoreGuidage) *scoreGuidage = 0;
    const int j = playerPoint.x() + playerPoint.y() * largeur;

    // Recense les caisses (colonnes = buts, lignes = caisses de la matrice).
    QVarLengthArray<int, 32> caisses;
    for (int i = 0; i < size; i++) {
        if (cases[i] == Level::tcCaisse || cases[i] == Level::tcGoalCaisse)
            caisses.append(i);
    }
    const int n = caisses.size();
    if (n == 0) return 0;

    // Garde-fou (§7.2) : les 43 niveaux ont nb caisses == nb buts, la matrice est
    // carrée. Si un .xsb futur amenait un déséquilibre, on retombe proprement sur
    // l'ancienne borne « chaque caisse vise son but le plus proche ».
    if (n != nbButs) {
        int h = 0;
        for (int c = 0; c < n; c++) {
            const int cell = caisses[c];
            h += distancePoussee[cell * maxRegions + regions[j * size + cell]];
        }
        return h;
    }

    // cout[caisse][but] = distance de cette caisse (depuis sa case, joueur du côté
    // regions[j][cell]) vers CE but. Inatteignable -> INF_COUPLAGE.
    QVarLengthArray<int, 256> cout(n * n);
    for (int c = 0; c < n; c++) {
        const int cell = caisses[c];
        const int r    = regions[j * size + cell];
        for (int b = 0; b < nbButs; b++) {
            const int d = distanceParBut[((qsizetype)b * size + cell) * maxRegions + r];
            cout[c * n + b] = (d < 0) ? INF_COUPLAGE : d;
        }
    }

    // Le guidage a besoin de l'appariement caisse<->but (l'identité) : on le
    // récupère dans 'affectation', qu'on jetait jusqu'ici.
    QVarLengthArray<int, 32> affectation(n);
    const int h = hongrois(cout.constData(), n, scoreGuidage ? affectation.data() : nullptr);

    // Score de DÉPARTAGE (§10.2) : ordre lexicographique des distances par but.
    // Les buts sont pris dans leur ordre d'index (priorité fixe) ; minimiser ce
    // score revient à finir le but 0 d'abord, puis le 1, etc. → un ordre canonique
    // de rangement qui casse la multiplicité des entrelacements (§9.4). L'état tout
    // rangé (toutes distances nulles) a le score minimal, donc A* plonge vers lui.
    if (scoreGuidage) {
        // Base ADAPTATIVE : autant de bits par but que 63 en autorise, pour que
        // l'encodage lexicographique tienne dans un qint64 QUEL QUE SOIT nbButs
        // (base^n <= 2^63). Jusqu'à ~11 buts la base dépasse toute distance réelle
        // (aucun clamp) ; au-delà les distances sont clampées, sans conséquence —
        // ces niveaux à beaucoup de buts sont dominés par le mou, le guidage n'y
        // change rien. Plus de désactivation, le guidage vaut sur tous les niveaux.
        const int    bits = qMax(1, 63 / n);
        const qint64 base = 1LL << bits;
        qint64 s = 0;
        for (int b = 0; b < n; b++) {
            qint64 d = cout[affectation[b] * n + b];
            if (d >= base) d = base - 1;
            s = s * base + d;
        }
        *scoreGuidage = s;
    }

    return h;
}

int Game::appliqueEtat(const quint16* cle) {
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

    // 2. La clé = [id des N caisses triés] + [id canonique de la zone du joueur]
    //    (cf. getEtat()). Sa longueur est tailleCle(), le dernier est le joueur.
    int nbSurBut = 0;
    for (int k = 0; k < nbCaisses; ++k) {
        const int idx = cle[k];
        if (cases[idx] == Level::tcGoal) { cases[idx] = Level::tcGoalCaisse; ++nbSurBut; }
        else                              cases[idx] = Level::tcCaisse;
    }

    const int idxJoueur = cle[nbCaisses];
    cases[idxJoueur] = (cases[idxJoueur] == Level::tcGoal) ? Level::tcGoalPlayer : Level::tcPlayer;
    playerPoint = QPoint(idxJoueur % largeur, idxJoueur / largeur);

    nbDep = 0;
    nbDepCaisse = 0;
    perdu = false;
    gagne = false;

    // Indispensable : sans ça 'gagne' resterait celui du modèle (faux), et le
    // solveur ne reconnaîtrait JAMAIS l'état gagnant qu'il vient de reconstruire.
    checkVictoire();
    return nbSurBut;   // gratuit : compté pendant le placement (diagnostic §10)
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

void Game::calculDistancePoussee() {
    maxRegions = 0;
    regions   = QVector<qint16>(size * size, -1);
    nbRegions = QVector<qint16>(size, 0);

    for (int b = 0; b < size; b++) {
        if (cases[b] == Level::tcMur) continue;      // pas de caisse sur un mur

        qint16 nb = 0;
        for (int depart = 0; depart < size; depart++) {
            if (cases[depart] == Level::tcMur || depart == b) continue;
            if (regions[depart * size + b] != -1) continue;      // déjà colorié

            QList<int> file;
            file.append(depart);
            regions[depart * size + b] = nb;

            while (!file.isEmpty()) {
                const int i = file.takeFirst();
                const int x = i % largeur, y = i / largeur;
                for (int d = 0; d < NB_DIRECTION; d++) {
                    // Tests de bornes INDISPENSABLES ici, contrairement au reste du
                    // fichier. La bordure n'est PAS toujours en murs : Level::load()
                    // complète les lignes courtes par des espaces (tcNone), et
                    // certains .xsb commencent leurs lignes par des espaces. Ces
                    // cases de remplissage, hors du contour du niveau, sont
                    // inatteignables pour le joueur — d'où l'hypothèse tenue
                    // ailleurs — mais ici on balaie TOUTES les cases non-mur.
                    const int nx = x + directions[d].dx;
                    const int ny = y + directions[d].dy;
                    if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;

                    const int ni = nx + ny * largeur;
                    if (cases[ni] == Level::tcMur || ni == b) continue;   // la caisse bloque
                    if (regions[ni * size + b] != -1) continue;
                    regions[ni * size + b] = nb;
                    file.append(ni);
                }
            }
            nb++;
        }
        nbRegions[b] = nb;
        maxRegions = qMax(maxRegions, (int)nb);
    }

    // Une table de distances PAR BUT (§7.2). Un BFS à rebours par but j, seul à
    // servir de source, remplit sa tranche distanceParBut[(j*size + b)*maxRegions + r].
    // distancePoussee (utilisée par casesMortes et checkDefaite) en devient le MIN
    // sur les buts — même valeur qu'un unique BFS multi-but simultané, mais on garde
    // en plus la distance vers CHAQUE but, dont getHeuristique() a besoin.
    nbButs = goals.size();
    distanceParBut = QVector<int>((qsizetype)nbButs * size * maxRegions, -1);
    distancePoussee = QVector<int>(size * maxRegions, -1);

    for (int j = 0; j < nbButs; j++) {
        const int g = goals[j];
        int* dpb = distanceParBut.data() + (qsizetype)j * size * maxRegions;   // tranche du but j

        QList<QPair<int,int>> file;                   // (case de la caisse, région du joueur)
        for (int r = 0; r < nbRegions[g]; r++) {      // le but, quel que soit le côté du joueur
            dpb[g * maxRegions + r] = 0;
            file.append({g, r});
        }

        while (!file.isEmpty()) {
            const auto [c, rc] = file.takeFirst();
            const int cx = c % largeur, cy = c / largeur;

            for (int d = 0; d < NB_DIRECTION; d++) {
                // On remonte une poussée : la caisse venait de b, poussée vers c dans
                // la direction d. Le joueur se tenait donc en p, deux cases en arrière.
                const int bx = cx -     directions[d].dx, by = cy -     directions[d].dy;
                const int px = cx - 2 * directions[d].dx, py = cy - 2 * directions[d].dy;

                // Bornes : cf. le flood-fill ci-dessus, la bordure n'est pas garantie
                // en murs (cases de remplissage hors contour).
                if (bx < 0 || bx >= largeur || by < 0 || by >= hauteur) continue;
                if (px < 0 || px >= largeur || py < 0 || py >= hauteur) continue;

                const int b = bx + by * largeur;
                const int p = px + py * largeur;

                if (cases[b] == Level::tcMur || cases[p] == Level::tcMur) continue;

                // APRÈS la poussée, le joueur se retrouve en b. Il doit donc appartenir
                // à la région rc — celle mesurée avec la caisse en c.
                if (regions[b * size + c] != rc) continue;

                // AVANT la poussée : caisse en b, joueur en p.
                const qint16 r = regions[p * size + b];
                if (r < 0) continue;

                if (dpb[b * maxRegions + r] == -1) {
                    dpb[b * maxRegions + r] = dpb[c * maxRegions + rc] + 1;
                    file.append({b, r});
                }
            }
        }

        // distancePoussee = min sur les buts (en ignorant les -1 = inatteignable).
        for (int k = 0; k < size * maxRegions; k++) {
            const int v = dpb[k];
            if (v == -1) continue;
            if (distancePoussee[k] == -1 || v < distancePoussee[k])
                distancePoussee[k] = v;
        }
    }

    // Ordre de REMPLISSAGE des buts (§10.5) : profondeur décroissante. Profondeur
    // d'un but = distance de poussée la plus GRANDE pour l'atteindre depuis une
    // caisse de départ (le fond de la salle est loin, les coins encore plus). Les
    // buts profonds/coincés passent donc en premier — l'ordre où on range à la main.
    //
    // ⚠️ Tenté puis ÉCARTÉ : trier par « enclavement = nombre de voisins qui sont
    // des buts » (remplir le cœur du groupe d'abord). Résultat DÉSASTREUX (niveau 4
    // à 1/20, niveau 5 cassé) : une caisse au centre d'une salle vide coupe le
    // joueur en deux. Le bon « enclavé » est « au fond, loin de l'entrée » — ce que
    // la profondeur capture déjà, pas « au cœur de la poche ».
    QVector<int> prof(nbButs, -1);
    for (int b = 0; b < nbButs; b++) {
        int best = -1;
        for (int c = 0; c < size; c++) {
            if (cases[c] != Level::tcCaisse && cases[c] != Level::tcGoalCaisse) continue;
            for (int r = 0; r < nbRegions[c]; r++) {
                const int d = distanceParBut[((qsizetype)b * size + c) * maxRegions + r];
                if (d > best) best = d;   // la plus grande distance d'accès = la plus profonde
            }
        }
        prof[b] = best;
    }
    // GOAL-ORDERING À REBOURS (§10.5). La profondeur remplissait en couches
    // concentriques (coins d'abord) → elle ENCLAVE le pourtour. Le bon ordre est
    // colonne par colonne, du fond vers l'entrée — on l'obtient en partant de la
    // salle PLEINE et en retirant les caisses dans l'ordre où on peut les TIRER
    // vers une case libre (les autres caisses-buts comptant comme obstacles) ;
    // l'ordre de retrait INVERSÉ est l'ordre de remplissage. Version géométrique :
    // on vérifie que la case d'arrivée du tirage ET la case d'appui du joueur sont
    // libres, sans prouver que le joueur les atteint — suffisant pour l'ordre de
    // salle. À caisses également tirables, on retire la plus PROCHE de l'entrée
    // (prof minimale) : le retrait va entrée→fond, le remplissage fond→entrée.
    QVector<bool> estBut(size, false);
    QVector<int>  butDe(size, -1);
    for (int b = 0; b < nbButs; b++) { estBut[goals[b]] = true; butDe[goals[b]] = b; }

    QVector<bool> retire(nbButs, false);
    QVector<int>  retrait;
    auto libre = [&](int x, int y) -> bool {
        if (x < 0 || x >= largeur || y < 0 || y >= hauteur) return false;
        const int c = x + y * largeur;
        if (cases[c] == Level::tcMur) return false;
        return !(estBut[c] && !retire[butDe[c]]);   // caisse-but encore posée = occupée
    };

    for (int step = 0; step < nbButs; step++) {
        int choisi = -1, meilleurScore = 0, meilleurProf = INT_MAX;
        for (int b = 0; b < nbButs; b++) {
            if (retire[b]) continue;
            const int cx = goals[b] % largeur, cy = goals[b] / largeur;
            // Score de sortie : 2 = la caisse peut être tirée vers une case HORS
            // buts (bord de la salle, sortie directe) ; 1 = seulement vers un but
            // déjà libéré (intérieur). On retire d'abord les score 2, ce qui vide
            // la salle COLONNE PAR COLONNE de l'entrée vers le fond au lieu de la
            // dépiauter en couches.
            int score = 0;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int dx = cx - directions[d].dx,   dy = cy - directions[d].dy;
                const int ax = cx - 2*directions[d].dx, ay = cy - 2*directions[d].dy;
                if (libre(dx, dy) && libre(ax, ay)) {
                    const int s = estBut[dx + dy * largeur] ? 1 : 2;
                    if (s > score) score = s;
                }
            }
            if (score == 0) continue;   // pas tirable
            if (score > meilleurScore || (score == meilleurScore && prof[b] < meilleurProf)) {
                meilleurScore = score; meilleurProf = prof[b]; choisi = b;
            }
        }
        if (choisi < 0) break;               // reste des caisses non sortables (fallback plus bas)
        retire[choisi] = true;
        retrait.append(choisi);
    }
    // Les buts non sortables (jamais tirables) restent à remplir en dernier ;
    // on les met en tête du retrait (donc en queue du remplissage), par prof.
    QVector<int> reste;
    for (int b = 0; b < nbButs; b++) if (!retire[b]) reste.append(b);
    std::sort(reste.begin(), reste.end(), [&prof](int a, int b){ return prof[a] < prof[b]; });

    ordreButs.clear();
    for (int k = retrait.size() - 1; k >= 0; k--) ordreButs.append(retrait[k]);   // retrait inversé
    for (int b : reste) ordreButs.append(b);
}

bool Game::remplissageOrdonne() const {
    bool vuVide = false;
    for (int k = 0; k < nbButs; k++) {
        if (cases[goals[ordreButs[k]]] == Level::tcGoalCaisse) {
            if (vuVide) return false;   // but rempli APRÈS un but plus profond vide : désordre
        } else {
            vuVide = true;
        }
    }
    return true;
}

int Game::nbCaissesSurBut() const {
    int n = 0;
    for (int i = 0; i < size; ++i)
        if (cases[i] == Level::tcGoalCaisse) ++n;
    return n;
}

int Game::butActif() const {
    for (int k = 0; k < nbButs; k++)
        if (cases[goals[ordreButs[k]]] != Level::tcGoalCaisse)
            return ordreButs[k];
    return -1;
}

bool Game::macroVersBut(int idxCaisse, int indexBut, QVector<QPair<int,int>>& poussees) {
    const int caseBut = goals[indexBut];
    const int* dpb = distanceParBut.constData() + (qsizetype)indexBut * size * maxRegions;

    int c = idxCaisse;
    for (int garde = 0; c != caseBut && garde <= 2 * size; garde++) {
        const QVector<bool> zone = getZoneJoueur();
        const int joueurIdx = playerPoint.x() + playerPoint.y() * largeur;
        const int rAvant = regions[joueurIdx * size + c];
        if (rAvant < 0) return false;
        const int dCur = dpb[c * maxRegions + rAvant];
        if (dCur <= 0) return false;                 // -1 (inatteignable) ou déjà arrivé

        const int cx = c % largeur, cy = c / largeur;
        bool avance = false;
        for (int d = 0; d < NB_DIRECTION && !avance; d++) {
            const int devx = cx + directions[d].dx, devy = cy + directions[d].dy;   // case caisse après
            const int appx = cx - directions[d].dx, appy = cy - directions[d].dy;   // appui joueur
            if (devx < 0 || devx >= largeur || devy < 0 || devy >= hauteur) continue;
            if (appx < 0 || appx >= largeur || appy < 0 || appy >= hauteur) continue;
            const int devant = devx + devy * largeur;
            const int appui  = appx + appy * largeur;
            if (!isLibre(devant)) continue;          // arrivée occupée (mur / autre caisse)
            if (!zone[appui]) continue;              // joueur ne peut pas se placer derrière
            const int rApres = regions[c * size + devant];
            if (rApres < 0) continue;
            if (dpb[devant * maxRegions + rApres] == dCur - 1) {   // cette poussée avance vers le but
                if (!pousse(c, (Game::EDirection)d)) return false;
                poussees.append({c, d});
                c = devant;
                avance = true;
            }
        }
        if (!avance) return false;                   // aucune poussée ne fait avancer : bloqué
    }
    return c == caseBut;
}
