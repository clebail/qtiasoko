// zonembut — LES ZONES D'EMBUT D'UN NIVEAU, EN .xsb (2026-08-22, idée utilisateur).
//
// Une zone d'embut est l'ENCLOS qui enferme un bloc de buts adjacents. L'outil
// n'en calcule RIEN lui-même : il appelle `Game::zonesEmbut()` (game.cpp), en
// exemplaire unique — la règle du §7, celle qui a fait remonter
// `butMureLocalement` de `mesures/ordre.cpp` dans le moteur. Ici on ne fait que
// DESSINER ce que le moteur a décidé.
//
//   zonembut               les 32 niveaux, un .xsb par zone
//   zonembut 16            un seul niveau, imprimé aussi à l'écran
//   zonembut p21.xsb       un plateau exporté (même convention que bench/ordre/pas0)
//   SEUIL=n zonembut …     le seuil de cul-de-sac (défaut 3), pour le balayer
//
// SORTIE : `zone_nivNN_zK.xsb`, un par zone, dans le répertoire courant.
// Convention de caractères de l'app (MainWindow::onExportXsb) — un fichier produit
// ici se recharge donc tel quel dans `image`, `bench` ou l'application.
//   '#' mur de l'enclos · '.' embut · '*' caisse DÉJÀ POSÉE sur un embut
//   ' ' le sol, les portes, et tout ce qui est hors de la zone
// ⚠️ Les caisses hors embut et le joueur sont RETIRÉS : la zone décrit la
// géométrie du rangement, pas la position d'une partie.
// ⚠️ Une PORTE est un trou dans le mur, pas un caractère : rien dans le format
// .xsb ne peut la marquer sans faire de la sortie un faux .xsb (`Level::load`
// traduit tout caractère inconnu en case vide — le piège §7). Les coordonnées
// des portes partent donc sur la sortie standard, à côté du dessin.

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <cstdio>
#include "game.h"
#include "solveur.h"
#include <QEventLoop>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <algorithm>

// Le rendu d'une zone. Cadre = la boîte englobante de (l'enclos + ses murs + ses
// portes) : c'est ce qui fait apparaître la porte comme un TROU dans la ligne de
// mur, exactement comme sur un plateau complet.
// Les murs retenus sont ceux qui TOUCHENT la zone, diagonales comprises — sans les
// diagonales, le contour s'ouvre à chaque angle sortant et l'enclos ne se lit plus
// comme un enclos.
static QStringList dessine(const Game& g, const Game::ZoneEmbut& z, QPoint* origine = nullptr) {
    const int L = g.getLargeur(), H = g.getHauteur();
    QVector<bool> zone(L * H, false), mur(L * H, false), porte(L * H, false);
    for (int c : z.cases)  zone[c]  = true;
    for (int c : z.portes) porte[c] = true;

    static const int dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int dy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    for (int c = 0; c < L * H; c++) {
        if (!zone[c]) continue;
        for (int d = 0; d < 8; d++) {
            const int x = c % L + dx[d], y = c / L + dy[d];
            if (x < 0 || x >= L || y < 0 || y >= H) continue;
            if (g.getCase(x + y * L) == Level::tcMur) mur[x + y * L] = true;
        }
    }

    int x0 = L, x1 = -1, y0 = H, y1 = -1;
    for (int c = 0; c < L * H; c++) {
        if (!zone[c] && !mur[c] && !porte[c]) continue;
        const int x = c % L, y = c / L;
        x0 = qMin(x0, x); x1 = qMax(x1, x);
        y0 = qMin(y0, y); y1 = qMax(y1, y);
    }
    QStringList lignes;
    if (x1 < 0) return lignes;
    if (origine) *origine = QPoint(x0, y0);
    for (int y = y0; y <= y1; y++) {
        QString s;
        for (int x = x0; x <= x1; x++) {
            const int c = x + y * L;
            const Level::ETypeCase t = g.getCase(c);
            if (mur[c])                             s += '#';
            else if (zone[c] && t == Level::tcGoalCaisse) s += '*';
            else if (zone[c] && (t == Level::tcGoal || t == Level::tcGoalPlayer)) s += '.';
            else                                    s += ' ';
        }
        lignes << s;
    }
    return lignes;
}

// LE NIVEAU SIMPLE (2026-08-22, idée utilisateur — référence : niveau1embutSeul.txt).
// La zone d'embut, enrobée d'un COULOIR, refermée par un MUR, et les embuts non
// remplis pourvus chacun d'une caisse posée dans le couloir.
//
// Raison d'être : isoler le RANGEMENT du DÉMÊLAGE (§3/§4). Dans ces niveaux, il n'y
// a plus de transport — les caisses sont déjà à pied d'œuvre —, il ne reste que la
// question « dans quel ordre remplir, et par où entrer ». C'est le banc d'essai que
// le goal-ordering n'avait jamais eu.
//
// TROIS RÈGLES, toutes lues sur le croquis de l'utilisateur :
//   · les caisses vont dans la VOIE INTÉRIEURE du couloir (collées au mur de la
//     zone), jamais dans la voie extérieure, qui reste libre pour le personnage ;
//   · la zone GARDE SES MURS : une caisse n'entre que par une porte, et c'est ce qui
//     fait de ces niveaux un test d'ordre et non un test de transport ;
//   · le personnage est posé sur la case libre d'INDICE LE PLUS FAIBLE.
//
// ⚠️ POURQUOI LA VOIE INTÉRIEURE NE PRODUIT AUCUN DEADLOCK, et pourquoi un couloir
// d'UNE case en produirait : collée au mur de la zone, une caisse ne peut être
// poussée que le LONG de la voie (la pousser vers la zone la jetterait dans le mur ;
// la pousser vers l'extérieur demanderait au joueur de se tenir dans ce mur). Elle
// glisse donc jusqu'à la porte. Dans un couloir d'une seule case, une caisse posée
// dans un COUDE a ses deux voisins libres perpendiculaires : le joueur ne peut jamais
// s'aligner, elle est morte avant le premier coup.
static QStringList niveauSimple(const QStringList& zone,
                                const QVector<QPoint>& portesCadre,
                                const QSet<int>& zoneCadre,
                                QPoint* offset = nullptr) {
    const int zh = zone.size();
    int zw = 0;
    for (const QString& l : zone) zw = qMax(zw, l.size());
    auto at = [&](int x, int y) {
        return (y >= 0 && y < zh && x >= 0 && x < zone[y].size()) ? zone[y][x] : QChar(' ');
    };

    int nbCaisses = 0;
    for (const QString& l : zone) for (QChar c : l) if (c == '.') nbCaisses++;

    // LES CÔTÉS : 0 haut, 1 bas, 2 gauche, 3 droite.
    // Un côté OUVERT est un côté dont la bordure de la zone laisse passer (une
    // porte y débouche). On y pose les caisses en priorité : elles n'auront qu'à
    // glisser le long de leur voie pour entrer, sans faire le tour.
    const int longueur[4] = {zw, zw, zh, zh};

    // ⚠️ UN CÔTÉ EST OUVERT SI UNE VRAIE PORTE Y DÉBOUCHE — pas si la bordure du
    // cadre porte un blanc. Dans un `.xsb` un blanc est AMBIGU : sol de la zone,
    // porte, ou cellule hors zone tombée dans le cadre. Le premier jet lisait les
    // blancs et croyait le bas du niveau 18 ouvert : trois caisses sur quatre y
    // étaient posées, sans aucun moyen d'entrer. Le solveur a répondu ESPACE
    // ÉPUISÉ, et il avait raison.
    // Pour chaque porte on remonte le trajet d'entrée à REBOURS (la caisse arrive
    // de l'autre côté de la zone) : si on sort du cadre sans rencontrer ni mur ni
    // case de zone, ce côté est desservi par cette porte.
    static const int dxc[4] = {0, 0, -1, 1}, dyc[4] = {-1, 1, 0, 0};   // haut bas gauche droite
    bool ouvert[4] = {false, false, false, false};
    QSet<int> frontX, frontY;   // colonnes / lignes situées EN FACE d'une porte
    for (const QPoint& p : portesCadre) {
        for (int d = 0; d < 4; d++) {
            const int zx = p.x() + dxc[d], zy = p.y() + dyc[d];
            if (!zoneCadre.contains(zx + zy * zw)) continue;            // pas la zone de ce côté
            int x = p.x() - dxc[d], y = p.y() - dyc[d];
            bool sort = true;
            while (x >= 0 && x < zw && y >= 0 && y < zh) {
                if (at(x, y) == '#' || zoneCadre.contains(x + y * zw)) { sort = false; break; }
                x -= dxc[d]; y -= dyc[d];
            }
            if (!sort) continue;
            // le côté atteint est celui vers lequel on remontait
            // ⚠️ RIEN EN FACE D'UNE PORTE (constat utilisateur) : une caisse garée
            // pile devant oblige à la manœuvrer avant de pouvoir se servir de la
            // porte — un déplacement de plus avant la moindre poussée utile.
            if (dyc[d] > 0)      { ouvert[0] = true; frontX.insert(p.x()); }
            else if (dyc[d] < 0) { ouvert[1] = true; frontX.insert(p.x()); }
            else if (dxc[d] > 0) { ouvert[2] = true; frontY.insert(p.y()); }
            else                 { ouvert[3] = true; frontY.insert(p.y()); }
        }
    }
    // Les côtés ouverts d'abord, puis les plus longs : moins de voies à ouvrir.
    QVector<int> ordre = {0, 1, 2, 3};
    std::stable_sort(ordre.begin(), ordre.end(), [&](int a, int b) {
        if (ouvert[a] != ouvert[b]) return ouvert[a];
        return longueur[a] > longueur[b];
    });

    // COMBIEN DE VOIES PAR CÔTÉ. On étale d'abord une voie sur chaque côté utile,
    // puis on en rajoute — de sorte qu'un côté sans caisse garde une marge de 1
    // (le personnage y passe, ça suffit) au lieu d'être élargi pour rien. C'est
    // toute la différence avec l'anneau uniforme : le plateau ne grandit que là
    // où il le faut, et un plateau plus petit, c'est moins d'états à explorer.
    // ⚠️ UNIQUEMENT SUR LES CÔTÉS OUVERTS, et ce n'est PAS une optimisation : une
    // caisse posée sur un côté FERMÉ n'atteint JAMAIS la salle. Dans un couloir
    // elle ne peut que glisser le long de sa voie — la pousser perpendiculairement
    // demanderait au personnage de se tenir au-delà d'elle, donc DANS le mur.
    // Elle ne peut donc pas tourner un coin, et si la porte est de l'autre côté,
    // elle est perdue d'avance. Mesuré : le premier jet posait les caisses tout
    // autour et le solveur a rendu ESPACE ÉPUISÉ (et non budget dépassé) sur trois
    // zones — confirmé par `bench` en `macro` ET en `coupl-plongeon`.
    bool aucunOuvert = true;
    for (int c = 0; c < 4; c++) if (ouvert[c]) aucunOuvert = false;
    int voies[4] = {0, 0, 0, 0};
    QStringList out;

    for (int tentative = 0; tentative < 60; tentative++) {
        int reste = nbCaisses;
        for (int c = 0; c < 4; c++) voies[c] = 0;
        // Estimation : une voie tient (longueur+1)/2 caisses. Elle est OPTIMISTE
        // (le décalage des voies et les bords en retirent), d'où les voies
        // supplémentaires ajoutées à chaque tentative — sans elles la boucle de
        // réessai recalculait la même chose indéfiniment et rendait un plateau
        // VIDE, ce qui ne se signalait pas.
        while (reste > 0) {
            bool progres = false;
            for (int k = 0; k < 4 && reste > 0; k++) {
                const int c = ordre[k];
                if (!ouvert[c] && !aucunOuvert) continue;
                voies[c]++;
                reste -= qMin(reste, (longueur[c] + 1) / 2);
                progres = true;
            }
            if (!progres) break;
        }
        for (int t = 0; t < tentative; t++)
            for (int k = 0; k < 4; k++) {
                const int c = ordre[k];
                if (!ouvert[c] && !aucunOuvert) continue;
                voies[c]++;
                break;
            }

        // LA MARGE (règle utilisateur, 2026-08-22) :
        //   · 2 au moins sur un côté qui porte une PORTE — une voie pour les
        //     caisses, une pour le personnage derrière elles ;
        //   · 1 partout ailleurs. Cette case n'est pas du luxe : sans elle, les
        //     EXTRÉMITÉS d'une voie sont inutilisables — pour pousser la caisse du
        //     bout, le personnage devrait se tenir dans le coin, muré. C'est ce qui
        //     condamnait une caisse du niveau 10.
        // ⚠️ Le tour de la salle devient impossible : pour passer d'une porte à
        // l'autre, le personnage traverse la salle. Assumé, et fidèle au vrai
        // niveau, où il ne peut pas davantage faire le tour.
        // ⚠️ DEUX PARTOUT (décision utilisateur, 2026-08-22, après plusieurs tours
        // de réglage). Ce n'est pas optimal en surface, c'est ROBUSTE : avec deux
        // cases, une caisse peut tourner les coins — ses quatre voisines sont
        // libres dans l'angle — et rejoindre n'importe quelle porte, et le
        // personnage circule toujours. On cesse ainsi de devoir
        // DEVINER quelle porte sert à quoi — ce qu'aucune analyse statique ne sait
        // faire. Le cas qui a tranché : sur la zone 0 du niveau 18, la porte du
        // haut-gauche ne sert QU'AU PERSONNAGE, et trois caisses sur quatre doivent
        // passer par celle de droite. Rien dans la géométrie ne le dit ; il faut
        // pousser pour le savoir. Le couloir large laisse le solveur en décider.
        int marge[4];
        for (int c = 0; c < 4; c++) marge[c] = qMax(2, voies[c] + 1);

        const int W = zw + marge[2] + marge[3] + (marge[2] ? 1 : 0) + (marge[3] ? 1 : 0);
        const int H = zh + marge[0] + marge[1] + (marge[0] ? 1 : 0) + (marge[1] ? 1 : 0);
        const int c0 = marge[2] + (marge[2] ? 1 : 0), r0 = marge[0] + (marge[0] ? 1 : 0);
        QVector<QString> g(H, QString(W, ' '));
        if (marge[0]) for (int x = 0; x < W; x++) g[0][x] = '#';
        if (marge[1]) for (int x = 0; x < W; x++) g[H - 1][x] = '#';
        if (marge[2]) for (int y = 0; y < H; y++) g[y][0] = '#';
        if (marge[3]) for (int y = 0; y < H; y++) g[y][W - 1] = '#';
        for (int y = 0; y < zh; y++)
            for (int x = 0; x < zone[y].size(); x++)
                g[r0 + y][c0 + x] = zone[y][x];

        // LES CAISSES. Une voie court sur toute la largeur (ou hauteur) INTÉRIEURE
        // du couloir, pas seulement le long de la zone : c'est ce qui donne à la
        // caisse du bout une case derrière elle. On saute d'ailleurs la toute
        // première et la toute dernière case de la voie, contre le mur extérieur,
        // pour la même raison.
        reste = nbCaisses;
        for (int v = 1; reste > 0 && v <= 60; v++) {
            for (int k = 0; k < 4 && reste > 0; k++) {
                const int c = ordre[k];
                if (v > voies[c]) continue;
                if (!ouvert[c] && !aucunOuvert) continue;
                // ⚠️ UNE VOIE LONGE LA ZONE, ET RIEN DE PLUS. Un premier jet les
                // étendait sur toute la largeur du couloir pour rendre leurs
                // extrémités poussables ; les voies du haut et de gauche se
                // recouvraient alors dans les ANGLES et y empilaient des caisses en
                // paquet 2×2, immobiles — trois niveaux fabriqués insolubles de
                // plus. La marge de 1 sur les côtés fermés suffit : la caisse du
                // bout a le personnage juste à côté, dans le coin resté libre.
                const bool horiz = (c == 0 || c == 1);
                const int deb = horiz ? c0 : r0;
                const int lg = horiz ? zw : zh;
                const int aposer = qMin(reste, (lg + 1) / 2);
                const int pas = aposer > 0 ? qMax(2, lg / aposer) : 2;
                // ⚠️ VOIES ALIGNÉES, PAS EN QUINCONCE (constat utilisateur) : en
                // quinconce, la caisse du fond retombe DÉCALÉE par rapport à la
                // porte et doit encore glisser — au moins un déplacement avant
                // une poussée utile. Alignée, elle se pousse tout droit dans la
                // voie que sa devancière vient de libérer.
                for (int i = deb; i < deb + lg && reste > 0; i += pas) {
                    int x, y;
                    if (c == 0)      { x = i; y = r0 - v; }
                    else if (c == 1) { x = i; y = r0 + zh - 1 + v; }
                    else if (c == 2) { x = c0 - v; y = i; }
                    else             { x = c0 + zw - 1 + v; y = i; }
                    if (x < 0 || x >= W || y < 0 || y >= H || g[y][x] != ' ') continue;
                    if (horiz  && frontX.contains(x - c0)) continue;   // en face d'une porte
                    if (!horiz && frontY.contains(y - r0)) continue;
                    g[y][x] = '$';
                    reste--;
                }
            }
        }
        if (reste > 0) continue;                    // pas tout casé : une voie de plus

        for (int i = 0; i < W * H; i++)
            if (g[i / W][i % W] == ' ') { g[i / W][i % W] = '@'; break; }
        if (offset) *offset = QPoint(c0, r0);
        for (const QString& l : g) out << l;
        return out;
    }
    return out;
}

// L'ORDRE PROUVÉ, ET LA PORTE DE CHAQUE EMBUT (2026-08-22, objectif utilisateur).
//
// On résout le NIVEAU SIMPLE de la zone, puis on rejoue sa solution. Elle donne,
// sans rien deviner :
//   · l'ORDRE de remplissage — et il est PROUVÉ JOUABLE, puisqu'il vient d'une
//     partie gagnée, pas d'une précédence déduite. C'est l'autre moitié de
//     l'asymétrie du §6.6 : `precedenceGlobale` et `butMureLocalement` disent ce
//     qui est INTERDIT (condition nécessaire), une solution dit ce qui MARCHE ;
//   · la PORTE par laquelle chaque caisse est entrée ;
//   · d'où le QUOTA par porte — combien de caisses le démêlage devra livrer de
//     quel côté. C'est le cahier des charges de l'étape suivante.
//
// ⚠️ Les zones d'un même niveau sont INDÉPENDANTES (constat utilisateur) : on ne
// produit jamais d'ordre global, un jeu de données par zone.
//
// ⚠️ Un ordre n'est une PERMUTATION que si aucune caisse ne RESSORT d'un but en
// cours de route. Le §6.0 (point 8, niveau 18) montre une partie gagnante qui
// ressort 12 fois une caisse de son but — aucun `ordreButs` ne peut décrire ça.
// On compte donc les sorties : zéro, l'ordre est utilisable tel quel ; sinon il
// est signalé comme NON PERMUTABLE, et c'est un résultat, pas un échec.
struct Extraction {
    bool resolu = false;
    bool epuise = false;                 // espace épuisé (et non budget)
    QVector<int> ordre;                  // cases-buts, dans l'ordre de remplissage
    QVector<int> porte;                  // porte utilisée, même indexation qu'ordre
    int transits = 0;                    // caisses ayant QUITTÉ une case-but
    int etats = 0, poussees = 0;
};

static Extraction extrait(const QStringList& simple, const QString& chemin,
                          const QSet<int>& zoneCases, const QSet<int>& portes,
                          int budgetSecondes) {
    Extraction e;
    QFile f(chemin);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return e;
    { QTextStream o(&f); for (const QString& l : simple) o << l << "\n"; }
    f.close();

    Level level; level.load(chemin);
    if (!level.isLoaded()) return e;
    const Game depart(level, 0);
    const int L = depart.getLargeur(), taille = L * depart.getHauteur();

    // ⚠️ DEUX RÉGIMES, ET C'EST LA LEÇON DU JOUR. `coupl-plongeon` trouve VITE,
    // mais il embarque le régime d'engagement macro, que le §6.0 documente comme
    // INCOMPLET : son `AUCUNE` ne veut pas dire « insoluble », il veut dire « j'ai
    // épuisé mon espace tronqué ». Mesuré en direct : le niveau simple de la zone
    // du 15 en reçoit un — et l'utilisateur l'a résolu À LA MAIN dans l'app.
    // **Seul A* pur peut fournir une vérité.** On le rappelle donc en second, et
    // on ne parle d'insolubilité que s'il épuise, lui.
    QList<Game::EDirection> solution;
    for (int passe = 0; passe < 2 && !e.resolu; passe++) {
        const Solveur::EType type = passe == 0 ? Solveur::AstarMacroCouplagePlongeon
                                               : Solveur::Astar;
        bool aucune = false;
        Solveur* s = Solveur::creer(type, depart);
        QEventLoop boucle;
        QObject::connect(s, &Solveur::solutionTrouvee,
                         [&](QList<Game::EDirection> c, qint64 n) {
            solution = c; e.resolu = true; e.etats = (int)n; boucle.quit();
        });
        QObject::connect(s, &Solveur::aucuneSolution, [&]() { aucune = true; boucle.quit(); });
        QTimer::singleShot(budgetSecondes * 1000, &boucle, &QEventLoop::quit);
        s->start();
        boucle.exec();
        s->demanderArret();
        s->wait();
        delete s;
        // Seul l'épuisement d'A* PUR vaut preuve d'insolubilité.
        if (aucune && passe == 1) e.epuise = true;
        if (!aucune) break;                     // budget dépassé : inutile d'insister
    }
    if (!e.resolu) return e;

    // REJEU. Une poussée déplace exactement une caisse : on suit son identité pour
    // savoir par quelle porte ELLE est entrée, et non « une caisse quelconque ».
    Game g(depart);
    auto caisses = [&](const Game& j) {
        QSet<int> c;
        for (int i = 0; i < taille; i++)
            if (j.getCase(i) == Level::tcCaisse || j.getCase(i) == Level::tcGoalCaisse) c.insert(i);
        return c;
    };
    QSet<int> avant = caisses(g);
    QHash<int, int> porteDe;              // case de la caisse -> porte empruntée
    QHash<int, int> arrivee;              // case-but -> n° du coup de la DERNIÈRE pose
    int coup = 0;
    for (Game::EDirection d : solution) {
        g.deplace(d);
        coup++;
        const QSet<int> apres = caisses(g);
        if (apres == avant) continue;
        const QSet<int> partie = avant - apres, venue = apres - avant;
        if (partie.size() != 1 || venue.size() != 1) { avant = apres; continue; }
        const int de = *partie.begin(), vers = *venue.begin();
        e.poussees++;
        // la caisse hérite de son historique
        const int heritage = porteDe.value(de, -1);
        porteDe.remove(de);
        porteDe.insert(vers, heritage);
        // entrée dans la zone : la case QUITTÉE est la porte
        if (!zoneCases.contains(de) && zoneCases.contains(vers) && portes.contains(de))
            porteDe[vers] = de;
        if (g.getCase(vers) == Level::tcGoalCaisse) arrivee[vers] = coup;
        // ⚠️ TRANSIT, PAS SORTIE. Une caisse qui quitte une case-but n'a pas
        // forcément été « rangée puis dérangée » : le plus souvent elle ne fait
        // que PASSER dessus pour atteindre un but plus profond — c'est le cas dès
        // le niveau 1, où l'on ne peut pas atteindre (17,6) sans traverser (16,6).
        // Confondre les deux est exactement le piège qui a produit ~100 faux
        // positifs au premier jet de `fpporte.py` (§1, « une pose sur un but n'est
        // pas une livraison »). Ce qu'on compte ici est donc une information sur la
        // SALLE — peut-on la remplir sans jamais se servir d'un but comme couloir ?
        // — et non un défaut de l'ordre : l'ordre des poses DÉFINITIVES est
        // toujours une permutation.
        if (depart.getCase(de) == Level::tcGoal || depart.getCase(de) == Level::tcGoalCaisse) {
            e.transits++; arrivee.remove(de);
        }
        avant = apres;
    }
    QVector<int> buts = arrivee.keys().toVector();
    std::sort(buts.begin(), buts.end(), [&](int a, int b) { return arrivee[a] < arrivee[b]; });
    for (int b : buts) { e.ordre.append(b); e.porte.append(porteDe.value(b, -1)); }
    return e;
}

static int traite(const QString& source, int num, bool imprime, bool modeOrdre = false) {
    Level level;
    level.load(source);
    if (!level.isLoaded()) { fprintf(stderr, "zonembut: niveau introuvable (%s)\n", qPrintable(source)); return 0; }
    Game g(level, num);

    const int seuil = qEnvironmentVariableIsSet("SEUIL")
                    ? qEnvironmentVariableIntValue("SEUIL") : 4;
    const QVector<Game::ZoneEmbut> zones = g.zonesEmbut(seuil);
    const int L = g.getLargeur();

    printf("niveau %d — %d but%s, %d zone%s\n", num, g.getNbButs(), g.getNbButs() > 1 ? "s" : "",
           zones.size(), zones.size() > 1 ? "s" : "");
    for (int i = 0; i < zones.size(); i++) {
        const Game::ZoneEmbut& z = zones[i];
        QString portes;
        for (int c : z.portes) portes += QString(" (%1,%2)").arg(c % L).arg(c / L);
        printf("  zone %d : %d embuts, %d cases, %d porte%s%s\n", i,
               (int)z.buts.size(), (int)z.cases.size(), (int)z.portes.size(),
               z.portes.size() > 1 ? "s" : "",
               z.portes.isEmpty() ? " — AUCUNE (enclos fermé)" : qPrintable(portes));

        QPoint origine;
        const QStringList lignes = dessine(g, z, &origine);
        const QString nom = QString("zone_niv%1_z%2.xsb").arg(num, 2, 10, QChar('0')).arg(i);
        QFile f(nom);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            fprintf(stderr, "zonembut: écriture impossible (%s)\n", qPrintable(nom));
            return zones.size();
        }
        QTextStream out(&f);
        for (const QString& l : lignes) out << l << "\n";
        if (imprime) for (const QString& l : lignes) printf("    |%s\n", qPrintable(l));
        printf("    -> %s\n", qPrintable(nom));

        // LE NIVEAU SIMPLE dérivé de cette zone (cf. niveauSimple).
        // Les portes et les cases de la zone, en coordonnées du CADRE : c'est ce
        // qui permet à l'enrobage de savoir quels côtés sont réellement desservis,
        // au lieu de le deviner sur des blancs ambigus.
        QVector<QPoint> portesCadre;
        QSet<int> zoneCadre;
        // ⚠️ La LARGEUR MAX, pas celle de la première ligne : `dessine` rogne les
        // fins de ligne, donc elles n'ont pas toutes la même longueur, et
        // `niveauSimple` indexe sur le maximum. Un pas d'indexation différent des
        // deux côtés décalerait silencieusement toutes les portes.
        int lgCadre = 1;
        for (const QString& l : lignes) lgCadre = qMax(lgCadre, l.size());
        for (int c : z.cases)
            zoneCadre.insert((c % L - origine.x()) + (c / L - origine.y()) * lgCadre);
        for (int c : z.portes)
            portesCadre.append(QPoint(c % L - origine.x(), c / L - origine.y()));
        QPoint offset;
        const QStringList simple = niveauSimple(lignes, portesCadre, zoneCadre, &offset);
        const QString nomS = QString("simple_niv%1_z%2.xsb").arg(num, 2, 10, QChar('0')).arg(i);
        QFile fs(nomS);
        if (fs.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream os(&fs);
            for (const QString& l : simple) os << l << "\n";
        }
        int caisses = 0;
        for (const QString& l : simple) for (QChar c : l) if (c == '$') caisses++;
        printf("    -> %s  (%dx%d, %d caisses posees)\n",
               qPrintable(nomS), simple.isEmpty() ? 0 : simple.first().size(),
               (int)simple.size(), caisses);
        if (imprime) for (const QString& l : simple) printf("    |%s\n", qPrintable(l));

        if (!modeOrdre) continue;
        // ── L'ORDRE PROUVÉ ET LA PORTE DE CHAQUE EMBUT ──────────────────────────
        const int Ls = simple.isEmpty() ? 1 : simple.first().size();
        auto versSimple = [&](int c) {
            const int x = c % L - origine.x() + offset.x();
            const int y = c / L - origine.y() + offset.y();
            return x + y * Ls;
        };
        auto versReel = [&](int c) {
            const int x = c % Ls - offset.x() + origine.x();
            const int y = c / Ls - offset.y() + origine.y();
            return QPoint(x, y);
        };
        QSet<int> zoneCases, portesSimple;
        for (int c : z.cases)  zoneCases.insert(versSimple(c));
        for (int c : z.portes) portesSimple.insert(versSimple(c));

        const QString tmp = QString("simple_niv%1_z%2.xsb").arg(num, 2, 10, QChar('0')).arg(i);
        // BUDGET réglable : 30 s suffisent aux deux tiers du corpus, les grosses
        // salles à porte unique en demandent davantage.
        const int budget = qEnvironmentVariableIsSet("BUDGET")
                         ? qEnvironmentVariableIntValue("BUDGET") : 30;
        const Extraction e = extrait(simple, tmp, zoneCases, portesSimple, budget);
        if (!e.resolu) {
            // ⚠️ Le budget est AFFICHÉ, pas codé en dur dans le texte : la première
            // version disait « en 30 s » quoi qu'il arrive, y compris pendant un
            // balayage à 300 s. Un message qui ment sur son propre protocole est
            // pire qu'une absence de message.
            int posees = 0;
            for (const QString& l : lignes) posees += l.count('*');
            if (posees) printf("    [ordre] ⚠ %d caisse(s) DEJA POSEE(S) — cf. le cas du 15\n", posees);
            printf("    [ordre] %s en %d s — aucun ordre extrait\n",
                   e.epuise ? "ESPACE EPUISE (A* pur, insoluble)" : "budget depasse", budget);
            continue;
        }
        QHash<int,int> quota;
        QString det;
        for (int k = 0; k < e.ordre.size(); k++) {
            const QPoint b = versReel(e.ordre[k]);
            const QPoint p = e.porte[k] >= 0 ? versReel(e.porte[k]) : QPoint(-1, -1);
            det += QString("  %1. but (%2,%3) par porte %4\n").arg(k + 1, 2)
                   .arg(b.x()).arg(b.y())
                   .arg(e.porte[k] >= 0 ? QString("(%1,%2)").arg(p.x()).arg(p.y())
                                        : QString("— (deja posee au depart)"));
            if (e.porte[k] >= 0) quota[e.porte[k]]++;
        }
        QString parPorte;
        for (auto it = quota.constBegin(); it != quota.constEnd(); ++it) {
            const QPoint p = versReel(it.key());
            parPorte += QString(" (%1,%2)x%3").arg(p.x()).arg(p.y()).arg(it.value());
        }
        // ⚠️ SIGNALER LES CAISSES DÉJÀ POSÉES. Mesuré le 2026-08-22 : la zone du
        // niveau 15, privée de ses deux caisses posées, se résout en 171 états ;
        // avec elles, `coupl-plongeon` ÉPUISE son espace. Les ressortir d'un but
        // est le point dur. Sans cette mention, leur échec se relit comme une
        // difficulté ordinaire, ce qu'il n'est pas.
        int posees = 0;
        for (const QString& l : lignes) posees += l.count('*');
        if (posees) printf("    [ordre] ⚠ %d caisse(s) DEJA POSEE(S) dans cette zone\n", posees);
        printf("    [ordre] %d embuts ordonnes, %d poussees, %d etats%s\n",
               (int)e.ordre.size(), e.poussees, e.etats,
               e.transits ? qPrintable(QString(" — %1 transit(s) par un embut").arg(e.transits))
                          : " — aucun transit par un embut");
        printf("    [ordre] quota par porte :%s\n", qPrintable(parPorte.isEmpty() ? QString(" —") : parPorte));
        if (imprime) printf("%s", qPrintable(det));
        const QString nomO = QString("ordre_niv%1_z%2.txt").arg(num, 2, 10, QChar('0')).arg(i);
        QFile fo(nomO);
        if (fo.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream oo(&fo);
            oo << "# ordre PROUVE JOUABLE, extrait de la solution du niveau simple\n";
            oo << "# niveau " << num << " zone " << i << " — " << e.poussees << " poussees, "
               << e.etats << " etats, " << e.transits << " transit(s) par une case-but\n";
            oo << "# quota par porte :" << parPorte << "\n";
            oo << det;
        }
        printf("    -> %s\n", qPrintable(nomO));
    }
    return zones.size();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // Sans argument : les 32 niveaux d'un coup. C'est la demande d'origine — voir
    // d'un seul regard ce que la règle produit sur tout le corpus, un niveau ne
    // prouvant jamais rien tout seul (§6.6).
    const bool modeOrdre = (argc > 2 && QString(argv[2]) == "ordre")
                        || (argc > 1 && QString(argv[1]) == "ordre");
    if (argc < 2 || (argc == 2 && modeOrdre)) {
        int total = 0;
        for (int n = 1; n <= 32; n++)
            total += traite(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(n, 4, 10, QChar('0')), n, false, modeOrdre);
        printf("\n%d zones sur les 32 niveaux.\n", total);
        return 0;
    }
    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();
    traite(parChemin ? arg1
                     : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')),
           num, true, modeOrdre);
    return 0;
}
