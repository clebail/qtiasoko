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
#include <QSet>
#include <QMap>
#include <QPoint>
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

// Une case du champ, sur 4 colonnes : '####' mur, '  . ' hors d'atteinte,
// '<nn>' la distance. Assez large pour les niveaux à plus de 99 poussées.
static void imprimeCase(const Game& g, const QVector<int>& champ, int cell) {
    if (g.getCase(cell) == Level::tcMur) { printf("#### "); return; }
    if (champ[cell] < 0)                 { printf("   . "); return; }
    printf("%4d ", champ[cell]);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: pas0 <niveau|fichier.xsb> [champ]\n"); return 2; }

    // Accepte un CHEMIN .xsb comme bench/loi/ordre : les plateaux exportés du
    // mode hybride sont des positions de milieu de partie, et c'est justement là
    // qu'on veut interroger la macro.
    // ⚠️ Piège §7 : charger une position de milieu de partie RECALCULE tout le
    // statique. Ici c'est sans conséquence pour `distanceParBut`, qui ne dépend
    // que des MURS et des BUTS (une caisse seule, les autres ignorées) — mais
    // `ordreButs` et `casesMortes`, eux, sont ceux de CE plateau. Ne rien
    // conclure sur l'ordre du niveau d'origine à partir d'une de ses fixtures.
    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();
    const bool avecChamp = (argc > 2) && (QString(argv[2]) == "champ");
    const bool avecTrace = (argc > 2) && (QString(argv[2]) == "trace");
    const bool avecMulti = (argc > 2) && (QString(argv[2]) == "multi");
    const bool avecDetour = (argc > 2) && (QString(argv[2]) == "detour");

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "pas0: niveau introuvable (%s)\n", qPrintable(arg1)); return 2; }
    Game g(level, num);

    const int L = g.getLargeur();
    const QVector<bool>   zone0   = g.getZoneJoueur();
    const QVector<bool>&  zone    = zone0;
    const QVector<quint8> caisses = g.getCaissesDeplacable(zone);

    printf("=== %s — %d buts ===\n",
           parChemin ? qPrintable(arg1) : qPrintable(QString("niveau %1").arg(num)),
           g.getNbButs());
    printf("but actif (rang 0 de l'ordre courant) : ");
    const int actif = g.butActif();
    if (actif >= 0) printf("(%d,%d)\n\n", g.getCaseBut(actif) % L, g.getCaseBut(actif) / L);
    else            printf("aucun\n\n");

    // Mode 'champ' : les deux tables côte à côte pour le but actif. La BRUTE est
    // le trajet précalculé (ce que la macro croit devoir suivre) ; la JOUABLE est
    // ce que la descente monotone accepte réellement. Un écart entre les deux est
    // la réponse à « pourquoi la macro ne part pas par là ? ».
    if (avecChamp && actif >= 0) {
        const QVector<int> brut   = g.champDistanceBrut(actif);
        const QVector<int> jouable = g.champDistanceButActif();
        const int H = g.getHauteur();
        const QPoint pj = g.getPlayerPoint();

        printf("CHAMP BRUT — distanceParBut[but actif][case][région du joueur en (%d,%d)]\n",
               pj.x(), pj.y());
        printf("  (le trajet PRÉCALCULÉ ; '.' = but inatteignable depuis cette case)\n\n     ");
        for (int x = 0; x < L; x++) printf("%4d ", x);
        printf("\n");
        for (int y = 0; y < H; y++) {
            printf("%3d  ", y);
            for (int x = 0; x < L; x++) imprimeCase(g, brut, x + y * L);
            printf("\n");
        }

        printf("\nCHAMP JOUABLE — ce que la descente monotone accepte (caisses + coups légaux)\n\n     ");
        for (int x = 0; x < L; x++) printf("%4d ", x);
        printf("\n");
        for (int y = 0; y < H; y++) {
            printf("%3d  ", y);
            for (int x = 0; x < L; x++) imprimeCase(g, jouable, x + y * L);
            printf("\n");
        }
        printf("\n");
    }

    // Mode 'trace' : rejoue la descente monotone PAS À PAS pour une caisse, en
    // disant pour CHAQUE direction pourquoi elle est refusée. Réplique exacte de
    // la boucle de macroVersButBacktrack (game.cpp:3062) avec l'API publique
    // seule — la valeur d'arrivée se lit en POUSSANT sur une copie, parce que
    // avanceVersBut lit la distance avec le joueur d'APRÈS la poussée
    // (`regions[c * size + devant]`, game.cpp:2738) et pas celui d'avant. C'est
    // toute la subtilité : une case n'a pas la même distance selon le côté.
    if (avecTrace && actif >= 0) {
        const int caseBut = g.getCaseBut(actif);
        int c = -1;
        for (int i = 0; i < caisses.size() && c < 0; i++) if (caisses[i] != 0) c = i;
        if (argc > 3) {   // caisse imposée : pas0 <plateau> trace <x,y>
            const QStringList xy = QString(argv[3]).split(',');
            if (xy.size() == 2) c = xy[0].toInt() + xy[1].toInt() * L;
        }
        printf("TRACE de la descente — caisse (%d,%d) vers le but actif (%d,%d)\n\n",
               c % L, c / L, caseBut % L, caseBut / L);

        Game e(g);
        for (int pas = 0; pas < 200 && c != caseBut; pas++) {
            const QVector<int>  champ = e.champDistanceBrut(actif);
            const QVector<bool> zone  = e.getZoneJoueur();
            const int dCur = champ[c];
            printf("pas %2d : caisse (%2d,%2d)  reste %d  joueur (%d,%d)\n",
                   pas, c % L, c / L, dCur,
                   e.getPlayerPoint().x(), e.getPlayerPoint().y());
            if (dCur <= 0) { printf("         -> dCur <= 0, arrêt\n"); break; }

            int joue = -1;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int dx = (d == Game::dDroite) - (d == Game::dGauche);
                const int dy = (d == Game::dBas)    - (d == Game::dHaut);
                const int devx = c % L + dx, devy = c / L + dy;
                const int appx = c % L - dx, appy = c / L - dy;
                if (devx < 0 || devx >= L || devy < 0 || devy >= g.getHauteur()) continue;
                if (appx < 0 || appx >= L || appy < 0 || appy >= g.getHauteur()) continue;
                const int devant = devx + devy * L, appui = appx + appy * L;
                printf("         %-7s -> (%2d,%2d) : ", nomDir(d), devx, devy);
                if (e.getCase(devant) == Level::tcMur)   { printf("MUR\n");                continue; }
                if (e.getCase(devant) == Level::tcCaisse ||
                    e.getCase(devant) == Level::tcGoalCaisse) { printf("caisse\n");        continue; }
                if (!zone[appui]) { printf("appui (%d,%d) HORS ZONE\n", appx, appy);       continue; }
                // La distance d'arrivée, lue APRÈS la poussée : seul moyen exact.
                Game essai(e);
                if (!essai.pousse(c, (Game::EDirection)d)) { printf("pousse() refuse\n");  continue; }
                const int dApres = essai.champDistanceBrut(actif)[devant];
                if (dApres != dCur - 1) {
                    printf("reste %d, il faudrait %d  ❌ NON MONOTONE\n", dApres, dCur - 1);
                    continue;
                }
                printf("reste %d  ✅\n", dApres);
                if (joue < 0) joue = d;
            }
            if (joue < 0) { printf("         -> AUCUNE direction : BLOQUÉ\n"); break; }
            printf("         => joue %s\n", nomDir(joue));
            const int devant = c + ((joue == Game::dDroite) - (joue == Game::dGauche))
                                 + ((joue == Game::dBas)    - (joue == Game::dHaut)) * L;
            e.pousse(c, (Game::EDirection)joue);
            c = devant;
        }
        if (c == caseBut) printf("\n=> ARRIVÉE au but.\n");

        // Confrontation : la VRAIE fonction, sur le même couple. Un écart avec la
        // trace ci-dessus est un écart entre le contrat écrit et le contrat joué.
        {
            int cDep = -1;
            for (int i = 0; i < caisses.size() && cDep < 0; i++) if (caisses[i] != 0) cDep = i;
            if (argc > 3) {
                const QStringList xy = QString(argv[3]).split(',');
                if (xy.size() == 2) cDep = xy[0].toInt() + xy[1].toInt() * L;
            }
            Game f(g);
            QVector<QPair<int,int>> poussees;
            qint64 essais = 0;
            const bool amorce = g.macroPeutDemarrer(cDep, actif, zone0);
            const bool ok = f.macroVersButBacktrack(cDep, actif, poussees, &essais);
            printf("\nmacroPeutDemarrer      = %s\n", amorce ? "OUI" : "NON");
            printf("macroVersButBacktrack  = %s (%lld poussée(s), %lld branche(s))\n",
                   ok ? "OUI" : "NON", (long long)poussees.size(), (long long)essais);
            printf("isPerdu() après        = %s\n", f.isPerdu() ? "OUI" : "non");
            printf("=> l'UI affiche la macro : %s\n", (ok && !f.isPerdu()) ? "OUI" : "NON");
        }
        printf("\n");
    }

    // Mode 'multi' : COMBIEN de macros distinctes une même caisse peut-elle
    // produire ? Le solveur en enfile UNE par caisse (macroVersButBacktrack rend
    // à la première réussite, solveurastar.cpp:1049). On énumère ici TOUTES les
    // descentes monotones jusqu'au but, sans arrêt anticipé, et on compte les
    // ÉTATS distincts qu'elles produisent — c'est ça qui compte, pas le nombre de
    // chemins : deux routes de même longueur qui laissent le joueur au même
    // endroit donnent le MÊME état, que la dédup du solveur fusionnerait.
    // ⚠️ Toutes les routes monotones ont la MÊME longueur, par construction (chaque
    // pas retire exactement 1 à la distance) : le g des enfants est donc identique.
    // Seule la position finale du JOUEUR peut différer — et comme `h` est
    // joueur-aware, elle change `h`, donc `f`.
    if (avecMulti && actif >= 0) {
        const int caseBut = g.getCaseBut(actif);
        int cDep = -1;
        for (int i = 0; i < caisses.size() && cDep < 0; i++) if (caisses[i] != 0) cDep = i;
        if (argc > 3) {
            const QStringList xy = QString(argv[3]).split(',');
            if (xy.size() == 2) cDep = xy[0].toInt() + xy[1].toInt() * L;
        }

        struct Noeud { Game etat; int caisse; int prof; };
        QVector<Noeud> pile;
        pile.append({Game(g), cDep, 0});
        QSet<QByteArray> etatsDistincts;
        QMap<QByteArray, QPoint> joueurFinal;
        qint64 routes = 0, noeudsVus = 0;
        int longueur = -1;

        while (!pile.isEmpty() && noeudsVus < 200000) {
            const Noeud n = pile.takeLast();
            noeudsVus++;
            if (n.caisse == caseBut) {
                routes++;
                if (longueur < 0) longueur = n.prof;
                const QByteArray cle = n.etat.getEtat();
                etatsDistincts.insert(cle);
                joueurFinal.insert(cle, n.etat.getPlayerPoint());
                continue;
            }
            const QVector<int>  champ = n.etat.champDistanceBrut(actif);
            const QVector<bool> zone  = n.etat.getZoneJoueur();
            const int dCur = champ[n.caisse];
            if (dCur <= 0) continue;
            for (int d = 0; d < NB_DIRECTION; d++) {
                const int dx = (d == Game::dDroite) - (d == Game::dGauche);
                const int dy = (d == Game::dBas)    - (d == Game::dHaut);
                const int devx = n.caisse % L + dx, devy = n.caisse / L + dy;
                const int appx = n.caisse % L - dx, appy = n.caisse / L - dy;
                if (devx < 0 || devx >= L || devy < 0 || devy >= g.getHauteur()) continue;
                if (appx < 0 || appx >= L || appy < 0 || appy >= g.getHauteur()) continue;
                const int devant = devx + devy * L, appui = appx + appy * L;
                if (!zone[appui]) continue;
                Game suite(n.etat);
                if (!suite.pousse(n.caisse, (Game::EDirection)d) || suite.isPerdu()) continue;
                if (suite.champDistanceBrut(actif)[devant] != dCur - 1) continue;
                pile.append({std::move(suite), devant, n.prof + 1});
            }
        }

        printf("MULTI — caisse (%d,%d) vers le but actif (%d,%d)\n",
               cDep % L, cDep / L, caseBut % L, caseBut / L);
        printf("  chemins monotones complets : %lld  (longueur %d, identique pour tous)\n",
               (long long)routes, longueur);
        printf("  ÉTATS DISTINCTS produits   : %d   <- ce que le solveur pourrait enfiler\n",
               etatsDistincts.size());
        printf("  le solveur en enfile       : %d\n", routes > 0 ? 1 : 0);
        for (auto it = joueurFinal.constBegin(); it != joueurFinal.constEnd(); ++it)
            printf("     état distinct : joueur final (%d,%d)\n", it.value().x(), it.value().y());
        if (noeudsVus >= 200000) printf("  ⚠️ BUDGET ÉPUISÉ — chiffres minorants\n");
        printf("\n");
    }

    // Mode 'detour' : COMBIEN DE POUSSÉES EN TROP faudrait-il tolérer pour que la
    // macro aboutisse ? La descente est strictement monotone (avanceVersBut), donc
    // elle ne sait jouer QUE le trajet solo optimal. Quand celui-ci est bouché par
    // d'autres caisses, la question n'est pas « oui/non » mais « à quel prix ».
    //
    // On répond par une recherche BORNÉE où SEULE la caisse suivie bouge (les
    // autres sont des murs) : c'est un sous-solve à une caisse, donc bon marché.
    // Le résultat est un CHIFFRE — l'écart au trajet solo — qui dit ce que
    // coûterait une tolérance de détour dans le solveur, AVANT d'y toucher.
    // ⚠️ Ce n'est PAS une borne pour `h` : les autres caisses traitées en murs
    // SURESTIMENT (§4, « caisses manquantes = murs »). C'est un itinéraire, pas
    // une borne — la distinction est tout l'enjeu (journal-macro, 2026-08-07).
    if (avecDetour && actif >= 0) {
        const int caseBut = g.getCaseBut(actif);
        const int budget = (argc > 4) ? QString(argv[4]).toInt() : 8;
        printf("DETOUR — écart au trajet solo, budget %d poussées en trop\n\n", budget);
        printf("%-10s %-8s %-8s %s\n", "caisse", "solo", "réel", "écart");
        printf("---------------------------------------------\n");

        for (int dep = 0; dep < caisses.size(); dep++) {
            if (caisses[dep] == 0) continue;
            if (argc > 3 && QString(argv[3]) != "tout") {
                const QStringList xy = QString(argv[3]).split(',');
                if (xy.size() == 2 && dep != xy[0].toInt() + xy[1].toInt() * L) continue;
            }
            const int solo = g.champDistanceBrut(actif)[dep];
            if (solo < 0) { printf("(%2d,%2d)    %-8s %-8s %s\n", dep % L, dep / L, "-", "-", "but inatteignable"); continue; }

            // BFS en profondeur croissante, états dédupliqués par la clé du jeu
            // (caisses + ZONE du joueur) : c'est la même identité que le solveur.
            QVector<QPair<Game,int>> front;   // (état, case de la caisse suivie)
            front.append({Game(g), dep});
            QSet<QByteArray> vus;
            vus.insert(g.getEtat());
            int reel = -1;
            for (int prof = 1; prof <= solo + budget && reel < 0 && !front.isEmpty(); prof++) {
                QVector<QPair<Game,int>> suivant;
                for (const auto& n : front) {
                    const QVector<bool> zone = n.first.getZoneJoueur();
                    for (int d = 0; d < NB_DIRECTION && reel < 0; d++) {
                        const int dx = (d == Game::dDroite) - (d == Game::dGauche);
                        const int dy = (d == Game::dBas)    - (d == Game::dHaut);
                        const int devx = n.second % L + dx, devy = n.second / L + dy;
                        const int appx = n.second % L - dx, appy = n.second / L - dy;
                        if (devx < 1 || devx >= L - 1 || devy < 1 || devy >= g.getHauteur() - 1) continue;
                        if (appx < 0 || appx >= L || appy < 0 || appy >= g.getHauteur()) continue;
                        if (!zone[appx + appy * L]) continue;
                        Game e(n.first);
                        if (!e.pousse(n.second, (Game::EDirection)d) || e.isPerdu()) continue;
                        const int devant = devx + devy * L;
                        if (devant == caseBut) { reel = prof; break; }
                        const QByteArray cle = e.getEtat();
                        if (vus.contains(cle)) continue;
                        vus.insert(cle);
                        suivant.append({std::move(e), devant});
                    }
                    if (reel >= 0) break;
                }
                front = std::move(suivant);
            }
            if (reel < 0) printf("(%2d,%2d)    %-8d %-8s hors budget (+%d)\n", dep % L, dep / L, solo, "—", budget);
            else          printf("(%2d,%2d)    %-8d %-8d %+d%s\n", dep % L, dep / L, solo, reel, reel - solo,
                                 reel == solo ? "   (la macro devrait passer)" : "");
        }
        printf("\n");
    }

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
