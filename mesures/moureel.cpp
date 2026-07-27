// Harnais : OÙ NAÎT LE MOU DE h ? — l'observation directe de la congestion (§3).
//
//   moureel <niveau|fichier.xsb> [astar|macro]      (défaut : astar OPTIMAL)
//
// Le §3 établit  C* = Σ trajets solos + congestion,  où h capture EXACTEMENT le
// premier terme et RIEN du second. Le mou (2 à 12 poussées) est donc entièrement
// de la congestion — et c'est lui qui engendre la masse f < C* qui domine tous les
// gros niveaux. Mais le projet ne l'a jamais OBSERVÉ : la congestion a été DÉDUITE
// (« écarter une caisse assise sur le trajet d'une autre coûte 2 ») à partir de
// quatre niveaux et d'un raisonnement, jamais mesurée poussée par poussée.
//
// LE PRINCIPE, exact et gratuit. Sur un chemin OPTIMAL, le coût restant depuis un
// état vaut C* - g(état), par définition de l'optimalité. Le mou de cet état est
// donc connu sans le moindre sous-solve :
//
//     mou(etat) = (C* - g) - h(etat)
//
// En dérivant le long du chemin, chaque poussée fait h baisser de 1 (PRODUCTIVE :
// elle rapproche une caisse de son but au sens du couplage) ou pas (IMPROVISÉE :
// elle paie de la congestion). Et :
//
//     mou(depart) = somme des (1 + Δh) sur toutes les poussées
//
// donc la liste des poussées improductives EST la décomposition du mou, une par
// unité. C'est ce que cet outil imprime — avec, pour chacune, la géométrie au
// moment où elle survient.
//
// ⚠️ EXIGE UNE SOLUTION OPTIMALE. En mode 'macro' le régime d'engagement ne garantit
// pas l'optimalité (§10.5) : le mou calculé serait alors mélangé au surcoût de la
// macro. L'outil le signale mais laisse faire — sur les niveaux où macro atteint le
// canari (1, 2, 3, 6, 17), les deux coïncident.
#include <QCoreApplication>
#include <QString>
#include <QList>
#include <QVector>
#include <QHash>
#include <cstdio>
#include "level.h"
#include "game.h"
#include "solveur.h"

// Caisses collées à 'c' (4-voisinage) : la densité locale, seule mesure de
// « congestion » que le §4 n'ait pas réfutée (les PAIRES ont échoué parce que la
// congestion est une densité à 3+ caisses, pas une interaction 2-à-2).
static int caissesAutour(const Game& g, int c) {
    const int L = g.getLargeur(), H = g.getHauteur();
    const int x = c % L, y = c / L;
    static const int dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};
    int n = 0;
    for (int d = 0; d < 4; d++) {
        const int vx = x + dx[d], vy = y + dy[d];
        if (vx < 0 || vx >= L || vy < 0 || vy >= H) continue;
        const Level::ETypeCase t = g.getCase(vx + vy * L);
        if (t == Level::tcCaisse || t == Level::tcGoalCaisse) n++;
    }
    return n;
}

static bool surBut(const Game& g, int c) {
    const Level::ETypeCase t = g.getCase(c);
    return t == Level::tcGoalCaisse || t == Level::tcGoal || t == Level::tcGoalPlayer;
}


// ---- TRAJETS SOLOS : quelles cases une caisse doit-elle traverser, SEULE ? ----
// Graphe de poussées pur (relaxation : on ignore la marche du joueur — c'est le
// même graphe que 'distancePoussee'). Une poussée c -> c+d est possible si c+d
// n'est pas un mur et si l'appui c-d n'en est pas un.
// On en tire, pour une caisse et son but, l'ensemble des cases situées sur AU
// MOINS UN chemin optimal : d_avant(case) + d_arriere(case) == D.
struct Trajet {
    QVector<int> surChemin;   // 1 si la case est sur un chemin optimal
    QVector<int> couche;      // d_avant, pour repérer les cases OBLIGATOIRES
    int D = -1;
};

static QVector<int> bfsPoussees(const Game& g, int depart, bool arriere) {
    const int L = g.getLargeur(), H = g.getHauteur(), N = L * H;
    QVector<int> d(N, -1);
    if (depart < 0 || depart >= N) return d;
    static const int dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};
    QVector<int> file; file.append(depart); d[depart] = 0;
    for (int t = 0; t < file.size(); t++) {
        const int c = file[t], x = c % L, y = c / L;
        for (int k = 0; k < 4; k++) {
            // 'arriere' = on TIRE la caisse au lieu de la pousser : même arête,
            // parcourue à l'envers (le joueur se place de l'autre côté).
            const int nx = x + dx[k], ny = y + dy[k];
            const int ax = arriere ? x + 2*dx[k] : x - dx[k];
            const int ay = arriere ? y + 2*dy[k] : y - dy[k];
            if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
            if (ax < 0 || ax >= L || ay < 0 || ay >= H) continue;
            const int n = nx + ny * L, a = ax + ay * L;
            if (g.getCase(n) == Level::tcMur || g.getCase(a) == Level::tcMur) continue;
            if (d[n] >= 0) continue;
            d[n] = d[c] + 1; file.append(n);
        }
    }
    return d;
}

static Trajet trajetSolo(const Game& g, int caisse, int but) {
    Trajet t;
    const QVector<int> av = bfsPoussees(g, caisse, false);
    const QVector<int> ar = bfsPoussees(g, but, true);
    const int N = g.getLargeur() * g.getHauteur();
    t.surChemin.fill(0, N);
    t.couche = av;
    if (but < 0 || but >= N || av[but] < 0) return t;   // but inatteignable seul
    t.D = av[but];
    for (int c = 0; c < N; c++)
        if (av[c] >= 0 && ar[c] >= 0 && av[c] + ar[c] == t.D) t.surChemin[c] = 1;
    return t;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (argc < 2) { fprintf(stderr, "usage: moureel <niveau|fichier.xsb> [astar|macro]\n"); return 2; }
    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();
    const QString md = (argc > 2) ? argv[2] : "astar";

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "moureel: niveau introuvable (%s)\n", qPrintable(arg1)); return 2; }

    Game game(level, num);
    const Solveur::EType type = (md == "macro") ? Solveur::AstarMacro : Solveur::Astar;
    if (md == "macro")
        fprintf(stderr, "moureel: ATTENTION, 'macro' ne garantit pas l'optimalite —"
                        " le mou mesure alors aussi le surcout de la macro.\n");

    Solveur* s = Solveur::creer(type, game);

    QObject::connect(s, &Solveur::solutionTrouvee,
                     [num, game](QList<Game::EDirection> chemin, qint64) {
        Game g(game);
        const int L = g.getLargeur();

        // C* = poussées de la solution (optimale en mode astar).
        Game compte(game);
        for (Game::EDirection d : chemin) compte.deplace(d);
        const int cStar = compte.getNbDepCaisse();

        const int hDepart = g.getHeuristique(nullptr);
        const int mouTotal = cStar - hDepart;

        printf("== niveau %d : C* = %d, h(depart) = %d  ->  MOU = %d poussees ==\n",
               num, cStar, hDepart, mouTotal);
        if (mouTotal == 0) { printf("   (h est EXACTE sur ce niveau : rien a expliquer)\n"); }

        // ---- COMPTAGE : combien de caisses sont ASSISES SUR LE TRAJET d'une autre ?
        // C'est l'hypothèse issue de l'observation des reculs (§3) :
        //     mou attendu = 2 x (nombre de caisses a ecarter)
        {
            const int N = L * g.getHauteur();
            QVector<int> caisseParBut(g.getNbButs(), -1);
            g.getHeuristique(nullptr, -1, caisseParBut.data());

            QVector<int> buts;
            for (int i = 0; i < N; i++) {
                const Level::ETypeCase t = g.getCase(i);
                if (t == Level::tcGoal || t == Level::tcGoalCaisse || t == Level::tcGoalPlayer)
                    buts.append(i);
            }

            QVector<int> gene(N, 0);        // nb de trajets d'AUTRES caisses passant ici
            QVector<int> geneOblig(N, 0);   // ... dont la case est SEULE de sa couche
            int nTrajets = 0;
            for (int b = 0; b < buts.size() && b < caisseParBut.size(); b++) {
                const int caisse = caisseParBut[b];
                if (caisse < 0) continue;
                const Trajet tr = trajetSolo(g, caisse, buts[b]);
                if (tr.D < 0) continue;
                nTrajets++;
                // Une case est OBLIGATOIRE si elle est la seule de sa couche sur un
                // chemin optimal : tout chemin optimal y passe.
                QVector<int> parCouche(tr.D + 2, 0);
                for (int c = 0; c < N; c++)
                    if (tr.surChemin[c] && tr.couche[c] >= 0 && tr.couche[c] <= tr.D)
                        parCouche[tr.couche[c]]++;
                for (int c = 0; c < N; c++) {
                    if (!tr.surChemin[c] || c == caisse) continue;
                    gene[c]++;
                    if (parCouche[tr.couche[c]] == 1) geneOblig[c]++;
                }
            }

            // ---- CONFLIT CROISÉ (l'analogue du « linear conflict » du taquin) ----
            // A est sur le trajet OBLIGATOIRE de B *et* B sur celui de A : aucun
            // ordre ne résout ça, l'une des deux DOIT s'écarter puis revenir. C'est
            // le seul critère de conflit qui soit une PREUVE, donc le seul qui
            // pourrait entrer dans h sans la rendre inadmissible.
            {
                QVector<int> caisses, butDe;
                for (int b = 0; b < buts.size() && b < caisseParBut.size(); b++)
                    if (caisseParBut[b] >= 0) { caisses.append(caisseParBut[b]); butDe.append(buts[b]); }

                QVector<QVector<int>> oblig(caisses.size());
                for (int i = 0; i < caisses.size(); i++) {
                    const Trajet tr = trajetSolo(g, caisses[i], butDe[i]);
                    oblig[i].fill(0, N);
                    if (tr.D < 0) continue;
                    QVector<int> parCouche(tr.D + 2, 0);
                    for (int c = 0; c < N; c++)
                        if (tr.surChemin[c] && tr.couche[c] >= 0 && tr.couche[c] <= tr.D)
                            parCouche[tr.couche[c]]++;
                    for (int c = 0; c < N; c++)
                        if (tr.surChemin[c] && tr.couche[c] >= 0 && parCouche[tr.couche[c]] == 1)
                            oblig[i][c] = 1;
                }

                int croises = 0;
                for (int i = 0; i < caisses.size(); i++)
                    for (int j = i + 1; j < caisses.size(); j++)
                        if (oblig[i][caisses[j]] && oblig[j][caisses[i]]) {
                            croises++;
                            printf("   CONFLIT CROISE : caisses (%d,%d) et (%d,%d)\n",
                                   caisses[i] % L, caisses[i] / L, caisses[j] % L, caisses[j] / L);
                        }
                printf("   conflits CROISES (preuve)          : %d  -> mou predit %d\n",
                       croises, 2 * croises);
            }

            int surTrajet = 0, surTrajetOblig = 0;
            for (int i = 0; i < N; i++) {
                const Level::ETypeCase t = g.getCase(i);
                if (t != Level::tcCaisse && t != Level::tcGoalCaisse) continue;
                if (gene[i] > 0) surTrajet++;
                if (geneOblig[i] > 0) surTrajetOblig++;
            }
            printf("   trajets solos calcules : %d\n", nTrajets);
            printf("   caisses sur le trajet d'une autre  : %d  -> mou predit %d\n",
                   surTrajet, 2 * surTrajet);
            printf("   dont sur une case OBLIGATOIRE      : %d  -> mou predit %d\n",
                   surTrajetOblig, 2 * surTrajetOblig);
            printf("   MOU REEL                           : %d\n", mouTotal);
        }

        // PASSE 1 — rejouer en enregistrant TOUT, avec l'IDENTITÉ des caisses
        // (le Game ne la porte pas : on la reconstruit en suivant les poussées).
        struct Poussee { int num, id, from, to, dh, coup; };
        QVector<Poussee> poussees;
        QVector<int> joueurParCoup;
        QHash<int,int> idParCase;   // case -> identifiant de caisse
        {
            int id = 0;
            for (int i = 0; i < L * g.getHauteur(); i++) {
                const Level::ETypeCase t = g.getCase(i);
                if (t == Level::tcCaisse || t == Level::tcGoalCaisse) idParCase[i] = id++;
            }
        }

        int g_ = 0, sommeDh = 0, coup = 0;
        for (Game::EDirection d : chemin) {
            const int avant = g.getNbDepCaisse();
            const int hAvant = g.getHeuristique(nullptr);
            const QPoint p = g.getPlayerPoint();
            const int devant = g.caseApres(p.x() + p.y() * L, d);

            g.deplace(d);
            coup++;
            {
                const QPoint q = g.getPlayerPoint();
                joueurParCoup.append(q.x() + q.y() * L);
            }
            if (g.getNbDepCaisse() == avant) continue;   // simple marche

            g_++;
            const int dh = g.getHeuristique(nullptr) - hAvant;
            sommeDh += dh;
            const int arrivee = g.caseApres(devant, d);

            const int id = idParCase.value(devant, -1);
            idParCase.remove(devant);
            idParCase[arrivee] = id;
            poussees.append({g_, id, devant, arrivee, dh, coup});
        }

        // PASSE 2 — pour chaque RECUL, que devient la case libérée ? Le §3 affirme
        // « elle s'éloigne + devra revenir » (donc la MÊME caisse y repasse) et
        // « elle était assise sur le trajet d'une autre » (donc une AUTRE caisse y
        // passe). Ce sont deux hypothèses DIFFÉRENTES, et elles se départagent ici.
        int nRevient = 0, nAutreCaisse = 0, nNiUnNiAutre = 0, improductives = 0, nPurJoueur = 0;
        for (const Poussee& r : poussees) {
            if (r.dh == -1) continue;
            improductives++;

            int revient = -1, autre = -1, autreId = -1, passages = 0;
            for (const Poussee& q : poussees) {
                if (q.num <= r.num || q.to != r.from) continue;
                passages++;
                if (q.id == r.id) { if (revient < 0) revient = q.num; }
                else if (autre < 0) { autre = q.num; autreId = q.id; }
            }
            // CLASSEMENT DU RECUL. Si la MÊME caisse revient sur sa case et
            // qu'AUCUNE autre n'a bougé entre-temps, alors la configuration des
            // caisses est rigoureusement identique avant et après : ces deux
            // poussées n'ont servi qu'à DÉPLACER LE JOUEUR. Sinon, la caisse s'est
            // écartée pour laisser passer du monde — du démêlage de caisses.
            int bougentEntre = 0, traversentEntre = 0;
            if (revient >= 0) {
                for (const Poussee& q : poussees) {
                    if (q.num <= r.num || q.num >= revient) continue;
                    if (q.id == r.id) continue;
                    bougentEntre++;
                    if (q.to == r.from) traversentEntre++;
                }
            }

            int joueurApres = -1;   // le joueur REVIENT-il sur la case (hors le pas
            for (int c = r.coup + 1; c < joueurParCoup.size(); c++)   // qui suit la poussée)
                if (joueurParCoup[c] == r.from && (c > r.coup + 1 || joueurParCoup[c-1] != r.from)) { joueurApres = c; break; }

            printf("\nRECUL a la poussee %d : caisse #%d (%d,%d)->(%d,%d)\n",
                   r.num, r.id, r.from % L, r.from / L, r.to % L, r.to / L);
            printf("   case liberee (%d,%d) : %d passage(s) de caisse ensuite\n",
                   r.from % L, r.from / L, passages);
            if (revient >= 0) printf("   -> la MEME caisse #%d y revient a la poussee %d\n", r.id, revient);
            if (autre  >= 0)  printf("   -> une AUTRE caisse (#%d) la traverse a la poussee %d\n", autreId, autre);
            if (revient < 0 && autre < 0) printf("   -> AUCUNE caisse n'y repasse jamais\n");
            if (joueurApres >= 0) printf("   -> le joueur y repasse au coup %d\n", joueurApres);

            if (revient >= 0) { nRevient++; if (bougentEntre == 0) nPurJoueur++; }
            else if (autre >= 0) nAutreCaisse++;
            else nNiUnNiAutre++;
        }

        printf("\n-- BILAN --\n");
        printf("   poussees      : %d\n", g_);
        printf("   RECULS        : %d  (%.1f %%)\n",
               improductives, g_ ? 100.0 * improductives / g_ : 0.0);
        printf("   dont la meme caisse REVIENT sur la case liberee : %d\n", nRevient);
        // Le classement qui départage les DEUX branches de la definition du mou au
        // §3 (« ecarter une caisse assise sur le trajet d'une autre » / « ou qui
        // bloque le JOUEUR ») : si aucune autre caisse ne bouge entre le recul et
        // le retour, la config des caisses est identique avant/apres — les deux
        // poussees n'ont servi qu'au joueur.
        printf("      dont ALLER-RETOUR PUR (2 poussees pour le seul JOUEUR) : %d\n", nPurJoueur);
        printf("      dont ecart pour laisser passer d'autres caisses        : %d\n",
               nRevient - nPurJoueur);
        printf("   dont une AUTRE caisse la traverse               : %d\n", nAutreCaisse);
        printf("   dont AUCUNE caisse n'y repasse                  : %d\n", nNiUnNiAutre);
        const int controle = g_ + sommeDh;
        printf("   controle somme(1+dh) = %d, mou attendu = %d  -> %s\n",
               controle, mouTotal,
               controle == mouTotal ? "COHERENT" : "*** INCOHERENT (solution non optimale ?) ***");
        fflush(stdout);
        QCoreApplication::quit();
    });

    QObject::connect(s, &Solveur::aucuneSolution, [num]() {
        printf("niveau %d : AUCUNE solution\n", num);
        fflush(stdout);
        QCoreApplication::quit();
    });

    s->start();
    return app.exec();
}
