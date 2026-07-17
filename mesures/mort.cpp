// Cherche des DEADLOCKS NON DÉTECTÉS sur un niveau qu'on NE SAIT PAS résoudre.
//
//   mort <niveau> [nbEchantillons=150] [budget=100000] [plafond=200000] [macro|opt]
//
// `mou` ne marche que sur un niveau résolu : il lui faut C* et la liste complète
// des états dépilés, donc une résolution qui termine. Le 11 ne termine pas.
//
// Ici on renonce à C*. On pose une seule question, état par état :
//
//     cet état, réellement DÉPILÉ par le solveur, est-il MORT ?
//
// Réponse par un sous-solve COMPLET et BORNÉ (A* optimal, budget d'états) :
//   - solutionTrouvee          -> SOLUBLE (pas un deadlock).
//   - file vidée SOUS le budget -> MORT : aucune solution n'existe depuis lui.
//                                 Comme il a été dépilé, checkDefaite() ne l'a
//                                 PAS vu -> c'est un deadlock manqué.
//   - budget atteint            -> INCONNU (trop de caisses restantes pour trancher).
//
// ⚠️ Le sous-solve de classification doit être COMPLET (A* optimal, jamais macro,
// qui est incomplet et pourrait crier « aucune solution » sur un état soluble).
// Le collecteur, lui, peut être la macro (elle atteint vite des états profonds).
//
// La population échantillonnée = les états DÉPILÉS (DUMP_DEV), pas {f <= C*} : c'est
// ce que le solveur explore vraiment (cf. mou.cpp).
#include <QCoreApplication>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

std::vector<std::pair<QByteArray,int>>& etatsDeveloppes();
int& limiteDepilements();

// ⚠️ getEtat()->QByteArray écrit la clé en BIG-ENDIAN (game.cpp : cle[i]>>8 puis
// &0xFF), alors que appliqueEtat(const quint16*) lit du NATIF. Passer les octets
// bruts à appliqueEtat byte-swappe donc tous les index (idx 5 -> 1280) et
// reconstruit un plateau FAUX. On décode le big-endian à la main.
// (Le même piège est présent dans mou.cpp, ligne 98.)
static std::vector<quint16> decodeCle(const QByteArray& b) {
    std::vector<quint16> v(b.size() / 2);
    for (int i = 0; i < (int)v.size(); i++)
        v[i] = ((quint16)(unsigned char)b[2 * i] << 8) | (unsigned char)b[2 * i + 1];
    return v;
}

// TEST DE DEADLOCK BON MARCHÉ — « livraison » (candidat pour checkDefaite).
// Un but VIDE dont aucune caisse ne peut être poussée jusqu'à lui rend l'état
// insoluble. On réplique la logique de Game::distanceLivraison (privée) : BFS avant
// sur les poussées, sources = les caisses de l'état, obstacles = murs + caisses DÉJÀ
// sur but (V2, agressif), le joueur devant MARCHER jusqu'à la case d'appui à chaque
// pas (mais les autres caisses ne bloquent pas sa marche — relaxation optimiste).
//
// ⚠️ Deux sources d'ERREUR possibles (à mesurer, pas à supposer) :
//   - traiter une caisse-sur-but comme obstacle immuable est FAUX si la solution la
//     repousse (parking §4) -> faux positif possible. C'est le juge des résolus.
//   - la relaxation « les autres caisses ne bloquent pas la marche » va dans le sens
//     optimiste (elle sous-estime la mort) -> ne crée PAS de faux positif.
static bool livraisonMorte(const Game& g) {
    const int L = g.getLargeur(), H = g.getHauteur(), size = L * H;
    static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};   // N E S O (cf. game.cpp)
    auto tc  = [&](int c) { return g.getCase(c); };
    auto libreCase = [&](int x, int y) -> bool {
        if (x < 0 || x >= L || y < 0 || y >= H) return false;
        const auto t = tc(x + y * L);
        return t != Level::tcMur && t != Level::tcGoalCaisse;   // mur ou caisse posée = bloqué
    };
    auto joueurAtteint = [&](int depart, int cible, int caseCaisse) -> bool {
        if (depart == cible) return true;
        std::vector<char> vu(size, 0);
        std::vector<int> f; f.push_back(depart); vu[depart] = 1;
        for (size_t h = 0; h < f.size(); h++) {
            const int c = f[h]; if (c == cible) return true;
            const int cx = c % L, cy = c / L;
            for (int d = 0; d < 4; d++) {
                const int nx = cx + DX[d], ny = cy + DY[d];
                if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
                const int n = nx + ny * L;
                if (vu[n] || n == caseCaisse || !libreCase(nx, ny)) continue;
                vu[n] = 1; f.push_back(n);
            }
        }
        return false;
    };

    std::vector<int> dist(size, -1), joueurApres(size, -1), file;
    const int depart = g.getPlayerPoint().x() + g.getPlayerPoint().y() * L;
    for (int c = 0; c < size; c++) {
        const auto t = tc(c);
        if (t == Level::tcCaisse || t == Level::tcGoalCaisse) {
            dist[c] = 0; joueurApres[c] = depart; file.push_back(c);
        }
    }
    for (size_t h = 0; h < file.size(); h++) {
        const int c = file[h], cx = c % L, cy = c / L;
        for (int d = 0; d < 4; d++) {
            const int ax = cx + DX[d], ay = cy + DY[d], px = cx - DX[d], py = cy - DY[d];
            if (!libreCase(ax, ay) || !libreCase(px, py)) continue;
            const int a = ax + ay * L; if (dist[a] != -1) continue;
            if (!joueurAtteint(joueurApres[c], px + py * L, c)) continue;
            dist[a] = dist[c] + 1; joueurApres[a] = c; file.push_back(a);
        }
    }
    for (int c = 0; c < size; c++) {
        const auto t = tc(c);
        if ((t == Level::tcGoal || t == Level::tcGoalPlayer) && dist[c] == -1) return true;
    }
    return false;
}

// VERSION APPARIEMENT (condition de Hall). Le test par-but ci-dessus rate les
// deadlocks de CAPACITÉ (chaque but est atteignable par UNE caisse, mais pas assez de
// buts DISTINCTS — niv 9/11). Ici : pour chaque caisse à livrer, l'ensemble des buts
// vides qu'elle peut atteindre (BFS de livraison mono-source, mêmes relaxations) ;
// puis couplage maximum biparti. Pas de couplage saturant les buts ⇒ mort. Surensemble
// strict du test par-but (un but sans aucune caisse = pas d'arête = pas saturable),
// toujours une relaxation ⇒ censé sans faux positif.
static bool livraisonMorteCouplage(const Game& g) {
    const int L = g.getLargeur(), H = g.getHauteur(), size = L * H;
    static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
    auto tc = [&](int c) { return g.getCase(c); };
    auto libreCase = [&](int x, int y) -> bool {
        if (x < 0 || x >= L || y < 0 || y >= H) return false;
        const auto t = tc(x + y * L);
        return t != Level::tcMur && t != Level::tcGoalCaisse;
    };
    auto joueurAtteint = [&](int depart, int cible, int caseCaisse) -> bool {
        if (depart == cible) return true;
        std::vector<char> vu(size, 0);
        std::vector<int> f; f.push_back(depart); vu[depart] = 1;
        for (size_t h = 0; h < f.size(); h++) {
            const int c = f[h]; if (c == cible) return true;
            const int cx = c % L, cy = c / L;
            for (int d = 0; d < 4; d++) {
                const int nx = cx + DX[d], ny = cy + DY[d];
                if (nx < 0 || nx >= L || ny < 0 || ny >= H) continue;
                const int n = nx + ny * L;
                if (vu[n] || n == caseCaisse || !libreCase(nx, ny)) continue;
                vu[n] = 1; f.push_back(n);
            }
        }
        return false;
    };
    const int depart = g.getPlayerPoint().x() + g.getPlayerPoint().y() * L;
    auto deliveryDist = [&](int src) {
        std::vector<int> dist(size, -1), joueurApres(size, -1), file;
        dist[src] = 0; joueurApres[src] = depart; file.push_back(src);
        for (size_t h = 0; h < file.size(); h++) {
            const int c = file[h], cx = c % L, cy = c / L;
            for (int d = 0; d < 4; d++) {
                const int ax = cx + DX[d], ay = cy + DY[d], px = cx - DX[d], py = cy - DY[d];
                if (!libreCase(ax, ay) || !libreCase(px, py)) continue;
                const int a = ax + ay * L; if (dist[a] != -1) continue;
                if (!joueurAtteint(joueurApres[c], px + py * L, c)) continue;
                dist[a] = dist[c] + 1; joueurApres[a] = c; file.push_back(a);
            }
        }
        return dist;
    };

    std::vector<int> boxes, butsVides;
    for (int c = 0; c < size; c++) {
        const auto t = tc(c);
        if (t == Level::tcCaisse) boxes.push_back(c);                       // caisses à livrer
        else if (t == Level::tcGoal || t == Level::tcGoalPlayer) butsVides.push_back(c);
    }
    const int nb = (int)boxes.size(), ng = (int)butsVides.size();
    if (ng == 0) return false;
    std::vector<std::vector<int>> adj(nb);
    for (int i = 0; i < nb; i++) {
        const std::vector<int> dist = deliveryDist(boxes[i]);
        for (int j = 0; j < ng; j++) if (dist[butsVides[j]] != -1) adj[i].push_back(j);
    }
    // Couplage maximum (Kuhn), on sature les BUTS.
    std::vector<int> butDe(ng, -1);   // but j -> caisse
    std::function<bool(int, std::vector<char>&)> augmente =
        [&](int i, std::vector<char>& vu) -> bool {
            for (int j : adj[i]) {
                if (vu[j]) continue;
                vu[j] = 1;
                if (butDe[j] == -1 || augmente(butDe[j], vu)) { butDe[j] = i; return true; }
            }
            return false;
        };
    int couples = 0;
    for (int i = 0; i < nb; i++) { std::vector<char> vu(ng, 0); if (augmente(i, vu)) couples++; }
    return couples < ng;   // un but non couplé = insoluble
}

// Verdict d'un sous-solve borné depuis 'etat'. 'nbDepiles' = combien d'états le
// sous-solve a dépilé (pour distinguer file-vidée de budget-atteint).
enum Verdict { SOLUBLE, MORT, INCONNU };
static Verdict classifie(const Game& etat, int budget, int* nbDepiles) {
    limiteDepilements() = budget;
    etatsDeveloppes().clear();

    Solveur* s = Solveur::creer(Solveur::Astar, etat);   // OPTIMAL, complet
    bool trouve = false;
    QObject::connect(s, &Solveur::solutionTrouvee, s,
                     [&trouve](QList<Game::EDirection>, qint64) { trouve = true; },
                     Qt::DirectConnection);
    s->start();
    s->wait();
    delete s;

    const int depiles = (int)etatsDeveloppes().size();
    if (nbDepiles) *nbDepiles = depiles;
    if (trouve)            return SOLUBLE;
    if (depiles >= budget) return INCONNU;   // le plafond a coupé avant la fin
    return MORT;                             // file vidée pour de bon
}

// Écrit un état en .xsb, pour l'inspecter à l'œil (l'œil bat les composantes
// connexes, §11.4).
static void exporteXsb(const Game& g, const QString& chemin) {
    const int L = g.getLargeur(), H = g.getHauteur();
    QByteArray out;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            char c = ' ';
            switch (g.getCase(y * L + x)) {
                case Level::tcNone:       c = ' '; break;
                case Level::tcMur:        c = '#'; break;
                case Level::tcPlayer:     c = '@'; break;
                case Level::tcCaisse:     c = '$'; break;
                case Level::tcGoal:       c = '.'; break;
                case Level::tcGoalCaisse: c = '*'; break;
                case Level::tcGoalPlayer: c = '+'; break;
            }
            out.append(c);
        }
        out.append('\n');
    }
    FILE* f = fopen(chemin.toLocal8Bit().constData(), "w");
    if (f) { fwrite(out.constData(), 1, out.size(), f); fclose(f); }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        fprintf(stderr, "usage: mort <niveau> [nbEch=150] [budget=100000] [plafond=200000] [macro|opt]\n");
        return 2;
    }
    const int num      = QString(argv[1]).toInt();
    const int nbEch    = (argc > 2) ? QString(argv[2]).toInt() : 150;
    const int budget   = (argc > 3) ? QString(argv[3]).toInt() : 100000;
    const int plafond  = (argc > 4) ? QString(argv[4]).toInt() : 200000;
    const bool macro   = (argc > 5) ? (QString(argv[5]) != "opt") : true;

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game depart(level, num);
    const int nbButs = depart.getNbButs();

    // SELF-TEST endianness : reconstruire le DÉPART par les deux voies et comparer
    // au vrai board. Une seule doit round-tripper (meme joueur, memes caisses).
    if (argc > 2 && QString(argv[2]) == "selftest") {
        QByteArray k = depart.getEtat();
        Game raw(depart);  raw.appliqueEtat((const quint16*)k.constData());
        Game dec(depart);  dec.appliqueEtat(decodeCle(k).data());
        auto sig = [](const Game& g){
            int L=g.getLargeur(),H=g.getHauteur(),nb=0; QString s;
            for(int i=0;i<L*H;i++){ auto c=g.getCase(i); if(c==Level::tcCaisse||c==Level::tcGoalCaisse){nb++; s+=QString::number(i)+" ";} }
            return QString("joueur=(%1,%2) nbCaisses=%3 cases=[%4]")
                .arg(g.getPlayerPoint().x()).arg(g.getPlayerPoint().y()).arg(nb).arg(s.trimmed());
        };
        printf("depart : %s\n", sig(depart).toLocal8Bit().constData());
        printf("raw    : %s\n", sig(raw).toLocal8Bit().constData());
        printf("decode : %s\n", sig(dec).toLocal8Bit().constData());
        return 0;
    }

    // MODE PORTES : pour chaque premier coup, mesure l'effet sur la CONNECTIVITÉ
    // (zone atteignable du joueur) et croise avec le verdict mort/vivant. Teste
    // l'hypothèse « fermer une porte (créer un corral) => mort ».
    if (argc > 5 && QString(argv[5]) == "portes") {
        auto zoneSize = [](const Game& g){
            QVector<bool> z = g.getZoneJoueur(); int n=0; for(bool b:z) if(b) n++; return n;
        };
        // Cellules atteignables AVANT (depart) qui deviennent NON atteignables et
        // restent LIBRES après = corral créé par la poussée.
        auto corralCree = [&](const Game& avant, const Game& apres){
            QVector<bool> za = avant.getZoneJoueur(), zp = apres.getZoneJoueur();
            int n=0;
            for (int i=0;i<avant.getLargeur()*avant.getHauteur();i++){
                if (za[i] && !zp[i]) {
                    auto c = apres.getCase(i);
                    if (c==Level::tcNone || c==Level::tcGoal) n++;   // libre mais injoignable
                }
            }
            return n;
        };
        const int z0 = zoneSize(depart);
        printf("niveau %d — zone joueur au depart = %d cases\n\n", num, z0);
        QVector<bool> zone = depart.getZoneJoueur();
        QVector<quint8> caisses = depart.getCaissesDeplacable(zone);
        for (int i=0;i<caisses.size();i++){
            if(!caisses[i]) continue;
            for(int d=0; d<NB_DIRECTION; d++){
                if(!(caisses[i]&(1<<d))) continue;
                Game e(depart);
                if(!e.pousse(i,(Game::EDirection)d) || e.isPerdu()) continue;
                int dep; const Verdict v = classifie(e, budget, &dep);
                const char* nom = v==SOLUBLE?"SOLUBLE":(v==MORT?"MORT   ":"inconnu");
                printf("  caisse@%3d dir %d : zone %d -> %d (delta %+d), corral cree %d  => %s\n",
                       i, d, z0, zoneSize(e), zoneSize(e)-z0, corralCree(depart,e), nom);
            }
        }
        return 0;
    }

    // MODE ENFANTS : au lieu d'échantillonner ce que le solveur explore, on énumère
    // les enfants directs du DÉPART (chaque poussée légale) et on classe chacun. Test
    // direct de « beaucoup de premiers coups, un seul bon, le reste mort-né ». Un
    // enfant MORT = un premier coup qu'aucune détection actuelle ne coupe.
    // ⚠️ Un enfant SOLUBLE ne finit pas forcément sous budget (il reste ~14 caisses)
    // -> il ressort INCONNU, pas SOLUBLE. Seul le verdict MORT est concluant ici :
    //    il compte les premiers coups à espace-mort PETIT (donc détectables tôt).
    if (argc > 5 && QString(argv[5]) == "enfants") {
        QVector<bool> zone = depart.getZoneJoueur();
        QVector<quint8> caisses = depart.getCaissesDeplacable(zone);
        int total = 0, nSol = 0, nMort = 0, nInc = 0, nbXsb = 0;
        printf("niveau %d — classification des ENFANTS du depart (budget %d)\n\n", num, budget);
        for (int i = 0; i < caisses.size(); i++) {
            if (!caisses[i]) continue;
            for (int d = 0; d < NB_DIRECTION; d++) {
                if (!(caisses[i] & (1 << d))) continue;
                Game e(depart);
                if (!e.pousse(i, (Game::EDirection)d) || e.isPerdu()) continue;   // deja coupe
                total++;
                int depiles = 0;
                const Verdict v = classifie(e, budget, &depiles);
                const char* nom = v == SOLUBLE ? "SOLUBLE" : (v == MORT ? "MORT   " : "inconnu");
                printf("  caisse@%3d dir %d -> %s  (sous-solve %d etats)\n",
                       i, d, nom, depiles);
                if (v == SOLUBLE) nSol++;
                else if (v == MORT) {
                    nMort++;
                    Game g(depart); g.pousse(i, (Game::EDirection)d);
                    exporteXsb(g, QString("%1/mort_niv%2_enfant%3.xsb").arg(SCRATCH).arg(num).arg(nbXsb++));
                } else nInc++;
            }
        }
        printf("\n===== NIVEAU %d — ENFANTS DU DEPART =====\n", num);
        printf("  total enfants (non deja coupes) : %d\n", total);
        printf("  SOLUBLES (finis sous budget)    : %d\n", nSol);
        printf("  MORTS (mort-nes non detectes)   : %d  <- premiers coups a couper\n", nMort);
        printf("  inconnus (budget, ~14 caisses)  : %d  <- solubles OU morts a espace profond\n", nInc);
        if (nbXsb) printf("\n  %d enfants morts exportes en %s/mort_niv%d_enfant*.xsb\n", nbXsb, SCRATCH, num);
        fflush(stdout);
        return 0;
    }

    // MODE CMP : reproduit l'environnement de mou (collecte via A* optimal) et
    // classe chaque échantillon DEUX fois — cast brut (comme mou) vs decodeCle —
    // pour trancher empiriquement lequel est correct sur la population de mou.
    if (argc > 5 && QString(argv[5]) == "cmp") {
        limiteDepilements() = plafond;
        etatsDeveloppes().clear();
        { Solveur* s = Solveur::creer(Solveur::Astar, depart); s->start(); s->wait(); delete s; }
        std::vector<std::pair<QByteArray,int>> dev = etatsDeveloppes();
        printf("cmp niveau %d : %d etats (opt)\n", num, (int)dev.size());
        std::srand(12345);
        int rSol=0,rMort=0,rInc=0, dSol=0,dMort=0,dInc=0;
        const int n = std::min(nbEch, (int)dev.size());
        Game w(depart);
        for (int k=0;k<n;k++){
            const QByteArray& cle = dev[std::rand()%(int)dev.size()].first;
            int dep;
            w.appliqueEtat((const quint16*)cle.constData());              // RAW (mou)
            if (w.isGagne()) rSol++; else { Verdict v=classifie(w,budget,&dep); if(v==SOLUBLE)rSol++;else if(v==MORT)rMort++;else rInc++; }
            w.appliqueEtat(decodeCle(cle).data());                        // DECODE
            if (w.isGagne()) dSol++; else { Verdict v=classifie(w,budget,&dep); if(v==SOLUBLE)dSol++;else if(v==MORT)dMort++;else dInc++; }
        }
        printf("  RAW    (comme mou) : soluble %d / mort %d / inconnu %d\n", rSol,rMort,rInc);
        printf("  DECODE (corrige)   : soluble %d / mort %d / inconnu %d\n", dSol,dMort,dInc);
        return 0;
    }

    // MODE LIVRAISON : croise, sur chaque état échantillonné, le verdict ORACLE
    // (sous-solve optimal borné) et le TEST BON MARCHÉ livraisonMorte(). On mesure
    //   - le TAUX DE CAPTURE : parmi les MORTS oracle, combien le test attrape ;
    //   - et surtout le FAUX POSITIF : parmi les SOLUBLES oracle, combien le test
    //     déclare morts À TORT. Ce compte DOIT être 0 pour intégrer le test à
    //     checkDefaite (le canari). `mort N .. livraison [opt]` : collecte macro
    //     (profondeur) par défaut, opt en 7e arg.
    if (argc > 5 && (QString(argv[5]) == "livraison" || QString(argv[5]) == "couplage")) {
        const bool couplage = (QString(argv[5]) == "couplage");
        auto testMort = [&](const Game& gg){ return couplage ? livraisonMorteCouplage(gg) : livraisonMorte(gg); };
        const bool collMacro = !(argc > 6 && QString(argv[6]) == "opt");
        printf("niveau %d — %s : collecte %d etats (%s), classif oracle vs test\n",
               num, couplage ? "COUPLAGE" : "LIVRAISON", plafond, collMacro ? "macro" : "opt");
        fflush(stdout);
        limiteDepilements() = plafond;
        etatsDeveloppes().clear();
        { Solveur* s = Solveur::creer(collMacro ? Solveur::AstarMacro : Solveur::Astar, depart);
          s->start(); s->wait(); delete s; }
        std::vector<std::pair<QByteArray,int>> dev = etatsDeveloppes();
        printf("  %d etats collectes.\n", (int)dev.size());
        if (dev.empty()) { printf("rien a echantillonner\n"); return 1; }

        // Échantillon biaisé profondeur (comme le mode par défaut).
        Game sonde(depart);
        std::vector<int> posesOf(dev.size());
        for (size_t i = 0; i < dev.size(); i++)
            posesOf[i] = sonde.appliqueEtat(decodeCle(dev[i].first).data());
        std::srand(12345);
        std::vector<int> idx(dev.size());
        for (size_t i = 0; i < idx.size(); i++) idx[i] = (int)i;
        for (int i = (int)idx.size() - 1; i > 0; i--) std::swap(idx[i], idx[std::rand() % (i + 1)]);
        std::stable_sort(idx.begin(), idx.end(), [&](int a, int b){ return posesOf[a] > posesOf[b]; });
        const int n = std::min(nbEch, (int)dev.size());

        int TP=0, FN=0, FP=0, TN=0, incDead=0, incAlive=0;
        Game w(depart);
        for (int k = 0; k < n; k++) {
            w.appliqueEtat(decodeCle(dev[idx[k]].first).data());
            if (w.isGagne()) continue;                       // état gagnant : ni mort ni à tester
            const bool cheapDead = testMort(w);
            int dep; const Verdict v = classifie(w, budget, &dep);
            if (v == MORT)         { if (cheapDead) TP++; else FN++; }
            else if (v == SOLUBLE) { if (cheapDead) FP++; else TN++; }
            else                   { if (cheapDead) incDead++; else incAlive++; }
        }
        printf("\n===== NIVEAU %d — TEST LIVRAISON vs ORACLE (%d etats) =====\n", num, n);
        const int mort = TP + FN, sol = FP + TN;
        printf("  MORTS oracle   : %3d  -> test attrape %3d  (capture %2.0f %%)\n",
               mort, TP, mort ? 100.0*TP/mort : 0.0);
        printf("  SOLUBLES oracle: %3d  -> test dit mort %3d  (FAUX POSITIFS %2.0f %%) %s\n",
               sol, FP, sol ? 100.0*FP/sol : 0.0, FP ? "  <<< DANGER (casse le canari)" : "  <- 0, sur pour checkDefaite");
        printf("  INCONNUS       : %3d  -> test dit mort %3d (probables morts profonds)\n",
               incDead + incAlive, incDead);
        fflush(stdout);
        return 0;
    }

    // 1. COLLECTE : on laisse le solveur explorer jusqu'au plafond et on récupère
    //    les états qu'il a réellement dépilés.
    printf("niveau %d — collecte de %d etats depiles (%s)...\n",
           num, plafond, macro ? "macro" : "A* optimal");
    fflush(stdout);
    limiteDepilements() = plafond;
    etatsDeveloppes().clear();
    {
        Solveur* s = Solveur::creer(macro ? Solveur::AstarMacro : Solveur::Astar, depart);
        s->start();
        s->wait();
        delete s;
    }
    std::vector<std::pair<QByteArray,int>> dev = etatsDeveloppes();   // copie : les sous-solves l'ecrasent
    printf("  %d etats collectes.\n\n", (int)dev.size());
    if (dev.empty()) { printf("rien a echantillonner\n"); return 1; }

    // 2a. DISTRIBUTION des caisses posées sur TOUT le collecté : les deadlocks
    //     candidats vivent dans les états PROFONDS (salle à moitié remplie dans le
    //     mauvais ordre), pas dans le churn de la racine.
    Game sonde(depart);
    std::vector<int> posesOf(dev.size());
    std::vector<int> histoPose(nbButs + 1, 0);
    for (size_t i = 0; i < dev.size(); i++) {
        posesOf[i] = sonde.appliqueEtat(decodeCle(dev[i].first).data());
        histoPose[posesOf[i]]++;
    }
    printf("  distribution des caisses posees sur le collecte :\n");
    for (int p = 0; p <= nbButs; p++)
        if (histoPose[p]) printf("    %2d/%d : %d etats\n", p, nbButs, histoPose[p]);
    printf("\n");

    // 2b. ÉCHANTILLONNAGE reproductible, BIAISÉ VERS LA PROFONDEUR : on trie par
    //     nombre de caisses posées décroissant et on prend les plus profonds
    //     d'abord (départage aléatoire à profondeur égale). C'est là que le
    //     deadlock « salle qui se mure » se produirait.
    std::srand(12345);
    std::vector<int> idx(dev.size());
    for (size_t i = 0; i < idx.size(); i++) idx[i] = (int)i;
    for (int i = (int)idx.size() - 1; i > 0; i--) std::swap(idx[i], idx[std::rand() % (i + 1)]);
    std::stable_sort(idx.begin(), idx.end(),
                     [&](int a, int b){ return posesOf[a] > posesOf[b]; });
    const int n = std::min(nbEch, (int)dev.size());

    // 3. CLASSIFICATION + stratification par nombre de caisses posées.
    int nSol = 0, nMort = 0, nInc = 0;
    std::vector<int> mortParPose(nbButs + 1, 0), totParPose(nbButs + 1, 0);
    int nbXsb = 0;
    const int maxXsb = 12;

    // Corral d'un état RELATIF au départ : cases libres atteignables au départ mais
    // plus dans la zone du joueur ici (les boxes les ont scellées). Annule le
    // remplissage de bordure, constant. corral>0 = une région fermée existe.
    const QVector<bool> zoneDepart = depart.getZoneJoueur();
    const int taille = depart.getLargeur() * depart.getHauteur();
    auto corralEtat = [&](const Game& g){
        QVector<bool> z = g.getZoneJoueur(); int n=0;
        for (int i=0;i<taille;i++)
            if (zoneDepart[i] && !z[i]) { auto c=g.getCase(i); if(c==Level::tcNone||c==Level::tcGoal) n++; }
        return n;
    };
    long sommeCorralMort=0, sommeCorralSol=0; int mortAvecCorral=0, solAvecCorral=0;

    Game travail(depart);
    for (int k = 0; k < n; k++) {
        const int poses = travail.appliqueEtat(decodeCle(dev[idx[k]].first).data());
        totParPose[poses]++;

        if (travail.isGagne()) { nSol++; continue; }

        const int corral = corralEtat(travail);
        int depiles = 0;
        const Verdict v = classifie(travail, budget, &depiles);
        if (v == SOLUBLE) { nSol++; sommeCorralSol += corral; if (corral) solAvecCorral++; }
        else if (v == INCONNU) nInc++;
        else {
            nMort++;
            sommeCorralMort += corral; if (corral) mortAvecCorral++;
            mortParPose[poses]++;
            if (nbXsb < maxXsb) {
                // On réapplique : le sous-solve a écrasé 'travail' via le Game de
                // travail interne ? Non — classifie() clone. Mais par prudence, on
                // ré-applique avant d'exporter.
                travail.appliqueEtat(decodeCle(dev[idx[k]].first).data());
                exporteXsb(travail, QString("%1/mort_niv%2_%3.xsb")
                           .arg(SCRATCH).arg(num).arg(nbXsb));
                nbXsb++;
            }
        }
        if ((k + 1) % 25 == 0) {
            printf("  ... %d/%d classes  (sol %d / mort %d / inconnu %d)\n",
                   k + 1, n, nSol, nMort, nInc);
            fflush(stdout);
        }
    }

    printf("\n===== NIVEAU %d — DEADLOCKS NON DETECTES (%d etats depiles, budget %d) =====\n",
           num, n, budget);
    printf("  SOLUBLES              : %3d / %3d  (%2.0f %%)\n", nSol,  n, 100.0 * nSol  / n);
    printf("  MORTS (deadlock rate) : %3d / %3d  (%2.0f %%)  <- checkDefaite() les a rates\n",
           nMort, n, 100.0 * nMort / n);
    printf("  INCONNUS (budget)     : %3d / %3d  (%2.0f %%)  <- trop de caisses pour trancher\n",
           nInc,  n, 100.0 * nInc  / n);

    printf("\n  CORRAL (region scellee vs depart) :\n");
    if (nMort) printf("    MORTS    : %d/%d ont un corral>0 (%2.0f %%), taille moy %.1f\n",
                      mortAvecCorral, nMort, 100.0*mortAvecCorral/nMort, (double)sommeCorralMort/nMort);
    if (nSol)  printf("    SOLUBLES : %d/%d ont un corral>0 (%2.0f %%), taille moy %.1f\n",
                      solAvecCorral, nSol, 100.0*solAvecCorral/nSol, (double)sommeCorralSol/nSol);

    printf("\n  repartition des MORTS par nb de caisses posees :\n");
    for (int p = 0; p <= nbButs; p++)
        if (totParPose[p])
            printf("    %2d/%d posees : %3d morts / %3d echantillons\n",
                   p, nbButs, mortParPose[p], totParPose[p]);

    if (nbXsb)
        printf("\n  %d etats morts exportes en %s/mort_niv%d_*.xsb\n", nbXsb, SCRATCH, num);

    fflush(stdout);
    return 0;
}
