#include <QKeyEvent>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi(this);
    installEventFilter(this);

    Level lvl;
    lvl.load("level0001.xsb");
    game = Game(lvl, 1);

    wGame->setGame(&game);
}

MainWindow::~MainWindow() {
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        bool moved = false;

        switch (keyEvent->key()) {
            case Qt::Key_Up:    moved = game.haut();   break;
            case Qt::Key_Right: moved = game.droite(); break;
            case Qt::Key_Down:  moved = game.bas();    break;
            case Qt::Key_Left:  moved = game.gauche(); break;
            default: break;
        }

        if (moved)
            wGame->update();
    }

    return QObject::eventFilter(obj, event);
}
