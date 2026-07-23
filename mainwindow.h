#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "ui_mainwindow.h"
#include "game.h"
#include "solveur.h"
#include<iostream>

using namespace std;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private slots:
    void onNiveauChange(int index);
    void onIALance();
    void onSolutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores);
    void onAucuneSolution();
    // La recherche a été interrompue par un second clic sur le bouton.
    void onArretRecherche(qint64 etatsExplores);
    void onRevoir();
    void rejouerCoup();
    void onExportPassages();
    void onExportXsb();
    void onShowPassagesCaisse();
    void onShowChampButActif();
    // Clic sur une caisse (WGame::caseCliquee) : montre son trajet de macro
    // COMPLET vers le but actif, au lieu du champ par défaut (toutes les
    // caisses, un pas). Sans effet sur une autre case (mur, sol, joueur...).
    void onCaseCliquee(int idx);
    // Reçoit du solveur l'état où un nouveau record de caisses posées est atteint.
    void onNouveauMax(Game etatMax, int nbRangees);
    // Bascule l'affichage entre le plateau courant et l'état-max mémorisé.
    void onToggleEtatMax(int state);
    // Fait suivre la vue au perso pendant qu'il se déplace.
    void onJoueurDeplace(QPoint centre);
private:
    // Recale la vue sur le perso sans qu'il ait à bouger (ouverture d'un niveau,
    // retour au départ avant un rejeu). Différé d'un tour de boucle : au moment
    // où on l'appelle, le QScrollArea n'a pas encore redimensionné WGame pour le
    // nouveau plateau, et le centre calculé serait celui de l'ancien.
    void centrerSurJoueur();

    // Arme ou lève le voile d'attente. Un seul endroit qui en décide : il dépend
    // de DEUX choses (un solveur tourne, et l'utilisateur ne regarde pas
    // l'état-max), et les quatre appelants oublieraient l'une ou l'autre.
    void majSpinner();

    // Joue un coup et compte le passage de caisse s'il y en a un. Unique point
    // d'entrée : le clavier (humain) et le rejeu (solution) passent tous deux par
    // ici, sinon le compteur ne verrait que la moitié des poussées.
    bool joue(Game::EDirection dir);

    // Défait le dernier coup joué À LA MAIN (touche Retour arrière) : restaure
    // l'état et les compteurs de passage empilés avant ce coup.
    void annuleCoup();

    // Remet le compteur à l'état de DÉPART : 1 sous chaque caisse, 0 ailleurs.
    // Une caisse OCCUPE déjà sa case initiale — partir de 0 ferait afficher 0 sous
    // une caisse qui ne bouge jamais, comme si elle n'existait pas.
    void initPassages();

    // Recalcule le champ de distances vers le but actif de 'g' et le pousse à
    // wGame. À rappeler à chaque fois que le plateau AFFICHÉ change d'état
    // (coup joué, changement de niveau, bascule état-max) — contrairement à
    // 'passages', ce champ n'est pas cumulatif, il ne vaut que pour l'état
    // courant. Sans effet si la checkbox est décochée (juste un peu de calcul
    // perdu, jamais faux) : plus simple que de dupliquer la garde partout.
    void majChampButActif(const Game& g);

    // passages[case] = nombre de fois qu'une caisse a été poussée SUR cette case
    // depuis le chargement du niveau. CUMULÉ : une caisse qui repasse au même
    // endroit incrémente à nouveau.
    //
    // Vit ici et non dans Game : le solveur clone Game des millions de fois, un
    // QVector par état ferait exploser la mémoire (cf. étape 11).
    QVector<int> passages;

    // Rôle du numéro de niveau dans cbNiveau (Qt::UserRole sert déjà au nom de fichier).
    static constexpr int RoleNumero = Qt::UserRole + 1;

    void setControlesActifs(bool actifs);

    // Recale le bouton de résolution (libellé + activation) sur l'état réel.
    // Il est le seul contrôle à ne PAS suivre setControlesActifs() : pendant une
    // résolution tout est verrouillé, mais lui doit rester cliquable — c'est le
    // seul moyen de rendre la main.
    void majBoutonResoudre();

    // Libellé de repos de pbResoudre, capté depuis mainwindow.ui : le bouton
    // bascule en « Arrêter » pendant la recherche et doit pouvoir y revenir.
    QString texteResoudre;

    // Remet cbEtatMax au repos (décochée, désactivée, libellé sans compteur).
    void resetEtatMax();

    // Libellé de cbEtatMax tel que déclaré dans mainwindow.ui, capté au démarrage :
    // unique source du texte, pour qu'un renommage dans Designer suffise.
    QString texteEtatMax;

    Game game;

    // Historique des coups joués À LA MAIN, pour l'undo (Retour arrière). On empile
    // une COPIE de 'game' (COW : les tables statiques sont partagées, seul l'état de
    // jeu diffère) + les compteurs de passage, AVANT chaque coup humain. Le rejeu
    // automatique (timerRejeu) n'y touche pas ; vidé au (re)chargement du niveau.
    struct CoupHist { Game etat; QVector<int> passages; };
    QVector<CoupHist> historique;

    // État où le solveur a rangé le plus de caisses (diagnostic §10). Mémorisé
    // pour rester valide tant que WGame le pointe.
    Game gameMax;
    int maxRangeesVu = 0;
    Solveur *solveur = nullptr;
    QTimer timerRejeu;
    QList<Game::EDirection> coupsRestants;

    // Dernière solution trouvée pour le niveau courant, conservée intacte
    // (indépendamment de coupsRestants, consommée pendant le rejeu) pour
    // permettre de la revisionner via pbRevoir.
    Game derniereSolutionDepart;
    QList<Game::EDirection> derniereSolutionCoups;
    qint64 derniereSolutionEtats = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> begin;
};
#endif // MAINWINDOW_H
