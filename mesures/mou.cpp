// Mesure le VRAI mou de h sur les états que A* développe RÉELLEMENT.
//
//   mou <niveau> [nbEchantillons]
//
// Trois populations, et elles appellent trois travaux OPPOSÉS :
//
//   h*(s) == C* - g(s)  -> s est vraiment sur un chemin optimal.
//                          INÉVITABLE : aucune h, même parfaite, ne l'élaguerait.
//   h*(s)  > C* - g(s)  -> s est soluble mais hors chemin optimal.
//                          Une h plus tendue l'élaguerait ; mou = h* - h dit
//                          de combien.
//   pas de solution     -> s est un DEADLOCK que checkDefaite() n'a pas vu.
//                          Ni h ni pondération n'y peuvent rien : il faut une
//                          meilleure détection de deadlock. Et le gain est
//                          superlinéaire — on coupe l'état ET toute sa
//                          descendance.
//
// ⚠️ On échantillonne les états DÉPILÉS par le solveur (DUMP_DEV), et non
// l'ensemble {f <= C*} : A* s'arrête au but et n'en visite qu'une fraction
// (niveau 1 : 15 596 dépilés pour 392 066 dans {f <= C*}, soit 25x). Mesurer sur
// {f <= C*} donnerait des pourcentages qui ne décrivent aucun coût réel.
#include <QCoreApplication>
#include <QString>
#include <QByteArray>
#include <QVector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>
#include "level.h"
#include "game.h"
#include "solveur.h"

std::vector<std::pair<QByteArray,int>>& etatsDeveloppes();

// ⚠️ getEtat()->QByteArray écrit la clé en BIG-ENDIAN (game.cpp : cle[i]>>8 puis
// &0xFF), alors que appliqueEtat(const quint16*) lit du NATIF. Passer les octets
// bruts à appliqueEtat byte-swappe tous les index -> plateau VIDE (0 caisse), que
// checkVictoire() prend pour un état GAGNÉ. Bug historique de cet outil : il rendait
// « 100 % sur chemin, 0 deadlock » sur du vide. On décode le big-endian à la main.
static std::vector<quint16> decodeCle(const QByteArray& b) {
    std::vector<quint16> v(b.size() / 2);
    for (int i = 0; i < (int)v.size(); i++)
        v[i] = ((quint16)(unsigned char)b[2 * i] << 8) | (unsigned char)b[2 * i + 1];
    return v;
}

// Résout DEPUIS 'etat' en A* optimal, en synchrone. Rend le nombre de poussées
// optimal (h*), ou -1 s'il n'y a pas de solution (= deadlock).
static int resoudreDepuis(const Game& etat) {
    Solveur* s = Solveur::creer(Solveur::Astar, etat);

    int hStar = -1;
    QObject::connect(s, &Solveur::solutionTrouvee, s,
                     [&hStar, etat](QList<Game::EDirection> coups, qint64) {
        Game g(etat);
        int poussees = 0;
        for (Game::EDirection d : coups) {
            const int avant = g.getNbDepCaisse();
            g.deplace(d);
            if (g.getNbDepCaisse() > avant) poussees++;
        }
        hStar = g.isGagne() ? poussees : -1;
    }, Qt::DirectConnection);

    s->start();
    s->wait();
    delete s;
    return hStar;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: mou <niveau> [nbEchantillons]\n"); return 2; }
    const int num   = QString(argv[1]).toInt();
    const int nbEch = (argc > 2) ? QString(argv[2]).toInt() : 100;

    Level level;
    level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    Game depart(level, num);

    // 1. Résolution de référence : donne C* ET, via DUMP_DEV, la liste exacte
    //    des états dépilés.
    etatsDeveloppes().clear();
    const int cStar = resoudreDepuis(depart);
    if (cStar < 0) { printf("AUCUNE SOLUTION\n"); return 1; }

    std::vector<std::pair<QByteArray,int>> dev = etatsDeveloppes();   // copie : les sous-solves vont l'écraser
    printf("niveau %d : C* = %d poussees, %d etats developpes\n",
           num, cStar, (int)dev.size());

    // 2. Échantillonner parmi eux, et résoudre depuis chacun.
    std::srand(12345);   // reproductible
    std::vector<int> idx(dev.size());
    for (size_t i = 0; i < idx.size(); i++) idx[i] = (int)i;
    for (int i = (int)idx.size() - 1; i > 0; i--) std::swap(idx[i], idx[std::rand() % (i + 1)]);

    const int n = std::min(nbEch, (int)dev.size());

    int surChemin = 0, horsChemin = 0, deadlocks = 0;
    int sommeMou = 0, pireMou = 0;

    Game travail(depart);

    for (int k = 0; k < n; k++) {
        const QByteArray& cle = dev[idx[k]].first;
        const int g = dev[idx[k]].second;

        travail.appliqueEtat(decodeCle(cle).data());
        if (travail.isGagne()) { surChemin++; continue; }   // l'etat but lui-meme

        const int h = travail.getHeuristique();

        etatsDeveloppes().clear();                 // le sous-solve va y ecrire, on s'en fiche
        const int hStar = resoudreDepuis(travail);

        if (hStar < 0)                    deadlocks++;
        else if (hStar == cStar - g)      surChemin++;
        else {
            horsChemin++;
            sommeMou += hStar - h;
            pireMou = std::max(pireMou, hStar - h);
        }
    }

    printf("\n===== NIVEAU %d — VRAI MOU SUR LES ETATS DEPILES (%d echantillons) =====\n", num, n);
    printf("  sur un chemin optimal : %3d / %3d  (%2.0f %%)  <- INEVITABLES, aucune h ne les coupe\n",
           surChemin, n, 100.0 * surChemin / n);
    printf("  hors chemin, solubles : %3d / %3d  (%2.0f %%)  <- une h plus tendue les couperait\n",
           horsChemin, n, 100.0 * horsChemin / n);
    printf("  DEADLOCKS non detectes: %3d / %3d  (%2.0f %%)  <- checkDefaite() les rate\n",
           deadlocks, n, 100.0 * deadlocks / n);
    if (horsChemin)
        printf("  mou moyen sur les hors-chemin solubles : %.2f poussees  (max %d)\n",
               (double)sommeMou / horsChemin, pireMou);

    fflush(stdout);
    return 0;
}
