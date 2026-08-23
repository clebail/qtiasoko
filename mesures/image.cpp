// image — UN .xsb EN .png, AVEC LES SPRITES DE L'UI.
//
//   image <niveau|fichier.xsb> [sortie.png] [taille]
//         [--rejeu <poussees.txt>] [--stop <n>] [--loi] [--align]
//
// ── REJEU + LOI (2026-08-20) ─────────────────────────────────────────────────
// `--loi` peint les cases mortes et cercle le but actif, avec les MÊMES couleurs
// que `WGame::paintEvent` (gris #909090 α90 pour les mortes ordinaires, gris foncé
// #303030 α150 pour le surplus de la loi de l'ordre).
//
// ⚠️ POURQUOI IL FAUT REJOUER, ET NON CHARGER UN .xsb DE MILIEU DE PARTIE. Charger
// une position intermédiaire comme un niveau **recalcule tout le statique** pour ce
// plateau-là (§7) : `ordreParPrecedence`, `casesMortes` et `mortesLoi` tournent dans
// le ctor `Game(Level)` et ne connaissent que les caisses qu'on leur donne. Le but
// actif affiché et les cases mortes seraient donc ceux d'un AUTRE problème que celui
// que le solveur a réellement jugé. `--rejeu` charge le vrai niveau puis y applique
// les poussées : les tables restent celles du niveau, comme dans le run.
//
// Le fichier de poussées est celui de `jugeloi` (une ligne "<case> <dir>"), produit
// par `mesures/poussees_journal.py` — donc rejouable et déjà validé.
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

// L'INTÉRIEUR vient du MOTEUR (`Game::interieur()`, game.cpp) depuis le
// 2026-08-22 : cet outil en portait une copie, et le moteur en a désormais
// l'exemplaire unique (§7) — avec le repli sur un but quand le plateau n'a pas de
// joueur, que la copie locale n'avait pas.

int main(int argc, char** argv) {
    // QGuiApplication et non QCoreApplication : QPixmap exige une couche graphique.
    // ⚠️ En l'absence d'écran, exporter QT_QPA_PLATFORM=offscreen.
    QGuiApplication app(argc, argv);

    if (argc < 2) {
        fprintf(stderr, "usage: image <niveau|fichier.xsb> [sortie.png] [taille de case]\n");
        return 2;
    }

    // Les options nommées se lisent d'abord, pour que les positionnels ne les
    // ramassent pas (`--loi` retomberait sinon dans `sortie.png`).
    QString fichierRejeu;
    int     stop = -1;
    bool    montreLoi = false, align = false;
    QVector<QString> positionnels;
    for (int i = 1; i < argc; i++) {
        const QString a = argv[i];
        if      (a == "--loi")   montreLoi = true;
        else if (a == "--align") align = true;
        else if (a == "--rejeu" && i + 1 < argc) fichierRejeu = argv[++i];
        else if (a == "--stop"  && i + 1 < argc) stop = QString(argv[++i]).toInt();
        else positionnels.append(a);
    }
    if (positionnels.isEmpty()) {
        fprintf(stderr, "image: aucun niveau ni fichier donne\n");
        return 2;
    }

    const QString arg1 = positionnels[0];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) {
        fprintf(stderr, "image: niveau introuvable (%s)\n", qPrintable(arg1));
        return 2;
    }
    Game g(level, num);
    // L'ordre décide du BUT ACTIF, donc de la tranche de `mortesLoi` qu'on affiche :
    // dessiner la loi sous un autre ordre que celui du régime testé ne montrerait pas
    // ce que ce régime a jugé.
    if (align) g.setOrdreAlignement(true);

    // ── REJEU (cf. l'entête) ────────────────────────────────────────────────────
    if (!fichierRejeu.isEmpty()) {
        FILE* f = fopen(qPrintable(fichierRejeu), "r");
        if (!f) { fprintf(stderr, "image: %s illisible\n", qPrintable(fichierRejeu)); return 2; }
        char ligne[256];
        int n = 0;
        while (fgets(ligne, sizeof(ligne), f)) {
            int cell = -1, dir = -1;
            if (sscanf(ligne, "%d %d", &cell, &dir) != 2) continue;
            if (stop >= 0 && n >= stop) break;
            if (!g.pousse(cell, (Game::EDirection)dir)) {
                // Refuser bruyamment : dessiner un plateau issu d'un rejeu divergent
                // serait exactement l'erreur que cet outil existe pour éviter.
                fprintf(stderr, "image: poussee %d ILLEGALE (case %d dir %d) — rejeu abandonne\n",
                        n + 1, cell, dir);
                fclose(f); return 3;
            }
            n++;
        }
        fclose(f);
        fprintf(stderr, "[rejeu] %d poussees appliquees sur le niveau %d\n", n, num);
    }

    const QString sortie = (positionnels.size() > 1)
                               ? positionnels[1]
                               : (parChemin ? QFileInfo(arg1).completeBaseName() + ".png"
                                            : QString("level%1.png").arg(num, 4, 10, QChar('0')));
    // La taille de case est un paramètre ici (contrairement à l'UI, où c'est une
    // constante) : un plateau de 34 cases fait 2 176 px à 64, illisible en vignette
    // et lourd à l'écran. 48 est un bon compromis pour un export qu'on regarde.
    const int cote = (positionnels.size() > 2) ? positionnels[2].toInt() : 48;

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

    const QVector<bool> dedans = g.interieur();

    // ── LA LOI DE L'ORDRE, exactement comme l'UI la peint ───────────────────────
    // Deux gris DISTINCTS, et c'est délibéré (cf. wgame.cpp) : les cases mortes
    // ORDINAIRES sont un décor permanent qui n'apprend rien, le surplus de la LOI
    // dépend du but actif et change à chaque but rempli — c'est lui qu'on vient lire.
    // Les peindre du même gris rendrait la loi illisible.
    const int butCourant = montreLoi ? g.butActif() : -1;
    const QVector<bool> mortesLoi = montreLoi ? g.casesMortesLoi(butCourant) : QVector<bool>();

    // Le verdict EN TOUTES LETTRES sur stderr, pas seulement en aplats. Lire des
    // coordonnées à l'œil sur une image est précisément ce qui fait écrire des
    // légendes fausses — j'ai désigné deux fois la mauvaise caisse avant d'ajouter
    // ceci. L'image montre, le texte prouve.
    if (montreLoi) {
        fprintf(stderr, "[loi] but actif = (%d,%d)\n",
                butCourant >= 0 ? g.getCaseBut(butCourant) % L : -1,
                butCourant >= 0 ? g.getCaseBut(butCourant) / L : -1);
        QString listeMortes, listeCaisses;
        for (int c = 0; c < mortesLoi.size(); c++) {
            if (!mortesLoi[c]) continue;
            listeMortes += QString(" (%1,%2)").arg(c % L).arg(c / L);
            const Level::ETypeCase t = g.getCase(c);
            if (t == Level::tcCaisse || t == Level::tcGoalCaisse)
                listeCaisses += QString(" (%1,%2)%3").arg(c % L).arg(c / L)
                                    .arg(t == Level::tcGoalCaisse ? "[sur but]" : "");
        }
        fprintf(stderr, "[loi] cases mortes par la loi :%s\n",
                listeMortes.isEmpty() ? " aucune" : qPrintable(listeMortes));
        fprintf(stderr, "[loi] CAISSES posees sur une de ces cases :%s\n",
                listeCaisses.isEmpty() ? " aucune (etat NON elague)" : qPrintable(listeCaisses));
    }

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

            // Couche 2 bis — les cases mortes, PAR-DESSUS le sol et la caisse.
            // ⚠️ Les mortes ordinaires sont restreintes à l'INTÉRIEUR : tout le
            // remplissage hors contour est mort dans la table (il n'atteint aucun
            // but), et le peindre passerait le pourtour au gris pour ne rien dire.
            if (montreLoi && dedans.value(idx, false) && g.caseMorteOrdinaire(idx))
                p.fillRect(QRectF(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT)),
                           QColor(0x90, 0x90, 0x90, 90));
            if (idx < mortesLoi.size() && mortesLoi[idx])
                p.fillRect(QRectF(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT)),
                           QColor(0x30, 0x30, 0x30, 150));
        }
    }

    // Couche 2 ter — LE BUT ACTIF, cerclé. C'est lui qui décide de toute la tranche
    // grise ci-dessus : sans le voir, l'image montre un verdict sans son juge.
    if (butCourant >= 0) {
        const int cb = g.getCaseBut(butCourant);
        const QPointF coin((cb % L) * SPRITE_WIDTH, (cb / L) * SPRITE_HEIGHT);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0xff, 0x98, 0x00), 6));          // ambre, épais
        p.drawRect(QRectF(coin.x() + 4, coin.y() + 4, SPRITE_WIDTH - 8, SPRITE_HEIGHT - 8));
    }

    // Couche 3 — le perso, par-dessus la grille posée.
    // ⚠️ SEULEMENT S'IL Y EN A UN. Sans joueur, `playerPoint` vaut (0,0) et le
    // perso se faisait dessiner dans le coin, sur un mur — un plateau de ZONE
    // (mesures/zonembut) n'a pas de joueur, et l'image mentait. On relit la case
    // plutôt que de croire le point.
    const QPoint pj = g.getPlayerPoint();
    const int idxJoueur = pj.x() + pj.y() * L;
    if (pj.x() >= 0 && pj.x() < L && pj.y() >= 0 && pj.y() < H
        && (g.getCase(idxJoueur) == Level::tcPlayer
            || g.getCase(idxJoueur) == Level::tcGoalPlayer))
        perso.dessine(p, QPointF(pj.x() * SPRITE_WIDTH, pj.y() * SPRITE_HEIGHT));
    p.end();

    if (!img.save(sortie)) {
        fprintf(stderr, "image: ecriture impossible (%s)\n", qPrintable(sortie));
        return 1;
    }
    printf("%s  (%dx%d cases, %dx%d px)\n", qPrintable(sortie), L, H, img.width(), img.height());
    return 0;
}
