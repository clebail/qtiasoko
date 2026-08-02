// pas0 — POURQUOI LA MACRO NE DÉMARRE PAS, sur l'état de DÉPART d'un niveau.
//
//   pas0 <niveau>
//
// Question à laquelle il répond (constat utilisateur, 2026-08-01, niveau 12) :
// « le seul problème vient du fait que la macro ne se déclenche pas pour la
// première caisse ». Le journal hybride le confirme (100 % des états avant la
// première pose sont sans macro, dans les trois ordres essayés) mais ne dit pas
// POURQUOI, ni si le premier BUT y change quelque chose.
//
// Ici on ne joue rien : on interroge, depuis le plateau initial, chaque couple
// (caisse, but) avec le contrat EXACT de l'UI — `macroPeutDemarrer`, puis la
// descente `macroVersButBacktrack` menée au bout, puis `!isPerdu`.
//
// ⚠️ Le premier jet ne testait que `macroPeutDemarrer` et rendait « tous les buts
// amorçables » sur le 12, en contradiction avec le journal (100 % d'états sans
// macro). Les deux avaient raison : **amorcer n'est pas aboutir**. Ne pas réduire
// cet outil au pas 0, la question posée porte sur ce que l'UI AFFICHE.
//
// Les causes d'échec au pas 0 viennent de Game::diagnosticPas0, qui relâche la
// seule contrainte de zone sur avanceVersBut : aucune logique dupliquée ici.
//
// ⚠️ OUTIL DE CHANTIER (campagne hybride), à retirer avec elle.
#include <QCoreApplication>
#include <QString>
#include <cstdio>
#include "level.h"
#include "game.h"

static const char* nomDir(int d) {
    switch (d) {
    case Game::dHaut:   return "Haut";
    case Game::dDroite: return "Droite";
    case Game::dBas:    return "Bas";
    default:            return "Gauche";
    }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: pas0 <niveau>\n"); return 2; }
    const int num = QString(argv[1]).toInt();

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game g(level, num);

    const int L = g.getLargeur();
    const QVector<bool>   zone    = g.getZoneJoueur();
    const QVector<quint8> caisses = g.getCaissesDeplacable(zone);

    printf("=== niveau %d — état de DÉPART, %d buts ===\n", num, g.getNbButs());
    printf("but actif (rang 0 de l'ordre courant) : ");
    const int actif = g.butActif();
    if (actif >= 0) printf("(%d,%d)\n\n", g.getCaseBut(actif) % L, g.getCaseBut(actif) / L);
    else            printf("aucun\n\n");

    printf("%-10s %-10s  %s\n", "but", "amorçable", "détail (si non)");
    printf("---------------------------------------------------------------\n");

    for (int b = 0; b < g.getNbButs(); b++) {
        const int cb = g.getCaseBut(b);
        int nbOk = 0, nbBloque = 0;
        int nbJoueur = 0, nbDetour = 0, nbAutre = 0;
        QString exemple, exBloque;
        for (int c = 0; c < caisses.size(); c++) {
            if (caisses[c] == 0) continue;
            if (g.macroPeutDemarrer(c, b, zone)) {

                // ⚠️ Amorcer n'est PAS aboutir : macroPeutDemarrer ne teste que le
                // PREMIER pas. L'UI, elle, n'affiche une macro que si la descente va
                // au bout ET que l'état n'est pas perdu — c'est ce contrat-là qu'on
                // reproduit, sinon on répond à côté de la question posée.
                Game f(g);
                QVector<QPair<int,int>> poussees;
                qint64 essais = 0;
                if (f.macroVersButBacktrack(c, b, poussees, &essais) && !f.isPerdu()) { nbOk++; continue; }
                nbBloque++;
                if (exBloque.isEmpty()) {
                    const QVector<int> chemin = g.cheminMacro(c);
                    int best = -1, reste = -1;
                    for (int k = 0; k < chemin.size(); k++)
                        if (chemin[k] >= 0 && (reste < 0 || chemin[k] < reste)) { reste = chemin[k]; best = k; }
                    exBloque = QString("caisse (%1,%2) démarre puis BLOQUE en (%3,%4), reste %5 (%6 branche(s))")
                                   .arg(c % L).arg(c / L)
                                   .arg(best % L).arg(best / L).arg(reste).arg(essais);
                }
                continue;
            }
            QVector<QPair<int,int>> dirs;
            switch (g.diagnosticPas0(c, b, zone, &dirs)) {
            case Game::Pas0JoueurMauvaisCote:
                nbJoueur++;
                if (exemple.isEmpty() && !dirs.isEmpty())
                    exemple = QString("caisse (%1,%2) : %3 baisserait la distance, appui (%4,%5) hors zone")
                                  .arg(c % L).arg(c / L).arg(nomDir(dirs[0].first))
                                  .arg(dirs[0].second % L).arg(dirs[0].second / L);
                break;
            case Game::Pas0DetourRequis:      nbDetour++; break;
            default:                          nbAutre++;  break;
            }
        }
        const QByteArray col = nbOk > 0 ? QString("OUI (%1)").arg(nbOk).toUtf8()
                                        : QByteArray("NON");
        printf("(%2d,%2d)    %-10s  ", cb % L, cb / L, col.constData());
        if (nbOk == 0) {
            printf("amorcent puis bloquent:%d | pas 0 — détour:%d  joueur:%d  autre:%d",
                   nbBloque, nbDetour, nbJoueur, nbAutre);
            if (!exBloque.isEmpty()) printf("\n%22s%s", "", exBloque.toUtf8().constData());
            if (!exemple.isEmpty())  printf("\n%22s%s", "", exemple.toUtf8().constData());
        }
        printf("\n");
    }
    return 0;
}
