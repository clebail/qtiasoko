// image — UN .xsb EN .png, AVEC LES SPRITES DE L'UI.
//
//   image <niveau|fichier.xsb> [sortie.png] [taille]
//
// Raison d'être : on exporte des plateaux tout le temps (records du mode `record`,
// fixtures du mode hybride, positions de blocage) et on les lit en ASCII, ce qui
// marche mal — j'ai mal lu la géométrie du niveau 12 trois fois de suite, dans les
// deux sens, alors que la réponse était dans le dessin.
//
// ⚠️ RÉUTILISE LES CLASSES DE L'UI (Sprite/Mur/Caisse/Goal/GoalCaisse/Player/Sol) et
// la même logique de couches que `WGame::paintEvent`. Redessiner à côté produirait
// une image qui RESSEMBLE au jeu sans en être, et c'est précisément quand les deux
// divergent qu'on regarde une image pour comprendre un bug.
//
// Le contour intérieur/extérieur est calculé comme dans l'UI (flood-fill 4-connexe
// depuis le joueur, à travers tout sauf les murs) : c'est le contraste entre le sol
// grenu et le sol uni qui dessine la forme du plateau.
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QFileInfo>
#include <QVector>
#include <cstdio>

#include "level.h"
#include "game.h"
#include "sprite.h"
#include "sol.h"
#include "mur.h"
#include "caisse.h"
#include "goal.h"
#include "goalcaisse.h"
#include "player.h"

// Même teinte que le fond de l'UI (wgame.cpp).
static const QColor fondSable(222, 205, 180);

// Flood-fill 4-connexe depuis le joueur, à travers tout ce qui n'est pas un mur.
// Copié du contrat de WGame::calculeInterieur : les caisses ne bloquent pas, elles
// bougent — le contour du plateau, lui, ne bouge pas.
static QVector<bool> calculeInterieur(const Game& g) {
    const int L = g.getLargeur(), H = g.getHauteur();
    QVector<bool> dedans(L * H, false);
    const QPoint p = g.getPlayerPoint();
    if (p.x() < 0 || p.x() >= L || p.y() < 0 || p.y() >= H) return dedans;

    static const int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
    QVector<int> pile;
    const int depart = p.x() + p.y() * L;
    dedans[depart] = true;
    pile.append(depart);
    while (!pile.isEmpty()) {
        const int c = pile.takeLast();
        const int cx = c % L, cy = c / L;
        for (int d = 0; d < 4; d++) {
            const int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
            const int n = nx + ny * L;
            if (dedans[n] || g.getCase(n) == Level::tcMur) continue;
            dedans[n] = true;
            pile.append(n);
        }
    }
    return dedans;
}

int main(int argc, char** argv) {
    // QGuiApplication et non QCoreApplication : QPixmap exige une couche graphique.
    // ⚠️ En l'absence d'écran, exporter QT_QPA_PLATFORM=offscreen.
    QGuiApplication app(argc, argv);

    if (argc < 2) {
        fprintf(stderr, "usage: image <niveau|fichier.xsb> [sortie.png] [taille de case]\n");
        return 2;
    }

    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) {
        fprintf(stderr, "image: niveau introuvable (%s)\n", qPrintable(arg1));
        return 2;
    }
    const Game g(level, num);

    const QString sortie = (argc > 2) ? QString(argv[2])
                                      : (parChemin ? QFileInfo(arg1).completeBaseName() + ".png"
                                                   : QString("level%1.png").arg(num, 4, 10, QChar('0')));
    // La taille de case est un paramètre ici (contrairement à l'UI, où c'est une
    // constante) : un plateau de 34 cases fait 2 176 px à 64, illisible en vignette
    // et lourd à l'écran. 48 est un bon compromis pour un export qu'on regarde.
    const int cote = (argc > 3) ? QString(argv[3]).toInt() : 48;

    const int L = g.getLargeur(), H = g.getHauteur();
    QImage img(L * cote, H * cote, QImage::Format_ARGB32);
    img.fill(fondSable);

    QPainter p(&img);
    // Les sprites sont dessinés en 64 natif : dès qu'on s'en écarte il faut
    // interpoler, sinon les bords crénellent (même règle que WGame).
    if (cote != TUILE_SOURCE) p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    // Les classes de sprites dessinent en SPRITE_WIDTH ; on met la toile à l'échelle
    // voulue plutôt que de toucher à leur code.
    p.scale((qreal)cote / SPRITE_WIDTH, (qreal)cote / SPRITE_HEIGHT);

    Sol        solInterieur;      // grenu : dedans
    SolHors    solExterieur;      // uni : dehors
    Mur        mur;
    Caisse     caisse;
    GoalCaisse caisseSurBut;
    Goal       but;
    Player     perso;

    const QVector<bool> dedans = calculeInterieur(g);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            const int idx = x + y * L;
            const Level::ETypeCase c = g.getCase(idx);
            const QPointF coin(x * SPRITE_WIDTH, y * SPRITE_HEIGHT);

            if (c == Level::tcMur) { mur.dessine(p, coin); continue; }

            // Couche 1 — le sol, grenu dedans, uni dehors.
            const Sprite* sol = dedans.value(idx, false)
                                    ? static_cast<const Sprite*>(&solInterieur)
                                    : static_cast<const Sprite*>(&solExterieur);
            sol->dessine(p, coin);

            // Couche 2 — la caisse, ou le but qu'elle aurait masqué.
            if (c == Level::tcCaisse || c == Level::tcGoalCaisse) {
                const Sprite* s = (c == Level::tcGoalCaisse)
                                      ? static_cast<const Sprite*>(&caisseSurBut)
                                      : static_cast<const Sprite*>(&caisse);
                s->dessine(p, coin);
            } else if (c == Level::tcGoal || c == Level::tcGoalPlayer) {
                but.dessine(p, coin);
            }
        }
    }

    // Couche 3 — le perso, par-dessus la grille posée.
    const QPoint pj = g.getPlayerPoint();
    perso.dessine(p, QPointF(pj.x() * SPRITE_WIDTH, pj.y() * SPRITE_HEIGHT));
    p.end();

    if (!img.save(sortie)) {
        fprintf(stderr, "image: ecriture impossible (%s)\n", qPrintable(sortie));
        return 1;
    }
    printf("%s  (%dx%d cases, %dx%d px)\n", qPrintable(sortie), L, H, img.width(), img.height());
    return 0;
}
