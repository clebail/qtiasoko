#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "ui_mainwindow.h"
#include "game.h"
#include "solveur.h"

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
    void onRevoir();
    void rejouerCoup();
    void onExportPassages();
private:
    // Joue un coup et compte le passage de caisse s'il y en a un. Unique point
    // d'entrée : le clavier (humain) et le rejeu (solution) passent tous deux par
    // ici, sinon le compteur ne verrait que la moitié des poussées.
    bool joue(Game::EDirection dir);

    // Remet le compteur à l'état de DÉPART : 1 sous chaque caisse, 0 ailleurs.
    // Une caisse OCCUPE déjà sa case initiale — partir de 0 ferait afficher 0 sous
    // une caisse qui ne bouge jamais, comme si elle n'existait pas.
    void initPassages();

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

    Game game;
    Solveur *solveur = nullptr;
    QTimer timerRejeu;
    QList<Game::EDirection> coupsRestants;

    // Dernière solution trouvée pour le niveau courant, conservée intacte
    // (indépendamment de coupsRestants, consommée pendant le rejeu) pour
    // permettre de la revisionner via pbRevoir.
    Game derniereSolutionDepart;
    QList<Game::EDirection> derniereSolutionCoups;
    qint64 derniereSolutionEtats = 0;
};
#endif // MAINWINDOW_H
