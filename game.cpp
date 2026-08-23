#include <QtDebug>
#include <QVarLengthArray>
#include <QSet>
#include <QRegularExpression>
#include <QHash>
#include <QByteArray>
#include <QFile>
#include <QDir>
#include <algorithm>
#include <climits>
#include <utility>
#include <vector>
#include "game.h"

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
    calculCasesMortesLoi();
    calculPorteRequis();
}

Game::Game(const Game& other)
    : largeur(other.largeur), hauteur(other.hauteur), size(other.size),
      playerPoint(other.playerPoint),
      nbDep(other.nbDep), nbDepCaisse(other.nbDepCaisse), numNiveau(other.numNiveau),
      ordreDynamique(other.ordreDynamique), ordreLookahead(other.ordreLookahead),
      ordreAlignement(other.ordreAlignement), butCourant(other.butCourant),
      nbCaisses(other.nbCaisses),
    gagne(other.gagne), perdu(other.perdu), goals(other.goals), casesMortes(other.casesMortes),
    regions(other.regions), nbRegions(other.nbRegions), distancePoussee(other.distancePoussee),
    distanceParBut(other.distanceParBut), nbButs(other.nbButs),
    maxRegions(other.maxRegions), ordreButs(other.ordreButs),
    mortesLoi(other.mortesLoi), rangDeBut(other.rangDeBut),
    porteCases(other.porteCases), porteDebut(other.porteDebut)
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
    nbDep = other.nbDep;
    nbDepCaisse = other.nbDepCaisse;
    numNiveau = other.numNiveau;
    ordreDynamique = other.ordreDynamique;
    ordreLookahead = other.ordreLookahead;
    ordreAlignement = other.ordreAlignement;
    butCourant = other.butCourant;
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
    mortesLoi = other.mortesLoi;
    rangDeBut = other.rangDeBut;
    porteCases = other.porteCases;
    porteDebut = other.porteDebut;

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
      playerPoint(other.playerPoint),
      cases(other.cases),
      nbDep(other.nbDep), nbDepCaisse(other.nbDepCaisse), numNiveau(other.numNiveau),
      ordreDynamique(other.ordreDynamique), ordreLookahead(other.ordreLookahead),
      ordreAlignement(other.ordreAlignement), butCourant(other.butCourant),
      nbCaisses(other.nbCaisses),
      gagne(other.gagne), perdu(other.perdu),
      goals(std::move(other.goals)), casesMortes(std::move(other.casesMortes)),
      regions(std::move(other.regions)), nbRegions(std::move(other.nbRegions)),
      distancePoussee(std::move(other.distancePoussee)),
      distanceParBut(std::move(other.distanceParBut)), nbButs(other.nbButs),
      maxRegions(other.maxRegions), ordreButs(std::move(other.ordreButs)),
      mortesLoi(std::move(other.mortesLoi)), rangDeBut(std::move(other.rangDeBut)),
      porteCases(std::move(other.porteCases)), porteDebut(std::move(other.porteDebut))
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
    nbDep = other.nbDep;
    nbDepCaisse = other.nbDepCaisse;
    numNiveau = other.numNiveau;
    ordreDynamique = other.ordreDynamique;
    ordreLookahead = other.ordreLookahead;
    ordreAlignement = other.ordreAlignement;
    butCourant = other.butCourant;
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
    mortesLoi = std::move(other.mortesLoi);
    rangDeBut = std::move(other.rangDeBut);
    porteCases = std::move(other.porteCases);
    porteDebut = std::move(other.porteDebut);

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
                if(staticDeadlock(idx, idxJoueur, enCours)) {
                    perdu = true;
                    return;
                }

                if(dynamicDeadlock(idx)) {
                    perdu = true;
                    return;
                }

            };
        }
    }

}

bool Game::staticDeadlock(int idxCaisse, int idxJoueur, QVector<bool>& enCours) const {
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
    const qint16 r = regions.at(idxJoueur * size + idxCaisse);

    if(casesMortes.at(idxCaisse)
        || distancePoussee.at(idxCaisse * maxRegions + r) == -1
        || caisseGelee(idxCaisse, enCours)) {
        return true;
    }

    return false;
}
// Deadlock DYNAMIQUE par lookahead 1 coup (§10.6) : la caisse est condamnée si
// TOUTES ses poussées possibles la déposent sur une case statiquement perdue
// (morte, ou d'où elle n'atteint plus aucun but avec le joueur derrière). Aucune
// simulation : la case d'arrivée et sa région se LISENT (casesMortes /
// distancePoussee), on ne pousse rien.
//
// ⚠️ Faux positif ASSUMÉ (§3bis) : une direction bloquée par une CAISSE n'est pas
// comptée (dest/appui non libre), or cette caisse peut partir et libérer une issue
// vivante. C'est ce qui permet d'attraper les deadlocks dynamiques du 8/9 — mais
// ça peut, en théorie, condamner une caisse qui ne l'est pas. Le canari juge.
bool Game::dynamicDeadlock(int idxCaisse) const {
    const int cx = idxCaisse % largeur, cy = idxCaisse / largeur;
    int nbPoussable = 0, nbVersMort = 0;
    for (int d = 0; d < NB_DIRECTION; d++) {
        const int dx = directions[d].dx, dy = directions[d].dy;
        const int destX = cx + dx, destY = cy + dy;   // où va la caisse
        const int appX  = cx - dx, appY  = cy - dy;   // où se tient le joueur pour pousser
        if (destX < 0 || destX >= largeur || destY < 0 || destY >= hauteur) continue;
        if (appX  < 0 || appX  >= largeur || appY  < 0 || appY  >= hauteur) continue;
        const int dest = destX + destY * largeur, app = appX + appY * largeur;
        if (cases[dest] == Level::tcMur || estCaisse(dest)) continue;   // arrivée murée / occupée
        if (cases[app]  == Level::tcMur || estCaisse(app))  continue;   // appui muré / occupé
        nbPoussable++;
        // Après la poussée, le joueur serait en idxCaisse ; la région de la caisse
        // posée en dest, vue depuis là, est regions[idxCaisse * size + dest].
        const qint16 r = regions.at(idxCaisse * size + dest);
        if (r >= 0 && (casesMortes.at(dest) || distancePoussee.at(dest * maxRegions + r) == -1))
            nbVersMort++;
        // (r < 0 : direction douteuse — comptée poussable mais PAS mortelle, par
        //  prudence, pour ne pas inventer de deadlock.)
    }
    return nbPoussable > 0 && nbPoussable == nbVersMort;
}

bool Game::move(EDirection dir) {
    if (gagne || perdu) return false;
    const SDirection d = directions[(int)dir];
    QPoint playerPointNew(playerPoint.x() + d.dx, playerPoint.y() + d.dy);

    // Pas de test de bornes : la bordure du niveau est toujours en murs, le
    // joueur est donc toujours intérieur et playerPointNew reste dans la grille.
    int idx    = playerPoint.x()    + playerPoint.y()    * largeur;
    int idxNew = playerPointNew.x() + playerPointNew.y() * largeur;

    // Déplacement vers case vide ou goal
    if (isLibre(idxNew)) {
        cases[idxNew] = cases[idxNew] == Level::tcGoal ? Level::tcGoalPlayer : Level::tcPlayer;
        cases[idx]    = cases[idx]    == Level::tcPlayer ? Level::tcNone : Level::tcGoal;
        playerPoint   = playerPointNew;
        nbDep++;
        return true;
    }

    // Poussée de caisse
    if (cases[idxNew] == Level::tcCaisse || cases[idxNew] == Level::tcGoalCaisse)
        if(moveCaisse(cases, playerPoint, playerPointNew, d)) {
            playerPoint = playerPointNew;
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

void Game::getZoneJoueur(QVector<bool>& visite) const {
    // fill(v, n) ne réalloue que si la taille diffère : sur un tampon réutilisé
    // d'un appel à l'autre, c'est un simple memset. C'est tout l'intérêt de cette
    // surcharge — le flood-fill est le point le plus chaud du solveur (~10 appels
    // par état développé avant les correctifs du §6.3), et la version qui rend un
    // QVector allouait un tableau neuf à chaque fois.
    visite.fill(false, size);

    // File du parcours : au plus une entrée par case, donc dimensionnable au pire
    // cas d'emblée. En QVarLengthArray, elle tient sur la PILE pour tous les
    // plateaux usuels (size <= 512) — là où QList<short> faisait un malloc par appel.
    QVarLengthArray<short, 512> file(size);
    int tete = 0, fin = 0;
    short idx = playerPoint.x() + playerPoint.y() * largeur;

    file[fin++] = idx;
    visite[idx] = true;

    while(tete < fin) {
        short vHaut, vDroite, vBas, vGauche;

        idx = file[tete++];

        vHaut = idx - largeur;
        if(vHaut >= 0 && isLibre(vHaut) && !visite[vHaut]) {
            file[fin++] = vHaut;
            visite[vHaut] = true;
        }

        vDroite = idx + 1;
        if((idx % largeur) != largeur -1  && isLibre(vDroite) && !visite[vDroite]) {
            file[fin++] = vDroite;
            visite[vDroite] = true;
        }

        vBas = idx + largeur;
        if(vBas < largeur * hauteur && isLibre(vBas) && !visite[vBas]) {
            file[fin++] = vBas;
            visite[vBas] = true;
        }

        vGauche = idx - 1;
        if(idx % largeur != 0 && isLibre(vGauche) && !visite[vGauche]) {
            file[fin++] = vGauche;
            visite[vGauche] = true;
        }
    }
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

// LOI DE L'ORDRE (cf. game.h pour la règle et sa portée). Une réduction booléenne
// de `distanceParBut`, plus l'exemption d'alignement.
//
// ⚠️ APPELER APRÈS calculDistancePoussee() : elle lit `distanceParBut`, `ordreButs`
// et `nbButs`, que celle-ci produit. C'est la même dépendance que calculCaseMorte.
//
// ⚠️ L'ALIGNEMENT S'ARRÊTE AU PREMIER MUR (précision de l'utilisateur, 2026-08-04,
// après un premier jet qui le prenait au pied de la lettre). « Aligné » veut dire
// qu'on pourrait encore pousser la caisse EN LIGNE DROITE jusqu'au but, et un mur
// entre les deux l'interdit.
//
// ⚠️ AUCUNE MESURE NE DÉPARTAGE ENCORE LES DEUX VERSIONS — vérifié, pas supposé :
// le gabarit du niveau 16 rend 15 plateaux sur 15 avec l'une COMME avec l'autre
// (essayé le 2026-08-04). Cette version-ci tient donc de l'énoncé de son auteur, pas
// d'un juge. Et c'est la plus MORDANTE des deux : ses cases mortes sont un
// sur-ensemble de celles de la version littérale, donc si un faux positif doit
// apparaître, c'est ici qu'il apparaîtra d'abord. Un niveau où les deux diffèrent
// reste à trouver.
// UN COIN N'EST JAMAIS EXEMPTÉ (précision de l'utilisateur, 2026-08-04, sur
// relevé à l'écran du 16 puis du 6). Deux murs perpendiculaires : aucune des
// quatre poussées n'est possible, chacune demandant une destination ou un appui
// dans l'un des deux murs. Or « aligné » veut dire « on pourrait encore la
// pousser en ligne droite jusqu'au but » — d'un coin on ne la pousse nulle part,
// l'exemption n'a donc aucun sens là.
//
// Même ligne ou même colonne que le but, ET rien qu'on puisse traverser entre les
// deux. Les cases intermédiaires ne sont testées QUE sur les murs : une caisse ou
// une position de joueur sont de l'état, or cette table est statique — c'est la
// même convention que tout le reste du précalcul.
//
// Factorisé le 2026-08-19 pour `precedenceAlignement()`, qui teste le MÊME
// critère entre deux BUTS plutôt qu'entre un but et une case courante — cf. game.h.
bool Game::alignementLoi(int cell, int idxBut) const {
    const int g = goals[idxBut];
    if (cell == g) return true;
    const int gx = g % largeur, gy = g / largeur;
    const int x = cell % largeur, y = cell / largeur;
    const bool mN = (y == 0)           || cases[cell - largeur] == Level::tcMur;
    const bool mS = (y == hauteur - 1) || cases[cell + largeur] == Level::tcMur;
    const bool mO = (x == 0)           || cases[cell - 1]       == Level::tcMur;
    const bool mE = (x == largeur - 1) || cases[cell + 1]       == Level::tcMur;
    if ((mN || mS) && (mO || mE)) return false;   // coin : jamais exempté

    if (x != gx && y != gy) return false;
    const int dx = (gx > x) - (gx < x), dy = (gy > y) - (gy < y);
    for (int cx = x + dx, cy = y + dy; cx != gx || cy != gy; cx += dx, cy += dy)
        if (cases[cx + cy * largeur] == Level::tcMur) return false;
    return true;
}

void Game::calculCasesMortesLoi() {
    mortesLoi = QVector<bool>((qsizetype)nbButs * size, false);
    rangDeBut = QVector<int>(nbButs, -1);
    for (int k = 0; k < ordreButs.size(); k++) rangDeBut[ordreButs[k]] = k;

    if (maxRegions <= 0) return;        // niveau dégénéré : rien à calculer

    QVector<bool> estBut(size, false);
    for (int b : goals) estBut[b] = true;

    for (int j = 0; j < nbButs; j++) {
        const int* dpb = distanceParBut.constData() + (qsizetype)j * size * maxRegions;

        for (int c = 0; c < size; c++) {
            if (cases[c] == Level::tcMur) continue;

            // ⚠️ LA MORT DYNAMIQUE NE CONCERNE QUE LES CASES-BUTS (précision de
            // l'utilisateur, 2026-08-04 : « (3,1) c'est du sol, ce n'est pas un but,
            // donc ça ne peut pas être mort dynamique »). Une case ordinaire qui
            // n'atteint pas le but ACTIF reste un garage parfaitement licite : la
            // caisse qui s'y trouve attendra le but qu'elle sait servir, et rien ne
            // l'oblige à partir maintenant. La condamner serait un faux positif — et
            // c'est le §4 en énième déguisement (« interdire de remplir dans le
            // désordre »). Ce que la loi vise, c'est la caisse posée sur un BUT hors
            // de son tour, là où la table ordinaire ne voit jamais rien puisqu'un but
            // est sa propre graine du BFS à rebours.
            if (!estBut.at(c)) continue;

            // Atteignable depuis AU MOINS une région du joueur ? Même lecture que
            // calculCaseMorte, mais sur la tranche d'un seul but au lieu du min.
            bool atteint = false;
            for (int r = 0; r < nbRegions[c] && !atteint; r++)
                if (dpb[c * maxRegions + r] != -1) atteint = true;
            if (atteint) continue;

            // Alignée avec le but, mur non franchi, et pas un coin : du sol.
            if (alignementLoi(c, j)) continue;

            mortesLoi[(qsizetype)j * size + c] = true;
        }

        // TROISIÈME TEMPS DE LA LOI : « les buts déjà remplis sont des obstacles ».
        // Un but de rang INFÉRIEUR à celui-ci est rempli par construction — butActif()
        // rend le PREMIER but non rempli — donc sa case porte une caisse rangée à son
        // tour. Ce n'est pas une case où l'on pourrait poser : elle ne peut jamais
        // être « morte ». Sans ce temps-là, la règle condamnerait l'état juste après
        // chaque pose, sur presque tous les niveaux — la case d'un but rangé n'a
        // aucune raison d'atteindre le suivant.
        // ⚠️ Les buts de rang SUPÉRIEUR, eux, restent jugés : une caisse posée là est
        // hors de son tour, et c'est exactement ce que la loi vise.
        const int rangJ = rangDeBut[j];
        for (int m = 0; m < nbButs; m++)
            if (rangDeBut[m] < rangJ)
                mortesLoi[(qsizetype)j * size + goals[m]] = false;
    }
}

// PRÉCÉDENCE CAISSE → BUT (cf. game.h). Statique, calculée au chargement comme
// `precedenceGlobale`. O(caisses × buts × plateau) — quelques dizaines de milliers
// d'opérations, invisible dans le ctor.
void Game::calculPorteRequis() {
    porteCases.clear();
    porteDebut = QVector<int>(nbButs + 1, 0);
    if (nbButs == 0 || casesMortes.isEmpty()) return;

    const int depart = playerPoint.x() + playerPoint.y() * largeur;

    // A(C) pour chaque caisse hors but du départ. Une caisse DÉJÀ posée sur un but
    // n'est pas concernée : si elle gèle, elle gèle sur un but.
    QVector<int> caissesCell;
    QVector<QVarLengthArray<int, 4>> caissesAppuis;
    for (int c = 0; c < size; c++) {
        if (cases[c] != Level::tcCaisse) continue;
        const int cx = c % largeur, cy = c / largeur;
        QVarLengthArray<int, 4> appuis;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int ax = cx + directions[d].dx, ay = cy + directions[d].dy;
            const int px = cx - directions[d].dx, py = cy - directions[d].dy;
            if (ax < 0 || ax >= largeur || ay < 0 || ay >= hauteur) continue;
            if (px < 0 || px >= largeur || py < 0 || py >= hauteur) continue;
            const int a = ax + ay * largeur, p = px + py * largeur;
            if (cases[a] == Level::tcMur || cases[p] == Level::tcMur) continue;
            if (casesMortes.at(a)) continue;      // poussée suicide, pas une issue
            appuis.append(p);
        }
        if (appuis.isEmpty()) continue;           // immobile d'office : autre problème
        caissesCell.append(c);
        caissesAppuis.append(appuis);
    }

    // Pour chaque but : quelles caisses perdraient TOUS leurs appuis s'il était posé ?
    QVector<bool> vu(size);
    QVarLengthArray<int, 1024> file;
    QVector<QVector<int>> parBut(nbButs);
    for (int b = 0; b < nbButs; b++) {
        const int gb = goals[b];
        for (int i = 0; i < caissesCell.size(); i++) {
            const int cc = caissesCell[i];
            if (cc == gb) continue;
            if (depart == gb || depart == cc) continue;   // situation dégénérée

            // Marche du joueur, murs + le but posé + la caisse elle-même interdits.
            vu.fill(false);
            file.clear();
            vu[depart] = true; file.append(depart);
            bool atteint = false;
            for (int k = 0; k < file.size() && !atteint; k++) {
                const int cur = file[k], x = cur % largeur, y = cur / largeur;
                for (int d = 0; d < NB_DIRECTION; d++) {
                    const int nx = x + directions[d].dx, ny = y + directions[d].dy;
                    if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                    const int n = nx + ny * largeur;
                    if (vu[n] || cases[n] == Level::tcMur || n == gb || n == cc) continue;
                    vu[n] = true; file.append(n);
                }
            }
            for (int p : caissesAppuis[i]) if (vu[p]) { atteint = true; break; }
            if (!atteint) parBut[b].append(cc);
        }
    }

    int total = 0;
    for (int b = 0; b < nbButs; b++) {
        porteDebut[b] = porteCases.size();
        for (int c : parBut[b]) { porteCases.append(c); total++; }
    }
    porteDebut[nbButs] = porteCases.size();

    // Trace PASSIVE (§7) : elle n'ajoute ni ne coupe aucun comportement, donc elle ne
    // peut pas faire diverger l'app du bench, et elle dit d'un coup d'œil si le niveau
    // porte le motif. Muette quand il n'y en a pas, c'est-à-dire presque partout.
    if (total) {
        fprintf(stderr, "[PORTE] niveau %d — %d contrainte(s) caisse->but :", numNiveau, total);
        for (int b = 0; b < nbButs; b++)
            for (int i = porteDebut[b]; i < porteDebut[b + 1]; i++)
                fprintf(stderr, " caisse(%d,%d) avant but(%d,%d)",
                        porteCases[i] % largeur, porteCases[i] / largeur,
                        goals[b] % largeur, goals[b] / largeur);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

bool Game::porteBloquee(int idxBut) const {
    if (porteDebut.size() <= idxBut + 1) return false;
    for (int i = porteDebut.at(idxBut); i < porteDebut.at(idxBut + 1); i++)
        if (estCaisse(porteCases.at(i))) return true;
    return false;
}

// PORTE GÉNÉRALISÉ (cf. game.h) — DYNAMIQUE : contrairement à porteBloquee,
// tout est recalculé sur l'état COURANT à chaque appel, deux flood-fills.
//
// Factorisé en deux : le coeur prend 'r0' déjà calculé (partagé entre tous les
// candidats testés pour le MÊME jalon par porteGeneraliseeBloquee, qui appelle
// ceci une fois par caisse — sans partage, ce serait le même flood-fill refait
// nbCaisses fois).
bool Game::porteGeneraliseeCoupeAvecZone(int idxCaisse, int idxBut, const QVector<bool>& r0) const {
    const int gb = goals.at(idxBut);
    const int depart = playerPoint.x() + playerPoint.y() * largeur;
    if (depart == gb) return false;   // situation dégénérée (cf. calculPorteRequis)

    // r1 : zone de marche si 'idxCaisse' avait déjà quitté sa case pour occuper
    // 'gb'. Flood-fill dédié — celui de getZoneJoueur ne sait pas exempter une
    // case à la volée, et cette route n'est pas assez chaude pour justifier de
    // le lui apprendre.
    QVector<bool> r1(size, false);
    QVarLengthArray<short, 512> file(size);
    r1[depart] = true; file.append(depart);
    for (int k = 0; k < file.size(); k++) {
        const int idx = file[k], x = idx % largeur, y = idx / largeur;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int nx = x + directions[d].dx, ny = y + directions[d].dy;
            if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
            const int n = nx + ny * largeur;
            if (r1[n] || n == gb || cases[n] == Level::tcMur) continue;
            if (n != idxCaisse && estCaisse(n)) continue;
            r1[n] = true; file.append(n);
        }
    }

    auto accessible = [&](int cell, const QVector<bool>& r) {
        const int x = cell % largeur, y = cell / largeur;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int nx = x + directions[d].dx, ny = y + directions[d].dy;
            if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
            if (r.at(nx + ny * largeur)) return true;
        }
        return false;
    };

    // Caisses NON livrées (hors 'idxCaisse' elle-même) qui perdraient tout accès.
    for (int c = 0; c < size; c++) {
        if (c == idxCaisse || cases[c] != Level::tcCaisse) continue;
        if (accessible(c, r0) && !accessible(c, r1)) return true;
    }
    // Buts NON remplis (hors 'idxBut', qu'on occupe EXPRÈS — l'exclure a coupé
    // 90 faux positifs à la création du prédicat, cf. journal-hybride.md).
    for (int b = 0; b < nbButs; b++) {
        if (b == idxBut) continue;
        const int cell = goals.at(b);
        if (cases[cell] == Level::tcGoalCaisse) continue;
        if (accessible(cell, r0) && !accessible(cell, r1)) return true;
    }
    return false;
}

bool Game::porteGeneraliseeCoupe(int idxCaisse, int idxBut) const {
    QVector<bool> r0; getZoneJoueur(r0);
    return porteGeneraliseeCoupeAvecZone(idxCaisse, idxBut, r0);
}

// ⚠️ NE PAS tester idxCaisse=-1 (« aucune case libérée ») : c'est PLUS
// PESSIMISTE que la réalité — ignorer la case que la caisse choisie libère en
// partant peut faire manquer un contournement réel. Mesuré, un FAUX POSITIF
// PROUVÉ sur le niveau 25 (fpporte.py, 2026-08-18, variante « sans
// libération » : coupe (13,4)->(13,3) alors que la partie humaine gagnante
// joue exactement ce coup). D'où le balayage ci-dessous : la caisse RÉELLEMENT
// jouée dans une partie gagnante fait partie de ce balayage et y est TOUJOURS
// trouvée sûre (déduit de fpporte.py, 0 FP/1650 AVEC libération de la vraie
// caisse) — donc ce prédicat ne peut jamais être un faux positif sur un coup
// qu'une partie gagnante joue réellement.
bool Game::porteGeneraliseeBloquee(int idxBut) const {
    QVector<bool> r0; getZoneJoueur(r0);
    for (int c = 0; c < size; c++) {
        if (cases[c] != Level::tcCaisse) continue;
        if (!porteGeneraliseeCoupeAvecZone(c, idxBut, r0)) return false;   // une caisse sûre suffit
    }
    return true;
}

QVector<bool> Game::casesMortesLoi(int idxBut) const {
    if (idxBut < 0 || idxBut >= nbButs) return QVector<bool>();
    QVector<bool> v = mortesLoi.mid((qsizetype)idxBut * size, size);
    for (int c = 0; c < size; c++)
        if (casesMortes.at(c)) v[c] = false;      // déjà coupée par checkDefaite
    return v;
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

int Game::getHeuristique(qint64* scoreGuidage, int posJoueur, int* caisseParBut) const {
    if (scoreGuidage) *scoreGuidage = 0;
    if (caisseParBut) for (int b = 0; b < nbButs; b++) caisseParBut[b] = -1;
    const int j = (posJoueur >= 0) ? posJoueur
                                   : (playerPoint.x() + playerPoint.y() * largeur);

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
    const bool veutAffectation = (scoreGuidage != nullptr) || (caisseParBut != nullptr);
    const int h = hongrois(cout.constData(), n, veutAffectation ? affectation.data() : nullptr);

    // L'appariement, rendu en index de CASE (affectation[b] est un rang dans
    // 'caisses', pas une case — c'est le piège du §7 sur idxCaisse).
    if (caisseParBut)
        for (int b = 0; b < n; b++) caisseParBut[b] = caisses[affectation[b]];

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

bool Game::corralSMort(int s) const {
    const int idxJoueur = playerPoint.x() + playerPoint.y() * largeur;

    // ⚠️ tcNone est le SOL LIBRE dans ce projet (cf. isLibre), pas du vide : le
    // seul bloquant est tcMur. Le confondre avec un mur rend TOUTE case scellée
    // — c'est le premier bug de ce test, qui inventait des morts partout.
    // S doit être libre et sans le joueur : s'il y est, elle est accessible.
    if (!isLibre(s) || s == idxJoueur) return false;

    const int sx = s % largeur, sy = s / largeur;

    // 1. S est-elle scellée ? (4 voisins murs ou caisses)  Et on récolte au
    //    passage les caisses qui la bordent.
    int frontiere[NB_DIRECTION];
    int nbFrontiere = 0;
    bool scellee = true;
    for (int d = 0; d < NB_DIRECTION && scellee; d++) {
        const int vx = sx + directions[d].dx, vy = sy + directions[d].dy;
        if (vx < 0 || vx >= largeur || vy < 0 || vy >= hauteur) continue;   // hors grille = mur
        const int v = vx + vy * largeur;
        if (cases[v] == Level::tcMur) continue;
        if (estCaisse(v)) { frontiere[nbFrontiere++] = v; continue; }
        scellee = false;   // voisin libre (ou joueur) : le joueur peut entrer
    }
    if (!scellee || nbFrontiere == 0) return false;

    // 2. Classer chaque caisse-frontière par ses poussées LÉGALES. Une poussée est
    //    définitivement impossible si :
    //      - la destination est un MUR, ou l'appui est un MUR      (géométrie)
    //      - l'appui est DANS S                                    (circularité)
    //      - la destination est une case morte                     (branche perdante)
    //
    // ⚠️ Une caisse sur la destination ou sur l'appui ne prouve RIEN : elle peut
    // s'écarter plus tard. Tenir une caisse pour un obstacle permanent est le
    // piège qui a déjà tué quatre tests ici (gel naïf, h qui soustrait,
    // caisses=murs, gelées=murs — cf. plan.md §6.1). On ne conclut pas dans ce cas.
    //
    // ⚠️ Corollaire IMPORTANT (fonde le corral incrémental) : cette classification
    // ne dépend QUE de la géométrie statique (murs, cases mortes) et de S — jamais
    // de la position des autres caisses. Donc, S étant scellée, sa fatalité ne
    // change que si l'occupation de ses 4 voisines change.
    //
    // Trois classes :
    //   - LIBRE    : une poussée mène AILLEURS que dans S → la caisse peut quitter
    //                la région de S → on ne peut RIEN conclure, on abandonne S.
    //   - CAPTIVE  : au moins une poussée, mais TOUTES ont pour destination S.
    //   - IMMOBILE : aucune poussée légale.
    int nOffGoal = 0;
    bool existeCaptive = false;
    for (int k = 0; k < nbFrontiere; k++) {
        const int c = frontiere[k];
        if (cases[c] == Level::tcCaisse) nOffGoal++;   // tcGoalCaisse = déjà posée
        const int cx = c % largeur, cy = c / largeur;

        bool versAilleurs = false, versS = false;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int dx = directions[d].dx, dy = directions[d].dy;
            const int destX = cx + dx,  destY = cy + dy;
            const int appX  = cx - dx,  appY  = cy - dy;
            // Hors grille = mur (la bordure est murée) : poussée impossible.
            if (destX < 0 || destX >= largeur || destY < 0 || destY >= hauteur) continue;
            if (appX  < 0 || appX  >= largeur || appY  < 0 || appY  >= hauteur) continue;
            const int dest = destX + destY * largeur;
            const int app  = appX  + appY  * largeur;

            if (cases[dest] == Level::tcMur) continue;   // arrivée murée, pour toujours
            if (cases[app]  == Level::tcMur) continue;   // appui muré, pour toujours
            if (app == s)                    continue;   // LA circularité
            if (casesMortes.at(dest))        continue;   // branche déjà perdante

            if (dest == s) versS = true;                 // poussée POSSIBLE vers S
            else         { versAilleurs = true; break; } // poussée POSSIBLE ailleurs
        }
        if (versAilleurs) return false;   // caisse LIBRE : aucune conclusion sur S
        if (versS) existeCaptive = true;  // sinon : IMMOBILE (rien) ou CAPTIVE
    }

    // Toutes les caisses-frontière sont IMMOBILES ou CAPTIVES : aucune ne quitte la
    // région de S. Leurs positions finales sont figées, SAUF une seule captive qui
    // peut se garer dans S (qui n'a qu'UNE place et se re-scelle sitôt occupée). S
    // ne « recase » donc une caisse hors but que si elle est elle-même un but et
    // qu'une captive peut l'y prendre. S'il reste plus de caisses hors but que S
    // n'en peut recaser, au moins une gèle hors but pour toujours → MORT.
    //
    // Réduit EXACTEMENT à l'ancienne règle quand tout est immobile (existeCaptive =
    // false → capacite = 0 → mort ssi une caisse hors but) : rien ne change sur les
    // états déjà élagués ; n'AJOUTE que le cas des captives (la pince à deux
    // caisses, plan.md §6.1). Preuve : au plus une caisse quitte sa case, les k−1
    // autres restent gelées, donc nOffGoal − capacite caisses restent hors but.
    const int capacite = (cases[s] == Level::tcGoal && existeCaptive) ? 1 : 0;
    return nOffGoal > capacite;
}

bool Game::corralUnitaireMort() const {
    for (int s = 0; s < size; s++)
        if (corralSMort(s)) return true;
    return false;
}

bool Game::corralUnitaireMort(int caisseArrivee) const {
    // Version incrémentale, PROUVABLEMENT équivalente au balayage complet sur un
    // parent déjà jugé vivant (cf. la preuve dans game.h). Sceller une case S
    // exige de remplir son DERNIER voisin libre ; or la seule case qui a gagné une
    // caisse depuis le parent est 'caisseArrivee' (une transition ne déplace
    // qu'UNE caisse, et vers une seule case finale). Donc seules les 4 voisines de
    // 'caisseArrivee' peuvent être devenues scellées, et la fatalité d'un S déjà
    // scellé n'a pas pu changer (cf. corollaire dans corralSMort). Vérifié :
    // états identiques à l'unité contre le balayage complet (CORRAL=2).
    const int cx = caisseArrivee % largeur, cy = caisseArrivee / largeur;
    for (int d = 0; d < NB_DIRECTION; d++) {
        const int sx = cx + directions[d].dx, sy = cy + directions[d].dy;
        if (sx < 0 || sx >= largeur || sy < 0 || sy >= hauteur) continue;
        if (corralSMort(sx + sy * largeur)) return true;
    }
    return false;
}

// INSTRUMENTATION DE CHANTIER (étage 0 de la réserve « la clé du cache d'enclos
// ignore le JOUEUR », plan.md §6.1). Coupée par défaut. ⚠️ Elle n'AJOUTE et ne COUPE
// aucun comportement — elle ne fait que compter, donc elle ne peut pas faire diverger
// l'app du bench (le piège du §7) : les états rendus sont identiques à l'unité dans
// les deux régimes, et c'est le premier contrôle à faire. À RETIRER une fois la
// question tranchée, comme CORRAL_DETECT en son temps.
// 0 = coupée (défaut) ; 1 = ÉTAGE 0 (compter les collisions de zone joueur, coût
// nul : 0 % sur 17 macro, +0,13 % sur 9 macro) ; 2 = ÉTAGE 0 + ÉTAGE 1 (relancer le
// sous-solve sur les seules collisions pour croiser les verdicts — cher, un
// sous-solve par collision, jamais en régime).
static const int mesureCacheJoueur = qgetenv("CACHE_JOUEUR").toInt();
// Budget de l'ARBITRAGE des MORT→inconnu (étage 1). Large exprès : la question
// n'est pas « prouve-t-on au tarif de la prod » mais « l'état est-il mort, oui ou
// non ». Réglable pour vérifier qu'un « toujours inconnu » n'est pas un artefact
// de ce réglage-ci.
static const int budgetArbitrage =
    qgetenv("CACHE_JOUEUR_BUDGET").isEmpty() ? 10000 : qgetenv("CACHE_JOUEUR_BUDGET").toInt();

Game::EnclosInfo Game::detecteEnclosArrivee(int caisseArrivee, const QVector<bool>& zone,
                                            QVector<bool>& visite,
                                            QHash<QByteArray,VerdictEnclos>* cache,
                                            int budget) const {
    // Variante INCRÉMENTALE : un nouvel enclos ne peut apparaître qu'au contact de
    // la caisse qui vient d'arriver (c'est elle qui a pu scinder l'espace libre) —
    // même argument que le corral incrémental (game.h). On ne flood donc que depuis
    // les voisins LIBRES de 'caisseArrivee' hors zone joueur. Coût : O(1) quand rien
    // ne s'est scellé (le cas courant), O(région) sinon. 'visite' est maintenu
    // TOUT-FAUX entre appels (reset O(région) via 'touched') → aucun fill O(size).
    if (visite.size() != size) visite.fill(false, size);   // init unique
    QVarLengthArray<int, 256> touched;
    QVarLengthArray<int, 256> file;

    EnclosInfo info;
    const int ax = caisseArrivee % largeur, ay = caisseArrivee / largeur;

    for (int d0 = 0; d0 < NB_DIRECTION; d0++) {
        const int sx = ax + directions[d0].dx, sy = ay + directions[d0].dy;
        if (sx < 0 || sx >= largeur || sy < 0 || sy >= hauteur) continue;
        const int s = sx + sy * largeur;
        if (!isLibre(s) || zone[s] || visite[s]) continue;   // en zone / occupée / déjà vue

        // Flood cet enclos, en mesurant : cellules, buts vides, caisses-frontière et
        // caisses-frontière HORS but (celles qui font de l'enclos un piège potentiel).
        // On garde la LISTE des caisses-frontière pour le gate non-rouvrable.
        file.clear();
        QVarLengthArray<int, 16> frontBoxes;
        file.append(s); visite[s] = true; touched.append(s);
        int cells = 0, buts = 0, front = 0, frontHorsBut = 0;
        int tete = 0;
        while (tete < file.size()) {
            const int c = file[tete++];
            cells++;
            if (cases[c] == Level::tcGoal) buts++;
            const int cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int vx = cx + directions[d].dx, vy = cy + directions[d].dy;
                if (vx < 0 || vx >= largeur || vy < 0 || vy >= hauteur) continue;
                const int v = vx + vy * largeur;
                if (visite[v]) continue;
                if (cases[v] == Level::tcMur) continue;
                if (estCaisse(v)) {
                    visite[v] = true; touched.append(v);
                    front++;
                    frontBoxes.append(v);
                    if (cases[v] == Level::tcCaisse) frontHorsBut++;   // tcGoalCaisse = posée
                    continue;
                }
                visite[v] = true; touched.append(v); file.append(v);
            }
        }

        // PORTAIL BRUT : enclos scellé bordé d'au moins une caisse HORS but.
        if (frontHorsBut == 0) continue;
        info.candidats++;

        // GATE 1 — Hall (sous-dotation) : les caisses-frontière hors but ne peuvent,
        // si l'enclos est non-rouvrable, que se poser sur un but DANS l'enclos. S'il
        // y a au moins autant de buts que de caisses hors but, rien n'est prouvé.
        if (buts >= frontHorsBut) continue;

        // GATE 2 — non-rouvrable. Quand le joueur pousse une caisse-frontière b
        // (appui en zone), il finit SUR l'ancienne case de b, adjacent à l'enclos :
        // il y entre → rouvert. SAUF si la poussée envoie b sur son UNIQUE case
        // d'enclos (re-scellement, ex. la pince : les caisses ne peuvent que rentrer
        // dans S). Donc b est « rouvrant » ssi une poussée faisable (appui en zone,
        // dest libre) laisse à b un voisin d'enclos LIBRE ≠ dest. L'enclos est
        // non-rouvrable ssi aucune caisse-frontière n'est rouvrante.
        // (Case d'enclos = libre ET hors zone joueur.) ⚠️ Gate heuristique : exclure
        // à tort un vrai mort ne coûte qu'un élagage manqué, jamais un FP — c'est le
        // futur A* qui tranche. La pince PASSE ce gate (re-scellement).
        // Appartenance à CETTE région : ses cellules sont dans 'file' (petit). ⚠️ Ne
        // PAS utiliser « libre && hors zone » : ça inclurait des cases d'AUTRES
        // enclos scellés, qui ne rouvrent pas CELUI-CI (bug corrigé le 2026-07-27,
        // il faisait rater le corral bloquant du niveau 11).
        auto dansRegion = [&](int n){ for (int i = 0; i < file.size(); i++) if (file[i] == n) return true; return false; };
        bool rouvrable = false;
        for (int bi = 0; bi < frontBoxes.size() && !rouvrable; bi++) {
            const int b = frontBoxes[bi];
            const int bx = b % largeur, by = b / largeur;
            // Voisins de b DANS CETTE région.
            int voisEnclos[NB_DIRECTION], nv = 0;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int nx = bx + directions[d].dx, ny = by + directions[d].dy;
                if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                const int n = nx + ny * largeur;
                if (dansRegion(n)) voisEnclos[nv++] = n;
            }
            for (int d = 0; d < NB_DIRECTION && !rouvrable; d++) {
                const int sx = bx - directions[d].dx, sy = by - directions[d].dy;  // appui
                const int tx = bx + directions[d].dx, ty = by + directions[d].dy;  // dest
                if (sx < 0 || sx >= largeur || sy < 0 || sy >= hauteur) continue;
                if (tx < 0 || tx >= largeur || ty < 0 || ty >= hauteur) continue;
                const int s = sx + sy * largeur, t = tx + ty * largeur;
                if (!zone[s] || !isLibre(t)) continue;   // poussée infaisable depuis l'ouvert
                // Après la poussée (b libérée, dest=t occupée), b garde-t-il un
                // voisin d'enclos libre ≠ t ? Si oui, le joueur (arrivé en b) entre.
                for (int k = 0; k < nv; k++)
                    if (voisEnclos[k] != t) { rouvrable = true; break; }
            }
        }
        if (rouvrable) continue;

        // Candidat DUR : passe portail + Hall + non-rouvrable.
        info.durs++;
        info.cells      += cells;
        info.frontiere  += front;
        info.butsVides  += buts;

        // PREUVE strip + BFS borné, mémoïsée par frontière triée (mode cache).
        if (cache) {
            QVarLengthArray<int, 32> fs;
            for (int i = 0; i < frontBoxes.size(); i++) fs.append(frontBoxes[i]);
            std::sort(fs.begin(), fs.end());
            QByteArray key; key.resize(fs.size() * 2);
            for (int i = 0; i < fs.size(); i++) {
                key[2*i]   = (char)(fs[i] >> 8);
                key[2*i+1] = (char)(fs[i] & 0xff);
            }
            int verdict;
            auto it = cache->find(key);
            if (it != cache->end()) {
                verdict = it.value().verdict;
                info.cacheHits++;
                // ÉTAGE 0 (plan.md §6.1) — le verdict caché a été prouvé pour UNE
                // position de joueur ; on le transfère ici à une autre. Combien de
                // fois la zone diffère-t-elle vraiment ? On recalcule et on COMPTE,
                // sans rien changer au verdict rendu : le solve reste identique à
                // l'unité, seul le temps bouge (et ce surcoût EST le prix qu'aurait
                // la clé corrigée — donc on le mesure du même coup).
                if (mesureCacheJoueur >= 1 && it.value().zoneCanon >= 0) {
                    info.hitsTestes++;
                    if (zoneCanoniqueStrip(fs) != it.value().zoneCanon) {
                        info.hitsZoneDiff++;
                        if      (verdict == 0) info.diffMort++;
                        else if (verdict == 1) info.diffVivant++;
                        else                   info.diffInconnu++;
                        // ÉTAGE 1 — la zone diffère : que vaudrait le verdict ICI ?
                        // Seul juge possible de ce transfert (fp ne voit pas le cache,
                        // le canari ne voit pas un FP qui épargne le chemin optimal).
                        // ⚠️ 'verdict' n'est PAS réassigné : on rend le verdict caché,
                        // exactement comme en prod, sinon on mesurerait un autre solveur.
                        if (mesureCacheJoueur >= 2) {
                            int dev = 0;
                            const int vrai = sousSolveEnclos(fs, budget, &dev);
                            info.etage1States += dev;
                            auto idx = [](int v){ return v == 0 ? 0 : (v == 1 ? 1 : 2); };
                            info.etage1[3 * idx(verdict) + idx(vrai)]++;
                            // MORT caché mais INCONNU ici : le budget de prod ne
                            // suffit pas à refaire la preuve depuis cette position.
                            // Ni faux positif, ni transfert validé — on ARBITRE à
                            // budget large, c'est le seul moyen de conclure.
                            if (verdict == 0 && vrai == -1) {
                                int dev2 = 0;
                                const int arb = sousSolveEnclos(fs, budgetArbitrage, &dev2);
                                info.arbitreStates += dev2;
                                info.arbitre[idx(arb)]++;
                            }
                        }
                    }
                }
            }
            else {
                int dev = 0, zc = -1;
                verdict = sousSolveEnclos(fs, budget, &dev, &zc);
                cache->insert(key, VerdictEnclos{verdict, mesureCacheJoueur ? zc : -1});
                info.solveStates += dev;
            }
            if      (verdict == 0) info.dursMorts++;
            else if (verdict == 1) info.dursVivants++;
            else                   info.dursInconnus++;
        }
    }
    for (int i = 0; i < touched.size(); i++) visite[touched[i]] = false;   // reset O(région)

    return info;
}

bool Game::gateEnclosMort() const {
    // FULL-SCAN du gate (portail + Hall + non-rouvrable un-pas). Offline (fp) : on
    // s'autorise l'alloc de la zone et de 'visite'. Miroir du gate de
    // detecteEnclosArrivee ; ici on NE marque PAS les caisses dans 'visite' (chaque
    // enclos voit sa frontière complète, même si une caisse borde deux enclos).
    QVector<bool> zone; getZoneJoueur(zone);
    QVector<bool> visite(size, false);
    QVector<bool> dansRegion(size, false);   // appartenance à la région COURANTE
    QVarLengthArray<int, 512> file;

    for (int c0 = 0; c0 < size; c0++) {
        if (!isLibre(c0) || zone[c0] || visite[c0]) continue;

        file.clear();
        QVarLengthArray<int, 32> frontBoxes;
        file.append(c0); visite[c0] = true; dansRegion[c0] = true;
        int buts = 0, frontHorsBut = 0;
        int tete = 0;
        while (tete < file.size()) {
            const int c = file[tete++];
            if (cases[c] == Level::tcGoal) buts++;
            const int cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int vx = cx + directions[d].dx, vy = cy + directions[d].dy;
                if (vx < 0 || vx >= largeur || vy < 0 || vy >= hauteur) continue;
                const int v = vx + vy * largeur;
                if (cases[v] == Level::tcMur) continue;
                if (estCaisse(v)) {
                    frontBoxes.append(v);
                    if (cases[v] == Level::tcCaisse) frontHorsBut++;
                    continue;
                }
                if (visite[v]) continue;
                visite[v] = true; dansRegion[v] = true; file.append(v);
            }
        }

        // Reset de dansRegion à TOUT point de sortie (la région suivante repart
        // d'un tableau tout-faux). Lambda et non macro : même portée, même coût.
        auto resetRegion = [&]{ for (int i = 0; i < file.size(); i++) dansRegion[file[i]] = false; };

        if (frontHorsBut == 0) { resetRegion(); continue; }          // portail
        if (buts >= frontHorsBut) { resetRegion(); continue; }       // Hall

        bool rouvrable = false;                    // non-rouvrable (un pas)
        for (int bi = 0; bi < frontBoxes.size() && !rouvrable; bi++) {
            const int b = frontBoxes[bi];
            const int bx = b % largeur, by = b / largeur;
            // Voisins de b DANS CETTE région (pas « n'importe quelle case scellée » :
            // une case d'un AUTRE enclos ne rouvre pas CELUI-CI).
            int voisEnclos[NB_DIRECTION], nv = 0;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int nx = bx + directions[d].dx, ny = by + directions[d].dy;
                if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                const int n = nx + ny * largeur;
                if (dansRegion[n]) voisEnclos[nv++] = n;
            }
            for (int d = 0; d < NB_DIRECTION && !rouvrable; d++) {
                const int sx = bx - directions[d].dx, sy = by - directions[d].dy;
                const int tx = bx + directions[d].dx, ty = by + directions[d].dy;
                if (sx < 0 || sx >= largeur || sy < 0 || sy >= hauteur) continue;
                if (tx < 0 || tx >= largeur || ty < 0 || ty >= hauteur) continue;
                const int s = sx + sy * largeur, t = tx + ty * largeur;
                if (!zone[s] || !isLibre(t)) continue;
                for (int k = 0; k < nv; k++)
                    if (voisEnclos[k] != t) { rouvrable = true; break; }
            }
        }
        resetRegion();

        if (rouvrable) continue;
        return true;   // enclos DUR
    }
    return false;
}

int Game::zoneCanoniqueStrip(const QVarLengthArray<int, 32>& boxes) const {
    // Flood du joueur sur {murs + 'boxes'} — toutes les AUTRES caisses sont du sol
    // (c'est le strip). Rend le min des index atteints : deux positions de joueur de
    // la même zone donnent la même valeur, deux zones distinctes des valeurs
    // distinctes. Même définition, au bit près, que la lambda floodZone de
    // sousSolveEnclos ci-dessous, qui appelle cette méthode pour son état initial.
    const int n = boxes.size();
    QVarLengthArray<bool, 512> occ(size);
    for (int i = 0; i < size; i++) occ[i] = false;
    for (int i = 0; i < n; i++) occ[boxes[i]] = true;

    QVarLengthArray<bool, 512> vu(size);
    for (int i = 0; i < size; i++) vu[i] = false;

    const int joueur = playerPoint.x() + playerPoint.y() * largeur;
    QVarLengthArray<int, 512> file;
    file.append(joueur); vu[joueur] = true;
    int mn = joueur, t = 0;
    while (t < file.size()) {
        const int c = file[t++]; if (c < mn) mn = c;
        const int cx = c % largeur, cy = c / largeur;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int nx = cx + directions[d].dx, ny = cy + directions[d].dy;
            if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
            const int v = nx + ny * largeur;
            if (vu[v] || cases[v] == Level::tcMur || occ[v]) continue;
            vu[v] = true; file.append(v);
        }
    }
    return mn;
}

int Game::sousSolveEnclos(const QVarLengthArray<int, 32>& boxesInit, int budget,
                          int* developpesOut, int* zoneCanonOut) const {
    const int n = boxesInit.size();
    if (n == 0) {
        if (developpesOut) *developpesOut = 0;
        if (zoneCanonOut)  *zoneCanonOut  = -1;
        return 1;
    }

    auto estMur = [&](int c){ return cases[c] == Level::tcMur; };
    auto estBut = [&](int c){ return cases[c] == Level::tcGoal || cases[c] == Level::tcGoalCaisse
                                    || cases[c] == Level::tcGoalPlayer; };

    std::vector<int> start(boxesInit.begin(), boxesInit.end());
    std::sort(start.begin(), start.end());

    QVector<bool> occ(size, false), zone(size, false), zone2(size, false);
    QVarLengthArray<int, 512> fileFlood;

    // Zone joueur sur le board {murs + caisses 'boxes'} ; remplit 'zoneBuf' et rend
    // la case CANONIQUE (min) de la zone (pour dédupliquer les positions joueur).
    // ⚠️ zoneBuf est PARAMÉTRÉ : la zone de l'état courant (zone) et celle des
    // successeurs (zone2) sont des buffers SÉPARÉS — sinon le flood d'un successeur
    // écraserait la zone courante dont le test de poussée a encore besoin.
    auto floodZone = [&](const std::vector<int>& boxes, int joueur, QVector<bool>& zoneBuf) -> int {
        for (int i = 0; i < n; i++) occ[boxes[i]] = true;
        zoneBuf.fill(false, size);
        fileFlood.clear(); fileFlood.append(joueur); zoneBuf[joueur] = true;
        int mn = joueur, t = 0;
        while (t < fileFlood.size()) {
            const int c = fileFlood[t++]; if (c < mn) mn = c;
            const int cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int nx = cx + directions[d].dx, ny = cy + directions[d].dy;
                if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                const int v = nx + ny * largeur;
                if (zoneBuf[v] || estMur(v) || occ[v]) continue;
                zoneBuf[v] = true; fileFlood.append(v);
            }
        }
        for (int i = 0; i < n; i++) occ[boxes[i]] = false;
        return mn;
    };
    auto cle = [&](const std::vector<int>& boxes, int joueurNorm) -> QByteArray {
        QByteArray k; k.resize((n + 1) * 2);
        for (int i = 0; i < n; i++) { k[2*i] = (char)(boxes[i] >> 8); k[2*i+1] = (char)(boxes[i] & 0xff); }
        k[2*n] = (char)(joueurNorm >> 8); k[2*n+1] = (char)(joueurNorm & 0xff);
        return k;
    };
    auto gagne = [&](const std::vector<int>& boxes){ for (int i=0;i<n;i++) if(!estBut(boxes[i])) return false; return true; };
    auto estBoite = [&](int c, const std::vector<int>& boxes){ for (int i=0;i<n;i++) if(boxes[i]==c) return true; return false; };

    QSet<QByteArray> vus;
    std::vector<std::pair<std::vector<int>, int>> fileBFS;
    // Exemplaire UNIQUE du calcul de zone canonique (game.h) : la mesure de l'étage 0
    // recalcule EXACTEMENT ceci à un cache-hit, donc les deux ne peuvent pas diverger.
    // (floodZone remplissait zone2 au passage, mais ce contenu était jeté — zone2 est
    // réécrit par le premier successeur généré.)
    const int jn0 = zoneCanoniqueStrip(boxesInit);
    if (zoneCanonOut) *zoneCanonOut = jn0;
    vus.insert(cle(start, jn0));
    fileBFS.push_back({start, jn0});

    int developpes = 0;
    size_t tete = 0;
    while (tete < fileBFS.size()) {
        if (developpes >= budget) { if (developpesOut) *developpesOut = developpes; return -1; }
        const std::vector<int> boxes = fileBFS[tete].first;   // copie : on modifie fileBFS
        const int joueurNorm = fileBFS[tete].second;
        tete++; developpes++;
        if (gagne(boxes)) { if (developpesOut) *developpesOut = developpes; return 1; }

        floodZone(boxes, joueurNorm, zone);                 // remplit zone[] pour CE state
        // ⚠️ occ n'est PAS pré-marqué ici : floodZone(nb,...) plus bas gère occ en
        // propre et le remettrait à faux, corrompant l'état. On teste l'occupation
        // par appartenance directe (estBoite), n petit.
        for (int i = 0; i < n; i++) {
            const int b = boxes[i];
            const int bx = b % largeur, by = b / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int dx = directions[d].dx, dy = directions[d].dy;
                const int destX = bx + dx, destY = by + dy;
                const int appX  = bx - dx, appY  = by - dy;
                if (destX < 0 || destX >= largeur || destY < 0 || destY >= hauteur) continue;
                if (appX  < 0 || appX  >= largeur || appY  < 0 || appY  >= hauteur) continue;
                const int dest = destX + destY * largeur, app = appX + appY * largeur;
                if (estMur(dest) || estBoite(dest, boxes)) continue;   // arrivée bloquée
                if (!zone[app]) continue;                   // joueur ne peut pas pousser
                if (casesMortes.at(dest)) continue;         // case morte statique (prune sûr)
                std::vector<int> nb = boxes; nb[i] = dest; std::sort(nb.begin(), nb.end());
                const int jn = floodZone(nb, b, zone2);            // joueur finit sur l'ancienne case b
                const QByteArray k = cle(nb, jn);
                if (!vus.contains(k)) { vus.insert(k); fileBFS.push_back({nb, jn}); }
            }
        }
    }
    if (developpesOut) *developpesOut = developpes;
    return 0;   // file vidée sous budget → MORT
}

int Game::caseApres(int idxCase, EDirection dir) const {
    return idxCase + directions[dir].dx + directions[dir].dy * largeur;
}

int Game::caisseAssignee(int indexBut) const {
    if (indexBut < 0 || indexBut >= nbButs) return -1;
    QVarLengthArray<int, 32> parBut(nbButs);
    getHeuristique(nullptr, -1, parBut.data());
    return parBut[indexBut];
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

// Distance de LIVRAISON : nombre minimal de poussées pour amener UNE caisse, depuis
// sa case de départ, jusqu'à chaque case du plateau. `bloque` marque les buts déjà
// rangés, qui font obstacle.
//
// BFS avant sur les poussées : une caisse en c peut aller en c+d si c+d est libre
// (arrivée) ET le joueur MARCHE JUSQU'À c-d (case d'appui) — pas seulement qu'elle
// soit libre. Corrigé le 2026-07-20 : la version précédente ne pointait qu'un point
// libre, sans vérifier que le joueur peut physiquement s'y rendre ; elle laissait
// passer des livraisons qui exigent de traverser une case déjà condamnée. Reste une
// approximation à deux endroits : (a) les AUTRES caisses non livrées ne sont pas des
// obstacles pour la marche (seule la caisse EN COURS de déplacement l'est) ; (b) le
// joueur est supposé pouvoir rejoindre n'importe quelle caisse de départ. Les deux
// vont dans le sens optimiste — passage encore, pas interdiction abusive.
QVector<int> Game::distanceLivraison(const QVector<bool>& bloque) const {
    // JOUEUR-AWARE depuis le 2026-07-29 (§6.2). ⚠️ L'ancienne version ne retenait
    // qu'UNE position de joueur par case atteinte (`joueurApres[a] = c`) et ne
    // revisitait jamais une case déjà vue — exactement le défaut qui avait produit 86
    // faux positifs au test « but orphelin » (§6.1, 2026-07-21) et que `distanceParBut`
    // corrige depuis le §2.2 en indexant par RÉGION. Ici il rendait le test trop
    // PESSIMISTE : il ratait des routes, déclarait des buts non livrables, et la garde
    // anti-échouage de `ordreParPrecedence` refusait alors TOUT candidat — mesuré le
    // 2026-07-29 : échec dès le rang 0 sur les niveaux 18, 20, 23 et 25.
    //
    // L'état du BFS est donc le COUPLE (case de la caisse, zone du joueur), la zone
    // étant identifiée par sa case canonique (le plus petit index qu'elle contient) sur
    // le plateau {murs + buts déjà posés + la caisse}. Une même case atteinte « par
    // l'autre côté » est un état DIFFÉRENT et ouvre d'autres poussées.
    QVector<int> dist(size, -1);

    auto libreCase = [&](int c) -> bool { return cases[c] != Level::tcMur && !bloque[c]; };

    // Flood du joueur depuis 'depart', la caisse en 'caisse' faisant obstacle. Rend la
    // case CANONIQUE de la zone (min des index atteints), -1 si 'depart' est occupé ;
    // remplit 'out' quand l'appelant a besoin de tester l'appartenance.
    QVector<bool> vu(size, false);
    QVarLengthArray<int, 512> pile;
    auto zoneCanon = [&](int depart, int caisse, QVector<bool>* out) -> int {
        if (depart < 0 || depart == caisse || !libreCase(depart)) return -1;
        vu.fill(false);
        pile.clear();
        pile.append(depart);
        vu[depart] = true;
        int mn = depart;
        for (int t = 0; t < pile.size(); t++) {
            const int c = pile[t];
            if (c < mn) mn = c;
            const int cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int nx = cx + directions[d].dx, ny = cy + directions[d].dy;
                if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                const int n = nx + ny * largeur;
                if (vu[n] || n == caisse || !libreCase(n)) continue;
                vu[n] = true;
                pile.append(n);
            }
        }
        if (out) *out = vu;
        return mn;
    };

    const int depart = playerPoint.x() + playerPoint.y() * largeur;

    // Sources : les cases portant une caisse au chargement. Relaxation conservée — le
    // joueur est supposé pouvoir rejoindre n'importe laquelle, donc on part de sa
    // position réelle.
    QVector<int> distEtat(size * size, -1);       // (caisse, zoneCanon) -> distance
    QList<QPair<int,int>> file;                   // (caisse, zoneCanon)
    for (int c = 0; c < size; c++) {
        if (bloque[c]) continue;
        if (cases[c] != Level::tcCaisse && cases[c] != Level::tcGoalCaisse) continue;
        const int z = zoneCanon(depart, c, nullptr);
        if (z < 0) continue;
        dist[c] = 0;
        if (distEtat[c * size + z] == -1) {
            distEtat[c * size + z] = 0;
            file.append({c, z});
        }
    }

    QVector<bool> zone(size, false);
    while (!file.isEmpty()) {
        const QPair<int,int> etat = file.takeFirst();
        const int c = etat.first, z = etat.second;
        const int g = distEtat[c * size + z];
        // 'z' appartient à la zone par construction : flooder depuis lui la reconstruit.
        zoneCanon(z, c, &zone);

        const int cx = c % largeur, cy = c / largeur;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int ax = cx + directions[d].dx, ay = cy + directions[d].dy;   // arrivée
            const int px = cx - directions[d].dx, py = cy - directions[d].dy;   // appui joueur
            if (ax < 0 || ax >= largeur || ay < 0 || ay >= hauteur) continue;
            if (px < 0 || px >= largeur || py < 0 || py >= hauteur) continue;
            const int a = ax + ay * largeur, appui = px + py * largeur;
            if (!libreCase(a) || !libreCase(appui)) continue;
            if (!zone[appui]) continue;              // appui hors de portée du joueur
            // Poussée faite : la caisse est en 'a', le joueur occupe l'ancienne case 'c'.
            const int nz = zoneCanon(c, a, nullptr);
            if (nz < 0) continue;
            if (dist[a] == -1) dist[a] = g + 1;      // BFS : première visite = minimum
            if (distEtat[a * size + nz] == -1) {
                distEtat[a * size + nz] = g + 1;
                file.append({a, nz});
            }
        }
    }
    return dist;
}

// PRÉCÉDENCE GLOBALE — le TRAJET DE TIRAGE complet (§6.2, famille B, 2026-07-30).
//
// La règle de 2026-07-20 (ci-dessous) ne regarde que l'approche FINALE : caisse en
// G−d, joueur en G−2d. Elle rate le cas où cette approche est libre mais où AUCUNE
// caisse ne peut ARRIVER sur la case d'approche. C'est la limite que le plan nommait
// sans la corriger — « le rebours ne simule pas le trajet de tirage ». La règle :
//
//     G doit précéder B si, en traitant B comme OCCUPÉ, plus aucune caisse du
//     départ n'atteint G.
//
// Test : BFS de TIRAGE à rebours depuis G (on remonte les poussées — la caisse était
// en G−d, le joueur en G−2d), une fois par autre but B marqué occupé.
//
// ⚠️ RELAXATION OPTIMISTE (joueur supposé capable d'atteindre n'importe quel appui,
// aucune autre caisse sur le plateau) : « inatteignable » est donc une PREUVE, et
// « atteignable » ne promet rien. C'est une ARÊTE de précédence, pas un score — la
// forme qui a marché le 2026-07-20, pas celle qui a échoué le 19 (distance de sortie
// transformée en score, six variantes toutes réfutées).
//
// Statique (ne lit que les murs et les caisses de DÉPART), donc calculé une fois par
// niveau, comme casesMortes. O(buts² × plateau).
// Le fichier d'ordre injecté pour ce niveau, s'il existe — sinon une chaîne vide.
// EXEMPLAIRE UNIQUE du nom : `calculDistancePoussee` le lit, l'UI l'interroge pour
// dire dans le journal hybride que l'ordre affiché n'est PAS l'ordre calculé. Deux
// endroits qui devineraient le nom chacun de leur côté finiraient par diverger.
// ⚠️ OUTIL DE CHANTIER (campagne hybride, 2026-08-01), à retirer avec elle.
QString Game::cheminOrdreInjecte(int numNiveau) {
    const QString nom = QString("ordre_niveau_%1.txt").arg(numNiveau, 4, 10, QChar('0'));
    const QString ici = QDir::current().filePath(nom);
    return QFile::exists(ici) ? ici : QString();
}

// LES SALLES (§6.2, 2026-08-01) : composantes connexes du sous-graphe des cases-buts
// en 4-connexité. Deux buts voisins sont dans la même salle ; une salle est donc un
// bloc de buts d'un seul tenant. Mesuré sur les 35 niveaux : 30 n'ont qu'UNE salle
// (donc rien ne peut y changer), et six sont multi — 0 (trois salles d'un but !),
// 10 (28+4), 18 (7+2+2), 24 (20+2), 25 (17+2), 26 (12+1).
// ⚠️ Ce n'est PAS « les pièces du plateau » : le plateau est connexe pour le joueur.
// C'est l'adjacence des BUTS, et c'est elle qui décide si la macro peut enchaîner.
QVector<int> Game::sallesDeButs() const {
    QVector<int> salle(nbButs, -1);
    int n = 0;
    for (int i = 0; i < nbButs; i++) {
        if (salle[i] >= 0) continue;
        salle[i] = n;
        // Propagation jusqu'à saturation. nbButs ≤ 32 : le coût quadratique est
        // sans objet, et c'est du statique (appelé une fois, au chargement).
        for (bool encore = true; encore; ) {
            encore = false;
            for (int a = 0; a < nbButs; a++) {
                if (salle[a] != n) continue;
                const int ax = goals[a] % largeur, ay = goals[a] / largeur;
                for (int b = 0; b < nbButs; b++) {
                    if (salle[b] >= 0) continue;
                    const int bx = goals[b] % largeur, by = goals[b] / largeur;
                    if (qAbs(ax - bx) + qAbs(ay - by) == 1) { salle[b] = n; encore = true; }
                }
            }
        }
        n++;
    }
    return salle;
}

// L'INTÉRIEUR DU PLATEAU (cf. game.h). Le dehors d'un `.xsb` est fait d'espaces :
// sans ce flood, « non-mur » désigne aussi le remplissage hors contour, et toute
// mesure de géométrie déborde du plateau.
QVector<bool> Game::interieur() const {
    QVector<bool> dedans(size, false);
    if (!cases) return dedans;
    // ⚠️ SANS JOUEUR, `playerPoint` vaut (0,0) — le coin, donc un MUR sur tout
    // plateau normal — et le flood partirait d'une case qui n'existe pas, en la
    // déclarant intérieure au passage. Ça arrive pour de vrai : un `.xsb` de zone
    // produit par `mesures/zonembut` n'a pas de joueur. Repli sur un but, qui est
    // intérieur par construction.
    int depart = playerPoint.x() + playerPoint.y() * largeur;
    if (playerPoint.x() < 0 || playerPoint.x() >= largeur
        || playerPoint.y() < 0 || playerPoint.y() >= hauteur
        || cases[depart] == Level::tcMur)
        depart = goals.isEmpty() ? -1 : goals.first();
    if (depart < 0 || cases[depart] == Level::tcMur) return dedans;

    static const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
    QVector<int> pile;
    dedans[depart] = true;
    pile.append(depart);
    while (!pile.isEmpty()) {
        const int c = pile.takeLast();
        const int cx = c % largeur, cy = c / largeur;
        for (int d = 0; d < 4; d++) {
            const int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
            const int n = nx + ny * largeur;
            if (dedans[n] || cases[n] == Level::tcMur) continue;
            dedans[n] = true;
            pile.append(n);
        }
    }
    return dedans;
}

// LES ZONES D'EMBUT (cf. game.h pour les trois étages et ce que ça ne prouve pas).
// Statique, O(size × buts), appelé hors du chemin chaud : aucun réglage à faire
// entrer dans le solveur, aucune table à maintenir.
QVector<Game::ZoneEmbut> Game::zonesEmbut(int seuilCulDeSac) const {
    QVector<ZoneEmbut> zones;
    if (!cases || nbButs == 0) return zones;

    const QVector<bool> dedans = interieur();
    static const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
    auto voisin = [&](int c, int d) -> int {
        const int x = c % largeur + dx[d], y = c / largeur + dy[d];
        if (x < 0 || x >= largeur || y < 0 || y >= hauteur) return -1;
        return x + y * largeur;
    };
    auto estBut = [&](int c) {
        return cases[c] == Level::tcGoal || cases[c] == Level::tcGoalCaisse
            || cases[c] == Level::tcGoalPlayer;
    };

    // ÉTAGE 2 — LES GOULOTS. Au plus deux voisins libres : couloir, coude ou
    // cul-de-sac. ⚠️ Un BUT n'est jamais un goulot, sans quoi la queue de buts
    // d'une case de large du niveau 10 (x=17, y=10..14) se ferait trancher de sa
    // propre salle.
    QVector<bool> goulot(size, false);
    for (int c = 0; c < size; c++) {
        if (!dedans[c] || estBut(c)) continue;
        int n = 0;
        for (int d = 0; d < 4; d++) { const int v = voisin(c, d); if (v >= 0 && dedans[v]) n++; }
        if (n <= 2) goulot[c] = true;
    }

    // LES PORTES DOUBLES (calé sur le découpage à la main du niveau 10, et sur
    // le débordement mesuré du niveau 8 : 54 % du plateau avalé par une entrée
    // large de DEUX cases, que le test de goulot ne voit pas — chacune des deux
    // a trois voisins libres). Une paire de cases libres voisines est une porte
    // double si :
    //   (a) des MURS la ferment aux deux bouts de son axe — c'est une brèche dans
    //       une ligne de mur, pas le milieu d'une salle ;
    //   (b) la retirer COUPE le plateau en deux.
    // ⚠️ (b) n'est pas décoratif : sans elle, le couloir large de deux du niveau
    // 20 (colonnes x=14/15, douze rangées) se ferait trancher à chaque rangée —
    // il est bien fermé par des murs des deux côtés, mais on le contourne par la
    // colonne de buts, donc il ne coupe rien.
    QVector<bool> porteDouble(size, false);
    for (int c = 0; c < size; c++) {
        if (!dedans[c] || estBut(c)) continue;
        for (int axe = 0; axe < 2; axe++) {            // 0 = horizontal, 1 = vertical
            const int d = axe ? 2 : 0;                 // droite / bas
            const int b = voisin(c, d);
            if (b < 0 || !dedans[b] || estBut(b)) continue;
            const int avant = voisin(c, d ^ 1), apres = voisin(b, d);
            if (avant >= 0 && dedans[avant]) continue;  // (a) pas fermé en amont
            if (apres >= 0 && dedans[apres]) continue;  // (a) pas fermé en aval
            // (b) la paire coupe-t-elle ? Un flood depuis n'importe quelle case
            // libre restante doit atteindre TOUTES les autres.
            int depart = -1, reste = 0;
            for (int q = 0; q < size; q++)
                if (dedans[q] && q != c && q != b) { if (depart < 0) depart = q; reste++; }
            if (depart < 0) continue;
            QVector<bool> vu(size, false); QVector<int> pile;
            vu[depart] = true; pile.append(depart); int atteints = 1;
            while (!pile.isEmpty()) {
                const int q = pile.takeLast();
                for (int e = 0; e < 4; e++) {
                    const int v = voisin(q, e);
                    if (v < 0 || !dedans[v] || v == c || v == b || vu[v]) continue;
                    vu[v] = true; atteints++; pile.append(v);
                }
            }
            if (atteints < reste) { porteDouble[c] = true; porteDouble[b] = true; }
        }
    }
    for (int c = 0; c < size; c++) if (porteDouble[c]) goulot[c] = true;

    // LES FRAGMENTS : composantes connexes de l'intérieur PRIVÉ de ses goulots.
    // C'est le « corps » des salles, débarrassé de ce qui les relie.
    QVector<int> frag(size, -1);
    int nbFrag = 0;
    for (int c = 0; c < size; c++) {
        if (!dedans[c] || goulot[c] || frag[c] >= 0) continue;
        QVector<int> pile; pile.append(c); frag[c] = nbFrag;
        while (!pile.isEmpty()) {
            const int q = pile.takeLast();
            for (int d = 0; d < 4; d++) {
                const int v = voisin(q, d);
                if (v >= 0 && dedans[v] && !goulot[v] && frag[v] < 0) { frag[v] = nbFrag; pile.append(v); }
            }
        }
        nbFrag++;
    }
    // ÉTAGE 1 — UNE ZONE PAR SALLE DE BUTS, ÉTAGÈRES COMPRISES.
    //
    // `sallesDeButs()` regroupe les buts 4-ADJACENTS. Ça ne suffit pas : le niveau
    // 24 porte deux paquets — (1,1)..(10,1)/(1,2)..(10,2) et (13,1)/(13,2) — posés
    // sur la MÊME rangée, adossés au MÊME mur continu (la ligne y=0), séparés par
    // deux cases vides. C'est une seule étagère, donc une seule zone (constat
    // utilisateur, 2026-08-22).
    //
    // D'où une seconde règle de regroupement, purement statique : deux buts sont de
    // la même salle s'ils sont alignés (même ligne ou même colonne), que tout est
    // LIBRE entre eux, et qu'un mur CONTINU les longe du même côté sur toute la
    // longueur du segment. C'est « aligné sur le même côté d'un même mur, sans
    // détour ».
    //
    // ⚠️ LA FUSION SE FAIT ICI, PAS DANS `sallesDeButs()`, et ce n'est pas un détail
    // de style : le SOLVEUR utilise `sallesDeButs()` dans `ordreParPrecedence`
    // (lookahead de rang 0 confiné à la salle de tête, §6.2 — le correctif
    // multi-salles qui vaut ×7,5 sur le niveau 10). Changer ce regroupement
    // décalerait l'ordre de remplissage de tout le corpus. Les zones lisent la
    // géométrie ; elles ne touchent pas au moteur.
    const QVector<int> salleBrute = sallesDeButs();
    QVector<int> racine(nbButs);
    for (int b = 0; b < nbButs; b++) racine[b] = b;
    std::function<int(int)> trouve = [&](int b) { return racine[b] == b ? b : racine[b] = trouve(racine[b]); };
    for (int a2 = 0; a2 < nbButs; a2++)
        for (int b = 0; b < nbButs; b++)
            if (salleBrute[a2] == salleBrute[b]) racine[trouve(a2)] = trouve(b);

    auto etagere = [&](int ca, int cb) {
        const int ax = ca % largeur, ay = ca / largeur;
        const int bx = cb % largeur, by = cb / largeur;
        if (ax != bx && ay != by) return false;
        const int pas = (ay == by) ? 1 : largeur;
        const int d1 = qMin(ca, cb), d2 = qMax(ca, cb);
        if (d2 - d1 <= (int)pas) return false;                 // voisins : déjà traités
        for (int q = d1 + pas; q < d2; q += pas)
            if (!dedans[q]) return false;                      // un mur coupe le segment
        // un mur continu longe-t-il le segment, d'un côté ou de l'autre ?
        const int cote = (ay == by) ? largeur : 1;
        for (int signe = -1; signe <= 1; signe += 2) {
            bool plein = true;
            for (int q = d1; q <= d2 && plein; q += pas) {
                const int v = q + signe * cote;
                const int vx = v % largeur, vy = v / largeur;
                if (v < 0 || v >= size) { plein = false; break; }
                if (ay == by && vy != (q / largeur) + signe) { plein = false; break; }
                if (ax == bx && vx != q % largeur)           { plein = false; break; }
                if (cases[v] != Level::tcMur) plein = false;
            }
            if (plein) return true;
        }
        return false;
    };
    for (int a2 = 0; a2 < nbButs; a2++)
        for (int b = a2 + 1; b < nbButs; b++)
            if (etagere(goals[a2], goals[b])) racine[trouve(a2)] = trouve(b);

    QVector<int> salle(nbButs, -1);
    int nbSalles = 0;
    for (int b = 0; b < nbButs; b++) {
        const int r = trouve(b);
        if (salle[r] < 0) salle[r] = nbSalles++;
        salle[b] = salle[r];
    }
    for (int b = 0; b < nbButs; b++) salle[b] = salle[trouve(b)];

    for (int s = 0; s < nbSalles; s++) {
        QVector<bool> zone(size, false);
        for (int b = 0; b < nbButs; b++) {
            if (salle[b] != s) continue;
            zone[goals[b]] = true;
            const int f = frag[goals[b]];
            if (f >= 0) for (int c = 0; c < size; c++) if (frag[c] == f) zone[c] = true;
        }

        // ÉTAGE 3 — L'ABSORPTION. Un goulot qui borde la zone : on regarde ce
        // qu'il y a DERRIÈRE, sans repasser par la zone. « Ce qui est BOUCHÉ
        // appartient à la zone, ce qui DÉBOUCHE n'y appartient pas » (règle
        // choisie par l'utilisateur le 2026-08-22) : une poche est avalée —
        // c'est une alcôve de la salle, pas une sortie —, un débouché est une
        // PORTE et la croissance s'arrête là.
        //
        // ⚠️ LE TEST EST RELATIF, PAS UN SEUIL EN DUR : est avalée la poche plus
        // PETITE que la salle à laquelle elle s'accroche. C'est ce qui distingue
        // les deux cas, et ils sont franchement séparés — la poche du niveau 20
        // fait 4 cases contre 36 de zone, quand le débouché du niveau 1 en fait
        // 44 contre 10. Un seuil absolu marchait aussi (4, balayé sur 1-8) mais
        // ne disait rien : il fallait le régler par niveau plutôt que le déduire.
        // `seuilCulDeSac` ne sert plus que de PLANCHER, pour les salles minuscules
        // (le niveau 25 n'a que 3 cases de corps).
        // RESSERRAGE PAR COUPE (option (i), choisie par l'utilisateur le 2026-08-22).
        // La zone peut être ÉNORME quand rien ne sépare la salle de buts du
        // reste — niveau 21 : la salle et tout le milieu du plateau ne forment
        // qu'un seul fragment, faute du moindre rétrécissement entre les deux.
        // On cherche alors une COUPE de 1 ou 2 cases DANS la zone qui isole les
        // buts, et on ne garde que le côté des buts.
        //
        // ⚠️ Trois garde-fous, et chacun répare un cas vu à la mesure :
        //   · la coupe ne porte JAMAIS sur un but ;
        //   · tous les buts de la salle doivent rester du même côté (sinon la
        //     coupe ne sépare pas la salle, elle la casse) ;
        //   · l'autre côté doit être SUBSTANTIEL (> seuil). Sans ça, la paire
        //     {(14,7),(15,6)} du niveau 1 « coupe » — elle isole le seul coin
        //     (14,6) — et la salle se ferait charcuter par ses propres angles.
        // À taille de coupe égale on prend celle qui laisse le PLUS PETIT côté
        // buts : c'est la bouche de la salle, pas un rétrécissement lointain.
        // ⚠️ Une coupe à 2 cases n'a pas à être faite de cases VOISINES — c'est
        // tout l'intérêt ici. La bouche du 21 est {(9,9),(10,8)}, deux cases en
        // diagonale ayant chacune trois voisins libres : ni le goulot ni la porte
        // double ne pouvaient la voir.
        {
            QVector<int> dansZone;
            for (int q = 0; q < size; q++) if (zone[q]) dansZone.append(q);
            QVector<int> butsSalle;
            for (int b = 0; b < nbButs; b++) if (salle[b] == s) butsSalle.append(goals[b]);

            // Le côté des buts après retrait de la coupe, ou vide si la coupe est
            // invalide (buts séparés, ou autre côté insignifiant).
            // ⚠️ Tampons hoistés : la recherche teste O(cases²) paires, et deux
            // QVector<bool>(size) par paire coûtaient 16 s sur les 32 niveaux
            // contre moins de 2 ici. Statique et hors chemin chaud, mais un outil
            // qu'on relance à chaque idée doit rendre la main tout de suite.
            QVector<bool> vu(size, false);
            QVector<int> pile;
            auto coteButs = [&](int c1, int c2) {
                vu.fill(false);
                pile.clear();
                pile.append(butsSalle.first()); vu[butsSalle.first()] = true;
                int n = 1;
                while (!pile.isEmpty()) {
                    const int q = pile.takeLast();
                    for (int d = 0; d < 4; d++) {
                        const int v = voisin(q, d);
                        if (v < 0 || !zone[v] || v == c1 || v == c2 || vu[v]) continue;
                        vu[v] = true; n++; pile.append(v);
                    }
                }
                for (int b : butsSalle) if (!vu[b]) return QVector<bool>();   // buts séparés
                const int autre = dansZone.size() - n - (c1 >= 0) - (c2 >= 0);
                // ⚠️ TROIS, et c'est balayé, pas choisi. À 4 (l'ancien seuil), la
                // coupe (4,6) du niveau 18 était rejetée d'UNE case — son côté
                // gauche en fait exactement 4 — et la zone gardait un ENCLOS SANS
                // AUCUN EMBUT, repéré à l'œil par l'utilisateur sur la planche
                // contact. À 1 ou 2, la salle du 18 se fait au contraire réduire à
                // 3 cases. Mesuré sur les 8 zones de vérité terrain : 1, 2, 3 et 4
                // les laissent toutes identiques, seul le 18 bouge.
                if (autre <= 3) return QVector<bool>();                        // côté insignifiant
                return vu;
            };

            QVector<bool> meilleur; int meilleurTaille = -1;
            for (int taille = 1; taille <= 2 && meilleur.isEmpty(); taille++) {
                for (int i = 0; i < dansZone.size(); i++) {
                    const int c1 = dansZone[i];
                    if (estBut(c1)) continue;
                    for (int j = (taille == 1 ? i : i + 1);
                         j < (taille == 1 ? i + 1 : dansZone.size()); j++) {
                        const int c2 = (taille == 1) ? -1 : dansZone[j];
                        if (c2 >= 0 && estBut(c2)) continue;
                        const QVector<bool> cote = coteButs(c1, c2);
                        if (cote.isEmpty()) continue;
                        int n = 0;
                        for (int q = 0; q < size; q++) if (cote[q]) n++;
                        if (meilleurTaille < 0 || n < meilleurTaille) { meilleur = cote; meilleurTaille = n; }
                    }
                }
            }
            if (!meilleur.isEmpty()) zone = meilleur;
        }

        // Le CŒUR : la zone avant toute absorption. Sert au ménage final.
        const QVector<bool> coeur = zone;

        // ⚠️ LE PLAFOND EST UNE CONSTANTE, ET C'EST UNE RÉFUTATION MESURÉE, pas
        // une paresse. La version « scale-free » — avaler la poche si elle est
        // plus PETITE que la salle à laquelle elle s'accroche — est séduisante
        // (elle supprime la constante) et FAUSSE des deux façons possibles :
        //   · plafond suivant la zone en croissance : ça s'emballe (la zone
        //     grandit → le plafond grandit → elle avale plus gros). Niveau 21 :
        //     les 94 cases du plateau, zéro porte.
        //   · plafond figé sur le cœur de la salle : le cœur est DÉJÀ trop gros
        //     quand la salle de buts n'est séparée de rien (niveau 21 encore :
        //     94 cases), et les deux zones du niveau 25 fusionnent — ce que le
        //     découpage à la main interdit.
        // Une poche est petite dans l'ABSOLU (4 cases au niveau 20) ; un débouché
        // est le reste du plateau (44 cases au niveau 1). Les deux modes sont
        // franchement séparés, et le seuil balayé (4/5/6 donnent le même résultat)
        // tombe entre les deux.
        const int plafond = seuilCulDeSac;

        QVector<bool> porte(size, false);
        for (bool encore = true; encore; ) {
            encore = false;
            QVector<int> bord;
            for (int c = 0; c < size; c++) {
                if (!dedans[c] || zone[c] || porte[c]) continue;
                for (int d = 0; d < 4; d++) {
                    const int v = voisin(c, d);
                    if (v >= 0 && zone[v]) { bord.append(c); break; }
                }
            }
            for (int c : bord) {
                if (zone[c] || porte[c]) continue;
                QVector<int> derriere; QVector<bool> vu(size, false);
                derriere.append(c); vu[c] = true;
                for (int i = 0; i < derriere.size() && derriere.size() <= plafond; i++) {
                    const int q = derriere[i];
                    for (int d = 0; d < 4; d++) {
                        const int v = voisin(q, d);
                        if (v >= 0 && dedans[v] && !zone[v] && !vu[v]) { vu[v] = true; derriere.append(v); }
                    }
                }
                if (derriere.size() <= plafond) {
                    for (int q : derriere) zone[q] = true;
                    encore = true;
                    continue;
                }
                // ⚠️ PLUS DE PROLONGEMENT COLLINÉAIRE ICI — RETIRÉ le 2026-08-22,
                // le jour même où il avait été ajouté. Il courait tout droit
                // jusqu'au mur suivant pour reproduire une zone du niveau 25 ;
                // relu en images par l'utilisateur, il embarquait le COULOIR DE
                // SORTIE de cinq autres niveaux (2 à droite, 9 à gauche, 14 en
                // haut et en bas, 15 à droite, 25 lui-même à gauche) et faisait
                // FUSIONNER les deux zones du niveau 18. Bilan mesuré sur les 8
                // zones de la vérité terrain : il en gagnait UNE et en perdait
                // cinq ailleurs. Un mécanisme calé sur un seul cas ne survit pas
                // à son deuxième.
                porte[c] = true;
            }
        }

        // ⚠️ MÉNAGE — L'ABSORPTION RAMPE, et c'est un défaut mesuré (niveau 18,
        // repéré à l'œil par l'utilisateur sur la planche contact). Elle avale une
        // poche de 4 cases, puis 4 de plus depuis la zone agrandie, et finit par
        // s'annexer une PIÈCE ENTIÈRE SANS AUCUN EMBUT — 5 cases sur le 18. Le
        // plafond borne chaque bouchée, pas le repas.
        // On retire donc, à la fin, tout bloc ABSORBÉ (hors du cœur) qui ne
        // contient aucun embut et dépasse le plafond à lui seul. Une vraie alcôve
        // reste : elle est petite par définition.
        {
            QVector<bool> vu(size, false);
            for (int c = 0; c < size; c++) {
                if (!zone[c] || coeur[c] || vu[c]) continue;
                QVector<int> bloc, pile; pile.append(c); vu[c] = true;
                bool aUnBut = false;
                while (!pile.isEmpty()) {
                    const int q = pile.takeLast();
                    bloc.append(q);
                    if (estBut(q)) aUnBut = true;
                    for (int d = 0; d < 4; d++) {
                        const int v = voisin(q, d);
                        if (v >= 0 && zone[v] && !coeur[v] && !vu[v]) { vu[v] = true; pile.append(v); }
                    }
                }
                if (!aUnBut && bloc.size() > seuilCulDeSac)
                    for (int q : bloc) zone[q] = false;
            }
        }

        // Les PORTES se relisent sur la zone FINALE : toute case libre qui la
        // borde sans lui appartenir. Les recalculer ici plutôt que de traîner
        // celles de l'absorption évite qu'un resserrage postérieur laisse des
        // portes fantômes, pointant vers des cases désormais hors zone.
        for (int c = 0; c < size; c++) porte[c] = false;
        for (int c = 0; c < size; c++) {
            if (!dedans[c] || zone[c]) continue;
            for (int d = 0; d < 4; d++) {
                const int v = voisin(c, d);
                if (v >= 0 && zone[v]) { porte[c] = true; break; }
            }
        }

        ZoneEmbut z;
        for (int c = 0; c < size; c++) {
            if (zone[c]) z.cases.append(c);
            // ⚠️ Une porte avalée par une absorption plus tardive n'est plus une
            // porte : la zone l'a rejointe par un autre côté.
            else if (porte[c]) z.portes.append(c);
        }
        for (int b = 0; b < nbButs; b++) if (salle[b] == s) z.buts.append(b);

        // FUSION — deux salles de buts peuvent partager le MÊME enclos, et c'est
        // alors une seule zone qui porte les deux. Mesuré : 18, 24 et 26 rendaient
        // la même zone deux fois (le 26 : la salle de 12 buts et le but isolé sont
        // dans la même pièce). ⚠️ Le 25 est le contre-exemple à ne pas casser —
        // ses deux salles ont des enclos DISTINCTS, et l'utilisateur les compte
        // bien pour deux.
        int fusion = -1;
        for (int i = 0; i < zones.size() && fusion < 0; i++)
            for (int c : zones[i].cases) if (zone[c]) { fusion = i; break; }
        if (fusion >= 0) {
            QVector<bool> deja(size, false);
            for (int c : zones[fusion].cases) deja[c] = true;
            for (int c : z.cases) if (!deja[c]) { zones[fusion].cases.append(c); deja[c] = true; }
            std::sort(zones[fusion].cases.begin(), zones[fusion].cases.end());
            zones[fusion].buts += z.buts;
            QVector<int> portes;
            for (int c : zones[fusion].portes + z.portes)
                if (!deja[c] && !portes.contains(c)) portes.append(c);
            std::sort(portes.begin(), portes.end());
            zones[fusion].portes = portes;
        } else {
            zones.append(z);
        }
    }
    return zones;
}

QVector<QVector<int>> Game::precedenceGlobale() const {
    QVector<QVector<int>> requis(nbButs);

    QVector<bool> vu(size, false);
    QVarLengthArray<int, 512> file;
    // Une caisse du départ peut-elle atteindre 'but', 'occupe' étant infranchissable
    // (ni passage de caisse, ni appui du joueur) ? 'occupe' = -1 : aucun obstacle.
    auto atteintUneCaisse = [&](int but, int occupe) -> bool {
        vu.fill(false);
        file.clear();
        file.append(but);
        vu[but] = true;
        for (int t = 0; t < file.size(); t++) {
            const int c = file[t];
            // Le but lui-même ne compte pas comme point de départ : une caisse déjà
            // dessus n'aurait aucun trajet à faire.
            if (c != but && (cases[c] == Level::tcCaisse || cases[c] == Level::tcGoalCaisse))
                return true;
            const int cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int px = cx - directions[d].dx,     py = cy - directions[d].dy;      // caisse avant
                const int ax = cx - 2 * directions[d].dx, ay = cy - 2 * directions[d].dy;  // appui joueur
                if (px < 0 || px >= largeur || py < 0 || py >= hauteur) continue;
                if (ax < 0 || ax >= largeur || ay < 0 || ay >= hauteur) continue;
                const int pp = px + py * largeur, aa = ax + ay * largeur;
                if (cases[pp] == Level::tcMur || cases[aa] == Level::tcMur) continue;
                if (pp == occupe || aa == occupe) continue;
                if (!vu[pp]) { vu[pp] = true; file.append(pp); }
            }
        }
        return false;
    };

    for (int g = 0; g < nbButs; g++) {
        // Un but déjà inatteignable sans le moindre obstacle est un plateau douteux :
        // aucune arête à en tirer (tout but « le condamnerait », ce qui ne dit rien).
        if (!atteintUneCaisse(goals[g], -1)) continue;
        for (int b = 0; b < nbButs; b++) {
            if (b == g) continue;
            if (!atteintUneCaisse(goals[g], goals[b])) requis[b].append(g);
        }
    }
    return requis;
}

// PRÉCÉDENCE PAR ALIGNEMENT (cf. game.h). Contrairement à `precedenceGlobale`
// (reachability PHYSIQUE, aveugle au SENS de poussée), celle-ci reprend le
// critère exact de `caseMorteLoi` — reachable-vers-A (distanceParBut) OU
// aligné avec A (alignementLoi) — appliqué à la case d'un AUTRE but B. Si ni
// l'un ni l'autre, une caisse posée sur B pendant que A est actif serait
// condamnée par la loi.
//
// ⚠️ ARÊTE ASYMÉTRIQUE SEULEMENT (correction 2026-08-19, idée utilisateur — « il
// prend des caisses de la petite salle pour les mettre dans la grande »). Le
// premier jet ajoutait l'arête dès que B n'atteint pas A, SANS vérifier que A
// atteint B en retour — ce qui condamne aussi bien un vrai passage à SENS UNIQUE
// (niveau 6 : (2,5) atteint (1,5), jamais l'inverse) qu'une paire de buts dans
// deux salles simplement DISJOINTES (aucun des deux n'atteint l'autre, ce qui
// n'est PAS une précédence, juste une absence de rapport). Mesuré sur le niveau
// 10 (salle 28 + salle 4, cf. plan.md) : sans ce garde, la petite salle héritait
// d'une dette `attente` de 29-30 (quasi tous les autres buts), la reléguant en
// fin d'ordre alors qu'elle n'a aucune raison d'attendre — corrigé du même coup.
QVector<QVector<int>> Game::precedenceAlignement() const {
    QVector<QVector<int>> requis(nbButs);
    if (maxRegions <= 0) return requis;

    auto atteintDepuis = [&](int idxBut, int cell) {
        const int* dp = distanceParBut.constData() + (qsizetype)idxBut * size * maxRegions;
        for (int r = 0; r < nbRegions[cell]; r++)
            if (dp[cell * maxRegions + r] != -1) return true;
        return false;
    };

    for (int a = 0; a < nbButs; a++) {
        const int ga = goals[a];
        for (int b = 0; b < nbButs; b++) {
            if (b == a) continue;
            const int gb = goals[b];

            if (atteintDepuis(a, gb)) continue;         // B atteint A : rien à en tirer
            if (alignementLoi(gb, a)) continue;         // aligné avec A : sol légitime

            // L'ASYMÉTRIE : A doit pouvoir atteindre B, sinon les deux buts sont
            // simplement SANS RAPPORT (deux salles disjointes) et non-B→A ne prouve
            // rien sur leur ORDRE relatif.
            if (!atteintDepuis(b, ga)) continue;

            requis[a].append(b);   // A ne doit pas être actif tant que B n'est pas posé
        }
    }
    return requis;
}

// MURAGE LOCAL — la « précédence par approches » du §6.2, en exemplaire unique.
// Contrat, relaxations et raison d'être : cf. game.h.
bool Game::butMureLocalement(int idxBut, const QVector<bool>& bloque,
                             bool testeAccesJoueur) const {
    const int G  = goals[idxBut];
    const int gx = G % largeur, gy = G / largeur;

    // Le joueur peut-il joindre 'appui' en marchant, les murs, les buts de 'bloque'
    // et la caisse posée sur 'caisseCase' faisant obstacle ? Flood-fill 4-connexe
    // depuis sa position de DÉPART (cf. game.h pour la justification du départ).
    auto joignable = [&](int appui, int caisseCase) -> bool {
        const int dep = playerPoint.x() + playerPoint.y() * largeur;
        if (dep < 0 || dep >= size) return true;              // pas de joueur : on n'invente rien
        if (cases[dep] == Level::tcMur || bloque[dep] || dep == caisseCase) return true;
        if (dep == appui) return true;
        QVector<bool> vu(size, false);
        QVector<int>  pile;
        vu[dep] = true; pile.append(dep);
        while (!pile.isEmpty()) {
            const int c = pile.takeLast();
            const int cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int nx = cx + directions[d].dx, ny = cy + directions[d].dy;
                if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                const int n = nx + ny * largeur;
                if (vu[n] || cases[n] == Level::tcMur || bloque[n] || n == caisseCase) continue;
                if (n == appui) return true;
                vu[n] = true; pile.append(n);
            }
        }
        return false;
    };

    int viables = 0, libres = 0;
    for (int d = 0; d < NB_DIRECTION; d++) {
        const int cx = gx -     directions[d].dx, cy = gy -     directions[d].dy;  // la caisse
        const int ax = gx - 2 * directions[d].dx, ay = gy - 2 * directions[d].dy;  // l'appui
        if (cx < 0 || cx >= largeur || cy < 0 || cy >= hauteur) continue;
        if (ax < 0 || ax >= largeur || ay < 0 || ay >= hauteur) continue;
        const int cc = cx + cy * largeur, aa = ax + ay * largeur;
        if (cases[cc] == Level::tcMur || cases[aa] == Level::tcMur) continue;
        viables++;
        if (bloque[cc] || bloque[aa]) continue;
        // Le flood-fill ne se paie QUE sur les approches qui ont passé les deux tests
        // gratuits ci-dessus — c'est ce qui garde le coût du ctor Game(Level) à sa
        // place (le §6.2 garde la trace d'un chargement passé à 64 s).
        if (testeAccesJoueur && !joignable(aa, cc)) continue;
        libres++;
    }
    return (viables > 0 && libres == 0);
}

// Ordre de remplissage par PRÉCÉDENCE DE LIVRAISON (§6.2, session du 2026-07-20).
//
// Le fait mesuré : sur la salle du 11, l'ordre décide de tout (28 états contre 1,3 M
// avec un mauvais ordre). Et la contrainte est DÉMONTRABLE, pas devinable : pour poser
// une caisse sur un but il faut la pousser depuis une case voisine, le joueur deux
// cases derrière. Si toutes les approches d'un but passent par un autre but, ce but-là
// doit être rempli AVANT lui. Exemple de la salle : (5,11) n'est atteignable que depuis
// (4,11) — au nord l'appui est un mur, à l'est un mur, au sud (5,12) est déjà posé.
// Donc (5,11) strictement avant (4,11). Prouvé sans jouer.
//
// L'algorithme : glouton avant, avec GARDE ANTI-ÉCHOUAGE. À chaque pas on ne retient
// que les buts qui (a) sont livrables maintenant, et (b) dont la pose ne rend AUCUN but
// restant non-livrable. C'est cette garde qui met (4,11) en dernier toute seule : le
// remplir coupe la seule descente vers les rangées 11-13, donc il échoue tout le monde.
//
// ⚠️ Le tie-break, lui, n'est PAS démontré — c'est la préférence qui départage les
// ordres que la garde autorise, et c'est exactement là que la session du 2026-07-19
// s'est trompée six fois. Celui retenu (CONTIGUITÉ DE RUN, cf. `contiguite` plus bas)
// a été MESURÉ le 2026-07-20 : il fait 27 états sur 191 (bat l'oracle humain, 28) et
// résout le 190, sans perdre aucun niveau réel (le seul coût est le niveau 7 — un bloc
// plein — qui passe de 0,4 s à 7,5 s, très loin des 60 s). Juge : `bench 191 macro`.
// Pose `ordreParPrecedence()` dans `ordreButs`, en gardant le rebours en repli si
// elle ne rend pas une permutation complète (butActif() exige un ordre plein).
// Extrait de calculDistancePoussee pour que setOrdreLookahead() puisse rejouer
// EXACTEMENT la même installation — deux endroits qui recopieraient ce repli
// finiraient par diverger, c'est le motif du §7.
void Game::installeOrdreParPrecedence() {
    const QVector<int> parPrecedence = ordreParPrecedence();
    if (parPrecedence.size() == nbButs) ordreButs = parPrecedence;
}

// RÉGIME `ordre-look` (§6.2, 2026-08-08). Le drapeau vit dans le Game, comme
// `ordreDynamique` : posé par la fabrique sur l'état de départ, il se propage par
// copie. On RECALCULE l'ordre immédiatement — `ordreButs` est consommé par
// butActif() à chaque état, il ne peut pas rester périmé.
// ⚠️ N'agit QUE sur l'ordre. Si un ordre a été injecté par fichier (outil de
// chantier), il a écrasé `ordreButs` APRÈS l'installation ; le rappel ci-dessous le
// reperdrait en silence. D'où le garde : on ne touche à rien si une injection a eu lieu.
void Game::setOrdreLookahead(bool actif) {
    if (ordreLookahead == actif) return;
    ordreLookahead = actif;
    if (!cheminOrdreInjecte(numNiveau).isEmpty() || !qgetenv("ORDRE_HUMAIN").isEmpty()) {
        fprintf(stderr, "[ORDRE-LOOK] ordre INJECTE present — recalcul IGNORE\n");
        return;
    }
    installeOrdreParPrecedence();
}

// RÉGIME `loi` (2026-08-19) — même mécanique que `setOrdreLookahead` ci-dessus
// (drapeau, recalcul immédiat, garde d'injection), mais pour la précédence par
// alignement (cf. game.h : SCOPÉE ici plutôt que fondue dans l'ordre par défaut,
// parce qu'elle casse le niveau 10 une fois généralisée).
//
// ⚠️ APPELLE calculCasesMortesLoi() APRÈS installeOrdreParPrecedence(), et
// `setOrdreLookahead` ci-dessus NE LE FAIT PAS — piège trouvé en mesurant : sans
// ça, `ordreButs` change mais `rangDeBut`/`mortesLoi` restent ceux de l'ordre PAR
// DÉFAUT (calculés une fois dans le ctor `Game(Level)`, jamais revus). `caseMorteLoi`
// jugerait alors avec le mauvais rang — silencieusement, puisque `butActif()` lit
// le NOUVEL `ordreButs` pendant que la loi juge sur l'ANCIEN `rangDeBut`. Mesuré :
// sans cet appel, `bench 6 loi` retombe à `AUCUNE` malgré l'ordre corrigé.
void Game::setOrdreAlignement(bool actif) {
    if (ordreAlignement == actif) return;
    ordreAlignement = actif;
    if (!cheminOrdreInjecte(numNiveau).isEmpty() || !qgetenv("ORDRE_HUMAIN").isEmpty()) {
        fprintf(stderr, "[ORDRE-ALIGN] ordre INJECTE present — recalcul IGNORE\n");
        return;
    }
    installeOrdreParPrecedence();
    calculCasesMortesLoi();
}

QVector<int> Game::ordreParPrecedence() const {
    QVector<bool> bloque(size, false);
    QVector<bool> pose(nbButs, false);
    QVector<int>  ordre;

    // cell -> indice de but (-1 si la case n'est pas un but), pour raisonner sur
    // les RUNS de buts alignés (la contiguité du tie-break corridor).
    QVector<int> butDe(size, -1);
    for (int b = 0; b < nbButs; b++) butDe[goals[b]] = b;

    auto libre = [&](int x, int y) -> bool {
        if (x < 0 || x >= largeur || y < 0 || y >= hauteur) return false;
        const int c = x + y * largeur;
        return cases[c] != Level::tcMur && !bloque[c];
    };

    // "Dans un coin, ne gêne pas le perso" (mots de l'utilisateur) = degré du but
    // dans le graphe des cases encore libres : peu de voisins libres = un coin, où
    // se poser ne coupe aucun passage. Grand degré = un carrefour, à garder libre.
    auto degre = [&](int b) -> int {
        const int gx = goals[b] % largeur, gy = goals[b] / largeur;
        int n = 0;
        for (int d = 0; d < NB_DIRECTION; d++)
            if (libre(gx + directions[d].dx, gy + directions[d].dy)) n++;
        return n;
    };

    // PRÉCÉDENCE GLOBALE (§6.2 famille B) : combien de buts DOIVENT encore être posés
    // avant b ? 0 = b respecte toutes ses arêtes, il est posable dès maintenant.
    //
    // Utilisée en clé de TÊTE du tie-break, et non comme filtre dur. Deux raisons :
    //  - **Identité par construction** sur les niveaux dont l'ordre respecte déjà
    //    toutes les arêtes (mesuré le 2026-07-30 : 28 niveaux sur 33, dont les 14
    //    résolus, 190 et 191). Chez eux, le but élu à chaque rang a `attente == 0`,
    //    donc il reste le minimum du comparateur : ordre inchangé, canari intact.
    //  - **Dégradation gracieuse** : si aucun but sûr ne respecte ses arêtes, on en
    //    pose un quand même (le moins en retard) au lieu de bloquer, et c'est le
    //    backtracking qui tranche. Un ajout dont le cas d'échec n'est pas identique à
    //    l'existant serait un remplacement, pas un ajout (leçon du 2026-07-29).
    // ⚠️ La variante FILTRE DUR (retirer des `surs` tout but en dette, plus des couches
    // de repli équivalentes) a été codée et mesurée le 2026-07-30 : **cartes de rangs
    // identiques sur les 35 niveaux**, donc strictement INERTE. Retirée. Ne pas la
    // reproposer sans un cas qui la distingue.
    // PRÉCÉDENCE PAR ALIGNEMENT (2026-08-19, cf. game.h), fusionnée UNIQUEMENT si
    // `ordreAlignement` est armé. ⚠️ Mesuré en fusion INCONDITIONNELLE (donc dans
    // l'ordre PAR DÉFAUT, utilisé par TOUS les régimes) : corrige bien le niveau 6,
    // mais casse SÉVÈREMENT le niveau 10 (32 buts — 2/32 caisses posées en 227 000
    // états quand la référence en pose 32 en 250 000) et fait apparaître un murage
    // LOCAL sur 6 niveaux auparavant propres (2, 8, 9, 12, 21, 32). L'ajout mord
    // bien plus large que le seul motif visé — d'où le drapeau, armé SEULEMENT par
    // le régime d'essai `loi` (solveur.cpp), jamais l'ordre par défaut.
    const QVector<QVector<int>> precGlobale = precedenceGlobale();
    QVector<QVector<int>> requis = precGlobale;
    if (ordreAlignement) {
        const QVector<QVector<int>> align = precedenceAlignement();
        for (int b = 0; b < nbButs; b++)
            for (int g : align[b])
                if (!requis[b].contains(g)) requis[b].append(g);
    }
    auto attente = [&](int b) -> int {
        int n = 0;
        for (int g : requis[b]) if (!pose[g]) n++;
        return n;
    };

    // CONTIGUITÉ DE RUN (le tie-break retenu). Diagnostic (bench 191 macro,
    // oracle vs calculé) : l'ordre calculé ne diverge de l'ordre humain (28 états)
    // qu'à DEUX endroits, tous deux des mélanges LOCAUX dans une file droite de
    // buts alignés (la rangée y=13, la colonne x=1). Le principe qui recolle les
    // deux : dans un run droit de buts, on REMPLIT EN CONTINU — on étend un segment
    // déjà posé (ou on part d'un cul-de-sac mural), on ne saute pas une case.
    //
    // Pour le but b, on regarde les 4 directions. Une direction "étend un run" si le
    // voisin OPPOSÉ (g - d) est un but ENCORE VIDE (il reste du run à remplir de ce
    // côté) et le voisin (g + d) est soit un but DÉJÀ POSÉ (on prolonge un segment
    // rempli), soit un mur (on démarre à un cul-de-sac). On sépare les deux : étendre
    // un segment rempli prime sur démarrer à un mur (sinon les coins murés — (1,13) —
    // gagnent sur la continuation d'un balayage déjà lancé — (4,13)).
    auto contiguite = [&](int b) -> QPair<int,int> {   // {prolonge-rempli, part-du-mur}
        const int gx = goals[b] % largeur, gy = goals[b] / largeur;
        int prolonge = 0, mur = 0;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int ox = gx - directions[d].dx, oy = gy - directions[d].dy;   // opposé
            if (ox < 0 || ox >= largeur || oy < 0 || oy >= hauteur) continue;
            const int opp = butDe[ox + oy * largeur];
            if (opp < 0 || pose[opp]) continue;         // opposé pas un but vide -> pas un run à étendre
            const int nx = gx + directions[d].dx, ny = gy + directions[d].dy;   // voisin
            if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) { mur++; continue; }
            const int n = nx + ny * largeur;
            if (cases[n] == Level::tcMur) mur++;
            else { const int nb = butDe[n]; if (nb >= 0 && pose[nb]) prolonge++; }
        }
        return {prolonge, mur};
    };

    // LIVRABILITÉ DURCIE (§6.2, salle à deux bouches du 12). La garde ci-dessous ne
    // regarde que la reachability (dist != -1) — elle laisse filer la précédence
    // (15,y)<(13,y) parce que la « danse de coin » garde (15,y) livrable après avoir
    // bloqué (13,y). Mais cette livraison de secours est plus LONGUE. Signal gratuit,
    // déjà calculé par la garde : si poser b ALLONGE la livraison d'un autre but h
    // (apres > dist, au lieu de simplement rester ≥ 0), c'est que b est un appui de h
    // → h doit passer avant b. On PÉNALISE b d'autant de buts qu'il détourne, et le
    // tie-break préfère poser d'abord ceux qui ne détournent personne.
    //   LIVR_DURE=0 (défaut) coupé ; 1 = pénalité en tête ; 2 = après la contiguité.
    const int livrDure = qEnvironmentVariableIntValue("LIVR_DURE");

    // BACKTRACKING SUR LA GARDE (2026-07-29, §6.2 famille A). Le glouton pur se
    // peignait dans un coin : quand plus AUCUN but n'était « sûr », l'ancien code
    // RELÂCHAIT la garde (`surs = candidats`) et posait quand même — condamnant la
    // partie. Mesuré : 7 relâchements sur le 27, 6 sur le 22, 6 sur le 32, 2 sur le
    // 25, et **ZÉRO sur les 14 résolus, 190 et 191**. Le relâchement est donc le
    // symptôme exact des ordres murés, pas un incident bénin.
    //
    // On empile désormais les candidats sûrs PAR ORDRE DE PRÉFÉRENCE (le tie-break
    // ci-dessous) et on RECULE au lieu de forcer. Propriété qui rend l'ajout sûr :
    // sur un niveau où la garde ne se relâchait jamais, la pile ne recule jamais et
    // le premier candidat est toujours retenu → **ordre identique au glouton, canari
    // intact PAR CONSTRUCTION**. C'est la même correction que
    // `macroVersButBacktrack` (§6.3) : mémoriser les forks au lieu de les oublier.
    //
    // ⚠️ Ce n'est PAS la tentative n°5 du 2026-07-19 (« backtracking : rend le MÊME
    // ordre que le greedy »). Celle-là portait sur le 191, où la garde ne se relâche
    // jamais — la recherche n'avait donc rien à explorer. Ici on ne recule QUE sur un
    // échec avéré du modèle.
    struct Etage { QVector<int> choix; int essai = 0; };
    QVector<Etage> pile;
    QVector<int> meilleurOrdre;              // filet : le plus long ordre atteint
    // Budget BALAYÉ le 2026-07-29 (méthode CORRAL_BUDGET : on ne fige pas une constante
    // sans balayage). 50 = trop court, le 32 reste muré ; 200 = le 32 est corrigé ;
    // 1000 et 5600 n'apportent RIEN de plus et coûtent cher — le chargement du 22 passe
    // de 0,23 s à 0,91 s puis 4,67 s (ce calcul tourne dans le ctor Game(Level), donc à
    // chaque ouverture de niveau dans l'app). Figé à 200.
    // ⚠️ Un TEST EN DUR `numNiveau == 13 ? 100000 : 200` a vécu ici du 2026-07-30 au
    // 2026-07-31 — RETIRÉ. Ce qu'il avait mesuré reste un repère utile : le 13 est muré
    // au rang 14 à tout budget de 200 à 20 000 et devient SAIN à 100 000, donc un ordre
    // sain EXISTE et son absence est un coût de RECHERCHE, pas une limite théorique.
    // Mais il coûtait 64 s dans le ctor Game(Level) — donc à CHAQUE ouverture de niveau
    // dans l'app — et un numéro de niveau en dur dans le solveur est pire que le coût :
    // le canari ne dit rien d'un niveau traité à part, et la mesure d'à côté non plus.
    // ⚠️ ESCALADE DE BUDGET essayée puis RETIRÉE le 2026-07-31 (relancer une fois à
    // 100 000 quand la pile se vide budget à 0, c.-à-d. tronqué et non épuisé). Mesurée :
    // le 13 devient sain mais en **65 s**, le 18 n'escalade même pas (son espace est
    // RÉELLEMENT épuisé à 200 — aucun ordre sain n'existe dans ce modèle, aucun budget
    // n'y changera rien), et le **22 n'a pas fini en 9 minutes**. On échangeait un numéro
    // de niveau en dur contre un temps de chargement NON BORNÉ, dans le ctor Game(Level)
    // donc à chaque ouverture de niveau. Ne pas la reproposer sans traiter d'abord le
    // coût de `distanceLivraison`, rappelée pour chaque candidat de chaque rang.
    int budget = 200;

    while (ordre.size() < nbButs) {
        // L'étage du rang courant existe déjà (on y est revenu par backtracking) :
        // l'état pose/bloque a été restauré à l'identique, donc la liste de choix
        // aussi — on la RÉUTILISE, sinon on perdrait l'index d'essai et on
        // reboucterait indéfiniment sur le même candidat.
        if (pile.size() == ordre.size() + 1) {
            Etage& e = pile[pile.size() - 1];
            if (e.essai < e.choix.size() && budget > 0) {
                budget--;
                const int b = e.choix[e.essai];
                pose[b] = true;
                bloque[goals[b]] = true;
                ordre.append(b);
                if (ordre.size() > meilleurOrdre.size()) meilleurOrdre = ordre;
            } else {
                pile.removeLast();                       // cet étage est épuisé
                if (ordre.isEmpty() || pile.isEmpty()) break;   // plus rien à défaire
                const int d = ordre.takeLast();          // on défait le choix précédent
                pose[d] = false;
                bloque[goals[d]] = false;
                pile[pile.size() - 1].essai++;           // …et on essaie le suivant
            }
            continue;
        }

        const QVector<int> dist = distanceLivraison(bloque);

        // (a) livrables maintenant
        QVector<int> candidats;
        for (int b = 0; b < nbButs; b++)
            if (!pose[b] && dist[goals[b]] != -1) candidats.append(b);

        // (b) garde anti-échouage : poser ce but laisse-t-il tous les autres livrables ?
        //     + pénalité de détour (livrabilité durcie, cf. entête de la boucle).
        QVector<int> surs;
        QVector<int> penalite(nbButs, 0);
        for (int b : candidats) {
            bloque[goals[b]] = true;
            const QVector<int> apres = distanceLivraison(bloque);
            bool ok = true;
            int  pen = 0;
            for (int h = 0; h < nbButs; h++) {
                if (pose[h] || h == b) continue;
                if (apres[goals[h]] == -1)               ok = false;   // rendu inaccessible
                else if (apres[goals[h]] > dist[goals[h]]) pen++;      // livrable mais DÉTOURNÉ
                // ⚠️ Test INDÉPENDANT des deux précédents, jamais chaîné en `else` :
                // un but peut être à la fois détourné et muré, et c'est même le cas
                // courant — le détour est le symptôme, le murage est la preuve.
                // GARDE DE MURAGE LOCAL (2026-08-20). `distanceLivraison` est un
                // modèle de REACHABILITY : il dit « une caisse peut encore arriver
                // sur ce but », jamais « le joueur pourra encore la pousser dessus ».
                // Les deux divergent dès qu'un but sert d'APPUI à la dernière
                // manœuvre d'un autre — mesuré sur le 21 en régime `loi` : poser
                // (13,10) au rang 5 laisse (11,10) parfaitement « livrable » et le
                // condamne pourtant, ses deux dernières approches passant par
                // (13,10) et (11,12). Le plateau exporté ce jour-là est mort à 7/13,
                // avec deux buts qu'aucune poussée ne peut plus atteindre.
                // C'est la même espèce que le verrou du 10 ((17,2), 2026-08-19).
                if (butMureLocalement(h, bloque)) ok = false;
            }

            // LIVR_DURE=3 : reachability JOUEUR ancrée à sa vraie position (pas le
            // modèle relâché de distanceLivraison). Poser b (obstacle) déconnecte-t-il
            // l'appui d'un but restant de la zone où le perso peut MARCHER ? Un couloir
            // rempli qui mure le perso hors d'une bouche est puni lourdement. (Ne voit
            // que les murs + buts posés, PAS les caisses d'acheminement : borne haute.)
            if (livrDure == 3) {
                QVector<bool> vu(size, false);
                QList<int> f;
                const int dep = playerPoint.x() + playerPoint.y() * largeur;
                if (cases[dep] != Level::tcMur && !bloque[dep]) { f.append(dep); vu[dep] = true; }
                while (!f.isEmpty()) {
                    const int c = f.takeFirst();
                    const int cx = c % largeur, cy = c / largeur;
                    for (int d = 0; d < NB_DIRECTION; d++) {
                        const int nx = cx + directions[d].dx, ny = cy + directions[d].dy;
                        if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                        const int n = nx + ny * largeur;
                        if (vu[n] || cases[n] == Level::tcMur || bloque[n]) continue;
                        vu[n] = true; f.append(n);
                    }
                }
                for (int h = 0; h < nbButs; h++) {
                    if (pose[h] || h == b) continue;
                    const int gx = goals[h] % largeur, gy = goals[h] / largeur;
                    bool joignable = false;
                    for (int d = 0; d < NB_DIRECTION && !joignable; d++) {
                        const int ax = gx + directions[d].dx, ay = gy + directions[d].dy;
                        if (ax < 0 || ax >= largeur || ay < 0 || ay >= hauteur) continue;
                        if (vu[ax + ay * largeur]) joignable = true;
                    }
                    if (!joignable) pen += 1000;   // appui déconnecté du joueur
                }
            }

            penalite[b] = pen;
            bloque[goals[b]] = false;
            if (ok) surs.append(b);
        }
        const bool look0 = ordre.isEmpty() && ordreLookahead;
        QVector<int> options(nbButs, -1);
        if (look0) {
            for (int b : surs) {
                pose[b] = true; bloque[goals[b]] = true;
                const QVector<int> d2 = distanceLivraison(bloque);
                int n2 = 0;
                for (int c = 0; c < nbButs; c++) {
                    if (pose[c] || d2[goals[c]] == -1) continue;
                    bloque[goals[c]] = true;
                    const QVector<int> d3 = distanceLivraison(bloque);
                    bool ok2 = true;
                    for (int h = 0; h < nbButs && ok2; h++)
                        if (!pose[h] && h != c && d3[goals[h]] == -1) ok2 = false;
                    bloque[goals[c]] = false;
                    if (ok2) n2++;
                }
                pose[b] = false; bloque[goals[b]] = false;
                options[b] = n2;
            }
        }

        // TIE-BREAK = CONTIGUITÉ DE RUN (mesuré, cf. l'entête). Parmi les buts sûrs :
        //  1. prolonger un segment déjà posé (garder les runs droits contigus) ;
        //  2. sinon partir d'un cul-de-sac mural (amorcer un run) ;
        //  3. sinon le plus encoigné (peu de voisins libres = ne coupe aucun passage) ;
        //  4. sinon le plus proche de l'entrée.
        // On TRIE désormais au lieu de ne garder que le meilleur : le backtracking a
        // besoin des suivants. Le premier élément reste EXACTEMENT celui que l'ancien
        // code élisait (comparateur inchangé), d'où l'identité sur les niveaux qui ne
        // reculent jamais.
        auto mieuxQue = [&](int a, int c) -> bool {
            const int da = dist[goals[a]], dc = dist[goals[c]];
            const int ga = degre(a),       gc = degre(c);
            const auto ca = contiguite(a), cc = contiguite(c);
            const int pa = penalite[a],    pc = penalite[c];
            const int wa = attente(a),     wc = attente(c);
            if (wa != wc)                  return (wa < wc);   // précédence GLOBALE : une preuve, prime tout
            if ((livrDure == 1 || livrDure == 3) && pa != pc) return (pa < pc);  // durci : ne pas stranguler un appui
            if (ca.first  != cc.first)     return (ca.first  > cc.first);
            if (ca.second != cc.second)    return (ca.second > cc.second);

            if (livrDure == 2 && pa != pc) return (pa < pc);
            if (ga != gc)                  return (ga < gc);
            if (da != dc)                  return (da < dc);
            return a < c;                  // total, donc tri DÉTERMINISTE
        };
        std::stable_sort(surs.begin(), surs.end(), mieuxQue);

        // LOOKAHEAD DE RANG 0, CONFINÉ À LA SALLE DE TÊTE (2026-08-08). Appliqué
        // APRÈS le tri et seulement entre buts d'une MÊME salle : la règle existante
        // garde donc la main sur QUELLE salle commence — c'est le correctif
        // multi-salles du 2026-08-01 (×7,5 sur le 10), que la clé écrasait en faisant
        // passer la satellite de 4 buts en dernier. Ici elle ne décide plus que du
        // POINT DE DÉPART à l'intérieur de la salle élue.
        // ⚠️ Réordonner un sous-ensemble APRÈS le tri, plutôt que d'ajouter une clé au
        // comparateur : une clé qui ne s'applique qu'entre certains couples n'est pas
        // un ordre strict faible, et std::sort part alors en comportement indéfini.
        if (look0 && !surs.isEmpty()) {
            const QVector<int> salle = sallesDeButs();
            const int cible = salle[surs[0]];
            QVector<int> memeSalle, positions;
            for (int i = 0; i < surs.size(); i++)
                if (salle[surs[i]] == cible) { memeSalle.append(surs[i]); positions.append(i); }
            // Le bloc est trié par les clés EXISTANTES d'abord, `options` ne venant
            // qu'APRÈS la contiguïté — sinon on écrase `mur`, et le 190 repart de
            // (4,13) `mur0` au lieu de (5,13) `mur2`. `options` ne tranche donc que
            // les égalités que rien d'autre ne départage : c'est le cas des trois
            // candidats de tête du 12, tous `att0 prol0 mur2 deg2`.
            std::stable_sort(memeSalle.begin(), memeSalle.end(), [&](int a, int c) {
                const int wa = attente(a), wc = attente(c);
                if (wa != wc) return wa < wc;
                const auto ca = contiguite(a), cc = contiguite(c);
                if (ca.first  != cc.first)  return ca.first  > cc.first;
                if (ca.second != cc.second) return ca.second > cc.second;
                if (options[a] != options[c]) return options[a] > options[c];
                const int ga = degre(a), gc = degre(c);
                if (ga != gc) return ga < gc;
                const int da = dist[goals[a]], dc = dist[goals[c]];
                if (da != dc) return da < dc;
                return a < c;
            });
            for (int k = 0; k < positions.size(); k++) surs[positions[k]] = memeSalle[k];
        }
        // `TRACE_ORDRE=1` : les candidats de CHAQUE rang, triés, avec toutes leurs
        // clés. C'est le « diagnostic manquant » que le journal réclame depuis le
        // 2026-07-31 (« les candidats et leurs clés de tri au rang décisif ») — sans
        // lui on ne voit pas SUR QUELLE clé l'ordre calculé diverge de l'ordre humain.
        // ⚠️ Interrupteur d'ENVIRONNEMENT dans le solveur, ce que le §7 proscrit — la
        // seule raison pour laquelle il est tolérable ici est qu'il **n'IMPRIME**, il
        // ne coupe ni n'ajoute aucun comportement : l'app ne peut pas diverger du
        // bench. À retirer avec le chantier de l'ordre.
        if (qEnvironmentVariableIsSet("TRACE_ORDRE")) {
            fprintf(stderr, "[rang %2d] candidats sûrs (%d) :", (int)ordre.size(), (int)surs.size());
            for (int b : surs) {
                const auto c = contiguite(b);
                fprintf(stderr, "  (%d,%d)[att%d prol%d mur%d deg%d d%d]",
                        goals[b] % largeur, goals[b] / largeur,
                        attente(b), c.first, c.second, degre(b), dist[goals[b]]);
                if (options[b] >= 0) fprintf(stderr, "{opt%d}", options[b]);
            }
            fprintf(stderr, "\n");
        }

        // ⚠️ `surs` VIDE = le modèle vient de constater que tout choix condamne un but.
        // L'ancien code posait quand même (`surs = candidats`) ; on empile une liste
        // vide, ce qui déclenche le retour arrière au tour suivant. Le filet
        // `meilleurOrdre` garantit qu'on rendra malgré tout une permutation complète.
        pile.append(Etage{surs, 0});
    }

    // ⚠️ BACKTRACKING ÉPUISÉ (budget, ou aucun ordre sain n'existe dans ce modèle).
    // Le plus long préfixe atteint (`meilleurOrdre`) est un chemin d'EXPLORATION : le
    // reprendre donnait des ordres PIRES que le glouton (mesuré le 2026-07-29 — le 27
    // passait de sain à muré au rang 18, le 25 de muré au rang 10 à muré au rang 1).
    // On refait donc exactement l'ANCIEN glouton relâché. Le backtracking est un
    // BONUS : il ne peut qu'améliorer, jamais dégrader.
    if (ordre.size() < nbButs) {
        ordre.clear();
        pose.fill(false);
        bloque.fill(false);
        for (int step = 0; step < nbButs; step++) {
            const QVector<int> dist = distanceLivraison(bloque);
            QVector<int> candidats;
            for (int b = 0; b < nbButs; b++)
                if (!pose[b] && dist[goals[b]] != -1) candidats.append(b);
            QVector<int> surs;
            QVector<int> penalite(nbButs, 0);
            QVector<int> mures(nbButs, 0);
            for (int b : candidats) {
                bloque[goals[b]] = true;
                const QVector<int> apres = distanceLivraison(bloque);
                bool ok = true;
                int  pen = 0;
                for (int h = 0; h < nbButs; h++) {
                    if (pose[h] || h == b) continue;
                    if (apres[goals[h]] == -1)                 ok = false;
                    else if (apres[goals[h]] > dist[goals[h]]) pen++;
                }
                penalite[b] = pen;
                // MURAGES CAUSÉS PAR CE CHOIX (2026-08-20). Le repli tourne quand
                // AUCUN ordre sain n'existe — il ne peut donc pas exiger zéro murage,
                // mais rien ne l'oblige à les ignorer. On les COMPTE, et le tie-break
                // ci-dessous préfère celui qui en cause le moins.
                // ⚠️ Pourquoi ce n'est pas un filtre dur ici, contrairement à la
                // recherche gardée au-dessus : un filtre dur ne rendrait rien du tout
                // (mesuré sur le 18 — la pile se vide jusqu'au rang 0), et `butActif()`
                // exige une permutation COMPLÈTE. Compter au lieu d'exclure, c'est la
                // même dégradation gracieuse que `precedenceGlobale`, utilisée en clé
                // de tie-break et jamais en filtre (cf. l'entête de cette fonction).
                for (int h = 0; h < nbButs; h++) {
                    if (pose[h] || h == b) continue;
                    if (butMureLocalement(h, bloque)) mures[b]++;
                }
                bloque[goals[b]] = false;
                if (ok) surs.append(b);
            }
            if (surs.isEmpty()) surs = candidats;   // le relâchement d'origine
            if (surs.isEmpty()) break;
            int choisi = surs.first();
            for (int b : surs) {
                const int da = dist[goals[b]], dc = dist[goals[choisi]];
                const int ga = degre(b),       gc = degre(choisi);
                const auto ca = contiguite(b), cc = contiguite(choisi);
                const int pa = penalite[b],    pc = penalite[choisi];
                const int wa = attente(b),     wc = attente(choisi);
                const int ma = mures[b],       mc = mures[choisi];
                bool mieux;
                // Le murage est une PREUVE d'impossibilité (le but perd sa dernière
                // approche), là où `attente` n'est qu'une précédence de livraison :
                // il prime donc, y compris sur la précédence globale.
                if (ma != mc)                       mieux = (ma < mc);
                else if (wa != wc)                  mieux = (wa < wc);   // précédence GLOBALE
                else if ((livrDure == 1 || livrDure == 3) && pa != pc) mieux = (pa < pc);
                else if (ca.first  != cc.first)     mieux = (ca.first  > cc.first);
                else if (ca.second != cc.second)    mieux = (ca.second > cc.second);
                else if (livrDure == 2 && pa != pc) mieux = (pa < pc);
                else if (ga != gc)                  mieux = (ga < gc);
                else                                mieux = (da < dc);
                if (mieux) choisi = b;
            }
            pose[choisi] = true;
            bloque[goals[choisi]] = true;
            ordre.append(choisi);
        }
    }

    // `butActif()` exige une PERMUTATION complète : les buts jamais livrables (îlots,
    // niveaux dégénérés) ferment la liste.
    QVector<bool> dedans(nbButs, false);
    for (int b : ordre) dedans[b] = true;
    for (int b = 0; b < nbButs; b++) if (!dedans[b]) ordre.append(b);

    // POST-PASSE : TRI TOPOLOGIQUE STABLE sur les arêtes de précédence globale.
    //
    // Pourquoi elle est nécessaire, mesuré sur le 27 : ses buts (2,1)/(3,1)/(4,1) ne
    // sont JAMAIS livrables selon `distanceLivraison` (modèle avant, joueur-aware), le
    // glouton ne les choisit donc jamais et la boucle ci-dessus les colle en FIN de
    // liste — aux rangs 17-19, alors que (6,2), leur passage obligé, est posé au rang
    // 16. Leurs rangs n'étaient pas un choix, c'était un résidu. C'est le défaut nommé
    // au §6.2 : « l'ordre remplit de bas en haut ; il fallait l'inverse. »
    //
    // Le tri est STABLE au sens fort : on émet toujours le PREMIER but de l'ordre
    // courant dont tous les prédécesseurs obligatoires sont déjà émis. Donc si l'ordre
    // respecte déjà toutes ses arêtes, chaque but est prêt à son tour et la séquence
    // ressort INCHANGÉE — l'identité, et donc le canari, sont préservés par
    // construction sur les niveaux sains (28 sur 33 le 2026-07-30).
    //
    // ⚠️ Le tri porte AUSSI le groupement SALLE PAR SALLE (§6.2, 2026-08-01), et il
    // le porte ici plutôt qu'en post-passe séparée pour une raison de correction :
    // remonter en bloc les buts d'une salle APRÈS coup casserait les arêtes de
    // précédence que ce tri vient d'établir. En préférant, parmi les buts PRÊTS,
    // celui de la salle en cours, on obtient le groupement maximal **compatible**
    // avec les précédences — jamais au prix d'une violation.
    //
    // Pourquoi grouper : mesuré sur le 10 (28 buts + une satellite de 4), l'ordre
    // entrelace la satellite aux rangs 0/14/29/31, donc dès la première pose le but
    // actif part dans l'autre salle et plus AUCUNE macro n'est générée — 886 états
    // sur 1 658 sans macro dans la partie mesurée. Ce n'est pas « l'humain préfère
    // ne pas entrelacer », c'est « entrelacer rend la macro indisponible ».
    // L'ordre ENTRE salles n'est pas gravé (trois ordres inter-salles mesurés, tous
    // gagnants à +3 % près) : il tombe de la stabilité, la salle rencontrée en
    // premier passe en premier.
    {
        QVector<bool> emis(nbButs, false);
        const QVector<int> salle = sallesDeButs();
        int salleCourante = -1;
        QVector<int>  trie;
        trie.reserve(nbButs);
        auto pret = [&](int b) {
            for (int g : requis[b]) if (!emis[g]) return false;
            return true;
        };
        // `requis` mêle DEUX choses de statut inégal, et la distinction ne comptait
        // pas tant que rien ne les opposait : `precedenceGlobale` est une PREUVE
        // (sans ces buts, plus aucune caisse n'atteint celui-ci), `precedenceAlignement`
        // se décrit elle-même comme « un indice fort, pas une preuve » (game.h).
        // Quand les deux se contredisent — sur le 21 en `loi`, respecter l'indice
        // MURE un but, ce qui est une preuve d'impossibilité — c'est la preuve qui
        // doit gagner. D'où cette seconde lecture, qui ne retient que le global.
        auto pretPreuve = [&](int b) {
            for (int g : precGlobale[b]) if (!emis[g]) return false;
            return true;
        };
        // GARDE DE MURAGE LOCAL, SECONDE MOITIÉ (2026-08-20). La même garde existe
        // dans le glouton ci-dessus — et elle y est INERTE : mesuré le jour même,
        // cartes de rangs identiques sur les 35 niveaux, dans les deux modes. C'est
        // ICI que l'ordre se décide vraiment dès qu'il y a des arêtes de précédence,
        // parce que ce tri REORDONNE ce que le glouton avait choisi. Sur le 21 en
        // régime `loi`, le glouton rendait déjà l'ordre sain — le tri le défaisait
        // pour satisfaire les arêtes d'alignement, et remettait (13,10) au rang 5.
        // ⚠️ C'est un tri STABLE : il ne peut que CHOISIR PARMI LES PRÊTS, jamais
        // violer une arête. On ne fait donc que départager les prêts, exactement
        // comme le groupement par salle juste au-dessus.
        QVector<bool> bloqueTri(size, false);
        auto mureraitQuelquun = [&](int b) {
            bloqueTri[goals[b]] = true;
            bool m = false;
            for (int h = 0; h < nbButs && !m; h++)
                if (!emis[h] && h != b && butMureLocalement(h, bloqueTri)) m = true;
            bloqueTri[goals[b]] = false;
            return m;
        };
        // RECHERCHE AVEC RETOUR ARRIÈRE (2026-08-20), et non plus un glouton.
        // Le glouton myope se peint dans un coin, exactement comme celui du tri par
        // précédence avant le 2026-07-29 : mesuré sur le 21 en régime `loi`, éviter le
        // murage à chaque pas déplace simplement le murage — (11,10) sauvé au rang 8
        // condamne (13,10) au rang 9, parce qu'au rang 5 on avait déjà pris (13,9)
        // alors que rien ne murait ENCORE. Un ordre sain existait pourtant.
        // C'est la même correction qu'ailleurs dans ce fichier (`macroVersButBacktrack`,
        // la pile de `ordreParPrecedence`) : mémoriser les forks au lieu de les oublier.
        //
        // ⚠️ PROPRIÉTÉ QUI REND L'AJOUT SÛR — la pile ne recule JAMAIS sur un niveau
        // dont l'ordre était déjà sain : le premier candidat de chaque étage y est
        // exactement celui que l'ancien glouton élisait (mêmes passes, même parcours
        // de `ordre`), et il mène au bout sans échec. Ordre identique PAR CONSTRUCTION,
        // canari compris. On ne recule que sur un échec avéré.
        struct EtageTri { QVector<int> choix; int essai = 0; int salleAvant = -1; };
        QVector<EtageTri> pileTri;
        // Budget : le tri est O(nbButs) de profondeur et chaque étage coûte
        // O(nbButs² × 4). Ce code tourne dans le ctor Game(Level), donc à CHAQUE
        // ouverture de niveau dans l'app — le §6.2 garde la trace d'une escalade de
        // budget qui avait porté un chargement à 64 s. 500 tient largement les 35
        // niveaux (mesuré : aucun ne recule plus de quelques dizaines de fois).
        int budgetTri = 5000;
        bool triSain = false;

        // ── MÉMOÏSATION DES SOUS-ENSEMBLES ÉCHOUÉS (2026-08-23) ──────────────
        // LE BUDGET CI-DESSUS N'ÉTAIT PAS TROP PETIT : la recherche était
        // exponentielle pour rien. Tout ce dont dépend la suite — `pret`,
        // `pretPreuve`, `mureraitQuelquun`, donc `butMureLocalement` — est
        // fonction du seul ENSEMBLE des buts déjà émis, jamais de l'ORDRE dans
        // lequel on les a posés (position de départ du joueur fixe, géométrie
        // fixe). C'est le constat qui fonde `mesures/ordredp` (plan.md §6.0,
        // 2026-08-21) ; appliqué ICI, il ne demande pas un solveur séparé mais
        // une table : un sous-ensemble dont on a PROUVÉ qu'aucune suite ne mène
        // au bout ne se ré-explore pas, quel que soit le chemin qui y ramène.
        // Sans elle, un même sous-ensemble était redescendu k! fois et le budget
        // partait là-dedans — le niveau 200 (15 buts) saturait 4 000 reculs et
        // rendait un ordre MURÉ sur ses deux derniers buts.
        //
        // ⚠️ LA CLÉ PORTE AUSSI `salleCourante`, et l'oublier serait un vrai bug.
        // Deux chemins menant au même sous-ensemble peuvent laisser le joueur
        // dans des salles différentes, et les passes 1/2 ci-dessus classent les
        // candidats d'après elle : les deux états ne sont donc PAS équivalents.
        // Avec la salle, ils le sont rigoureusement.
        //
        // ⚠️ ON NE MÉMOÏSE QUE L'ÉPUISEMENT RÉEL des candidats d'un étage, jamais
        // un abandon par budget : marquer « échoué » un sous-ensemble qu'on a
        // seulement cessé d'explorer ferait rater un ordre sain, et le murage
        // reviendrait sans que rien ne le signale. C'est la différence entre
        // UNSAT et « budget épuisé » que le §6.0 réclame déjà de l'outil `ordredp`.
        //
        // Le budget reste, en garde-fou dur : ce code tourne dans le ctor
        // Game(Level), donc à chaque ouverture de niveau dans l'app (§6.2 garde la
        // trace d'une escalade qui avait porté un chargement à 64 s).
        QSet<quint64> echecsTri;
        quint64 masqueTri = 0;
        // 48 bits de masque + la salle au-dessus. Le corpus plafonne à 32 buts
        // (niveau 10) ; au-delà de 48 on retombe sur le budget seul, sans mémoire —
        // dégradation gracieuse, jamais un résultat faux.
        const bool memoTri = (nbButs <= 48);
        auto cleTri = [](quint64 masque, int s) {
            return masque | ((quint64)(s + 1) << 48);
        };

        while (true) {
            if (trie.size() == nbButs) { triSain = true; break; }

            if (pileTri.size() == trie.size() + 1) {
                EtageTri& e = pileTri[pileTri.size() - 1];
                if (e.essai < e.choix.size() && budgetTri > 0) {
                    const int b = e.choix[e.essai];
                    // Déjà prouvé stérile par un autre chemin : on n'y redescend
                    // pas, et surtout on ne paie pas de budget pour ça.
                    if (memoTri && echecsTri.contains(cleTri(masqueTri | (1ULL << b), salle[b]))) {
                        e.essai++;
                        continue;
                    }
                    budgetTri--;
                    emis[b] = true;
                    bloqueTri[goals[b]] = true;
                    salleCourante = salle[b];
                    trie.append(b);
                    masqueTri |= (1ULL << b);
                } else {
                    // ÉPUISEMENT RÉEL (tous les candidats essayés, aucun n'aboutit)
                    // = ce sous-ensemble est stérile, définitivement. Un abandon par
                    // BUDGET, lui, ne prouve rien et ne s'inscrit pas.
                    if (memoTri && e.essai >= e.choix.size())
                        echecsTri.insert(cleTri(masqueTri, e.salleAvant));
                    pileTri.removeLast();                            // cet étage est épuisé
                    if (trie.isEmpty() || pileTri.isEmpty()) break;  // espace épuisé
                    const int d = trie.takeLast();                   // on défait le choix d'avant
                    emis[d] = false;
                    bloqueTri[goals[d]] = false;
                    masqueTri &= ~(1ULL << d);
                    // ⚠️ La salle courante se restaure depuis l'étage OÙ L'ON REVIENT,
                    // pas depuis celui qu'on vient de jeter : `salleAvant` d'un étage
                    // est la salle d'AVANT sa propre pose. Prendre celle de l'étage
                    // jeté rendrait la salle de la pose qu'on est en train de défaire,
                    // et la passe « rester dans la salle en cours » classerait les
                    // candidats suivants sur une salle qui n'est plus la bonne.
                    salleCourante = pileTri[pileTri.size() - 1].salleAvant;
                    pileTri[pileTri.size() - 1].essai++;             // …et on essaie le suivant
                }
                continue;
            }

            // LES CANDIDATS D'UN ÉTAGE, par ordre de préférence — aucun ne mure qui que
            // ce soit, c'est la condition d'entrée dans cette recherche.
            QVector<int> choix;
            auto ajoute = [&](int b) { if (!choix.contains(b)) choix.append(b); };
            // 1) rester dans la salle en cours (le groupement du §6.2 : sauter de salle
            //    en salle rend la macro indisponible — 886 états sans macro sur le 10).
            for (int b : ordre)
                if (!emis[b] && salle[b] == salleCourante && pret(b) && !mureraitQuelquun(b)) ajoute(b);
            // 2) sinon changer de salle, la stabilité fixant laquelle.
            for (int b : ordre)
                if (!emis[b] && pret(b) && !mureraitQuelquun(b)) ajoute(b);
            // 3) RELÂCHER L'INDICE POUR SAUVER LA PREUVE. `precedenceAlignement` se
            //    décrit elle-même comme « un indice fort, pas une preuve » (game.h) ;
            //    un murage, lui, est une preuve d'impossibilité. Quand les deux se
            //    contredisent, on sacrifie l'indice. Inerte hors régime `loi` :
            //    `requis` et `precGlobale` y sont le même objet, donc cette passe ne
            //    peut rien trouver que 2) n'ait déjà vu.
            for (int b : ordre)
                if (!emis[b] && pretPreuve(b) && !mureraitQuelquun(b)) ajoute(b);

            pileTri.append(EtageTri{choix, 0, salleCourante});
        }

        // REPLI : aucun ordre sans murage trouvé (ou budget épuisé). On refait alors
        // EXACTEMENT l'ancien glouton, murages compris — la garde est un BONUS, elle
        // ne doit jamais rendre un ordre PIRE que celui d'avant. C'est la même
        // dégradation gracieuse que le repli du glouton par précédence ci-dessus.
        if (!triSain) {
            trie.clear();
            emis.fill(false);
            bloqueTri.fill(false);
            salleCourante = -1;
            masqueTri = 0;
            while (trie.size() < nbButs) {
                int choisi = -1;
                for (int b : ordre) {
                    if (emis[b] || salle[b] != salleCourante) continue;
                    if (pret(b)) { choisi = b; break; }
                }
                if (choisi < 0)
                    for (int b : ordre) {
                        if (emis[b]) continue;
                        if (pret(b)) { choisi = b; break; }
                    }
                // CYCLE (aucun but prêt) : le modèle optimiste se contredit — on émet
                // le premier restant plutôt que de boucler. `butActif()` exige une
                // permutation complète.
                if (choisi < 0)
                    for (int b : ordre) if (!emis[b]) { choisi = b; break; }
                emis[choisi] = true;
                salleCourante = salle[choisi];
                trie.append(choisi);
            }
        }
        ordre = trie;
    }
    return ordre;
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

    // Ordre par PRÉCÉDENCE DE LIVRAISON + contiguité de run (§6.2, 2026-07-20) —
    // remplace le rebours ci-dessus, qui ne testait la sortie qu'à UN pas et ratait
    // les précédences. Le rebours reste comme fallback si la précédence ne rend pas
    // une permutation complète (jamais observé, mais butActif() exige un ordre plein).
    installeOrdreParPrecedence();

    // ── INJECTION D'UN ORDRE À LA MAIN — OUTIL DE CHANTIER, JETABLE ──────────────
    // Remet ce que faisait `ORACLE_HUMAIN`, retiré à la promotion du 2026-07-20.
    // Raison d'être : on sait produire des ordres à la main (11 en juillet, 12 le
    // 2026-07-31) et on n'a aucun moyen de les MESURER sans en tirer d'abord une
    // règle. Or l'histoire du projet dit l'inverse — sur le 11, l'ordre est venu de
    // la main d'abord, la règle six approches plus tard.
    //
    // DEUX sources, même format « (x,y) (x,y) … » :
    //
    //   1. ORDRE_HUMAIN="(15,9) (15,8) …"  — variable d'ENVIRONNEMENT. ⚠️ Invisible
    //      depuis l'app lancée par un launcher (§7) : c'est l'outil du BENCH.
    //   2. le FICHIER `ordre_niveau_XXXX.txt` du répertoire courant — c'est le seul
    //      moyen d'injecter un ordre dans l'APP, donc de le jouer en mode hybride et
    //      de voir OÙ il coince. Même raison d'être, autre canal.
    //
    // Aucune des deux n'AJOUTE quoi que ce soit : elles écrasent un ordre déjà
    // calculé. Sans elles, rien ne change (§7 : « un défaut coupé se voit tout de
    // suite, un défaut manquant ne se voit jamais »). Et parce qu'un fichier oublié
    // dans un coin changerait le comportement en SILENCE, l'injection est BRUYANTE :
    // elle s'annonce sur stderr, et l'UI la répète dans le journal hybride — sans
    // quoi on dépouillerait un jour une partie en croyant lire l'ordre calculé.
    //
    // ⚠️ OUTIL DE CHANTIER, À RETIRER avec le reste de la campagne hybride.
    QByteArray inj = qgetenv("ORDRE_HUMAIN");
    const QString fic = cheminOrdreInjecte(numNiveau);
    if (inj.isEmpty() && !fic.isEmpty()) {
        QFile f(fic);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            inj = f.readAll();
            fprintf(stderr, "[ORDRE_FICHIER] lecture de %s\n", qPrintable(fic));
        }
    }
    if (!inj.isEmpty()) {
        QVector<int> voulu;
        const QRegularExpression re("\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
        auto it = re.globalMatch(QString::fromLocal8Bit(inj));
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const int cell = m.captured(1).toInt() + m.captured(2).toInt() * largeur;
            int b = -1;
            for (int k = 0; k < nbButs; k++) if (goals[k] == cell) { b = k; break; }
            if (b < 0) {
                fprintf(stderr, "[ORDRE_HUMAIN] (%s,%s) n'est pas un but de ce niveau — IGNORÉ\n",
                        qPrintable(m.captured(1)), qPrintable(m.captured(2)));
                voulu.clear();
                break;
            }
            if (voulu.contains(b)) {
                fprintf(stderr, "[ORDRE_HUMAIN] but (%s,%s) cité DEUX FOIS — IGNORÉ\n",
                        qPrintable(m.captured(1)), qPrintable(m.captured(2)));
                voulu.clear();
                break;
            }
            voulu.append(b);
        }
        // ⚠️ Tout ou rien : `butActif()` exige une PERMUTATION COMPLÈTE. Un ordre
        // partiel laisserait des buts hors liste et ferait rendre n'importe quoi —
        // on préfère refuser bruyamment et garder l'ordre calculé.
        if (voulu.size() == nbButs) {
            ordreButs = voulu;
            fprintf(stderr, "[ORDRE_HUMAIN] ordre injecté : %d buts\n", nbButs);
        } else if (!inj.isEmpty()) {
            fprintf(stderr, "[ORDRE_HUMAIN] %d buts lus pour %d attendus — ordre calculé CONSERVÉ\n",
                    (int)voulu.size(), nbButs);
        }
        fflush(stderr);
    }
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
    if (!ordreDynamique) {
        for (int k = 0; k < nbButs; k++)
            if (cases[goals[ordreButs[k]]] != Level::tcGoalCaisse)
                return ordreButs[k];
        return -1;
    }

    // ORDRE DYNAMIQUE (§6.2, cf. game.h). Le JALON : tant que le but choisi n'est pas
    // rempli, on le rend tel quel — aucun calcul. C'est ce qui borne le coût à nbButs
    // passes par chemin au lieu d'une par état.
    // ⚠️ La porte fait partie du JALON, pas seulement du choix : une caisse peut venir
    // se garer sur une case de porte APRÈS qu'on a élu le but. Sans ce test, le cache
    // rendrait un but devenu non mûr jusqu'au prochain remplissage — c'est-à-dire
    // exactement pendant la phase où le mal se fait.
    if (butCourant >= 0 && cases[goals[butCourant]] != Level::tcGoalCaisse
        && !porteBloquee(butCourant) && !porteGeneraliseeBloquee(butCourant))
        return butCourant;

    // Jalon atteint (ou premier appel) : on rechoisit depuis l'ÉTAT COURANT.
    // `bloque` = les buts déjà rangés, qui font obstacle — même convention que
    // `ordreParPrecedence`. distanceLivraison amorce son BFS sur les caisses
    // réellement présentes et la position réelle du joueur, donc son verdict porte
    // bien sur cet état-ci et pas sur le plateau de départ.
    QVector<bool> bloque(size, false);
    int nbPosees = 0;
    for (int b = 0; b < nbButs; b++)
        if (cases[goals[b]] == Level::tcGoalCaisse) { bloque[goals[b]] = true; nbPosees++; }
    const QVector<int> dist = distanceLivraison(bloque);

    // TRACE de l'ordre RÉELLEMENT suivi (diagnostic, §6.2). On n'imprime qu'à la
    // PREMIÈRE apparition d'un couple (but choisi, nb de caisses posées) : la sortie
    // est donc bornée par nbButs² lignes au pire, là où tracer chaque appel noierait
    // le terminal sous des millions de lignes. stderr, comme la jauge et les lignes
    // [record]/[plongeon] — les flux s'entrelacent, donc chaque ligne est DATÉE en
    // dépilements sans qu'on ait à toucher à une seule signature. Purement passive :
    // elle n'ajoute ni ne coupe aucun comportement, donc elle ne peut pas faire
    // diverger l'app du bench (§7), et elle marche dans l'UI sans variable d'env.
    auto choisit = [&](int b, const char* voie) -> int {
        static QSet<int> vus;
        const int cle = b * 1000 + nbPosees;
        if (!vus.contains(cle)) {
            vus.insert(cle);
            int rang = -1;
            for (int k = 0; k < nbButs; k++) if (ordreButs[k] == b) { rang = k; break; }
            fprintf(stderr, "[ordre] posees %2d/%d -> but (%d,%d) rang %d %s\n",
                    nbPosees, nbButs, goals[b] % largeur, goals[b] / largeur, rang, voie);
            fflush(stderr);
        }
        butCourant = b;
        return b;
    };

    // ⚠️ GARDE ANTI-ÉCHOUAGE — la moitié du critère qui manquait au premier jet
    // (2026-07-31, diagnostic utilisateur sur `plateau_niveau13.xsb`). Se contenter de
    // « ce but est-il livrable ? » laisse SAUTER un but dont le remplissage en condamne
    // un autre : sur le 13, le solveur a posé (14,5), (14,6) et (14,7) en laissant
    // (14,8) vide — or (14,8) est enclavé entre les murs (13,8)/(15,8), sa seule
    // approche est une caisse en (14,7) poussée vers le bas avec le joueur en (14,6),
    // et (14,10) est un mur. Les deux cases d'appui se retrouvent occupées.
    // ⚠️ Position MAUVAISE, pas prouvée morte : les caisses de (14,5) et (14,7) peuvent
    // sortir latéralement ((13,5)/(15,5)/(13,7)/(15,7) sont libres), et un A* pur lancé
    // dessus place encore 3 caisses en 2,3 M états avant d'être coupé sans verdict.
    // `ordreParPrecedence` ne commet pas cette faute parce qu'il exige,
    // EN PLUS de la livrabilité, que poser un but laisse tous les autres livrables.
    // On reprend donc ici exactement sa garde — même modèle, même `bloque`.
    int premierLivrable = -1;
    for (int k = 0; k < nbButs; k++) {
        const int b = ordreButs[k];
        if (cases[goals[b]] == Level::tcGoalCaisse) continue;   // déjà rangé
        if (dist[goals[b]] == -1) continue;                     // plus livrable d'ici : on PASSE
        // PORTE (§6.2, 2026-08-04) : ce but condamnerait une caisse encore en place,
        // qui n'a plus d'appui une fois qu'il est posé. Il n'est pas mûr — on PASSE,
        // on ne coupe rien. Sur le 16, c'est (12,7) rang 0 tant que (10,6) est occupée.
        // ⚠️ Placé AVANT `premierLivrable` : ce filet ne doit pas non plus le retenir,
        // sinon le relâchement d'en dessous reposerait le but qu'on vient d'écarter.
        // PORTE GÉNÉRALISÉ (§6.0, 2026-08-18) : même principe, étendu à N'IMPORTE
        // QUELLE caisse/but qui perdrait accès — pas seulement les appuis propres.
        if (porteBloquee(b) || porteGeneraliseeBloquee(b)) continue;
        if (premierLivrable < 0) premierLivrable = b;            // filet, cf. plus bas

        bloque[goals[b]] = true;
        const QVector<int> apres = distanceLivraison(bloque);
        bloque[goals[b]] = false;
        bool sur = true;
        for (int h = 0; h < nbButs && sur; h++) {
            if (h == b || cases[goals[h]] == Level::tcGoalCaisse) continue;
            if (apres[goals[h]] == -1) sur = false;              // b condamnerait h
        }
        if (sur) return choisit(b, "");
    }

    // Aucun but SÛR : on relâche sur le premier livrable, exactement comme le glouton
    // statique (`surs.isEmpty() → surs = candidats`). Le modèle est OPTIMISTE, donc son
    // « tout choix condamne un but » n'est pas une preuve — mieux vaut avancer que
    // rendre un but que plus aucune caisse n'atteint.
    if (premierLivrable >= 0) return choisit(premierLivrable, "(RELACHE : aucun but sur)");

    // Aucun but restant n'est livrable selon ce modèle (qui est OPTIMISTE, §6.2 : il
    // ignore les autres caisses comme obstacles de marche). On ne peut donc rien
    // conclure de sa négation — on retombe sur l'ordre statique plutôt que de rendre
    // -1, qui signifierait « gagné » à l'appelant.
    // ⚠️ LA PORTE VAUT AUSSI ICI — oubli du premier jet, rattrapé sur la trace du 16 :
    // le repli rendait `ordreButs[0]`, c'est-à-dire précisément le but que la
    // contrainte écarte, et il le faisait PLUS souvent qu'avant la greffe (10 fois
    // contre 7). Un test posé sur le chemin nominal et pas sur le repli ne tient pas :
    // c'est le repli qui sert quand ça va mal, donc exactement quand la contrainte
    // compte. On préfère donc un but non bloqué…
    for (int k = 0; k < nbButs; k++) {
        const int b = ordreButs[k];
        if (cases[goals[b]] == Level::tcGoalCaisse) continue;
        if (porteBloquee(b) || porteGeneraliseeBloquee(b)) continue;
        return choisit(b, "(REPLI STATIQUE : aucun but livrable)");
    }
    // … et on ne se retrouve à en rendre un bloqué que s'ils le sont TOUS. Rendre -1
    // ici signifierait « gagné » à l'appelant : le dernier recours doit exister.
    for (int k = 0; k < nbButs; k++)
        if (cases[goals[ordreButs[k]]] != Level::tcGoalCaisse)
            return choisit(ordreButs[k], "(REPLI STATIQUE : tous les buts sont bloques par une porte)");
    butCourant = -1;
    return -1;
}

int Game::avanceVersBut(int c, int d, int dCur, const int* dpb,
                        const QVector<bool>& zone) const {
    const int cx = c % largeur, cy = c / largeur;
    const int devx = cx + directions[d].dx, devy = cy + directions[d].dy;   // case caisse après
    const int appx = cx - directions[d].dx, appy = cy - directions[d].dy;   // appui joueur
    if (devx < 0 || devx >= largeur || devy < 0 || devy >= hauteur) return -1;
    if (appx < 0 || appx >= largeur || appy < 0 || appy >= hauteur) return -1;
    const int devant = devx + devy * largeur;
    const int appui  = appx + appy * largeur;
    // ⚠️ Le JOUEUR n'est pas un obstacle : il libère sa propre case en marchant
    // vers l'appui avant que la caisse n'avance (pousse() le téléporte). Même
    // exemption que getCaissesDeplacable (game.cpp:664), qui l'avait déjà et
    // dont c'est la seule différence avec ce test — sans elle, la descente
    // refusait TOUTE poussée qui ramène la caisse sur la case d'où on vient de
    // la pousser, c'est-à-dire tout DEMI-TOUR : le joueur s'y tient forcément.
    const int idxPlayer = playerPoint.x() + playerPoint.y() * largeur;
    if (!isLibre(devant) && devant != idxPlayer) return -1;   // mur / autre caisse
    if (!zone[appui]) return -1;             // joueur ne peut pas se placer derrière
    const int rApres = regions[c * size + devant];
    if (rApres < 0) return -1;
    return (dpb[devant * maxRegions + rApres] == dCur - 1) ? devant : -1;
}

QVector<int> Game::champDistanceButActif() const {
    const int b = butActif();
    if (b < 0) return {};

    QVector<int> champ(size, -1);
    const int joueurIdx = playerPoint.x() + playerPoint.y() * largeur;
    const int* dpb = distanceParBut.constData() + (qsizetype)b * size * maxRegions;
    const QVector<bool> zone = getZoneJoueur();

    // Une caisse à la fois — comme macroPeutDemarrer/macroVersBut le feraient
    // pour chaque candidate. dCur se lit avec la position RÉELLE du joueur
    // (c'est bien elle, avant toute poussée) ; les voisins, eux, passent par
    // avanceVersBut — seule source de vérité sur ce qui est un coup légal.
    for (int cell = 0; cell < size; cell++) {
        if (cases[cell] != Level::tcCaisse && cases[cell] != Level::tcGoalCaisse) continue;

        const int rAvant = regions[joueurIdx * size + cell];
        if (rAvant < 0) continue;
        const int dCur = dpb[cell * maxRegions + rAvant];
        if (dCur < 0) continue;
        champ[cell] = dCur;
        if (dCur == 0) continue;   // déjà sur le but : rien à avancer (garde de macroPeutDemarrer)

        for (int d = 0; d < NB_DIRECTION; d++) {
            const int devant = avanceVersBut(cell, d, dCur, dpb, zone);
            if (devant >= 0) champ[devant] = dCur - 1;
        }
    }
    return champ;
}

QVector<int> Game::champDistanceBrut(int indexBut) const {
    if (indexBut < 0 || indexBut >= nbButs) return {};

    QVector<int> champ(size, -1);
    const int joueurIdx = playerPoint.x() + playerPoint.y() * largeur;
    const int* dpb = distanceParBut.constData() + (qsizetype)indexBut * size * maxRegions;

    for (int cell = 0; cell < size; cell++) {
        if (cases[cell] == Level::tcMur) continue;
        // La région se lit avec le joueur RÉEL : c'est ce que fait le solveur
        // (getHeuristique, avanceVersBut), et c'est là qu'est tout l'intérêt —
        // une même case n'a pas la même distance selon le côté où est le joueur.
        const int r = regions[joueurIdx * size + cell];
        if (r < 0) continue;
        champ[cell] = dpb[cell * maxRegions + r];
    }
    return champ;
}

QVector<int> Game::cheminMacro(int idxCaisse) const {
    if (cases[idxCaisse] != Level::tcCaisse && cases[idxCaisse] != Level::tcGoalCaisse) return {};
    const int b = butActif();
    if (b < 0) return {};

    const int* dpb = distanceParBut.constData() + (qsizetype)b * size * maxRegions;
    const int joueurIdx = playerPoint.x() + playerPoint.y() * largeur;
    const int rAvant = regions[joueurIdx * size + idxCaisse];
    if (rAvant < 0) return {};
    const int dCur = dpb[idxCaisse * maxRegions + rAvant];
    if (dCur < 0) return {};

    QVector<int> champ(size, -1);
    champ[idxCaisse] = dCur;

    // Copie jetable : macroVersBut POUSSE réellement la caisse (pousse(),
    // checkDefaite compris) — *this doit rester intact, c'est un simple clic
    // de diagnostic, pas un coup joué.
    Game copie(*this);
    QVector<QPair<int, int>> poussees;
    copie.macroVersBut(idxCaisse, b, poussees);

    // Chaque élément de 'poussees' est (case AVANT le coup, direction) : la
    // case d'arrivée s'en déduit par translation, comme dans macroVersBut
    // lui-même. La distance décroît d'exactement 1 par construction — c'est
    // un trajet réellement joué, pas une lecture indépendante par case.
    int d = dCur;
    for (const auto& p : poussees) {
        const int c   = p.first;
        const int dir = p.second;
        const int devant = c + directions[dir].dx + directions[dir].dy * largeur;
        d--;
        champ[devant] = d;
    }
    return champ;
}

QVector<bool> Game::arbreMacro(int idxCaisse, qint64 budgetNoeuds) const {
    QVector<bool> visite(size, false);
    if (cases[idxCaisse] != Level::tcCaisse && cases[idxCaisse] != Level::tcGoalCaisse) return visite;
    const int b = butActif();
    if (b < 0) return visite;
    const int caseBut = goals[b];
    const int* dpb = distanceParBut.constData() + (qsizetype)b * size * maxRegions;

    visite[idxCaisse] = true;

    // Pile de branches à explorer : (état à cet instant, case de la caisse).
    // Comme macroVersButBacktrack, mais on ne s'arrête PAS au premier succès
    // — on empile TOUTES les directions qui avancent, pas seulement celles
    // en réserve, pour matérialiser la totalité de l'arbre.
    struct Noeud { Game etat; int caisse; };
    QVector<Noeud> pile;
    pile.append({Game(*this), idxCaisse});

    qint64 budget = budgetNoeuds;
    while (!pile.isEmpty() && budget-- > 0) {
        Noeud n = pile.takeLast();
        if (n.caisse == caseBut) continue;

        QVector<bool> zone = n.etat.getZoneJoueur();
        const int joueurIdx = n.etat.playerPoint.x() + n.etat.playerPoint.y() * largeur;
        const int rAvant = n.etat.regions[joueurIdx * size + n.caisse];
        if (rAvant < 0) continue;
        const int dCur = dpb[n.caisse * maxRegions + rAvant];
        if (dCur <= 0) continue;

        for (int d = 0; d < NB_DIRECTION; d++) {
            const int devant = n.etat.avanceVersBut(n.caisse, d, dCur, dpb, zone);
            if (devant < 0) continue;
            Game suite(n.etat);
            if (!suite.pousse(n.caisse, (Game::EDirection)d) || suite.isPerdu()) continue;
            visite[devant] = true;
            pile.append({std::move(suite), devant});
        }
    }
    return visite;
}

bool Game::macroPeutDemarrer(int idxCaisse, int indexBut, const QVector<bool>& zone) const {
    // Le PREMIER pas de macroVersBut, sans rien copier ni modifier. Si c'est non,
    // la macro échouerait au pas 0 — mesuré (mesures/macro) : 48,5 % des tentatives
    // du niveau 11, chacune payant jusqu'ici une copie complète de Game pour rien.
    //
    // La condition est partagée avec la boucle (avanceVersBut) : elle ne peut pas
    // en diverger. Ne JAMAIS la réécrire ici — ce serait un filtre qui écarte des
    // macros réellement jouables, donc une perte silencieuse d'enfants.
    if (idxCaisse == goals[indexBut]) return true;   // déjà sur le but : macro triviale
    const int* dpb = distanceParBut.constData() + (qsizetype)indexBut * size * maxRegions;
    const int joueurIdx = playerPoint.x() + playerPoint.y() * largeur;
    const int rAvant = regions[joueurIdx * size + idxCaisse];
    if (rAvant < 0) return false;
    const int dCur = dpb[idxCaisse * maxRegions + rAvant];
    if (dCur <= 0) return false;
    for (int d = 0; d < NB_DIRECTION; d++)
        if (avanceVersBut(idxCaisse, d, dCur, dpb, zone) >= 0) return true;
    return false;
}

Game::ECausePas0 Game::diagnosticPas0(int idxCaisse, int indexBut, const QVector<bool>& zone,
                                      QVector<QPair<int,int>>* dirsAppui) const {
    if (dirsAppui) dirsAppui->clear();

    // Le même préambule que macroPeutDemarrer, aux mêmes conditions — mais chaque
    // sortie devient une CAUSE au lieu d'un `false` indifférencié.
    if (idxCaisse == goals[indexBut]) return Pas0DejaSurBut;
    const int* dpb = distanceParBut.constData() + (qsizetype)indexBut * size * maxRegions;
    const int joueurIdx = playerPoint.x() + playerPoint.y() * largeur;
    const int rAvant = regions[joueurIdx * size + idxCaisse];
    if (rAvant < 0) return Pas0HorsRegion;
    const int dCur = dpb[idxCaisse * maxRegions + rAvant];
    if (dCur <= 0) return Pas0ButInatteignable;

    // Zone TOTALE : le joueur supposé capable d'atteindre n'importe quel appui.
    // Une direction qui passe ici mais pas avec la zone réelle isole exactement
    // la contrainte de placement du joueur, et rien d'autre.
    const QVector<bool> zoneTotale(size, true);
    int bloqueesParLeJoueur = 0;   // compté à part : le verdict ne doit pas
                                   // dépendre de la présence du pointeur de sortie
    for (int d = 0; d < NB_DIRECTION; d++) {
        if (avanceVersBut(idxCaisse, d, dCur, dpb, zone) >= 0) return Pas0Demarre;
        if (avanceVersBut(idxCaisse, d, dCur, dpb, zoneTotale) < 0) continue;
        bloqueesParLeJoueur++;
        if (dirsAppui) {
            const int ax = idxCaisse % largeur - directions[d].dx;
            const int ay = idxCaisse / largeur - directions[d].dy;
            dirsAppui->append({d, ax + ay * largeur});
        }
    }
    return bloqueesParLeJoueur > 0 ? Pas0JoueurMauvaisCote : Pas0DetourRequis;
}

bool Game::macroVersBut(int idxCaisse, int indexBut, QVector<QPair<int,int>>& poussees,
                        const QVector<bool>* zoneInitiale) {
    const int caseBut = goals[indexBut];
    const int* dpb = distanceParBut.constData() + (qsizetype)indexBut * size * maxRegions;

#ifdef INSTRUM_MACRO
    StatsMacro& st = statsMacro();
    st.tentatives++;
    int forks = 0;
    // Compte l'échec 'quoi' au pas 'pas', avec la distance restante 'reste'.
    auto echec = [&](qint64& quoi, int pas, int reste) {
        quoi++;
        if (forks) st.echecAvecFork++;
        if ((size_t)pas >= st.histoEchecPas.size()) st.histoEchecPas.resize(pas + 1, 0);
        st.histoEchecPas[pas]++;
        st.resteAuBlocage += reste;
        st.forksTotal += forks;
        st.pasTotal += pas;
    };
#endif

    // La zone du joueur, recalculée UNIQUEMENT quand elle est périmée. Au premier
    // pas, le plateau n'a pas encore bougé : c'est exactement la zone que
    // l'appelant vient de calculer pour cet état, et qu'il nous passe (il en a
    // besoin de son côté pour getCaissesDeplacable). Comme il essaie une macro
    // par caisse candidate — ~5 par état développé —, la recalculer ici faisait
    // la MOITIÉ des flood-fills du solveur (mesuré : 4,36 M sur 8,8 M au niveau
    // 11, cf. mesures/macro). 'zoneCourante' à nullptr = à recalculer.
    QVector<bool> zoneLocale;
    const QVector<bool>* zoneCourante = zoneInitiale;

    int c = idxCaisse;
    for (int garde = 0; c != caseBut && garde <= 2 * size; garde++) {
        if (!zoneCourante) {
            getZoneJoueur(zoneLocale);   // tampon réutilisé d'un pas à l'autre
            zoneCourante = &zoneLocale;
        }
        const QVector<bool>& zone = *zoneCourante;
        const int joueurIdx = playerPoint.x() + playerPoint.y() * largeur;
        const int rAvant = regions[joueurIdx * size + c];
        if (rAvant < 0) {
#ifdef INSTRUM_MACRO
            echec(st.echecRegion, garde, 0);
#endif
            return false;
        }
        const int dCur = dpb[c * maxRegions + rAvant];
        if (dCur <= 0) {                             // -1 (inatteignable) ou déjà arrivé
#ifdef INSTRUM_MACRO
            echec(st.echecDistance, garde, 0);
#endif
            return false;
        }

        auto avanceVers = [&](int d) { return avanceVersBut(c, d, dCur, dpb, zone); };

#ifdef INSTRUM_MACRO
        // Combien de descentes optimales s'offraient ICI ? Plus d'une = la boucle
        // ci-dessous fait un choix ARBITRAIRE (première dans l'ordre de l'énum),
        // sur lequel elle ne reviendra jamais.
        int nbCand = 0;
        for (int d = 0; d < NB_DIRECTION; d++) if (avanceVers(d) >= 0) nbCand++;
        if (nbCand > 1) forks++;
#endif

        bool avance = false;
        for (int d = 0; d < NB_DIRECTION && !avance; d++) {
            const int devant = avanceVers(d);
            if (devant < 0) continue;
            if (!pousse(c, (Game::EDirection)d)) {
#ifdef INSTRUM_MACRO
                echec(st.echecPousse, garde, dCur);
#endif
                return false;
            }
            poussees.append({c, d});
            c = devant;
            avance = true;
            zoneCourante = nullptr;   // la caisse a bougé : la zone est PÉRIMÉE
        }
        if (!avance) {                               // aucune poussée ne fait avancer : bloqué
#ifdef INSTRUM_MACRO
            echec(st.echecBloque, garde, dCur);
#endif
            return false;
        }
    }
#ifdef INSTRUM_MACRO
    if (c == caseBut) {
        st.succes++;
        if (forks) st.succesAvecFork++;
        st.forksTotal += forks;
        st.pasTotal += poussees.size();
        if (poussees.size() >= (int)st.histoSuccesLong.size())
            st.histoSuccesLong.resize(poussees.size() + 1, 0);
        st.histoSuccesLong[poussees.size()]++;
    }
#endif
    return c == caseBut;
}

bool Game::macroVersButBacktrack(int idxCaisse, int indexBut, QVector<QPair<int,int>>& poussees,
                                  qint64* essaisOut, qint64 budgetBranches) {
    if (cases[idxCaisse] != Level::tcCaisse && cases[idxCaisse] != Level::tcGoalCaisse) {
        if (essaisOut) *essaisOut = 0;
        return false;
    }
    const int caseBut = goals[indexBut];
    const int* dpb = distanceParBut.constData() + (qsizetype)indexBut * size * maxRegions;

    // Un fork mémorisé : l'état du plateau À CET INSTANT (une seule caisse a
    // bougé depuis le départ, les tables statiques restent partagées COW —
    // la copie reste bon marché), la case de la caisse, la direction NON
    // essayée, et le chemin joué jusque-là. Rejouer depuis un fork = repartir
    // de cette copie plutôt que de tenter un « undo » manuel (plus sûr : on
    // ne réinvente pas la logique de move()/checkDefaite).
    struct Fork { Game etat; int caisse; int direction; QVector<QPair<int,int>> chemin; };
    QVector<Fork> pile;

    Game etat(*this);
    int c = idxCaisse;
    QVector<QPair<int,int>> chemin;
    qint64 essais = 1;

    // Joue la direction 'd' (déjà connue valide par avanceVersBut) sur
    // 'etat' : peut encore échouer via pousse() (checkDefaite au passage,
    // comme echecPousse dans macroVersBut) — dans ce cas la branche est
    // morte, on ne le découvre parfois qu'ici.
    auto joue = [&](int d) -> bool {
        const int devant = c + directions[d].dx + directions[d].dy * largeur;
        if (!etat.pousse(c, (Game::EDirection)d)) return false;
        chemin.append({c, d});
        c = devant;
        return true;
    };

    bool bloque = false;
    for (;;) {
        if (!bloque) {
            while (c != caseBut) {
                QVector<bool> zone = etat.getZoneJoueur();
                const int joueurIdx = etat.playerPoint.x() + etat.playerPoint.y() * largeur;
                const int rAvant = etat.regions[joueurIdx * size + c];
                if (rAvant < 0) { bloque = true; break; }
                const int dCur = dpb[c * maxRegions + rAvant];
                if (dCur <= 0) { bloque = true; break; }

                QVarLengthArray<int, NB_DIRECTION> candidats;
                for (int d = 0; d < NB_DIRECTION; d++)
                    if (etat.avanceVersBut(c, d, dCur, dpb, zone) >= 0) candidats.append(d);
                if (candidats.isEmpty()) { bloque = true; break; }

                // Empile les branches non retenues AVANT de jouer la
                // première : dans l'ordre inverse, pour dépiler dans l'ordre
                // de l'énum si plusieurs sont un jour ouvertes.
                for (int i = candidats.size() - 1; i >= 1; i--)
                    pile.append({etat, c, candidats[i], chemin});

                if (!joue(candidats[0])) { bloque = true; break; }
            }
        }

        if (!bloque && c == caseBut) {
            poussees = chemin;
            // macroVersBut mute *this DIRECTEMENT (pousse() est appelé sur
            // l'objet lui-même) : l'appelant (solveurastar.cpp) récupère le
            // résultat en relisant 'e' après coup, pas via une valeur de
            // retour. Ici tout le travail se fait sur la copie locale 'etat'
            // (nécessaire pour pouvoir revenir en arrière) — il FAUT la
            // recopier dans *this avant de rendre la main, sinon l'appelant
            // voit un état inchangé (silencieusement dupliqué du parent,
            // rejeté par la dédup : c'est exactement le bug qui a cassé le
            // canari au premier essai — cf. plan.md).
            *this = std::move(etat);
            if (essaisOut) *essaisOut = essais;
#ifdef INSTRUM_MACRO
            StatsMacro& st = statsMacro();
            st.btTentatives++;
            st.btSucces++;
            if (essais > 1) st.btSuccesApresBacktrack++;
            st.btEssaisTotal += essais;
            if (essais > st.btEssaisMax) st.btEssaisMax = essais;
#endif
            return true;
        }

        if (pile.isEmpty() || essais >= budgetBranches) {
            // Échec : *this finit « partiellement modifié », comme
            // macroVersBut (contrat documenté en game.h) — l'appelant traite
            // toujours 'e' comme une copie jetable dans ce cas.
            *this = std::move(etat);
            if (essaisOut) *essaisOut = essais;
#ifdef INSTRUM_MACRO
            StatsMacro& st = statsMacro();
            st.btTentatives++;
            st.btEssaisTotal += essais;
            if (essais > st.btEssaisMax) st.btEssaisMax = essais;
#endif
            return false;
        }

        Fork f = pile.takeLast();
        etat = f.etat;
        c = f.caisse;
        chemin = f.chemin;
        essais++;
        bloque = !joue(f.direction);
    }
}

#ifdef INSTRUM_MACRO
StatsMacro& statsMacro() { static StatsMacro s; return s; }
#endif
