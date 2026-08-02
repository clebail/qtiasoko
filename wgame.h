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

    // Champ de distances vers le but ACTIF (Game::champDistanceButActif) : le
    // gradient que descend la goal macro. 'caseBut' (index plat, -1 = aucun
    // but actif) est surligné pour montrer QUEL but ce champ vise — sans lui
    // les nombres n'ont pas de référence. Vide/-1 = rien à afficher (état
    // gagné, ou champ non recalculé).
    void setChampButActif(const QVector<int>& champ, int caseBut);
    void showChampButActif(bool show);
    // ORDRE DE REMPLISSAGE VISIBLE (2026-07-31) : quand c'est armé, le but ACTIF
    // reste bleu et tous les autres buts vides passent en SABLE. Sert à lire d'un
    // coup d'œil, sur l'état-max d'un run qui n'aboutit pas, si l'ordre de pose est
    // celui qu'on croit — c'est comme ça qu'on a vu sur le 13 que la colonne x=14
    // se remplissait à l'envers. Ne dépend PAS de la case « champ de distances » :
    // les deux surcouches sont indépendantes.
    void setMontreOrdreButs(bool on);

    // Toutes les cases visitées par AU MOINS UNE branche de l'arbre de
    // macro d'une caisse (Game::arbreMacro) : surlignage plat, une seule
    // couleur — pas un gradient de nombres, juste « ce chemin-ci marche
    // aussi ». Vide efface l'overlay.
    void setArbreMacro(const QVector<bool>& visite);

    // ZONE DU JOUEUR (2026-08-02, idée utilisateur) : aplat sur les cases qu'il
    // peut atteindre à cet instant, plus leur nombre dans le panneau (« Zj »).
    // Sert à trancher `O` contre `E` pendant l'annotation — `O` est défini par
    // l'utilisateur comme « le perso couvre plus de cases OU d'autres cases »,
    // ce qui ne se lit ni sur le seul cardinal ni sur le seul dessin.
    // Aucune donnée à tenir à jour : WGame recalcule au tracé (cf. wgame.cpp).
    void showZoneJoueur(bool on) { showZone = on; update(); }
    bool zoneJoueurVisible() const { return showZone; }

    // LA POUSSÉE COURANTE EST-ELLE ANNOTABLE ? (2026-08-02, constat utilisateur :
    // « si une poussée n'est pas la mienne, il faut que je le voie en interface, je
    // ne peux pas le deviner »). idxCase = la caisse qui vient d'être poussée, -1
    // si le coup courant n'est pas une poussée (ou hors annotation) ; 'choisie'
    // distingue un CHOIX du joueur d'une poussée de goal macro. Une intention sur
    // un coup de macro ne veut rien dire — la macro n'a rien décidé — et c'était
    // jusqu'ici lisible seulement dans le texte de la barre d'état.
    void setPousseeCourante(int idxCase, bool choisie);

    // APERÇU DES POUSSÉES QUI VIENNENT (2026-08-02, constat utilisateur : « je ne
    // vois qu'un coup à la fois, alors que pour choisir entre E et R j'ai besoin de
    // voir plusieurs coups »). ordre[case] = rang de la poussée à venir qui amènera
    // une caisse sur cette case (1 = la prochaine), -1 ailleurs ; choisi[case] dit
    // si c'est un CHOIX ou une macro. Peint en chiffres sur le plateau : on lit d'un
    // coup d'œil où vont les caisses, donc quelle intention on est en train de
    // regarder, et à quel moment cliquer pour signaler une macro manquante.
    void setApercuSuite(const QVector<int>& ordre, const QVector<bool>& choisi);

    // MODE HYBRIDE (2026-08-01) : on joue à la main pendant que l'UI montre ce
    // que le solveur ferait. Deux surcouches, armées ensemble par cette bascule.
    void setModeHybride(bool on);
    // rangs[case] = rang de remplissage du but qui occupe cette case (0 = posé en
    // premier), -1 partout ailleurs. C'est `ordreButs` rendu lisible sur le
    // plateau. STATIQUE : calculé au chargement du niveau, il ne bouge pas d'un
    // coup à l'autre — d'où un setter séparé de celui des macros.
    void setRangsButs(const QVector<int>& rangs);
    // Les goal macros jouables DANS L'ÉTAT COURANT, vers le but actif — le
    // régime d'engagement du solveur, rejoué à la main (cf. MainWindow).
    // 'trajets' = union des cases traversées (aplat bleu, même code que
    // l'arbre de macro) ; 'caisses' = les caisses qui les amorcent, cerclées
    // parce que ce sont elles qu'il faut cliquer pour lancer la macro. Vides
    // = aucune macro jouable, l'overlay disparaît (l'information la plus utile
    // du mode : le solveur retomberait ici sur des poussées simples).
    void setMacrosJouables(const QVector<bool>& trajets, const QVector<bool>& caisses);
    // Les cases SIGNALÉES à la main (clic droit) : « ici, il aurait dû se passer
    // quelque chose ». Cerclées de rouge, effacées au coup suivant — un repère ne
    // vaut que pour l'état où il a été posé.
    void setSignales(const QVector<bool>& cases);

    // Fait glisser le perso — et la caisse qu'il pousse — de 'depart' vers la
    // case où 'game' le montre DÉJÀ : à appeler après le coup, seul l'affichage
    // retarde. Un coup joué pendant un glissement le remplace, sans rattrapage :
    // le plateau est de toute façon à jour, seule l'image saute.
    void animerCoup(Game::EDirection dir, QPoint depart, bool poussee);

    // Coupe net un glissement en cours (undo : on saute directement à l'état
    // restauré, sans laisser l'animation redessiner une position périmée).
    void arreteAnimation();

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

    // Case cliquée (index plat), pour le diagnostic « chemin de macro » : un
    // clic sur une caisse doit pouvoir déclencher l'affichage de son trajet
    // complet vers le but actif. WGame ne sait pas lire le plateau pour filtrer
    // (juste convertir une position écran en case) : à MainWindow de décider
    // si c'est une caisse jouable.
    void caseCliquee(int idx);

    // Case SIGNALÉE (clic droit) : « il manque quelque chose ici ». Sert à
    // consigner dans le journal un désaccord entre le joueur et le solveur —
    // typiquement une caisse dont on attendait une goal macro. WGame ne juge
    // rien, il ne fait que rapporter la case.
    void caseSignalee(int idx);

protected:
    void paintEvent(QPaintEvent *event) override;
    // Arme l'infobulle qui déplie les abréviations, mais seulement au-dessus du
    // panneau de stats : ailleurs le plateau doit rester muet.
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    const Game *game = nullptr;
    qint64 etatsExplores = 0;
    QVector<int> passages;
    bool show = false;
    double duree = 0;

    QVector<int> champButActif;
    int caseButActif = -1;
    bool showChamp = false;
    bool montreOrdreButs = false;
    QVector<bool> arbreMacro;
    bool showZone = false;
    int  caissePoussee = -1;
    bool pousseeChoisie = false;
    QVector<int>  apercuOrdre;
    QVector<bool> apercuChoisi;

    bool modeHybride = false;
    QVector<int>  rangsButs;
    QVector<bool> macroTrajets;
    QVector<bool> macroCaisses;
    QVector<bool> signales;

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
