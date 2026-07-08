#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi(this);
    installEventFilter(this);

    // Sinon le combo/bouton gardent le focus clavier après un clic, et les
    // flèches ne remontent plus jusqu'à l'eventFilter pour faire jouer le joueur.
    cbNiveau->setFocusPolicy(Qt::NoFocus);
    pbIA->setFocusPolicy(Qt::NoFocus);
    pbRevoir->setFocusPolicy(Qt::NoFocus);

    const QStringList fichiers = QDir::current().entryList(QStringList() << "level????.xsb", QDir::Files, QDir::Name);
    for (const QString& fichier : fichiers) {
        bool ok = false;
        int numero = fichier.mid(5, 4).toInt(&ok);
        cbNiveau->addItem(QString("Niveau %1").arg(ok ? numero : cbNiveau->count() + 1), fichier);
    }

    connect(cbNiveau, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onNiveauChange);
    connect(pbIA, &QPushButton::clicked, this, &MainWindow::onIALance);
    connect(pbRevoir, &QPushButton::clicked, this, &MainWindow::onRevoir);
    connect(&timerRejeu, &QTimer::timeout, this, &MainWindow::rejouerCoup);
    timerRejeu.setInterval(150);

    if (cbNiveau->count() > 0) {
        onNiveauChange(0);
    }
}

MainWindow::~MainWindow() {
}

void MainWindow::onNiveauChange(int index) {
    if (index < 0) return;

    Level lvl;
    lvl.load(cbNiveau->itemData(index).toString());
    game = Game(lvl, index + 1);

    derniereSolutionCoups.clear();
    pbRevoir->setEnabled(false);

    wGame->setGame(&game);
    wGame->setEtatsExplores(0);
    wGame->update();
}

void MainWindow::onIALance() {
    if (solveur) return;   // résolution déjà en cours

    cbNiveau->setEnabled(false);
    pbIA->setEnabled(false);

    solveur = new Solveur(game, this);
    connect(solveur, &Solveur::solutionTrouvee, this, &MainWindow::onSolutionTrouvee);
    connect(solveur, &Solveur::aucuneSolution, this, &MainWindow::onAucuneSolution);
    connect(solveur, &QThread::finished, solveur, &QObject::deleteLater);
    solveur->start();
}

void MainWindow::onSolutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores) {
    solveur = nullptr;

    // 'game' n'a pas encore bougé (le solveur travaillait sur sa propre copie) :
    // c'est le point de départ à conserver pour pouvoir revisionner plus tard.
    derniereSolutionDepart = game;
    derniereSolutionCoups = chemin;
    derniereSolutionEtats = etatsExplores;
    pbRevoir->setEnabled(true);

    wGame->setEtatsExplores(etatsExplores);

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, "Solveur",
        QString("Solution trouvée : %1 coups (%2 états explorés).\nVoir la résolution ?")
            .arg(chemin.size())
            .arg(WGame::formaterMillier(etatsExplores)));

    if (reponse == QMessageBox::Yes) {
        coupsRestants = chemin;
        timerRejeu.start();   // laisse cbNiveau/pbIA désactivés le temps du rejeu
    } else {
        cbNiveau->setEnabled(true);
        pbIA->setEnabled(true);
    }
}

void MainWindow::onAucuneSolution() {
    solveur = nullptr;
    cbNiveau->setEnabled(true);
    pbIA->setEnabled(true);
    wGame->setEtatsExplores(0);
    QMessageBox::information(this, "Solveur", "Aucune solution trouvée pour ce niveau.");
}

void MainWindow::onRevoir() {
    if (timerRejeu.isActive() || derniereSolutionCoups.isEmpty()) return;

    game = derniereSolutionDepart;
    coupsRestants = derniereSolutionCoups;
    wGame->setGame(&game);
    wGame->setEtatsExplores(derniereSolutionEtats);

    cbNiveau->setEnabled(false);
    pbIA->setEnabled(false);
    pbRevoir->setEnabled(false);

    timerRejeu.start();
}

void MainWindow::rejouerCoup() {
    if (coupsRestants.isEmpty()) {
        timerRejeu.stop();
        cbNiveau->setEnabled(true);
        pbIA->setEnabled(true);
        pbRevoir->setEnabled(!derniereSolutionCoups.isEmpty());
        return;
    }

    Game::EDirection dir = coupsRestants.takeFirst();
    game.deplace(dir);
    wGame->update();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        if (timerRejeu.isActive()) {
            return QObject::eventFilter(obj, event);   // laisse la souris/clavier tranquille pendant le rejeu
        }

        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        bool moved = false;

        switch (keyEvent->key()) {
            case Qt::Key_Up:    moved = game.haut();   break;
            case Qt::Key_Right: moved = game.droite(); break;
            case Qt::Key_Down:  moved = game.bas();    break;
            case Qt::Key_Left:  moved = game.gauche(); break;
            default: break;
        }

        if (moved) {
            wGame->setEtatsExplores(0);   // l'humain joue, plus de stat IA à afficher
            wGame->update();
        }
    }

    return QObject::eventFilter(obj, event);
}
