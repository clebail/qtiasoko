#include <QtDebug>
#include <QVarLengthArray>
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

// Interrupteur de mesure du test de livraison (§6.1), pour comparer les régimes SUR
// LE MÊME BINAIRE. DÉFAUT = 0, coupé : le test s'est révélé FAUX POSITIF (mesuré
// par mesures/fp.cpp le 2026-07-21, cf. game.h). Variantes :
//   1 flood-fill du joueur, toutes caisses posées obstacles   — FP (2/3/5/6/7/17)
//   2 idem, marche lue dans 'regions'                         — FP (idem)
//   3 lecture de distanceParBut, aucun obstacle-caisse        — SÛR, mais 0 capture
//   4 comme 2, seules les caisses GELÉES font obstacle        — FP (2 et 17)
//   5 comme 4, mais testé par le solveur sur les états enfilés — FP (idem)
//   6 comme 2 sans aucun obstacle-caisse (diagnostic)         — FP (17)
static int livraisonMode() {
    static const int mode = qgetenv("LIVRAISON").toInt();
    return mode;
}

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
      playerPoint(other.playerPoint),
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
      playerPoint(other.playerPoint),
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

    // En dernier, parce que c'est le plus cher : le but orphelin (§6.1). Il voit
    // ce qu'aucun test par caisse ne peut voir — un but que le PLATEAU ENTIER ne
    // sait plus alimenter. Modes 1 à 4 seulement : au-delà, c'est le SOLVEUR qui
    // l'appelle, sur les états qu'il enfile (cf. game.h — ici, il tuerait les
    // goal macros en cours de route).
    const int modeLiv = livraisonMode();
    if (modeLiv >= 1 && modeLiv <= 4 && butNonLivrable()) {
        perdu = true;
        return;
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

// Test de LIVRAISON, dit « BUT ORPHELIN » (§6.1, mesuré le 2026-07-20 suite 3) :
// un but VIDE vers lequel PLUS AUCUNE caisse ne peut être poussée rend l'état
// insoluble. Mesuré par un BFS de poussées AVANT, multi-source (toutes les
// caisses), dans une relaxation où :
//  - une seule caisse bouge à la fois — les autres caisses NON posées sont
//    traversées comme du sol, par la caisse comme par le joueur (relaxation
//    franche : elle ne peut qu'agrandir l'ensemble atteignable) ;
//  - le joueur doit réellement MARCHER jusqu'à la case d'appui, sans traverser
//    la caisse qu'il pousse. C'est ce qui donne au test sa capture : sans cette
//    marche il ne dit presque rien de plus que casesMortes.
//
// ⚠️ Un point n'est PAS une relaxation : les caisses DÉJÀ POSÉES sur un but sont
// tenues pour des obstacles fixes, alors que le vrai jeu autorise à en ressortir
// une pour livrer ailleurs. C'est ce qui CASSE LE CANARI (niveau 2 en macro :
// 131 → 133 poussées, mesuré le 2026-07-21) — la sûreté « 0 faux positif » de
// mesures/mort.cpp était de l'échantillonnage, pas une preuve, exactement comme
// pour le test par COUPLAGE (§6.1).
//
// D'où les modes prouvables 3 et 4, où le seul obstacle est PERMANENT :
//   3 — aucune caisse n'est un obstacle ; le test se lit dans les tables
//       précalculées (distanceParBut/regions), coût O(buts × caisses) ;
//   4 — seules les caisses GELÉES sur un but font obstacle (le gel est
//       permanent par construction, c'est déjà l'hypothèse de caisseGelee).
// Dans les deux cas, toute livraison réellement jouable reste jouable dans la
// relaxation : l'impossibilité constatée est une PREUVE.
//
// Capture mesurée (modes non sûrs) parmi les états morts : 96 % (niv 7), 70 % (3),
// 50 % (6), 45 % (17) ; 0 % sur 9/11, dont les morts sont d'un autre type.
bool Game::butNonLivrable(int variante) const {
    // Tampons réutilisés d'un appel à l'autre : ce test est dans le chemin chaud
    // (une fois par poussée), une allocation par appel le rendrait inutilisable.
    // 'vu'/'marqueJ' sont datés par un compteur (stamp) plutôt que remis à zéro.
    static thread_local std::vector<qint64> vu, marqueJ;
    static thread_local std::vector<int> joueurApres, file, fileJ;
    static thread_local qint64 stamp = 0, stampJ = 0;
    if ((int)vu.size() != size) {
        vu.assign(size, 0); marqueJ.assign(size, 0); joueurApres.assign(size, -1);
        stamp = 0; stampJ = 0;
    }

    const int mode = variante ? variante : livraisonMode();
    const int idxJoueur = playerPoint.x() + playerPoint.y() * largeur;

    // Combien de buts restent à livrer ? Aucun : rien à prouver.
    int restants = 0;
    for (int b = 0; b < goals.size(); b++) {
        const Level::ETypeCase t = cases[goals.at(b)];
        if (t == Level::tcGoal || t == Level::tcGoalPlayer) restants++;
    }
    if (restants == 0) return false;

    // MODE 3 — le test PROUVABLE bon marché : aucune caisse ne fait obstacle, si
    // bien que « telle caisse peut-elle atteindre tel but ? » se LIT dans
    // distanceParBut (caisse seule, joueur-aware), déjà calculée par niveau. C'est
    // le symétrique exact de staticDeadlock : celui-ci coupe quand une CAISSE
    // n'atteint plus aucun but, celui-là quand un BUT n'est atteint par aucune
    // caisse. Même argument d'admissibilité, donc même sûreté.
    if (mode == 3) {
        for (int b = 0; b < nbButs; b++) {
            const Level::ETypeCase tb = cases[goals.at(b)];
            if (tb != Level::tcGoal && tb != Level::tcGoalPlayer) continue;   // déjà rempli
            bool livrable = false;
            for (int cell = 0; cell < size && !livrable; cell++) {
                if (cases[cell] != Level::tcCaisse && cases[cell] != Level::tcGoalCaisse) continue;
                const qint16 r = regions.at(idxJoueur * size + cell);
                if (r < 0) continue;
                if (distanceParBut.at(((qsizetype)b * size + cell) * maxRegions + r) >= 0)
                    livrable = true;
            }
            if (!livrable) return true;
        }
        return false;
    }

    // MODE 4 — les caisses GELÉES sur un but, elles, ne repartiront jamais : les
    // compter comme obstacles reste une preuve. C'est le mode 2 avec ce seul
    // durcissement légitime.
    static thread_local std::vector<char> fige;
    if (mode == 4) {
        fige.assign(size, 0);
        QVector<bool> enCours(size, false);
        for (int c = 0; c < size; c++)
            if (cases[c] == Level::tcGoalCaisse && caisseGelee(c, enCours)) fige[c] = 1;
    }

    // Sol franchissable dans la relaxation : les murs, toujours ; les caisses
    // posées sur un but selon le mode (1/2 : toutes — NON PROUVÉ ; 4 : seulement
    // les gelées).
    auto libre = [&](int c) -> bool {
        if (cases[c] == Level::tcMur) return false;
        if (mode == 6) return true;          // aucune caisse n'est un obstacle (diagnostic)
        if (mode == 4) return !fige[c];
        return cases[c] != Level::tcGoalCaisse;
    };

    // Variante RAPIDE (LIVRAISON=2) : la marche du joueur se LIT dans 'regions'
    // (composantes du plateau moins UNE caisse, précalculées par niveau) au lieu
    // d'un flood-fill par case. O(1) au lieu de O(size), mais le joueur y traverse
    // les caisses posées sur but — relaxation SUPPLÉMENTAIRE, donc encore moins de
    // faux positifs que la version fidèle, et moins de capture. La caisse poussée,
    // elle, reste bloquée par les caisses posées dans les deux variantes.
    const bool rapide = (mode == 2 || mode == 4 || mode == 6);
    auto joueurAtteintRapide = [&](int depart, int cible, int caseCaisse) -> bool {
        if (depart == cible) return true;
        const qint16 rd = regions.at(depart * size + caseCaisse);
        return rd != -1 && rd == regions.at(cible * size + caseCaisse);
    };

    // Marque la zone où le joueur peut marcher, la caisse poussée étant en
    // 'caseCaisse' (qu'il ne traverse pas) et lui-même partant de 'depart'.
    auto floodJoueur = [&](int depart, int caseCaisse) {
        stampJ++;
        fileJ.clear();
        fileJ.push_back(depart);
        marqueJ[depart] = stampJ;
        for (size_t h = 0; h < fileJ.size(); h++) {
            const int c = fileJ[h], cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int nx = cx + directions[d].dx, ny = cy + directions[d].dy;
                if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                const int n = nx + ny * largeur;
                if (marqueJ[n] == stampJ || n == caseCaisse || !libre(n)) continue;
                marqueJ[n] = stampJ;
                fileJ.push_back(n);
            }
        }
    };

    stamp++;
    file.clear();
    for (int c = 0; c < size; c++) {
        if (cases[c] == Level::tcCaisse || cases[c] == Level::tcGoalCaisse) {
            vu[c] = stamp;
            joueurApres[c] = idxJoueur;
            file.push_back(c);
        }
    }

    for (size_t h = 0; h < file.size(); h++) {
        const int c = file[h], cx = c % largeur, cy = c / largeur;

        // D'abord les directions plausibles, SANS faire marcher le joueur : le
        // flood-fill est le poste coûteux, et la plupart des cases dépilées
        // n'ouvrent sur rien de neuf.
        int arrivee[NB_DIRECTION], appui[NB_DIRECTION], n = 0;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int ax = cx + directions[d].dx, ay = cy + directions[d].dy;
            const int px = cx - directions[d].dx, py = cy - directions[d].dy;
            if (ax < 0 || ax >= largeur || ay < 0 || ay >= hauteur) continue;
            if (px < 0 || px >= largeur || py < 0 || py >= hauteur) continue;
            const int a = ax + ay * largeur, p = px + py * largeur;
            if (vu[a] == stamp || !libre(a) || !libre(p)) continue;
            arrivee[n] = a; appui[n] = p; n++;
        }
        if (n == 0) continue;

        if (!rapide) floodJoueur(joueurApres[c], c);
        for (int k = 0; k < n; k++) {
            if (rapide ? !joueurAtteintRapide(joueurApres[c], appui[k], c)
                       : marqueJ[appui[k]] != stampJ) continue;
            const int a = arrivee[k];
            vu[a] = stamp;
            joueurApres[a] = c;   // la poussée faite, le joueur est là où était la caisse
            file.push_back(a);
            const Level::ETypeCase t = cases[a];
            // Sortie anticipée : dès que tous les buts vides sont livrables, le
            // test ne dira rien — inutile de finir le BFS.
            if ((t == Level::tcGoal || t == Level::tcGoalPlayer) && --restants == 0) return false;
        }
    }

    return true;   // il reste un but vide qu'aucune caisse ne peut atteindre
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
    QVector<int> dist(size, -1);
    QVector<int> joueurApres(size, -1);   // case où se tient le joueur une fois la caisse arrivée ici
    QList<int> file;

    auto libreCase = [&](int c) -> bool { return cases[c] != Level::tcMur && !bloque[c]; };
    auto libre = [&](int x, int y) -> bool {
        if (x < 0 || x >= largeur || y < 0 || y >= hauteur) return false;
        return libreCase(x + y * largeur);
    };

    // Le joueur atteint-il `cible` depuis `depart` en marchant sur du sol libre,
    // sans traverser `caseCaisse` (la caisse qu'on est en train de déplacer y est) ?
    auto joueurAtteint = [&](int depart, int cible, int caseCaisse) -> bool {
        if (depart == cible) return true;
        QVector<bool> vu(size, false);
        QList<int> f;
        f.append(depart);
        vu[depart] = true;
        while (!f.isEmpty()) {
            const int c = f.takeFirst();
            if (c == cible) return true;
            const int cx = c % largeur, cy = c / largeur;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int nx = cx + directions[d].dx, ny = cy + directions[d].dy;
                if (nx < 0 || nx >= largeur || ny < 0 || ny >= hauteur) continue;
                const int n = nx + ny * largeur;
                if (vu[n] || n == caseCaisse || !libreCase(n)) continue;
                vu[n] = true;
                f.append(n);
            }
        }
        return false;
    };

    const int depart = playerPoint.x() + playerPoint.y() * largeur;

    // Sources : les cases où une caisse se trouve au chargement du niveau. Le joueur
    // est supposé pouvoir rejoindre n'importe laquelle (relaxation (b) ci-dessus).
    for (int c = 0; c < size; c++) {
        if (bloque[c]) continue;
        if (cases[c] == Level::tcCaisse || cases[c] == Level::tcGoalCaisse) {
            dist[c] = 0;
            joueurApres[c] = depart;
            file.append(c);
        }
    }

    while (!file.isEmpty()) {
        const int c = file.takeFirst();
        const int cx = c % largeur, cy = c / largeur;
        for (int d = 0; d < NB_DIRECTION; d++) {
            const int ax = cx + directions[d].dx, ay = cy + directions[d].dy;   // arrivée
            const int px = cx - directions[d].dx, py = cy - directions[d].dy;   // appui joueur
            if (!libre(ax, ay) || !libre(px, py)) continue;
            const int a = ax + ay * largeur;
            if (dist[a] != -1) continue;
            const int appui = px + py * largeur;
            if (!joueurAtteint(joueurApres[c], appui, c)) continue;   // appui inatteignable à pied
            dist[a] = dist[c] + 1;
            joueurApres[a] = c;   // la poussée faite, le joueur est là où était la caisse
            file.append(a);
        }
    }
    return dist;
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

    for (int step = 0; step < nbButs; step++) {
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
        // Si la garde ne laisse rien passer, on la relâche plutôt que de rendre un ordre
        // incomplet : `butActif()` doit toujours trouver un but, et un ordre imparfait
        // ne fait que ralentir la macro (il ne peut pas produire de fausse solution).
        if (surs.isEmpty()) surs = candidats;
        if (surs.isEmpty()) break;

        // TIE-BREAK = CONTIGUITÉ DE RUN (mesuré, cf. l'entête). Parmi les buts sûrs :
        //  1. prolonger un segment déjà posé (garder les runs droits contigus) ;
        //  2. sinon partir d'un cul-de-sac mural (amorcer un run) ;
        //  3. sinon le plus encoigné (peu de voisins libres = ne coupe aucun passage) ;
        //  4. sinon le plus proche de l'entrée.
        int choisi = surs.first();
        for (int b : surs) {
            const int da = dist[goals[b]], dc = dist[goals[choisi]];
            const int ga = degre(b),       gc = degre(choisi);
            const auto ca = contiguite(b), cc = contiguite(choisi);
            const int pa = penalite[b],    pc = penalite[choisi];
            bool mieux;
            if ((livrDure == 1 || livrDure == 3) && pa != pc) mieux = (pa < pc);  // durci : ne pas stranguler un appui
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

    // Les buts jamais livrables (îlots, niveaux dégénérés) finissent la liste : l'ordre
    // doit rester une PERMUTATION complète, sinon butActif() rend n'importe quoi.
    for (int b = 0; b < nbButs; b++) if (!pose[b]) ordre.append(b);
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
    const QVector<int> parPrecedence = ordreParPrecedence();
    if (parPrecedence.size() == nbButs) ordreButs = parPrecedence;
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

int Game::avanceVersBut(int c, int d, int dCur, const int* dpb,
                        const QVector<bool>& zone) const {
    const int cx = c % largeur, cy = c / largeur;
    const int devx = cx + directions[d].dx, devy = cy + directions[d].dy;   // case caisse après
    const int appx = cx - directions[d].dx, appy = cy - directions[d].dy;   // appui joueur
    if (devx < 0 || devx >= largeur || devy < 0 || devy >= hauteur) return -1;
    if (appx < 0 || appx >= largeur || appy < 0 || appy >= hauteur) return -1;
    const int devant = devx + devy * largeur;
    const int appui  = appx + appy * largeur;
    if (!isLibre(devant)) return -1;         // arrivée occupée (mur / autre caisse)
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
