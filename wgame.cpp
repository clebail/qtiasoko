#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <cmath>
#include "wgame.h"

// Le sable de la planche (Ground_Sand), en uni : c'est le fond du widget, sous
// la marge des règles et partout où le widget déborde du plateau. Même teinte
// que la tuile hors plateau, donc la jointure ne se voit pas.
static const QColor fondSable(0xec, 0xe3, 0xce);

WGame::WGame(QWidget *parent) : QWidget(parent) {
    timerAnim.setInterval(16);   // ~60 images par seconde
    connect(&timerAnim, &QTimer::timeout, this, &WGame::avanceAnimation);

    // Sans bouton enfoncé, mouseMoveEvent n'arrive que si le suivi est armé —
    // et c'est le survol seul qui doit déclencher la légende des stats.
    setMouseTracking(true);

    timerSpinner.setInterval(60);   // les « Z » montent lentement, 16 images/s suffisent
    connect(&timerSpinner, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
}

void WGame::setResolution(bool enCours) {
    if (resolution == enCours) return;   // sinon le cycle des « Z » repartirait de zéro
    resolution = enCours;

    if (enCours) {
        chronoSpinner.start();
        timerSpinner.start();
    } else {
        timerSpinner.stop();
    }

    update();
}

void WGame::setGame(const Game *g) {
    game = g;

    // Changer de plateau (autre niveau, ou bascule vers l'état-max) invalide le
    // glissement en cours : il partait d'une case qui n'existe plus.
    timerAnim.stop();
    animEnCours = false;
    animCaisseIdx = -1;
    direction = Game::dBas;
    pasMarche = 0;

    calculeInterieur();

    // Le niveau peut changer de dimensions : updateGeometry() force le QScrollArea
    // à relire minimumSizeHint() et à recalculer ses barres de défilement.
    updateGeometry();
    update();
}

// Remplissage 4-connexe depuis le joueur, à travers tout ce qui n'est pas un
// mur. Les caisses ne bloquent pas : elles bougent, le contour du plateau non.
void WGame::calculeInterieur() {
    interieur.clear();
    if (!game || !game->isLoaded()) return;

    const int L = game->getLargeur();
    const int H = game->getHauteur();
    if (L <= 0 || H <= 0) return;

    interieur = QVector<bool>(L * H, false);

    const QPoint p = game->getPlayerPoint();
    if (p.x() < 0 || p.x() >= L || p.y() < 0 || p.y() >= H) return;

    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};

    QVector<int> pile;
    const int depart = p.x() + p.y() * L;
    interieur[depart] = true;
    pile.append(depart);

    while (!pile.isEmpty()) {
        const int idx = pile.takeLast();
        const int x = idx % L, y = idx / L;

        for (int d = 0; d < 4; d++) {
            const int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;

            const int n = nx + ny * L;
            if (interieur[n] || game->getCase(n) == Level::tcMur) continue;

            interieur[n] = true;
            pile.append(n);
        }
    }
}

QSize WGame::sizeHint() const {
    return minimumSizeHint();
}

QSize WGame::minimumSizeHint() const {
    if (!game || !game->isLoaded())
        return QSize(20 * SPRITE_WIDTH, 20 * SPRITE_HEIGHT);
    // Sprites fixes + une case de marge tout autour (règles de numéros).
    return QSize((game->getLargeur() + 2) * SPRITE_WIDTH,
                 (game->getHauteur() + 2) * SPRITE_HEIGHT);
}

void WGame::setEtatsExplores(qint64 n) {
    etatsExplores = n;
    update();
}

void WGame::setPassages(const QVector<int>& p) {
    passages = p;
    update();
}

void WGame::setChampButActif(const QVector<int>& champ, int caseBut) {
    champButActif = champ;
    caseButActif = caseBut;
    update();
}

void WGame::setArbreMacro(const QVector<bool>& visite) {
    arbreMacro = visite;
    update();
}

void WGame::setApercuSuite(const QVector<int>& ordre, const QVector<bool>& choisi) {
    apercuOrdre = ordre;
    apercuChoisi = choisi;
    update();
}

void WGame::setPousseeCourante(int idxCase, bool choisie) {
    if (caissePoussee == idxCase && pousseeChoisie == choisie) return;   // pas de repaint inutile
    caissePoussee = idxCase;
    pousseeChoisie = choisie;
    update();
}

void WGame::setModeHybride(bool on) {
    modeHybride = on;
    update();
}

void WGame::setRangsButs(const QVector<int>& rangs) {
    rangsButs = rangs;
    update();
}

void WGame::setMacrosJouables(const QVector<bool>& trajets, const QVector<bool>& caisses) {
    macroTrajets = trajets;
    macroCaisses = caisses;
    update();
}

void WGame::setSignales(const QVector<bool>& cases) {
    signales = cases;
    update();
}

QString WGame::formaterMillier(qint64 n) {
    QString s = QString::number(n);
    for (int i = s.length() - 3; i > 0; i -= 3) {
        s.insert(i, ' ');
    }
    return s;
}

void WGame::animerCoup(Game::EDirection dir, QPoint depart, bool poussee) {
    if (!game || !game->isLoaded()) return;

    const QPoint arrivee = game->getPlayerPoint();

    direction = static_cast<int>(dir);
    pasMarche++;
    animDepart = QPointF(depart);

    // La caisse poussée est juste devant le perso, dans le même sens. game la
    // montre déjà arrivée : il faut l'effacer de sa case pour la redessiner en
    // chemin, sinon on en verrait deux.
    animCaisseIdx = -1;
    if (poussee) {
        const QPoint c = arrivee + (arrivee - depart);
        if (c.x() >= 0 && c.x() < game->getLargeur() && c.y() >= 0 && c.y() < game->getHauteur())
            animCaisseIdx = c.x() + c.y() * game->getLargeur();
    }

    animEnCours = true;
    chronoAnim.start();
    timerAnim.start();
    update();
}

void WGame::arreteAnimation() {
    timerAnim.stop();
    animEnCours   = false;
    animCaisseIdx = -1;
    update();
}

void WGame::avanceAnimation() {
    if (chronoAnim.elapsed() >= dureeAnimation) {
        timerAnim.stop();
        animEnCours = false;
        animCaisseIdx = -1;
    }

    // Émis même sur l'image de fin : c'est elle qui cale la vue sur la case
    // d'arrivée exacte.
    emit joueurDeplace(centreJoueur());

    update();
}

QPoint WGame::centreJoueur() const {
    const QPointF p = positionPerso();
    return (coinCase(p.x(), p.y()) + QPointF(SPRITE_WIDTH / 2.0, SPRITE_HEIGHT / 2.0)).toPoint();
}

qreal WGame::progressionAnim() const {
    if (!animEnCours) return 1.0;
    return qBound(0.0, qreal(chronoAnim.elapsed()) / dureeAnimation, 1.0);
}

QPointF WGame::positionPerso() const {
    if (!game || !game->isLoaded()) return QPointF();

    const QPointF arrivee(game->getPlayerPoint());
    if (!animEnCours) return arrivee;

    const qreal t = progressionAnim();
    return animDepart * (1.0 - t) + arrivee * t;
}

QPointF WGame::coinCase(qreal x, qreal y) const {
    if (!game || !game->isLoaded()) return QPointF(x * SPRITE_WIDTH, y * SPRITE_HEIGHT);

    const qreal margX = (width()  - game->getLargeur() * SPRITE_WIDTH)  / 2.0;
    const qreal margY = (height() - game->getHauteur() * SPRITE_HEIGHT) / 2.0;
    return QPointF(margX + x * SPRITE_WIDTH, margY + y * SPRITE_HEIGHT);
}

void WGame::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(event->rect(), fondSable);

    if (!game || !game->isLoaded()) return;

    // Les tuiles ne sont redimensionnées que si SPRITE_WIDTH s'écarte de leur
    // taille native : dans ce cas il faut interpoler, sinon les bords crénellent.
    if (SPRITE_WIDTH != TUILE_SOURCE)
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const int largeur = game->getLargeur();
    const int hauteur = game->getHauteur();

    // ZONE DU JOUEUR (2026-08-02) — les cases qu'il peut atteindre MAINTENANT, les
    // caisses faisant obstacle. ⚠️ Calculée ICI, à chaque tracé, à partir du Game
    // qu'on affiche : une zone stockée par MainWindow serait périmée dès qu'une
    // caisse bouge, et une surcouche fausse ne se voit pas — c'est exactement le
    // piège documenté pour la zone du 1ᵉʳ pas de la macro (§6.3). Un flood-fill
    // par repaint ne coûte rien à l'échelle d'une UI.
    QVector<bool> zoneJoueur;
    if (showZone) game->getZoneJoueur(zoneJoueur);

    // CASES MORTES DE LA LOI DE L'ORDRE — même parti que la zone ci-dessus : lues
    // sur le Game affiché, à chaque tracé. La tranche dépend du BUT ACTIF, donc elle
    // change dès qu'un but se remplit ; la stocker la rendrait fausse en silence.
    QVector<bool> mortesLoi;
    if (showMortesLoi) mortesLoi = game->casesMortesLoi(game->butActif());

    for (int y = 0; y < hauteur; y++) {
        for (int x = 0; x < largeur; x++) {
            const int idx = x + y * largeur;
            const Level::ETypeCase c = game->getCase(idx);
            const QPointF coin = coinCase(x, y);

            if (c == Level::tcMur) {
                mur.dessine(painter, coin);   // tuile opaque : rien à dessiner dessous
                continue;
            }

            // Couche 1 — le sol. C'est le contraste entre le grenu (dedans) et
            // l'uni (dehors) qui dessine le contour du plateau.
            const Sprite *sol = interieur.value(idx, false)
                                    ? static_cast<const Sprite *>(&solInterieur)
                                    : static_cast<const Sprite *>(&solExterieur);
            sol->dessine(painter, coin);

            // Couche 2 — la caisse, ou le but qu'elle aurait masqué. La caisse en
            // cours de poussée est sautée ici : elle est dessinée en chemin, une
            // fois toute la grille posée.
            const bool aCaisse = (c == Level::tcCaisse || c == Level::tcGoalCaisse) && idx != animCaisseIdx;
            const bool aBut    = (c == Level::tcGoal || c == Level::tcGoalCaisse || c == Level::tcGoalPlayer);

            if (aCaisse) {
                const Sprite *s = (c == Level::tcGoalCaisse)
                                      ? static_cast<const Sprite *>(&caisseSurBut)
                                      : static_cast<const Sprite *>(&caisse);
                s->dessine(painter, coin);
            } else if (aBut) {
                // BLEU pour le but actif, SABLE pour les autres (cf. wgame.h). Hors
                // de ce mode, tous les buts restent bleus comme avant.
                but.dessine(painter, coin,
                            (montreOrdreButs && idx != caseButActif) ? 1 : 0);
            }

            // Zone du joueur : aplat VIOLET. ⚠️ Deux réglages payés d'un
            // aller-retour : l'alpha était à 55 (invisible à l'écran, alors qu'il
            // semblait suffisant en lisant le code) et la teinte était verte, donc
            // à un cheveu du vert des macros jouables. Le violet n'est utilisé
            // nulle part ailleurs, et 100 d'alpha se voit sans masquer le sprite.
            // Ce qu'on vient y lire, c'est la DIFFÉRENCE entre deux instants :
            // quelles cases s'ouvrent quand une caisse bouge (compte « Zj »).
            // CASES MORTES — aplat gris, peint EN PREMIER de toutes les surcouches.
            // C'est une propriété du décor (« ici une caisse serait perdue »), pas un
            // événement : elle doit passer SOUS le trajet d'une macro ou la zone du
            // joueur, qui eux se lisent par-dessus. Gris neutre, seule teinte encore
            // libre — bleu = macro, violet = zone, vert = caisse amorçable, rouge =
            // case signalée.
            //
            // DEUX GRIS, et la distinction est le fond de l'affaire (constat
            // utilisateur, 2026-08-04 : « c'était bien de voir TOUTES les cases mortes
            // du plateau, pas seulement les dynamiques ») :
            //   PÂLE  = la table ORDINAIRE, vraie pour toute la partie. C'est là que
            //           vivent les coins qui ne sont pas des buts — rien n'en sort,
            //           donc ils n'atteignent aucun but, donc ils y sont depuis
            //           toujours. Ne bouge jamais.
            //   FONCÉ = le SURPLUS de la loi de l'ordre, qui dépend du but ACTIF et
            //           change donc à chaque but rempli. C'est ce qu'on vient lire.
            // Les afficher du même gris rendrait la loi illisible : ses quelques cases
            // se noieraient dans un décor permanent qui, lui, n'apprend rien.
            // ⚠️ Restreint à l'INTÉRIEUR du plateau : tout le remplissage hors contour
            // est mort dans la table ordinaire (il n'atteint évidemment aucun but), et
            // le peindre passerait tout le pourtour au gris pour ne rien dire.
            if (showMortesLoi && interieur.value(idx, false) && game->caseMorteOrdinaire(idx)) {
                painter.fillRect(QRectF(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT)),
                                  QColor(0x90, 0x90, 0x90, 90));
            }
            if (idx < mortesLoi.size() && mortesLoi[idx]) {
                painter.fillRect(QRectF(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT)),
                                  QColor(0x30, 0x30, 0x30, 150));
            }

            if (idx < zoneJoueur.size() && zoneJoueur[idx]) {
                painter.fillRect(QRectF(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT)),
                                  QColor(0x9c, 0x27, 0xb0, 100));
            }

            // Arbre de macro (Game::arbreMacro) : toutes les cases visitées
            // par AU MOINS UNE branche, en aplat translucide — pas de
            // nombres, juste « ce chemin marche aussi », par-dessus le sol
            // et la caisse/le but pour rester visible sur les deux.
            if (idx < arbreMacro.size() && arbreMacro[idx]) {
                painter.fillRect(QRectF(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT)),
                                  QColor(0x21, 0x96, 0xf3, 110));
            }

            // MODE HYBRIDE — les macros que le solveur pourrait engager ICI.
            // Même aplat bleu que l'arbre de macro ci-dessus : c'est le même
            // objet (un trajet de goal macro), il n'y a aucune raison de le
            // peindre autrement selon qui l'a demandé.
            if (modeHybride && idx < macroTrajets.size() && macroTrajets[idx]) {
                painter.fillRect(QRectF(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT)),
                                  QColor(0x21, 0x96, 0xf3, 110));
            }

            // La caisse qui AMORCE une macro : c'est elle qu'il faut cliquer
            // pour la jouer, donc elle est cerclée et pas seulement teintée —
            // sinon rien ne la distingue des cases de son propre trajet.
            if (modeHybride && idx < macroCaisses.size() && macroCaisses[idx]) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));
                QPen pen(QColor(0x76, 0xff, 0x03));
                pen.setWidth(3);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(r.adjusted(1.5, 1.5, -1.5, -1.5));
            }

            // LA CAISSE DU COUP COURANT, pendant une annotation. Orange ÉPAIS =
            // c'est TON choix, tu peux étiqueter ; gris fin et pointillé = c'est la
            // goal macro, il n'y a rien à annoter. Dessiné avant le rouge des cases
            // signalées, qui reste prioritaire (plus rare, plus précieux).
            // ⚠️ Ni vert ni bleu : les deux sont déjà pris par les macros jouables
            // du mode hybride, et c'est justement de macro qu'il s'agit de NE PAS
            // se tromper ici.
            if (idx >= 0 && idx == caissePoussee) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));
                QPen pen(pousseeChoisie ? QColor(0xff, 0x91, 0x00) : QColor(0x61, 0x61, 0x61));
                pen.setWidth(pousseeChoisie ? 5 : 2);
                if (!pousseeChoisie) pen.setStyle(Qt::DashLine);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(r.adjusted(2.5, 2.5, -2.5, -2.5));
            }

            // APERÇU : où les caisses vont arriver dans les poussées qui viennent.
            // Le chiffre est le rang (1 = la prochaine). ORANGE = poussée CHOISIE,
            // gris = macro : on voit donc du même coup d'œil où va la caisse ET
            // quand arrive le prochain coup annotable — c'est-à-dire quand cliquer
            // pour signaler une macro manquante. Au CENTRE de la case, avec halo
            // noir comme les autres nombres, pour rester lisible sur un sprite.
            if (idx < apercuOrdre.size() && apercuOrdre[idx] > 0) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));
                const bool ch = idx < apercuChoisi.size() && apercuChoisi[idx];
                QFont f = painter.font();
                f.setPointSize(13 * SPRITE_WIDTH / 32);
                f.setBold(true);
                painter.setFont(f);
                const QString t = QString::number(apercuOrdre[idx]);
                painter.setPen(QColor(0, 0, 0, 220));
                for (int dx = -1; dx <= 1; dx++)
                    for (int dy = -1; dy <= 1; dy++)
                        if (dx || dy) painter.drawText(r.translated(dx, dy), Qt::AlignCenter, t);
                painter.setPen(ch ? QColor(0xff, 0x91, 0x00) : QColor(0xbd, 0xbd, 0xbd));
                painter.drawText(r, Qt::AlignCenter, t);
            }

            // Case signalée à la main : rouge, et par-dessus le cercle vert des
            // macros jouables — si les deux tombent au même endroit, c'est le
            // signalement qui doit se voir, il est plus rare et plus précieux.
            if (idx < signales.size() && signales[idx]) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));
                QPen pen(QColor(0xff, 0x17, 0x44));
                pen.setWidth(3);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(r.adjusted(1.5, 1.5, -1.5, -1.5));
                painter.drawLine(r.topLeft() + QPointF(4, 4), r.bottomRight() - QPointF(4, 4));
                painter.drawLine(r.topRight() + QPointF(-4, 4), r.bottomLeft() + QPointF(4, -4));
            }

            // Rang de remplissage du but (ordreButs). En HAUT de case : les
            // passages occupent le centre et le champ de distances le bas, les
            // trois surcouches doivent pouvoir être lues ensemble. Dessiné
            // aussi sous une caisse déjà posée — savoir qu'elle occupe le rang
            // 12 alors que le 5 est encore vide est justement l'information
            // qu'on vient chercher.
            if (modeHybride && idx < rangsButs.size() && rangsButs[idx] >= 0) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));

                QFont f = painter.font();
                f.setPointSize(11 * SPRITE_WIDTH / 32);
                f.setBold(true);
                painter.setFont(f);

                const QString t = QString::number(rangsButs[idx]);
                painter.setPen(QColor(0, 0, 0, 200));
                for (int dx = -1; dx <= 1; dx++)
                    for (int dy = -1; dy <= 1; dy++)
                        if (dx || dy)
                            painter.drawText(r.translated(dx, dy), Qt::AlignTop | Qt::AlignHCenter, t);

                painter.setPen(QColor(0xff, 0xd5, 0x4f));   // ambre, distinct du blanc et du bleu clair
                painter.drawText(r, Qt::AlignTop | Qt::AlignHCenter, t);
            }

            // Compteur de passages, par-dessus la case. Les murs n'en ont
            // jamais.
            if (show && idx < passages.size() && passages[idx] > 0 && c != Level::tcMur) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));

                QFont f = painter.font();
                f.setPointSize(13 * SPRITE_WIDTH / 32);   // suit la taille des cases
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

            // Champ de distances vers le but actif : le gradient que descend
            // la goal macro (Game::champDistanceButActif). En bas de case, un
            // autre coin que les passages, pour rester lisible si les deux
            // sont cochés en même temps. -1 (inatteignable depuis là) n'est
            // pas dessiné : ça reviendrait à marquer la quasi-totalité du
            // plateau hors de la région du joueur.
            if (showChamp && idx < champButActif.size() && champButActif[idx] >= 0 && c != Level::tcMur) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));

                QFont f = painter.font();
                f.setPointSize(11 * SPRITE_WIDTH / 32);
                f.setBold(true);
                painter.setFont(f);

                const QString t = QString::number(champButActif[idx]);
                painter.setPen(QColor(0, 0, 0, 200));
                for (int dx = -1; dx <= 1; dx++)
                    for (int dy = -1; dy <= 1; dy++)
                        if (dx || dy)
                            painter.drawText(r.translated(dx, dy), Qt::AlignBottom | Qt::AlignHCenter, t);

                painter.setPen(QColor(0x4f, 0xc3, 0xf7));   // bleu clair, distinct du blanc des passages
                painter.drawText(r, Qt::AlignBottom | Qt::AlignHCenter, t);
            }

            // Surligne le but ACTIF : sans lui, le champ de distances ci-dessus
            // n'a pas de référence (vers QUEL but on compte).
            if (showChamp && idx == caseButActif) {
                const QRectF r(coin, QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));
                QPen pen(QColor(0xff, 0x6f, 0x00));
                pen.setWidth(3);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(r.adjusted(1.5, 1.5, -1.5, -1.5));
            }
        }
    }

    // Règles de repérage (x = colonne, y = ligne), dans la marge autour du
    // plateau, en cohérence avec la notation (x,y). Colonnes au-dessus, lignes
    // à gauche ; dessinées hors du plateau pour ne masquer aucune case.
    {
        QFont fnum = painter.font();
        fnum.setPointSize(9 * SPRITE_WIDTH / 32);
        fnum.setBold(true);
        painter.setFont(fnum);
        painter.setPen(QColor(0xc0, 0x39, 0x2b));
        for (int x = 0; x < largeur; x++) {
            const QRectF r(coinCase(x, -1), QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));
            painter.drawText(r, Qt::AlignCenter, QString::number(x));
        }
        for (int y = 0; y < hauteur; y++) {
            const QRectF r(coinCase(-1, y), QSizeF(SPRITE_WIDTH, SPRITE_HEIGHT));
            painter.drawText(r, Qt::AlignCenter, QString::number(y));
        }
    }

    // Le perso et la caisse qu'il pousse, par-dessus la grille : pendant un
    // glissement ils chevauchent deux cases, et le perso dépasse en hauteur de la
    // sienne.
    const QPointF p = positionPerso();

    if (animEnCours && animCaisseIdx >= 0) {
        // La caisse reste exactement une case devant le perso pendant tout le
        // glissement — ils avancent du même pas, il suffit donc de décaler.
        const QPointF pas = QPointF(game->getPlayerPoint()) - animDepart;
        const Sprite *s = (game->getCase(animCaisseIdx) == Level::tcGoalCaisse)
                              ? static_cast<const Sprite *>(&caisseSurBut)
                              : static_cast<const Sprite *>(&caisse);
        s->dessine(painter, coinCase(p.x() + pas.x(), p.y() + pas.y()));
    }

    // Deux poses par case traversée : le cycle avance pendant le glissement, au
    // lieu de figer une image par coup.
    const int sousPas = pasMarche * 2 + ((animEnCours && progressionAnim() >= 0.5) ? 1 : 0);
    perso.dessine(painter, coinCase(p.x(), p.y()), Player::frame(direction, sousPas));

    // Les stats restent collées au coin haut-gauche de ce qui est VISIBLE : le
    // plateau est plus grand que la vue et défile pour suivre le perso, un texte
    // posé sur le widget sortirait de l'écran.
    QRect vue = visibleRegion().boundingRect();
    if (vue.isEmpty()) vue = rect();

    // Avant le panneau : le voile couvre le plateau, mais les stats doivent
    // rester lisibles — c'est là qu'on suit les états explorés pendant l'attente.
    if (resolution) dessineSpinner(painter, vue);

    QFont font = painter.font();
    font.setPointSize(14);
    font.setBold(true);
    painter.setFont(font);

    // Libellés abrégés : le panneau flotte au-dessus du plateau, chaque
    // caractère de trop est une case masquée.
    //   Nv niveau, Dj déplacements du joueur, Pc poussées de caisses, Ex états
    //   explorés, Tr temps de résolution.
    QStringList stats = {
        QString("Nv : %1").arg(game->getNumNiveau()),
        QString("Dj : %1").arg(game->getNbDep()),
        QString("Pc : %1").arg(game->getNbDepCaisse()),
        QString("Ex : %1").arg(formaterMillier(etatsExplores)),
        QString("Tr : %1s").arg(duree),
    };
    // Zj : le CARDINAL de la zone du joueur. C'est lui qu'on lit en naviguant ◀ ▶ —
    // l'aplat dit QUELLES cases, ce nombre dit COMBIEN, et une manœuvre qui déplace
    // la zone à cardinal constant se voit alors sur l'aplat et pas sur le nombre.
    // Les deux sont nécessaires, c'est le sens de « plus de cases OU d'autres ».
    if (showZone) {
        QVector<bool> z; game->getZoneJoueur(z);
        stats << QString("Zj : %1").arg(z.count(true));
    }
    // Écrit EN TOUTES LETTRES, en plus du cadre sur la caisse : le cadre dit
    // LAQUELLE, ce mot dit si on a le droit d'étiqueter. Deux témoins du même fait,
    // c'est ce qui a sauvé deux mesures aujourd'hui (POUSSE caisse contre POUSSE,
    // et le pluriel de « macros jouables »).
    if (caissePoussee >= 0)
        stats << (pousseeChoisie ? QString("Coup : A TOI") : QString("Coup : macro"));

    // Fond opaque sous le texte : il croise sinon les chiffres de la règle des
    // colonnes, et se perd dans les tuiles dès qu'il passe sur le plateau.
    const QFontMetrics fm(font);
    int largeurTexte = 0;
    for (const QString& s : stats) largeurTexte = qMax(largeurTexte, fm.horizontalAdvance(s));

    const int interligne = fm.height() + 2;
    rectPanneau = QRect(vue.left() + 8, vue.top() + 8,
                        largeurTexte + 20, interligne * stats.size() + 12);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 215));
    painter.drawRoundedRect(rectPanneau, 6, 6);

    painter.setPen(QColor(0x29, 0x80, 0xb9));
    for (int i = 0; i < stats.size(); i++) {
        painter.drawText(rectPanneau.left() + 10, rectPanneau.top() + 6 + fm.ascent() + i * interligne, stats[i]);
    }
}

// Le perso s'endort le temps que le solveur cherche. Il n'y a pas de pose de
// sommeil dans la planche : c'est la vue de face, au repos, qu'on fait respirer
// sous trois « Z » qui montent.
void WGame::dessineSpinner(QPainter& painter, const QRect& vue) {
    // Voile sombre plutôt qu'opaque : le plateau reste devinable derrière, on
    // comprend qu'il est figé et non disparu.
    painter.fillRect(vue, QColor(0, 0, 0, 120));

    const qreal t = (chronoSpinner.elapsed() % cycleSpinner) / qreal(cycleSpinner);
    const QPointF centre(vue.center());

    // Doublé, sinon il se perd au milieu du plateau ; et soulevé au rythme d'une
    // respiration lente.
    const qreal souffle = std::sin(t * 2 * M_PI) * 2.0;

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.save();
    painter.translate(centre.x(), centre.y() + souffle);
    painter.scale(2.0, 2.0);
    perso.dessine(painter, QPointF(-SPRITE_WIDTH / 2.0, -SPRITE_HEIGHT / 2.0),
                  Player::frame(Game::dBas, 0));
    painter.restore();

    // Trois « Z » décalés d'un tiers de cycle : ils s'élèvent en grossissant et
    // s'effacent, de sorte qu'il y en a toujours un en vue.
    QFont fz = painter.font();
    fz.setBold(true);
    for (int i = 0; i < 3; i++) {
        const qreal p = std::fmod(t + i / 3.0, 1.0);

        fz.setPointSize(11 + int(p * 9));
        painter.setFont(fz);
        painter.setPen(QColor(255, 255, 255, int(230 * (1.0 - p))));
        painter.drawText(QPointF(centre.x() + SPRITE_WIDTH * 0.9 + p * 14,
                                 centre.y() - SPRITE_HEIGHT * 0.6 - p * 46), "Z");
    }

    QFont ft = painter.font();
    ft.setPointSize(13);
    ft.setBold(true);
    painter.setFont(ft);
    painter.setPen(QColor(255, 255, 255, 230));
    painter.drawText(QRect(vue.left(), int(centre.y() + SPRITE_HEIGHT * 1.3), vue.width(), 40),
                     Qt::AlignHCenter | Qt::AlignTop, tr("Résolution en cours…"));
}

void WGame::mouseMoveEvent(QMouseEvent *event) {
    setToolTip(rectPanneau.contains(event->pos())
                   ? tr("<b>Nv</b> niveau<br>"
                        "<b>Dj</b> déplacements du joueur<br>"
                        "<b>Pc</b> poussées de caisses<br>"
                        "<b>Ex</b> états explorés par le solveur<br>"
                        "<b>Tr</b> temps de résolution")
                   : QString());

    QWidget::mouseMoveEvent(event);
}

void WGame::mousePressEvent(QMouseEvent *event) {
    if (game && game->isLoaded()
        && (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) {
        // Inverse de coinCase() : mêmes marges, la case est le quotient entier
        // (pas le point) — un clic n'importe où sur une tuile doit désigner
        // cette case entière.
        const qreal margX = (width()  - game->getLargeur() * SPRITE_WIDTH)  / 2.0;
        const qreal margY = (height() - game->getHauteur() * SPRITE_HEIGHT) / 2.0;
        const int cx = static_cast<int>(std::floor((event->pos().x() - margX) / SPRITE_WIDTH));
        const int cy = static_cast<int>(std::floor((event->pos().y() - margY) / SPRITE_HEIGHT));

        if (cx >= 0 && cx < game->getLargeur() && cy >= 0 && cy < game->getHauteur()) {
            const int idx = cx + cy * game->getLargeur();
            if (event->button() == Qt::LeftButton) emit caseCliquee(idx);
            else                                   emit caseSignalee(idx);
        }
    }

    QWidget::mousePressEvent(event);
}

void WGame::showPassage(bool show) {
    this->show = show;
    update();
}

void WGame::showChampButActif(bool show) {
    showChamp = show;
    update();
}

void WGame::setMontreOrdreButs(bool on) {
    montreOrdreButs = on;
    update();
}

void WGame::setDuree(double duree) {
    this->duree = duree;
    update();
}
