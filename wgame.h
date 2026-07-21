#ifndef WGAME_H
#define WGAME_H

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>
#include "caisse.h"
#include "game.h"
#include "goal.h"
#include "goalcaisse.h"
#include "mur.h"
#include "player.h"
#include "sol.h"
#include "sprite.h"

class WGame : public QWidget
{
    Q_OBJECT

public:
    explicit WGame(QWidget *parent = nullptr);

    void setGame(const Game *game);
    void setEtatsExplores(qint64 n);
    // Nombre de fois qu'une CAISSE a été poussée SUR chaque case depuis le début
    // de la partie (cumulé : une caisse qui repasse incrémente à nouveau). Vide =
    // rien à afficher. Le compteur est tenu par MainWindow, pas par Game : le
    // solveur clone Game des millions de fois, un QVector par état ferait exploser
    // la mémoire.
    void setPassages(const QVector<int>& p);
    static QString formaterMillier(qint64 n);
    void showPassage(bool show);
    void setDuree(double duree);

    // Fait glisser le perso — et la caisse qu'il pousse — de 'depart' vers la
    // case où 'game' le montre DÉJÀ : à appeler après le coup, seul l'affichage
    // retarde. Un coup joué pendant un glissement le remplace, sans rattrapage :
    // le plateau est de toute façon à jour, seule l'image saute.
    void animerCoup(Game::EDirection dir, QPoint depart, bool poussee);

    // Durée d'un glissement. Le rejeu automatique enchaîne un coup toutes les
    // 150 ms : rester en dessous, sinon chaque animation est tronquée par la
    // suivante et le perso paraît se téléporter.
    static constexpr int dureeAnimation = 120;

    // Centre du perso en pixels, dans les coordonnées de ce widget. Interpolé
    // pendant un glissement. Sert à recaler la vue sur lui à l'ouverture d'un
    // niveau, sans attendre qu'il bouge.
    QPoint centreJoueur() const;

    // Voile d'attente pendant qu'un solveur travaille : le plateau ne bouge plus
    // et n'attend rien du joueur, autant le dire. C'est MainWindow qui décide —
    // l'affichage de l'état-max le lève, puisqu'il sert justement à suivre la
    // résolution en cours.
    void setResolution(bool enCours);

    // Taille naturelle du plateau (cases de SPRITE_WIDTH) + une bande d'une case
    // tout autour pour les règles de numéros x/y. minimumSizeHint est la clé : le
    // QScrollArea (widgetResizable) agrandit le widget pour remplir la vue quand
    // elle est plus grande, mais jamais sous ce minimum → barres de défilement dès
    // que le niveau dépasse en hauteur OU en largeur. Aucun zoom.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // Centre du perso, en pixels et dans les coordonnées de ce widget, émis à
    // chaque image d'un glissement. MainWindow s'en sert pour faire suivre la
    // vue : en 64 px le plateau dépasse largement la fenêtre.
    void joueurDeplace(QPoint centre);

protected:
    void paintEvent(QPaintEvent *event) override;
    // Arme l'infobulle qui déplie les abréviations, mais seulement au-dessus du
    // panneau de stats : ailleurs le plateau doit rester muet.
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    const Game *game = nullptr;
    qint64 etatsExplores = 0;
    QVector<int> passages;
    bool show = false;
    double duree = 0;

    // Cases atteignables par le joueur en ignorant les caisses : l'intérieur du
    // plateau. Un .xsb écrit un espace aussi bien pour un sol praticable que pour
    // le vide autour du niveau — seul ce remplissage les sépare, et c'est lui qui
    // décide entre sol grenu et sol uni. Ne dépend que des murs, donc calculé une
    // fois par plateau.
    QVector<bool> interieur;

    // Membres et non pointeurs : ces sprites n'ont pas d'état, ils ne font que
    // nommer une région de la planche. Plus de new/delete, plus d'alias à ne pas
    // double-libérer.
    Sol        solInterieur;
    SolHors    solExterieur;
    Mur        mur;
    Caisse     caisse;
    GoalCaisse caisseSurBut;
    Goal       but;
    Player     perso;

    // --- glissement en cours ---
    QTimer        timerAnim;
    QElapsedTimer chronoAnim;
    bool    animEnCours = false;
    QPointF animDepart;               // case d'où part le perso (l'arrivée est dans game)
    int     animCaisseIdx = -1;       // case d'arrivée de la caisse poussée : à ne PAS dessiner
                                      // en place, elle est encore en chemin. -1 = pas de poussée.
    int     direction = Game::dBas;   // sens du regard, choisit la pose
    int     pasMarche = 0;            // rang dans le cycle de marche, un cran par coup joué

    // Emprise du panneau de stats, telle que la dernière peinture l'a posée : il
    // suit la partie visible du plateau, donc sa position n'est connue qu'après
    // coup. Sert à savoir si la souris le survole.
    QRect rectPanneau;

    // --- voile d'attente ---
    QTimer        timerSpinner;
    QElapsedTimer chronoSpinner;
    bool    resolution = false;
    // Durée d'un tour complet du « Z » : assez lent pour évoquer le sommeil, pas
    // au point de paraître figé.
    static constexpr int cycleSpinner = 1800;

    void calculeInterieur();
    void avanceAnimation();
    void dessineSpinner(QPainter& painter, const QRect& vue);
    // Avancement du glissement, de 0 à 1. Vaut 1 hors animation.
    qreal progressionAnim() const;
    // Case du perso en coordonnées réelles : interpolée pendant un glissement,
    // entière le reste du temps.
    QPointF positionPerso() const;
    // Coin haut-gauche en pixels de la case (x,y), x et y réels. Centralise le
    // centrage du plateau dans le widget.
    QPointF coinCase(qreal x, qreal y) const;
};

#endif // WGAME_H
