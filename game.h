#ifndef GAME_H
#define GAME_H

#include <QByteArray>
#include <QMetaType>
#include <QPair>
#include <QPoint>
#include <QVector>
#include "level.h"

#define NB_DIRECTION                4
#define NB_COIN_TO_CHECK            2
#define NB_MUR_TO_CHECK             2

#ifdef INSTRUM_MACRO
#include <vector>
// Instrumentation hors-ligne de la GOAL MACRO (harnais mesures/macro). Ne compile
// que dans les harnais : le code produit ne définit jamais INSTRUM_MACRO.
//
// Question posée : quand macroVersBut() échoue, est-ce parce que la caisse est
// VRAIMENT bloquée, ou parce que la descente a pris arbitrairement l'une des
// plusieurs descentes optimales possibles et s'est peinte dans un coin ? La
// boucle prend la première direction décroissante dans l'ordre de l'énumération
// et ne revient jamais dessus — 'forks' compte les pas où une ALTERNATIVE de même
// coût existait, donc les points où un backtracking aurait eu de quoi mordre.
struct StatsMacro {
    qint64 tentatives = 0, succes = 0;
    qint64 echecRegion = 0;     // le joueur n'est plus dans une région valide
    qint64 echecDistance = 0;   // but inatteignable depuis cette caisse (d < 0)
    qint64 echecBloque = 0;     // aucune poussée n'avance  <- LE CAS INTÉRESSANT
    qint64 echecPousse = 0;     // pousse() a refusé (deadlock au passage)
    qint64 echecAvecFork = 0;   // échec ALORS QU'un choix arbitraire avait eu lieu
    qint64 succesAvecFork = 0;
    qint64 forksTotal = 0;      // nombre de pas offrant >= 2 descentes optimales
    qint64 pasTotal = 0;        // pas réellement joués (succès + échecs)
    qint64 resteAuBlocage = 0;  // somme des distances restantes au moment du blocage
    std::vector<qint64> histoEchecPas;    // à quel pas l'échec survient
    std::vector<qint64> histoSuccesLong;  // longueur des chaînes réussies
};
StatsMacro& statsMacro();
#endif

class Game {
    // Accès aux membres privés pour les tests unitaires (tests/tst_getetat.cpp).
    friend class TestGetEtat;

public:
    typedef enum { dHaut, dDroite, dBas, dGauche} EDirection;
    typedef struct _SDirection {
        int dx, dy;
    }SDirection;

    Game();
    Game(const Level& level, int numNiveau = 1);
    Game(const Game& other);
    Game& operator=(const Game& other);

    // Sémantique de déplacement : vole le tableau 'cases' de 'other', puis remet
    // son pointeur à nullptr. Pas de 'const' sur le paramètre — on doit MODIFIER
    // la source pour lui prendre son pointeur ; un 'const Game&&' compilerait
    // mais forcerait une copie profonde, donc ne servirait à rien.
    //
    // 'noexcept' n'est pas décoratif : std::vector n'utilise le déplacement lors
    // d'une réallocation QUE si le move ctor est noexcept — sinon il recopie
    // silencieusement, pour conserver sa garantie forte en cas d'exception. Sans
    // ce mot-clé, le vecteur qui porte le tas d'A* recopierait profondément tous
    // ses états à chaque doublement, et le move ctor n'aurait servi à rien.
    Game(Game&& other) noexcept;
    Game& operator=(Game&& other) noexcept;

    ~Game();
    bool isLoaded() const;
    bool haut();
    bool droite();
    bool bas();
    bool gauche();
    bool deplace(EDirection dir) { return move(dir); }
    int getNbDep() const { return nbDep; }
    int getNbDepCaisse() const { return nbDepCaisse; }
    int getNbButs() const { return nbButs; }
    // Nombre de caisses posées sur un but. Recompte (O(size)) — pour le BFS, qui
    // porte des Game complets ; A* utilise le retour d'appliqueEtat, gratuit.
    int nbCaissesSurBut() const;
    int getNumNiveau() const { return numNiveau; }
    bool isGagne() const { return gagne; }
    bool isPerdu() const { return perdu; }
    int getLargeur() const { return largeur; }
    int getHauteur() const { return hauteur; }
    QPoint getPlayerPoint() const { return playerPoint; }
    Level::ETypeCase getCase(int idx) const { return cases[idx]; }
    // Zone atteignable par le joueur sans pousser de caisse (flood-fill sur
    // les cases libres). Coûteuse à calculer : à réutiliser via les surcharges
    // ci-dessous quand plusieurs requêtes portent sur le même état (le
    // solveur appelle typiquement getEtat() ET getCaissesDeplacable() par
    // état exploré).
    QVector<bool> getZoneJoueur() const { QVector<bool> v; getZoneJoueur(v); return v; }
    // Même chose dans un tampon FOURNI, réutilisable d'un appel à l'autre : à
    // taille déjà bonne, plus aucune allocation (ni le QVector rendu, ni la file
    // du parcours). C'est la forme du chemin chaud — le flood-fill est le point
    // le plus appelé du solveur. ⚠️ Le tampon doit être détenu en propre : si une
    // copie du QVector traîne ailleurs, le fill() initial détache et réalloue,
    // ce qui annule tout le bénéfice (sans nuire à la correction).
    void getZoneJoueur(QVector<bool>& visite) const;
    // Longueur de la clé d'état, en shorts : les N caisses + la case canonique du
    // joueur. CONSTANTE sur toute une résolution — aucune caisse n'apparaît ni ne
    // disparaît —, ce qui permet au solveur de ranger toutes ses clés bout à bout
    // dans une arène (cf. cle.h).
    int tailleCle() const { return nbCaisses + 1; }
    // Écrit la clé dans 'cle', qui doit pouvoir accueillir tailleCle() shorts.
    // C'est la forme qu'utilise le solveur : elle écrit directement dans l'arène,
    // sans allouer.
    void getEtat(quint16* cle) const { getEtat(cle, getZoneJoueur()); }
    void getEtat(quint16* cle, const QVector<bool>& zone) const;
    // Même clé, en QByteArray (short big-endian par case). Ne sert plus au
    // solveur — un malloc par clé était son principal poste mémoire —, mais reste
    // commode pour comparer deux états à l'unité, dans les tests.
    QByteArray getEtat() const { return getEtat(getZoneJoueur()); }
    QByteArray getEtat(const QVector<bool>& zone) const;
    QVector<quint8> getCaissesDeplacable() const { return getCaissesDeplacable(getZoneJoueur()); }
    QVector<quint8> getCaissesDeplacable(const QVector<bool>& zone) const;
    bool isLibre(const QPoint& p) const;
    // Borne inférieure du nombre de poussées restantes, par COUPLAGE de coût
    // minimal (algorithme hongrois) entre les caisses et les buts.
    //
    // Chaque coût cout[caisse][but] est la distance EXACTE d'une caisse SEULE vers
    // CE but précis, tenant compte de l'accessibilité du joueur (distanceParBut,
    // sous-produit joueur-aware : une caisse coupe le plateau, et selon le côté où
    // il se trouve elle n'est pas poussable dans les mêmes sens). On prend ensuite
    // l'affectation bijective caisses<->buts de somme minimale.
    //
    // Admissible (§7.2) : toute solution réelle réalise une bijection caisses<->buts
    // dont le coût est >= la somme de cette affectation ; retirer les autres caisses
    // ne fait que LIBÉRER le joueur. Domine strictement l'ancienne « chaque caisse
    // vise son but le plus proche » (qui relâchait la contrainte de distinction) :
    // elle corrige les COLLISIONS de buts (N caisses réclamant le même but), erreur
    // dominante des niveaux à beaucoup de caisses.
    // Coupe approchée (§10.5) : vrai si les buts sont remplis dans l'ordre de
    // PROFONDEUR (ordreButs) — un but rempli ne suit jamais un but vide plus
    // profond. Interdit les rangements « dans le désordre » de la salle de buts.
    // ⚠️ Peut rendre insoluble un niveau dont l'ordre optimal contrarie celui-ci
    // (rare, ~1/32 d'après le jeu à la main) → à utiliser en mode anytime avec un
    // repli sans coupe.
    bool remplissageOrdonne() const;
    // Index du but ACTIF (§10.5) : le plus profond (ordreButs) pas encore rempli,
    // ou -1 si tous le sont (état gagnant). C'est la cible de la goal macro.
    int butActif() const;
    // GOAL MACRO (§10.5) : pousse la caisse en 'idxCaisse' jusqu'au but d'index
    // 'indexBut', le long de son trajet solo, en vérifiant à CHAQUE pas que la
    // poussée est réellement jouable dans l'état courant (case d'arrivée libre,
    // joueur capable d'atteindre l'appui — les autres caisses comptent). Joue les
    // poussées sur *this et les empile dans 'poussees' ((case de la caisse, dir))
    // pour la reconstruction. Rend true si le but est atteint ; false si la caisse
    // se bloque en route (l'état est alors partiellement modifié — l'appelant
    // travaille sur une copie jetable).
    // 'zoneInitiale' (optionnel) : la zone du joueur pour l'état COURANT, quand
    // l'appelant l'a déjà sous la main. Elle n'est valable que pour le premier
    // pas — dès qu'une caisse bouge, la macro la recalcule elle-même. Le solveur
    // essaie une macro par caisse candidate (~5 par état) et dispose déjà de
    // cette zone : sans ce paramètre, la moitié des flood-fills du solveur sont
    // des recalculs à l'identique.
    bool macroVersBut(int idxCaisse, int indexBut, QVector<QPair<int,int>>& poussees,
                      const QVector<bool>* zoneInitiale = nullptr);
    // Filtre bon marché : la macro pourrait-elle faire AU MOINS UN pas ? Répond
    // sans copier le Game ni rien modifier — c'est le premier pas de
    // macroVersBut, dont il partage la condition exacte (avanceVersBut). Un
    // 'false' garantit que macroVersBut échouerait au pas 0.
    bool macroPeutDemarrer(int idxCaisse, int indexBut, const QVector<bool>& zone) const;
    int getHeuristique() const { return getHeuristique(nullptr); }
    // Surcharge : calcule aussi le SCORE DE GUIDAGE (§10.2) via l'appariement du
    // couplage. Ordre lexicographique des distances-restantes par but (priorité =
    // index du but) : à f et g égaux, A* préfère le score le plus PETIT, ce qui
    // impose un ordre canonique de rangement et casse la multiplicité (§9.4).
    // Pur tie-break : sans effet sur l'optimalité. scoreGuidage peut être nul.
    int getHeuristique(qint64* scoreGuidage) const;
    // Applique une poussée sans faire marcher le joueur : le TÉLÉPORTE sur la
    // case d'appui, puis pousse via move() (qui fait checkVictoire/checkDefaite).
    // Précondition, NON vérifiée : la case d'appui doit être dans la zone du
    // joueur — ce que getCaissesDeplacable() garantit déjà pour tout bit qu'elle
    // pose. Permet au solveur de ne calculer le chemin de marche (coûteux) que
    // sur les enfants réellement retenus, et non sur tous les doublons.
    // 'nbDep' ne comptera qu'un coup au lieu de la marche complète.
    bool pousse(int idxCaisse, EDirection dir);
    // Réécrit le plateau à partir d'une clé getEtat() : les caisses et le joueur
    // sont replacés, les murs et les buts ne bougent pas. Permet au solveur de ne
    // PAS transporter un Game complet (~700 o) dans sa file ouverte, mais juste la
    // clé (~22 o) — la file d'A* est le principal poste mémoire.
    //
    // Le joueur est replacé sur la case CANONIQUE de sa zone (le min des ids
    // atteignables, cf. getMinIdx), pas forcément là où il était. Sans effet :
    // getEtat() normalise déjà à cette case, pousse() téléporte, et
    // checkVictoire()/checkDefaite() ne dépendent que des caisses.
    //
    // 'cle' pointe sur tailleCle() shorts, au format de getEtat().
    // Renvoie le nombre de caisses posées sur un but (compté gratuitement pendant
    // le placement), pour la jauge de progression du diagnostic (§10).
    int appliqueEtat(const quint16* cle);
    // Deadlock de LIVRAISON (§6.1) : vrai s'il reste un but VIDE qu'aucune caisse
    // ne peut plus atteindre. 'variante' choisit la relaxation (cf. game.cpp) ;
    // 0 = celle de la variable d'environnement LIVRAISON (défaut : test COUPÉ).
    //
    // ⚠️ DÉSACTIVÉ, ET POUR CAUSE (mesuré le 2026-07-21 avec mesures/fp.cpp, qui
    // rejoue une solution GAGNANTE et interroge le test sur chacun de ses états —
    // tous solubles par construction, donc toute détection est un faux positif
    // PROUVÉ). Deux défauts indépendants :
    //   1. le BFS de livraison n'est PAS joueur-aware — il ne retient qu'UNE
    //      position de joueur par case atteinte, alors qu'une même case atteinte
    //      « par l'autre côté » ouvre d'autres poussées. C'est la faille du
    //      prototype mesures/mort.cpp, et elle rend 86 faux positifs sur le 17 ;
    //   2. tenir les caisses posées pour des obstacles fixes est faux, même
    //      restreint aux caisses GELÉES (1 faux positif sur le 2).
    // Seule la variante 3, qui lit distanceParBut (joueur-aware, elle), est sûre —
    // et elle ne capture rien de plus que staticDeadlock.
    //
    // PUBLIC parce que le point d'appel naturel, checkDefaite, est le mauvais :
    // marquer 'perdu' sur un état INTERMÉDIAIRE de goal macro fait avorter la
    // macro entière (move() refuse de jouer sur un état perdu) — niveaux 3 et 5
    // perdus. Le solveur peut donc l'appeler sur les états qu'il ENFILE.
    bool butNonLivrable(int variante = 0) const;
private:
    int largeur = 0;
    int hauteur = 0;
    int size = 0;
    QPoint playerPoint;
    Level::ETypeCase *cases = nullptr;
    int nbDep = 0;
    int nbDepCaisse = 0;
    int numNiveau = 1;
    int nbCaisses = 0;
    bool gagne = false;
    bool perdu = false;
    QList<int> goals;
    QVector<bool> casesMortes;

    // regions[CASE * size + CAISSE] = id de la composante connexe de CASE, sur un
    // plateau où le seul obstacle (hors murs) est une caisse posée en CAISSE.
    //
    // ⚠️ L'ordre des indices n'est PAS arbitraire : la CASE est en index majeur.
    // Le chemin chaud (getHeuristique, checkDefaite) interroge toujours avec la
    // case du JOUEUR fixe et la caisse qui varie — cet ordre rend ces lectures
    // contiguës. L'ordre inverse coûtait un défaut de cache par caisse, et
    // doublait le temps par état.
    QVector<qint16> regions, nbRegions;
    QVector<int> distancePoussee;

    // distanceParBut[(BUT * size + CASE) * maxRegions + REGION] = nombre minimal de
    // poussées pour amener une caisse SEULE de CASE vers CE but précis, le joueur
    // étant dans REGION (composante de plateau-moins-cette-caisse). Une table par
    // but, calculée par un BFS à rebours par but (cf. calculDistancePoussee).
    //
    // distancePoussee en est le min sur les buts — même valeur qu'un BFS multi-but
    // simultané, donc casesMortes/checkDefaite sont inchangés. distanceParBut sert
    // au couplage hongrois de getHeuristique() : cout[caisse][but] direct.
    QVector<int> distanceParBut;
    int nbButs = 0;
    int maxRegions = 0;

    // Ordre de REMPLISSAGE des buts (§10.5) : ordreButs[k] = indice du but à
    // remplir en k-ième. Les plus PROFONDS d'abord (coins, culs-de-sac où une
    // caisse serait gelée), en remontant vers l'entrée. Profondeur = distance de
    // poussée depuis la caisse la plus proche. Statique, partagé par COW.
    QVector<int> ordreButs;

    bool move(EDirection dir);
    bool moveCaisse(Level::ETypeCase *cases, QPoint playerPoint, QPoint caissePoint, SDirection direction);
    void checkVictoire();
    void checkDefaite();
    bool staticDeadlock(int idxCaisse, int idxJoueur,  QVector<bool>& enCours) const;
    bool dynamicDeadlock(int idxCaisse) const;
    short getMinIdx(const QVector<bool>& zone) const;
    bool isLibre(int idx) const;
    void calculCaseMorte();
    // Test de gel : une caisse est gelée si elle est bloquée sur LES DEUX axes.
    // 'enCours' est la garde de récursion (cf. game.cpp).
    bool caisseGelee(int idxCaisse, QVector<bool>& enCours) const;
    bool bloqueeSurAxe(int idxCaisse, EDirection dirA, EDirection dirB, QVector<bool>& enCours) const;
    bool estCaisse(int idx) const;
    // Une poussée de la caisse en 'c' vers 'd' la fait-elle AVANCER vers le but
    // (distance dCur -> dCur-1), le joueur pouvant se mettre à l'appui ? Rend la
    // case d'arrivée, ou -1. 'dpb' = la tranche de distanceParBut du but visé.
    // Exemplaire UNIQUE de la condition de descente : macroVersBut et
    // macroPeutDemarrer s'en servent tous les deux (cf. game.cpp).
    int avanceVersBut(int c, int d, int dCur, const int* dpb,
                      const QVector<bool>& zone) const;
    void calculDistancePoussee();

// Distance de livraison d'une caisse (poussées) depuis les caisses de départ, les
// buts marqués dans `bloque` faisant obstacle. -1 = inatteignable.
QVector<int> distanceLivraison(const QVector<bool>& bloque) const;

// Ordre de remplissage déduit de la PRÉCÉDENCE DE LIVRAISON (§6.2, 2026-07-20) :
// glouton avant + garde anti-échouage. Rend une permutation des indices de buts.
QVector<int> ordreParPrecedence() const;
};

Q_DECLARE_METATYPE(Game::EDirection)
// Permet de transporter un Game complet par signal queued (thread solveur -> UI),
// pour l'affichage de l'état-max (§10, diagnostic). Copie profonde du plateau, mais
// les tables statiques sont en COW : le coût reste modéré et l'émission est rare.
Q_DECLARE_METATYPE(Game)

#endif // GAME_H
