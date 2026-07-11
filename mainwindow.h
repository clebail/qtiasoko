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
private:
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
