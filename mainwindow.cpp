#include <cmath>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTextStream>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi(this);
    installEventFilter(this);

    // Libellé de repos de cbEtatMax, capté depuis le .ui : c'est la seule copie du
    // texte, resetEtatMax() y revient après l'avoir suffixé du compteur (n/total).
    texteEtatMax = cbEtatMax->text();

    for (const Solveur::SType& t : Solveur::types()) {
        cbSolveur->addItem(t.libelle, static_cast<int>(t.type));
    }

    // Niveaux lus depuis les ressources et non depuis QDir::current() : le
    // répertoire courant n'est pas celui des sources (shadow build de Qt Creator,
    // ou '/' quand le .app est lancé depuis le Finder) et aucun niveau n'était
    // trouvé. Embarqués dans le binaire, ils sont indépendants du cwd.
    //
    // Le numéro du fichier est la seule source de vérité : il nomme l'entrée du
    // combo *et* alimente Game, sinon l'overlay de WGame affiche un autre numéro
    // que celui sélectionné.
    const QDir dossier(":/levels");
    const QStringList fichiers = dossier.entryList(QStringList() << "level????.xsb", QDir::Files, QDir::Name);
    for (const QString& fichier : fichiers) {
        bool ok = false;
        const int numero = fichier.mid(5, 4).toInt(&ok);
        if (!ok) continue;   // les ???? du filtre ne sont pas forcément des chiffres

        cbNiveau->addItem(QString("Niveau %1").arg(numero), dossier.filePath(fichier));
        cbNiveau->setItemData(cbNiveau->count() - 1, numero, RoleNumero);
    }

    connect(cbNiveau, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onNiveauChange);
    connect(pbResoudre, &QPushButton::clicked, this, &MainWindow::onIALance);
    connect(pbRevoir, &QPushButton::clicked, this, &MainWindow::onRevoir);
    connect(pbExport, &QPushButton::clicked, this, &MainWindow::onExportPassages);
    connect(pbExportXsb, &QPushButton::clicked, this, &MainWindow::onExportXsb);
    connect(cbNotePassages, &QCheckBox::stateChanged, this, &MainWindow::onShowPassagesCaisse);
    connect(cbEtatMax, &QCheckBox::stateChanged, this, &MainWindow::onToggleEtatMax);
    connect(&timerRejeu, &QTimer::timeout, this, &MainWindow::rejouerCoup);
    timerRejeu.setInterval(150);

    // Game transporté par signal queued (thread solveur -> UI) pour l'état-max.
    qRegisterMetaType<Game>("Game");

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
    game = Game(lvl, cbNiveau->itemData(index, RoleNumero).toInt());

    derniereSolutionCoups.clear();
    pbRevoir->setEnabled(false);

    // Nouveau niveau : l'état-max du précédent n'a plus de sens.
    resetEtatMax();

    initPassages();

    wGame->setGame(&game);
    wGame->setEtatsExplores(0);
    wGame->setDuree(0);
    wGame->setPassages(passages);
    wGame->update();
}

void MainWindow::initPassages() {
    const int size = game.getLargeur() * game.getHauteur();
    passages = QVector<int>(size, 0);

    // Une caisse occupe déjà sa case de départ : le compte y démarre à 1.
    for (int i = 0; i < size; i++) {
        const Level::ETypeCase c = game.getCase(i);
        if (c == Level::tcCaisse || c == Level::tcGoalCaisse) passages[i] = 1;
    }
}

// Un coup, et le comptage du passage de caisse s'il y en a un.
bool MainWindow::joue(Game::EDirection dir) {
    const QPoint avant  = game.getPlayerPoint();
    const int    caisses = game.getNbDepCaisse();

    if (!game.deplace(dir)) return false;

    const QPoint apres = game.getPlayerPoint();
    const bool poussee = game.getNbDepCaisse() > caisses;

    if (poussee) {
        // Le joueur a avancé d'une case ; la caisse qu'il vient de pousser est
        // juste devant lui, dans le même sens.
        const QPoint delta = apres - avant;
        const QPoint caisse = apres + delta;

        const int idx = caisse.x() + caisse.y() * game.getLargeur();
        if (idx >= 0 && idx < passages.size()) passages[idx]++;

        wGame->setPassages(passages);

        // Trace brute des mouvements (§6.2, session du 2026-07-20) : rejoue le
        // niveau 11/190/191/192 à la main devant cette sortie, puis copie la
        // console — ça donne le trajet RÉEL entre deux poses, pas juste l'ordre
        // final. Déjà perdu une fois sur un checkout, donc qDebug plutôt qu'un
        // export : rien à retirer avant de commit.
        const bool posee = game.getCase(idx) == Level::tcGoalCaisse;
        qDebug().noquote() << QString("[mouv] joueur (%1,%2)->(%3,%4) POUSSE caisse ->(%5,%6)%7")
                                  .arg(avant.x()).arg(avant.y()).arg(apres.x()).arg(apres.y())
                                  .arg(caisse.x()).arg(caisse.y())
                                  .arg(posee ? " [POSE]" : "");
    } else {
        qDebug().noquote() << QString("[mouv] joueur (%1,%2)->(%3,%4)")
                                  .arg(avant.x()).arg(avant.y()).arg(apres.x()).arg(apres.y());
    }

    return true;
}

// Contrôles verrouillés pendant qu'un solveur tourne ou qu'une solution se
// rejoue : changer de niveau ou de solveur sous les pieds du thread laisserait
// 'game' et le rejeu désynchronisés.
void MainWindow::setControlesActifs(bool actifs) {
    cbNiveau->setEnabled(actifs);
    cbSolveur->setEnabled(actifs);
    pbResoudre->setEnabled(actifs);
}

// Remet la case état-max à zéro : décochée, désactivée, libellé sans compteur.
// L'état-max appartient à UNE résolution d'UN niveau — changer l'un ou l'autre le
// périme, et gameMax pointerait sur un plateau qui n'a plus rien à voir.
void MainWindow::resetEtatMax() {
    maxRangeesVu = 0;
    cbEtatMax->setChecked(false);
    cbEtatMax->setEnabled(false);
    cbEtatMax->setText(texteEtatMax);
}

void MainWindow::onIALance() {
    if (solveur) return;   // résolution déjà en cours

    setControlesActifs(false);
    wGame->setDuree(0.0);

    // Nouvelle résolution : on repart d'un état-max vierge.
    resetEtatMax();

    const auto type = static_cast<Solveur::EType>(cbSolveur->currentData().toInt());
    solveur = Solveur::creer(type, game, this);
    connect(solveur, &Solveur::solutionTrouvee, this, &MainWindow::onSolutionTrouvee);
    connect(solveur, &Solveur::aucuneSolution, this, &MainWindow::onAucuneSolution);
    connect(solveur, &Solveur::nouveauMaxCaisses, this, &MainWindow::onNouveauMax);
    connect(solveur, &QThread::finished, solveur, &QObject::deleteLater);
    solveur->start();

    begin = chrono::high_resolution_clock::now();
}

void MainWindow::onSolutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores) {
    solveur = nullptr;

    // 'game' n'a pas encore bougé (le solveur travaillait sur sa propre copie) :
    // c'est le point de départ à conserver pour pouvoir revisionner plus tard.
    derniereSolutionDepart = game;
    derniereSolutionCoups = chemin;
    derniereSolutionEtats = etatsExplores;
    pbRevoir->setEnabled(true);

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> diff = end - begin;

    wGame->setEtatsExplores(etatsExplores);
    wGame->setDuree(std::ceil(diff.count()));

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, "Solveur",
        QString("Solution trouvée : %1 coups (%2 états explorés).\nVoir la résolution ?")
            .arg(chemin.size())
            .arg(WGame::formaterMillier(etatsExplores)));

    if (reponse == QMessageBox::Yes) {
        coupsRestants = chemin;
        timerRejeu.start();   // laisse les contrôles désactivés le temps du rejeu
    } else {
        setControlesActifs(true);
    }
}

void MainWindow::onAucuneSolution() {
    solveur = nullptr;
    setControlesActifs(true);
    wGame->setEtatsExplores(0);
    QMessageBox::information(this, "Solveur", "Aucune solution trouvée pour ce niveau.");
}

void MainWindow::onRevoir() {
    if (timerRejeu.isActive() || derniereSolutionCoups.isEmpty()) return;

    game = derniereSolutionDepart;
    coupsRestants = derniereSolutionCoups;

    // On repart du départ : le compteur aussi (1 sous chaque caisse), sinon deux
    // visionnages cumulent leurs passages.
    initPassages();

    wGame->setGame(&game);
    wGame->setEtatsExplores(derniereSolutionEtats);
    wGame->setPassages(passages);

    setControlesActifs(false);
    pbRevoir->setEnabled(false);

    timerRejeu.start();
}

void MainWindow::rejouerCoup() {
    if (coupsRestants.isEmpty()) {
        timerRejeu.stop();
        setControlesActifs(true);
        pbRevoir->setEnabled(!derniereSolutionCoups.isEmpty());
        return;
    }

    Game::EDirection dir = coupsRestants.takeFirst();
    joue(dir);
    wGame->update();
}

// Export texte de la carte des passages, à côté de la grille du niveau, pour
// pouvoir la lire et l'annoter hors de l'app.
void MainWindow::onExportPassages() {
    const QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter les passages",
        QString("passages_niveau%1.txt").arg(game.getNumNiveau(), 2, 10, QChar('0')),
        "Texte (*.txt)");
    if (chemin.isEmpty()) return;

    QFile f(chemin);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Écriture impossible.");
        return;
    }

    const int L = game.getLargeur(), H = game.getHauteur();
    QTextStream out(&f);

    out << "Niveau " << game.getNumNiveau() << "\n";
    out << "Passages de caisse par case (cumulé : une caisse qui repasse compte à nouveau).\n";
    out << "Poussées jouées : " << game.getNbDepCaisse()
        << "   Déplacements : " << game.getNbDep() << "\n\n";

    // La grille, pour situer.
    out << "-- grille --\n";
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            switch (game.getCase(x + y * L)) {
                case Level::tcMur:        out << '#'; break;
                case Level::tcCaisse:     out << '$'; break;
                case Level::tcGoalCaisse: out << '*'; break;
                case Level::tcGoal:       out << '.'; break;
                case Level::tcPlayer:     out << '@'; break;
                case Level::tcGoalPlayer: out << '+'; break;
                default:                  out << ' '; break;
            }
        }
        out << "\n";
    }

    // Les passages, alignés sur la même grille (3 colonnes par case pour que les
    // nombres à 2 chiffres restent lisibles).
    out << "\n-- passages (3 caracteres par case) --\n";
    int total = 0, pic = 0;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            const int idx = x + y * L;
            if (game.getCase(idx) == Level::tcMur) { out << "###"; continue; }

            const int n = (idx < passages.size()) ? passages[idx] : 0;
            total += n;
            if (n > pic) pic = n;

            if (n == 0) out << "  .";
            else        out << QString("%1").arg(n, 3);
        }
        out << "\n";
    }

    out << "\ntotal des passages : " << total << "   maximum sur une case : " << pic << "\n";
    f.close();

    QMessageBox::information(this, "Export",
        QString("Écrit : %1\n%2 passages, pic à %3 sur une case.")
            .arg(chemin).arg(total).arg(pic));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        if (timerRejeu.isActive()) {
            return QObject::eventFilter(obj, event);   // laisse la souris/clavier tranquille pendant le rejeu
        }

        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        bool moved = false;

        switch (keyEvent->key()) {
            case Qt::Key_Up:    moved = joue(Game::dHaut);   break;
            case Qt::Key_Right: moved = joue(Game::dDroite); break;
            case Qt::Key_Down:  moved = joue(Game::dBas);    break;
            case Qt::Key_Left:  moved = joue(Game::dGauche); break;
            default: break;
        }

        if (moved) {
            wGame->setEtatsExplores(0);   // l'humain joue, plus de stat IA à afficher
            wGame->update();
        }
    }

    return QObject::eventFilter(obj, event);
}

void MainWindow::onExportXsb() {
    // Exporte le plateau AFFICHÉ (état-max si la case est cochée, sinon le plateau
    // courant) au format .xsb, pour l'inspecter/le partager facilement.
    const Game& g = cbEtatMax->isChecked() ? gameMax : game;

    const QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter le plateau (.xsb)",
        QString("plateau_niveau%1.xsb").arg(g.getNumNiveau(), 2, 10, QChar('0')),
        "Sokoban (*.xsb)");
    if (chemin.isEmpty()) return;

    QFile f(chemin);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Écriture impossible.");
        return;
    }

    const int L = g.getLargeur(), H = g.getHauteur();
    QTextStream out(&f);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            switch (g.getCase(x + y * L)) {
                case Level::tcMur:        out << '#'; break;
                case Level::tcCaisse:     out << '$'; break;
                case Level::tcGoalCaisse: out << '*'; break;
                case Level::tcGoal:       out << '.'; break;
                case Level::tcPlayer:     out << '@'; break;
                case Level::tcGoalPlayer: out << '+'; break;
                default:                  out << ' '; break;
            }
        }
        out << "\n";
    }
}

void MainWindow::onShowPassagesCaisse() {
    wGame->showPassage(cbNotePassages->isChecked());
    pbExport->setEnabled(cbNotePassages->isChecked());
}

void MainWindow::onNouveauMax(Game etatMax, int nbRangees) {
    gameMax = etatMax;
    maxRangeesVu = nbRangees;
    cbEtatMax->setEnabled(true);
    cbEtatMax->setText(QString("%1 (%2/%3)")
                           .arg(texteEtatMax).arg(nbRangees).arg(gameMax.getNbButs()));
    // Si l'utilisateur regarde déjà l'état-max, le rafraîchir en direct.
    if (cbEtatMax->isChecked())
        wGame->update();
}

void MainWindow::onToggleEtatMax(int state) {
    if (state == Qt::Checked) {
        timerRejeu.stop();               // fige l'affichage sur l'état-max
        wGame->setGame(&gameMax);
    } else {
        wGame->setGame(&game);
    }
    wGame->update();
}
