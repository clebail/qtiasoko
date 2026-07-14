#ifndef GAME_H
#define GAME_H

#include <QByteArray>
#include <QMetaType>
#include <QPoint>
#include <QVector>
#include "level.h"

#define NB_DIRECTION                4
#define NB_COIN_TO_CHECK            2
#define NB_MUR_TO_CHECK             2

class Game {
    // Accès aux membres privés pour les tests unitaires (tests/tst_getetat.cpp).
    friend class TestGetEtat;

public:
    typedef enum { dHaut, dDroite, dBas, dGauche} EDirection;
    typedef struct _SDirection {
        int dx, dy;
    }SDirection;

    typedef struct _SPlayerDirection {
        SDirection direction;
        int playerDirection;
    }SPlayerDirection;

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
    int getNumNiveau() const { return numNiveau; }
    bool isGagne() const { return gagne; }
    bool isPerdu() const { return perdu; }
    int getLargeur() const { return largeur; }
    int getHauteur() const { return hauteur; }
    QPoint getPlayerPoint() const { return playerPoint; }
    int getPlayerDirection() const { return playerDirection; }
    Level::ETypeCase getCase(int idx) const { return cases[idx]; }
    // Zone atteignable par le joueur sans pousser de caisse (flood-fill sur
    // les cases libres). Coûteuse à calculer : à réutiliser via les surcharges
    // ci-dessous quand plusieurs requêtes portent sur le même état (le
    // solveur appelle typiquement getEtat() ET getCaissesDeplacable() par
    // état exploré).
    QVector<bool> getZoneJoueur() const;
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
    int getHeuristique() const;
    // Applique une poussée sans faire marcher le joueur : le TÉLÉPORTE sur la
    // case d'appui, puis pousse via move() (qui fait checkVictoire/checkDefaite).
    // Précondition, NON vérifiée : la case d'appui doit être dans la zone du
    // joueur — ce que getCaissesDeplacable() garantit déjà pour tout bit qu'elle
    // pose. Permet au solveur de ne calculer le chemin de marche (coûteux) que
    // sur les enfants réellement retenus, et non sur tous les doublons.
    // 'nbDep' ne comptera qu'un coup au lieu de la marche complète.
    bool pousse(int idxCaisse, EDirection dir);
    // MACRO-POUSSÉE : pousse, puis CONTINUE tant que la caisse est dans un couloir
    // selon la direction de poussée (les deux côtés perpendiculaires sont des murs),
    // qu'elle n'est pas sur un but, et qu'elle peut encore avancer. Rend le nombre
    // de poussées effectuées (0 si la première a échoué).
    //
    // Pourquoi : dans un tel couloir, la caisse n'a plus AUCUNE alternative — le
    // joueur ne peut pas la prendre de côté, il n'y a pas la place. S'y arrêter
    // n'apporte donc rien... sauf à créer un point de branchement où le solveur peut
    // lâcher cette caisse pour aller en pousser une autre. Ces états intermédiaires
    // sont tous distincts et tous optimaux (§9.1) : la déduplication ne les attrape
    // pas, et ils font exploser l'exploration en entrelacements (§9.4).
    //
    // On s'ARRÊTE sur un but : c'est une destination légitime.
    //
    // ⚠️ Réservé à A*, qui pondère ses arêtes par le nombre de poussées. Le BFS
    // explore par couches en supposant des arêtes de coût 1 : lui donner des macros
    // de coût k lui ferait rendre des solutions NON optimales — et c'est justement
    // notre étalon d'optimalité.
    int pousseMacro(int idxCaisse, EDirection dir);
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
    void appliqueEtat(const quint16* cle);
private:
    int largeur = 0;
    int hauteur = 0;
    int size = 0;
    QPoint playerPoint;
    int playerDirection = 0;
    Level::ETypeCase *cases = nullptr;
    int nbDep = 0;
    int nbDepCaisse = 0;
    int numNiveau = 1;
    int nbCaisses = 0;
    bool gagne = false;
    bool perdu = false;
    QList<int> goals;
    QVector<bool> casesMortes;

    // couloirs[case] : bits COULOIR_H / COULOIR_V. Une caisse posée là ne peut
    // bouger QUE sur l'axe correspondant — les deux côtés perpendiculaires sont des
    // murs, le joueur ne peut pas la prendre de flanc. Statique (ne dépend que des
    // murs), calculée une fois, partagée par COW entre tous les clones.
    QVector<quint8> couloirs;

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

    bool move(EDirection dir);
    bool moveCaisse(Level::ETypeCase *cases, QPoint playerPoint, QPoint caissePoint, SDirection direction);
    void checkVictoire();
    void checkDefaite();
    short getMinIdx(const QVector<bool>& zone) const;
    bool isLibre(int idx) const;
    void calculCaseMorte();
    // Test de gel : une caisse est gelée si elle est bloquée sur LES DEUX axes.
    // 'enCours' est la garde de récursion (cf. game.cpp).
    bool caisseGelee(int idxCaisse, QVector<bool>& enCours) const;
    bool bloqueeSurAxe(int idxCaisse, EDirection dirA, EDirection dirB, QVector<bool>& enCours) const;
    bool estCaisse(int idx) const;
    void calculDistancePoussee();
    void calculCouloirs();
};

// Bits de Game::couloirs. COULOIR_H : les cases du DESSUS et du DESSOUS sont des
// murs -> une caisse ici ne glisse qu'horizontalement. COULOIR_V : idem gauche et
// droite -> elle ne glisse que verticalement.
static const quint8 COULOIR_H = 1;
static const quint8 COULOIR_V = 2;

Q_DECLARE_METATYPE(Game::EDirection)

#endif // GAME_H
