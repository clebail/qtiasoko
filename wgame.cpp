#include <QPainter>
#include "wgame.h"
#include "player.h"
#include "mur.h"
#include "caisse.h"
#include "goal.h"
#include "goalcaisse.h"

WGame::WGame(QWidget *parent) : QWidget(parent) {
    initSprites();
}

WGame::~WGame() {
    freeSprites();
}

void WGame::initSprites() {
    sprites[0] = nullptr;
    sprites[1] = new Mur();
    sprites[2] = new Player();
    sprites[3] = new Caisse();
    sprites[4] = new Goal();
    sprites[5] = new GoalCaisse();
    sprites[6] = sprites[2]; // GoalPlayer réutilise le sprite Player
}

void WGame::freeSprites() {
    sprites[6] = nullptr; // alias, ne pas double-free
    for (int i = 1; i < NB_SPRITE; ++i) {
        delete sprites[i];
        sprites[i] = nullptr;
    }
}

void WGame::setGame(const Game *g) {
    game = g;
    update();
}

void WGame::setEtatsExplores(qint64 n) {
    etatsExplores = n;
    update();
}

void WGame::setPassages(const QVector<int>& p) {
    passages = p;
    update();
}

QString WGame::formaterMillier(qint64 n) {
    QString s = QString::number(n);
    for (int i = s.length() - 3; i > 0; i -= 3) {
        s.insert(i, ' ');
    }
    return s;
}

void WGame::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    if (game && game->isLoaded()) {
        painter.fillRect(rect(), QColor(255, 236, 179));

        QString statNiveau = QString("Niveau : %1").arg(game->getNumNiveau());
        QString statDep    = QString("Déplacements : %1").arg(game->getNbDep());
        QString statCaisse = QString("Caisses : %1").arg(game->getNbDepCaisse());
        QString statEtats  = QString("Etats explores : %1").arg(formaterMillier(etatsExplores));
        QString statDuree  = QString("Temps de résolution : %1s").arg(duree);

        const int largeur = game->getLargeur();
        const int hauteur = game->getHauteur();
        QSize pSize = painter.window().size();
        int margX = (pSize.width() - largeur * SPRITE_WIDTH) / 2;
        int margY = (pSize.height() - hauteur * SPRITE_HEIGHT) / 2;

        for (int y = 0; y < hauteur; y++) {
            for (int x = 0; x < largeur; x++) {
                int idx = x + y * largeur;
                Level::ETypeCase c = game->getCase(idx);
                Sprite *s = sprites[(int)c];
                int idxS = (c == Level::tcPlayer || c == Level::tcGoalPlayer) ? game->getPlayerDirection() : 0;

                if (s) {
                    painter.drawImage(QPoint(margX + x * SPRITE_WIDTH, margY + y * SPRITE_HEIGHT), s->getImage(idxS));
                }

                // Compteur de passages, par-dessus la case. Les murs n'en ont
                // jamais.
                if (show && idx < passages.size() && passages[idx] > 0 && c != Level::tcMur) {
                    const QRect r(margX + x * SPRITE_WIDTH, margY + y * SPRITE_HEIGHT,
                                  SPRITE_WIDTH, SPRITE_HEIGHT);

                    QFont f = painter.font();
                    f.setPointSize(13);
                    f.setBold(true);
                    painter.setFont(f);

                    // Halo sombre puis chiffre clair : lisible sur n'importe quel
                    // sprite, sans avoir à connaître sa couleur.
                    const QString t = QString::number(passages[idx]);
                    painter.setPen(QColor(0, 0, 0, 200));
                    for (int dx = -1; dx <= 1; dx++)
                        for (int dy = -1; dy <= 1; dy++)
                            if (dx || dy)
                                painter.drawText(r.translated(dx, dy), Qt::AlignCenter, t);

                    painter.setPen(QColor(255, 255, 255));
                    painter.drawText(r, Qt::AlignCenter, t);
                }
            }
        }

        QFont font = painter.font();
        font.setPointSize(14);
        font.setBold(true);
        painter.setFont(font);

        painter.setPen(QColor(0x29, 0x80, 0xb9));
        painter.drawText(8, 24, statNiveau);
        painter.drawText(8, 46, statDep);
        painter.drawText(8, 68, statCaisse);
        painter.drawText(8, 90, statEtats);
        painter.drawText(8, 112, statDuree);
    }
}

void WGame::showPassage(bool show) {
    this->show = show;
    update();
}

void WGame::setDuree(double duree) {
    this->duree = duree;
    update();
}
