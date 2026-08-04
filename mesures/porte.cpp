// porte <niv|plateau.xsb> — LA PRÉCÉDENCE CAISSE → BUT (§6.2, 2026-08-04).
//
// Toutes les précédences du projet sont but → but (`ordreParPrecedence`,
// `precedenceGlobale`). Celle-ci est d'une autre espèce, et elle sort du niveau 16 :
//
//   Soit une caisse C, et A(C) l'ensemble des cases d'appui dont le joueur a besoin
//   pour la pousser dans une direction quelconque. Si remplir un but G prive le
//   joueur de TOUT A(C), alors C doit avoir été déplacée AVANT que G ne soit rempli
//   — sinon elle est gelée à vie, sur du sol, hors but.
//
// Sur le 16 elle rend, sans qu'on lui souffle quoi que ce soit : la caisse (10,6)
// n'a qu'une poussée légale (vers l'ouest, appui (11,6) — les trois autres butent
// sur les murs (12,6) et (11,5)) ; (11,6) ne s'atteint qu'en descendant la colonne
// de buts x=12 ; donc toute caisse posée dans cette colonne condamne (10,6). Et
// comme (12,7) est le rang 0, il faut dégager (10,6) AVANT LA PREMIÈRE POSE.
// Mesuré : la partie humaine bascule exactement là — la poussée qui déloge (10,6)
// fait passer le niveau de « AUCUNE en 1 état » à « résolu en 45 états ».
//
// ⚠️ CE N'EST PAS UN ÉLAGAGE, et ça ne doit jamais le devenir. Le §6.2 l'a tranché
// le 2026-07-30 : « la précédence est un objet d'ORDONNANCEMENT, pas de SOLUBILITÉ »
// — la précédence par paires, utilisée comme test de mort, a fait 9 niveaux sur 10
// en faute au juge `fp`. Ici on ne coupe rien : on DÉCRIT une contrainte d'ordre.
//
// ⚠️ RELAXATION OPTIMISTE, donc une contrainte trouvée est une PREUVE et un silence
// ne promet rien : le BFS de marche ignore toutes les autres caisses (le joueur est
// donc plus libre que dans le vrai jeu) et ne bloque que les murs, le but G testé et
// la caisse C elle-même. Si même à ce régime le joueur n'atteint aucun appui, il ne
// l'atteindra jamais.
#include <QPoint>
#include <QVector>
#include <cstdio>
#include "game.h"
#include "level.h"

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: porte <niv|plateau.xsb>\n"); return 2; }
    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "porte: niveau introuvable (%s)\n", argv[1]); return 2; }
    Game g(level, num);

    const int L = g.getLargeur(), H = g.getHauteur(), N = L * H;
    const int depart = g.getPlayerPoint().x() + g.getPlayerPoint().y() * L;
    static const int dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};

    auto mur = [&](int c) { return g.getCase(c) == Level::tcMur; };

    // Marche du joueur, murs + 'bloqueA' + 'bloqueB' interdits. Rend la zone.
    auto zone = [&](int bloqueA, int bloqueB) {
        QVector<bool> vu(N, false);
        QVector<int> file;
        if (depart == bloqueA || depart == bloqueB) return vu;
        vu[depart] = true; file.append(depart);
        for (int i = 0; i < file.size(); i++) {
            const int c = file[i], x = c % L, y = c / L;
            for (int d = 0; d < 4; d++) {
                const int nx = x + dx[d], ny = y + dy[d];
                if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
                const int n = nx + ny * L;
                if (vu[n] || mur(n) || n == bloqueA || n == bloqueB) continue;
                vu[n] = true; file.append(n);
            }
        }
        return vu;
    };

    printf("=== %s — precedence CAISSE -> BUT ===\n",
           parChemin ? qPrintable(arg1) : qPrintable(QString("niveau %1").arg(num)));

    int total = 0, immobiles = 0, caisses = 0;
    for (int c = 0; c < N; c++) {
        // Une caisse DÉJÀ posée sur un but n'est pas concernée : si elle gèle, elle
        // gèle sur un but, et la partie reste gagnable.
        if (g.getCase(c) != Level::tcCaisse) continue;
        caisses++;
        const int cx = c % L, cy = c / L;

        // A(C) : les appuis d'au moins une poussée géométriquement possible.
        QVector<int> appuis;
        for (int d = 0; d < 4; d++) {
            const int ax = cx + dx[d], ay = cy + dy[d];          // destination
            const int px = cx - dx[d], py = cy - dy[d];          // appui du joueur
            if (ax < 0 || ax >= L || ay < 0 || ay >= H) continue;
            if (px < 0 || px >= L || py < 0 || py >= H) continue;
            const int a = ax + ay * L, p = px + py * L;
            if (mur(a) || mur(p)) continue;
            // ⚠️ UNE POUSSÉE QUI TUE LA CAISSE N'EST PAS UNE ISSUE. Sans ce test la
            // règle ne trouve rien sur le 16 : la caisse (10,6) a deux poussées
            // géométriques, mais celle vers l'est la dépose en (11,6), d'où aucune
            // poussée ne sort jamais. Compter son appui reviendrait à dire « elle
            // peut encore bouger » alors qu'elle ne peut que se suicider.
            if (g.caseMorteOrdinaire(a)) continue;
            appuis.append(p);
        }
        if (appuis.isEmpty()) {
            printf("  caisse (%2d,%2d) : IMMOBILE des le depart (aucune poussee geometrique)\n", cx, cy);
            immobiles++;
            continue;
        }

        // Quels buts, une fois remplis, la coupent de TOUS ses appuis ?
        QVector<int> bloqueurs;
        for (int b = 0; b < g.getNbButs(); b++) {
            const int gb = g.getCaseBut(b);
            if (gb == c) continue;
            const QVector<bool> z = zone(gb, c);
            bool accessible = false;
            for (int p : appuis) if (z[p]) { accessible = true; break; }
            if (!accessible) bloqueurs.append(b);
        }
        if (bloqueurs.isEmpty()) continue;

        total++;
        printf("  caisse (%2d,%2d) : a degager AVANT %d but(s) —", cx, cy, (int)bloqueurs.size());
        int rangMin = 1 << 30;
        for (int b : bloqueurs) {
            const int gb = g.getCaseBut(b);
            printf(" (%d,%d)r%d", gb % L, gb / L, g.rangDuBut(b));
            rangMin = qMin(rangMin, g.rangDuBut(b));
        }
        printf("   => avant le rang %d\n", rangMin);
    }

    printf("  --- %d caisse(s) sur %d contrainte(s), %d immobile(s) des le depart\n",
           total, caisses, immobiles);
    return 0;
}
