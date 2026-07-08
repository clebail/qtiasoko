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
    QByteArray getEtat() const { return getEtat(getZoneJoueur()); }
    QByteArray getEtat(const QVector<bool>& zone) const;
    QVector<quint8> getCaissesDeplacable() const { return getCaissesDeplacable(getZoneJoueur()); }
    QVector<quint8> getCaissesDeplacable(const QVector<bool>& zone) const;
    bool isLibre(const QPoint& p) const;
    // Somme, pour chaque caisse actuellement sur le plateau, de sa distance
    // (en tirages) au but le plus proche — ignore les autres caisses, donc
    // toujours <= au coût réel : heuristique admissible pour A*/IDA*.
    int getHeuristique() const;
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
    bool gagne = false;
    bool perdu = false;
    QList<int> goals;
    QVector<bool> casesMortes;
    QVector<int> distanceButs;

    bool move(EDirection dir);
    bool moveCaisse(Level::ETypeCase *cases, QPoint playerPoint, QPoint caissePoint, SDirection direction);
    void checkVictoire();
    void checkDefaite();
    short getMinIdx(const QVector<bool>& zone) const;
    bool isLibre(int idx) const;
    void calculCaseMorte();
};

Q_DECLARE_METATYPE(Game::EDirection)

#endif // GAME_H
