// loi <niv> — LA LOI DE L'ORDRE, imprimée puis JUGÉE (§6.2, 2026-08-04).
//
// La loi (règle complète dans game.h) recalcule les cases mortes but par but : une
// caisse ne peut pas se tenir sur une case d'où plus rien n'atteint le BUT ACTIF,
// sauf si elle est alignée avec lui sans mur entre les deux.
//
// Cet outil sert à DEUX choses, et la seconde est la seule qui compte :
//   `loi <niv>`                — imprime la table, un plateau par but, dans le
//                                format exact du gabarit de dessin ;
//   `loi <niv> <gabarit.txt>`  — LE TEST EN OR : compare la table CALCULÉE au
//                                dessin FAIT À LA MAIN sur ce gabarit, case par
//                                case, et rend un code de sortie non nul au
//                                moindre écart.
//
// ⚠️ Pourquoi ce juge existe. La loi n'est pas dérivée d'un théorème : elle a été
// dessinée à la main sur les quinze plateaux du niveau 16, puis jugée sur 24 parties
// humaines gagnantes. Le code qui la met en œuvre n'a donc aucune vérité de
// référence — sauf ce dessin. Un écart ici veut dire que le solveur n'applique pas
// la règle qui a été validée, et le canari ne le dirait jamais (un élagage trop
// mordant ne casse que des niveaux qu'on ne finit pas de toute façon).
//
// ⚠️ Le gabarit porte un DÉCALAGE de 3 colonnes (marge de gauche du dessin). Il est
// lu ici sur la géométrie, pas en dur : la ligne du but actif donne (x,y), on
// retrouve la colonne du 'A' correspondant, et l'écart est le décalage.
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QVector>
#include <cstdio>
#include "game.h"
#include "level.h"

// Le SURPLUS de la loi pour le but 'b' : les cases qu'elle tue et que la table
// ordinaire ne tuait pas. C'est l'objet que l'utilisateur a dessiné — les cases déjà
// mortes le sont dans tous les cas, les marquer n'aurait rien appris, et le
// remplissage hors contour (mort partout) noierait le plateau.
static QVector<QPoint> mortes(const Game& g, int b) {
    QVector<QPoint> v;
    const int L = g.getLargeur(), H = g.getHauteur();
    for (int c = 0; c < L * H; c++)
        if (g.caseMorteLoi(b, c) && !g.caseMorteOrdinaire(c)) v.append(QPoint(c % L, c / L));
    return v;
}

static QString liste(const QVector<QPoint>& v) {
    QString s;
    for (const QPoint& p : v) s += QString("(%1,%2)").arg(p.x()).arg(p.y());
    return s.isEmpty() ? QString("[]") : s;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: loi <niv|plateau.xsb> [gabarit.txt]\n"); return 2; }
    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "loi: niveau introuvable (%s)\n", argv[1]); return 2; }
    Game g(level, num);

    // ⚠️ UN PLATEAU DE MILIEU DE PARTIE N'EST PAS SON NIVEAU (§7). Chargé ici, il
    // recalcule TOUT le statique — `ordreButs` compris, depuis SES caisses. Les
    // verdicts par-caisse (gel) restent valides, ils ne dépendent que de la
    // géométrie et de l'état ; les RANGS, eux, ne sont pas ceux du vrai niveau.
    if (parChemin) {
        const int actif = g.butActif();
        printf("=== %s ===\n", qPrintable(arg1));
        printf("but actif (recalcule pour CE plateau) : ");
        if (actif < 0) printf("aucun (etat gagnant)\n");
        else printf("(%d,%d) rang %d\n", g.getCaseBut(actif) % g.getLargeur(),
                    g.getCaseBut(actif) / g.getLargeur(), g.rangDuBut(actif));
        printf("GEL HORS TOUR : %s\n", g.geleHorsTour(actif) ? "🎯 OUI — cet etat serait COUPE"
                                                             : "non — rien de gele hors tour");
        const QVector<bool> m = g.casesMortesLoi(actif);
        QVector<QPoint> v;
        for (int c = 0; c < m.size(); c++) if (m[c]) v.append(QPoint(c % g.getLargeur(), c / g.getLargeur()));
        printf("cases mortes de la loi pour ce but : %s\n", qPrintable(liste(v)));
        return 0;
    }

    const QVector<int>& ordre = g.getOrdreButs();
    const int L = g.getLargeur();

    if (argc < 3) {
        printf("=== NIVEAU %d — LOI DE L'ORDRE, %d buts ===\n\n", num, (int)ordre.size());
        for (int k = 0; k < ordre.size(); k++) {
            const int b = ordre[k], cell = g.getCaseBut(b);
            const QVector<QPoint> m = mortes(g, b);
            printf("rang %2d (%2d,%2d)  %3d mortes  %s\n",
                   k, cell % L, cell / L, (int)m.size(), qPrintable(liste(m)));
        }
        return 0;
    }

    // ── LE TEST EN OR ─────────────────────────────────────────────────────────
    QFile f(argv[2]);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "loi: gabarit illisible (%s)\n", argv[2]); return 2;
    }
    QTextStream in(&f);
    const QStringList lignes = in.readAll().split('\n');

    static const QRegularExpression reTitre(
        "but actif\\s*:\\s*rang\\s*(\\d+)\\s*\\((\\d+),\\s*(\\d+)\\)");

    int plateaux = 0, ecarts = 0;
    for (int i = 0; i < lignes.size(); i++) {
        const auto m = reTitre.match(lignes[i]);
        if (!m.hasMatch()) continue;
        const int rang = m.captured(1).toInt();
        const int bx = m.captured(2).toInt(), by = m.captured(3).toInt();
        if (rang < 0 || rang >= ordre.size()) continue;
        const int b = ordre[rang], cell = g.getCaseBut(b);
        if (cell % L != bx || cell / L != by) {
            printf("rang %2d : ⚠️ le gabarit vise (%d,%d), l'ordre calculé rend (%d,%d)"
                   " — gabarit périmé, comparaison abandonnée\n", rang, bx, by, cell % L, cell / L);
            ecarts++;
            continue;
        }

        // Les lignes du plateau : jusqu'au prochain titre ou à la fin. Le décalage se
        // déduit du 'A' (le but actif), dont on connaît les vraies coordonnées.
        QStringList board;
        for (int j = i + 1; j < lignes.size(); j++) {
            if (reTitre.match(lignes[j]).hasMatch()) break;
            board.append(lignes[j]);
        }
        int decalage = -1, ligne0 = -1;
        for (int y = 0; y < board.size(); y++) {
            const int col = board[y].indexOf('A');
            if (col >= 0) { decalage = col - bx; ligne0 = y - by; break; }
        }
        if (decalage < 0) { printf("rang %2d : pas de 'A' dans le gabarit, ignoré\n", rang); continue; }

        // Le dessin : toutes les cases marquées d'un X.
        QVector<QPoint> dessin;
        for (int y = 0; y < board.size(); y++)
            for (int x = 0; x < board[y].size(); x++)
                if (board[y][x] == 'X') dessin.append(QPoint(x - decalage, y - ligne0));

        QVector<QPoint> calc = mortes(g, b);
        std::sort(dessin.begin(), dessin.end(), [](QPoint a, QPoint c) {
            return a.y() != c.y() ? a.y() < c.y() : a.x() < c.x(); });
        std::sort(calc.begin(), calc.end(), [](QPoint a, QPoint c) {
            return a.y() != c.y() ? a.y() < c.y() : a.x() < c.x(); });

        plateaux++;
        if (dessin == calc) {
            printf("rang %2d (%2d,%2d)  %s  == dessin\n", rang, bx, by, qPrintable(liste(calc)));
        } else {
            ecarts++;
            printf("rang %2d (%2d,%2d)  ⚠️ ÉCART\n", rang, bx, by);
            printf("            calculé : %s\n", qPrintable(liste(calc)));
            printf("            dessiné : %s\n", qPrintable(liste(dessin)));
            QVector<QPoint> enTrop, manquants;
            for (const QPoint& p : calc)   if (!dessin.contains(p)) enTrop.append(p);
            for (const QPoint& p : dessin) if (!calc.contains(p))   manquants.append(p);
            if (!enTrop.isEmpty())
                printf("            EN TROP (la loi codée est trop mordante) : %s\n",
                       qPrintable(liste(enTrop)));
            if (!manquants.isEmpty())
                printf("            MANQUANTS (la loi codée est trop faible) : %s\n",
                       qPrintable(liste(manquants)));
        }
    }

    printf("\n%s — %d plateaux comparés, %d écart(s)\n",
           ecarts ? "❌ LOI NON CONFORME AU DESSIN" : "🎯 LOI VÉRIFIÉE SUR TOUS LES PLATEAUX",
           plateaux, ecarts);
    return ecarts ? 1 : 0;
}
