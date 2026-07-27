// ordre — POURQUOI la macro se mure : l'ordre de remplissage respecte-t-il la
// précédence par approches ?
//
//   ordre <niveau|fichier.xsb>
//
// LA QUESTION. `ordreButs` (calculé par `Game::ordreParPrecedence`, game.cpp) dicte
// à la goal macro quel but remplir ensuite. S'il est mauvais, la macro se mure toute
// seule : elle pose des caisses qui bouchent l'accès aux buts restants, et le solveur
// plafonne à mi-chemin sans jamais émettre « aucune solution » (le Groupe B du §0).
// C'est LOUD mais silencieux : on voit le plafonnement, jamais la cause.
//
// LA RÈGLE MESURÉE (plan.md §6.2, 2026-07-20 — c'est un THÉORÈME, pas un score) :
//
//   « Pour chaque but, énumérer les approches (case de la caisse + case d'appui du
//     joueur). Si toutes les approches viables passent par un autre but, ce but doit
//     être rempli APRÈS lui. »
//
// Pour poser une caisse sur le but G en la poussant dans la direction d, il faut la
// caisse en G−d et le joueur en G−2d. L'approche est VIABLE si ces deux cases
// existent et ne sont pas des murs ; elle est BOUCHÉE si l'une des deux porte un but
// DÉJÀ REMPLI (une caisse posée ne bouge plus dans le régime d'engagement).
//
// CE QUE FAIT L'OUTIL. Il déroule l'ordre calculé et vérifie qu'à son tour, chaque
// but a encore au moins une approche libre. Le premier but qui n'en a plus est le
// point de murage — avec les buts responsables, nommés.
//
// ⚠️ RELAXATION OPTIMISTE, donc les violations sont des PREUVES et les « OK » ne
// prouvent rien. On ignore l'acheminement (une caisse peut-elle seulement ARRIVER
// sur la case d'approche ?) et les autres caisses. Un ordre déclaré sain ici peut
// donc encore échouer — c'est la limite connue du §6.2, « le rebours ne simule pas
// le trajet de tirage ». Un ordre déclaré muré, lui, l'est vraiment.

#include <QCoreApplication>
#include <QVector>
#include <QSet>
#include <cstdio>
#include "game.h"
#include "precedencepaires.h"
#include "level.h"

static const int DX[4] = { 0, 1, 0, -1 };
static const int DY[4] = { -1, 0, 1, 0 };
static const char* NOM[4] = { "Haut", "Droite", "Bas", "Gauche" };

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { fprintf(stderr, "usage: ordre <niveau|fichier.xsb>\n"); return 2; }

    const QString arg1 = argv[1];
    const bool parChemin = arg1.endsWith(".xsb");
    const int num = parChemin ? 0 : arg1.toInt();

    Level level;
    level.load(parChemin ? arg1
                         : QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num, 4, 10, QChar('0')));
    if (!level.isLoaded()) { fprintf(stderr, "ordre: niveau introuvable (%s)\n", qPrintable(arg1)); return 2; }

    const Game game(level, num);
    const int L = game.getLargeur(), H = game.getHauteur();
    const QVector<int>& ordre = game.getOrdreButs();
    const int nb = game.getNbButs();

    auto xy = [&](int c) { return QString("(%1,%2)").arg(c % L).arg(c / L); };
    auto mur = [&](int c) { return game.getCase(c) == Level::tcMur; };

    // case -> indice de but, -1 sinon
    QVector<int> butDe(L * H, -1);
    for (int b = 0; b < nb; b++) butDe[game.getCaseBut(b)] = b;

    printf("=== %s — %d buts, plateau %dx%d ===\n\n",
           qPrintable(parChemin ? arg1 : QString::number(num)), nb, L, H);

    if (ordre.size() != nb) {
        printf("⚠️  ordreButs a %d entrées pour %d buts — la précédence n'a PAS rendu une\n"
               "    permutation complète, c'est le REBOURS qui sert de fallback (game.cpp).\n\n",
               ordre.size(), nb);
    }

    // --- CARTE : chaque but porte son RANG de remplissage, en base 36 -------------
    printf("Carte des rangs (0-9a-z, '.' = but hors ordre) :\n");
    QVector<int> rangDe(L * H, -1);
    for (int k = 0; k < ordre.size(); k++) rangDe[game.getCaseBut(ordre[k])] = k;
    for (int y = 0; y < H; y++) {
        printf("  ");
        for (int x = 0; x < L; x++) {
            const int c = x + y * L;
            if (mur(c))            putchar('#');
            else if (butDe[c] >= 0) {
                const int r = rangDe[c];
                putchar(r < 0 ? '.' : (r < 10 ? char('0' + r) : char('a' + r - 10)));
            }
            else                   putchar(' ');
        }
        putchar('\n');
    }

    // --- DÉROULÉ : à son tour, ce but a-t-il encore une approche libre ? ----------
    printf("\nDéroulé de l'ordre (une approche = caisse en G−d, joueur en G−2d) :\n");
    QSet<int> remplis;                       // cases des buts déjà posés
    int premierMurage = -1;
    for (int k = 0; k < ordre.size(); k++) {
        const int b = ordre[k], G = game.getCaseBut(b);
        const int gx = G % L, gy = G / L;

        int viables = 0, libres = 0;
        QString detail, coupables;
        for (int d = 0; d < 4; d++) {
            const int cx = gx - DX[d],     cy = gy - DY[d];       // case de la caisse
            const int ax = gx - 2 * DX[d], ay = gy - 2 * DY[d];   // appui du joueur
            if (cx < 0 || cx >= L || cy < 0 || cy >= H) continue;
            if (ax < 0 || ax >= L || ay < 0 || ay >= H) continue;
            const int cc = cx + cy * L, aa = ax + ay * L;
            if (mur(cc) || mur(aa)) continue;                     // approche impossible
            viables++;
            const bool bloqCaisse = remplis.contains(cc);
            const bool bloqAppui  = remplis.contains(aa);
            if (!bloqCaisse && !bloqAppui) {
                libres++;
                detail += QString(" %1").arg(NOM[d]);
            } else {
                if (bloqCaisse) coupables += QString(" %1[%2]").arg(NOM[d]).arg(xy(cc));
                if (bloqAppui)  coupables += QString(" %1{%2}").arg(NOM[d]).arg(xy(aa));
            }
        }

        const bool mure = (viables > 0 && libres == 0);
        if (mure && premierMurage < 0) premierMurage = k;
        printf("  %2d. but %-8s  approches viables=%d  libres=%d  %s%s\n",
               k, qPrintable(xy(G)), viables, libres,
               mure ? "  <<< MURÉ, bouché par :" : (libres ? qPrintable(detail) : "  (aucune approche viable — but INATTEIGNABLE dès le départ)"),
               mure ? qPrintable(coupables) : "");
        remplis.insert(G);
    }

    // --- PRÉCÉDENCE GLOBALE : le TRAJET DE TIRAGE, pas seulement le dernier pas ---
    //
    // La règle du §6.2 ne regarde que l'approche FINALE (caisse en G−d, joueur en
    // G−2d) : elle rate le cas où l'approche est libre mais où AUCUNE caisse ne peut
    // arriver sur la case d'approche. C'est la limite que le plan avait nommée sans
    // la corriger — « le rebours ne simule pas le trajet de tirage ».
    //
    // LA GÉNÉRALISATION, en une phrase :
    //     G doit précéder B si, en traitant B comme OCCUPÉ, plus aucune caisse du
    //     départ ne peut atteindre G.
    //
    // Test : BFS de TIRAGE à rebours depuis G (on remonte les poussées : la caisse
    // était en G−d, le joueur en G−2d), une fois sans obstacle puis une fois par but
    // B marqué occupé. Relaxation OPTIMISTE — joueur supposé capable d'atteindre
    // n'importe quel appui, aucune autre caisse — donc « inatteignable » est une
    // PREUVE, et « atteignable » ne promet rien. O(buts² × plateau).
    //
    // EXEMPLAIRE UNIQUE — le BFS vit dans `mesures/precedencepaires.h`, partagé avec
    // le juge `fp -3`. Les deux ne peuvent donc pas diverger, et ce que `fp` déclare
    // sûr est exactement ce que cet outil affiche. Principe déjà appliqué à
    // `avanceVersBut` (§6.3) : un test dupliqué finit toujours par mentir.
    auto atteintUneCaisse = [&](int G, int bloque, int bloque2 = -1) {
        return PrecedencePaires::atteintUneCaisse(game, G, bloque, bloque2);
    };

    printf("\nPrécédence GLOBALE (trajet de tirage complet, B traité comme occupé) :\n");
    int violations = 0, aretes = 0, inatteignables = 0;
    QVector<bool> simple(nb * nb, false);   // simple[b*nb+b2] : B2 SEUL affame b
    QVector<bool> utilisable(nb, false);    // but atteignable sans le moindre obstacle
    for (int b = 0; b < nb; b++) {
        const int G = game.getCaseBut(b);
        if (!atteintUneCaisse(G, -1)) {
            printf("  ⚠️  but %s : INATTEIGNABLE même sans aucun obstacle (plateau douteux)\n",
                   qPrintable(xy(G)));
            inatteignables++;
            continue;
        }
        utilisable[b] = true;
        for (int b2 = 0; b2 < nb; b2++) {
            if (b2 == b) continue;
            const int B = game.getCaseBut(b2);
            if (atteintUneCaisse(G, B)) continue;      // B n'est pas nécessaire
            simple[b * nb + b2] = true;
            aretes++;
            if (rangDe[G] > rangDe[B]) {               // …mais il est rempli AVANT
                violations++;
                printf("  ❌ %s (rang %d) DOIT précéder %s (rang %d) — sans %s, plus aucune\n"
                       "     caisse n'atteint %s.\n",
                       qPrintable(xy(G)), rangDe[G], qPrintable(xy(B)), rangDe[B],
                       qPrintable(xy(B)), qPrintable(xy(G)));
            }
        }
    }
    printf("  → %d arêtes de précédence, %d VIOLÉES par l'ordre calculé", aretes, violations);
    if (inatteignables) printf(", %d buts inatteignables", inatteignables);
    printf("\n");

    // --- PRÉCÉDENCE PAR PAIRES : deux buts occupés SIMULTANÉMENT -----------------
    //
    // POURQUOI. Sur le 13, la précédence simple ne rend AUCUNE arête vers (14,6), et
    // le niveau se mure quand même : ce but est enclavé entre les murs (13,6)/(15,6),
    // donc ses seules approches sont verticales — appui en (14,4) par le haut, appui
    // en (14,8) par le bas. Aucun but SEUL ne ferme les deux routes ; la PAIRE, oui.
    // La précédence simple est structurellement aveugle à ce motif.
    //
    // LA RÈGLE, exactement celle du §6.2 avec deux obstacles au lieu d'un :
    //     {B1,B2} affame G si, B1 ET B2 traités comme occupés, plus aucune caisse
    //     n'atteint G.
    //
    // Même relaxation OPTIMISTE, donc même statut : une arête est une PREUVE
    // d'ordonnancement, un « rien trouvé » ne promet rien. O(buts³ × plateau) —
    // 1 920 BFS sur le 13, ~16 000 sur le 10 (32 buts) : statique, calculable au
    // chargement comme casesMortes.
    //
    // ⚠️ NE SONT COMPTÉES QUE LES PAIRES NON SUBSUMÉES : si B1 seul affame déjà G,
    // la paire n'apporte aucune information neuve et gonflerait le compte pour rien.
    //
    // ⚠️⚠️ CE N'EST PAS (ENCORE) UNE PREUVE DE DEADLOCK. L'arête dit « si B1 et B2
    // RESTENT occupés, G est perdu ». Or une caisse peut ressortir d'un but — le
    // projet y tient explicitement (§4 : « le découpage une-caisse-à-la-fois est
    // incomplet, il interdit le parking temporaire et le ressortir-d'un-but »). Pour
    // en faire un élagage il faudrait prouver B1 et B2 IMMOBILES, et le point fixe de
    // gel s'effondre justement sur ce plateau. Donc : DIAGNOSTIC ici, rien dans le
    // solveur, tant que `mort`/`fp` n'ont pas chiffré capture et faux positifs.
    printf("\nPrécédence par PAIRES (deux buts occupés simultanément) :\n");
    QVector<PrecedencePaires::Arete> pairesV, simplesV;
    int inatteignablesEtat = 0;
    const int violeesP = PrecedencePaires::violations(game, &pairesV, &simplesV, &inatteignablesEtat);
    for (const auto& a : simplesV)
        printf("  ❌ %s occupé et %s VIDE — plus aucune caisse n'atteint %s (bloqueur SIMPLE)\n",
               qPrintable(xy(a.B1)), qPrintable(xy(a.G)), qPrintable(xy(a.G)));
    for (const auto& a : pairesV)
        printf("  ❌ {%s,%s} occupés et %s VIDE — plus aucune caisse n'atteint %s\n",
               qPrintable(xy(a.B1)), qPrintable(xy(a.B2)), qPrintable(xy(a.G)), qPrintable(xy(a.G)));
    printf("  → %d arête-paire(s) violée(s), %d simple(s)", violeesP, simplesV.size());
    if (inatteignablesEtat) printf(", %d but(s) déjà inatteignable(s) sans obstacle", inatteignablesEtat);
    printf("\n");
    if (violeesP || simplesV.size())
        printf("  ⚠️  INDICE SEULEMENT, PAS UN VERDICT — ce test a des FAUX POSITIFS\n"
               "     PROUVÉS (fp -3, 2026-07-30) : 1 à 14 détections sur les chemins\n"
               "     GAGNANTS de 9 niveaux résolus sur 10. Une violation ne prouve donc\n"
               "     RIEN sur la solubilité ; elle dit seulement que si ces caisses ne\n"
               "     ressortent jamais de leur but, le but visé est perdu.\n");

    printf("\n");
    if (premierMurage >= 0) {
        const int G = game.getCaseBut(ordre[premierMurage]);
        printf("❌ ORDRE MURÉ au rang %d, sur le but %s.\n"
               "   Toutes ses approches passent par des buts remplis AVANT lui : la précédence\n"
               "   du §6.2 est VIOLÉE, et ce n'est donc pas la limite « trajet de tirage » —\n"
               "   c'est un défaut de `ordreParPrecedence` (game.cpp), corrigeable.\n",
               premierMurage, qPrintable(xy(G)));
    } else {
        printf("✅ Aucun murage détecté par la précédence LOCALE.\n"
               "   ⚠️ Ça ne veut PAS dire que l'ordre est jouable : ce test ignore\n"
               "   l'ACHEMINEMENT (une caisse peut-elle atteindre la case d'approche ?).\n"
               "   Si la macro se mure quand même, c'est la limite connue du §6.2 — le\n"
               "   rebours ne simule pas le trajet de tirage — et il faut la traiter là.\n");
    }
    return 0;
}
