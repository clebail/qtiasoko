#include <QPainter>
#include "wgame.h"

WGame::WGame(QWidget *parent) : QWidget(parent) {
}

WGame::~WGame() {
}

void WGame::setGame(const Game *g) {
    game = g;
    update();
}

void WGame::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    if (game && game->isLoaded()) {
        game->draw(&painter);

        painter.fillRect(rect(), QColor(0, 0, 0, 80));

        QString statNiveau = QString("Niveau : %1").arg(game->getNumNiveau());
        QString statDep    = QString("Déplacements : %1").arg(game->getNbDep());
        QString statCaisse = QString("Caisses : %1").arg(game->getNbDepCaisse());

        QFont font = painter.font();
        font.setPointSize(14);
        font.setBold(true);
        painter.setFont(font);

        painter.setPen(QColorConstants::Black);
        painter.drawText(9, 25, statNiveau);
        painter.drawText(9, 47, statDep);
        painter.drawText(9, 69, statCaisse);

        painter.setPen(QColorConstants::White);
        painter.drawText(8, 24, statNiveau);
        painter.drawText(8, 46, statDep);
        painter.drawText(8, 68, statCaisse);
    }
}
