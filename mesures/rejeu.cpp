// rejeu — REJOUE UNE PARTIE HUMAINE ET INTERROGE LE TEST DE DÉFAITE.
//
// Protocole du juge FP (§1) : sur une partie GAGNÉE, tous les états traversés sont
// solubles PAR CONSTRUCTION. Toute détection de défaite y est donc un FAUX POSITIF
// PROUVÉ — pas une opinion, une contradiction.
//
//   rejeu <niveau.xsb> <hybride_niveau_XXXX.txt>
//
// ✅ ÉTENDU AUX MACROS le 2026-08-23 (plan.md §6.0, « OÙ REPRENDRE », point 1). Le
// journal hybride ne confrontait le solveur qu'aux poussées choisies À LA MAIN : sur
// la partie du 200, 133 coups sur 140 échappaient au juge. À chaque `[macro] LANCEE`
// du journal, cet outil interroge maintenant `jugeMacro` (jugemacro.h, exemplaire
// unique partagé avec l'UI) sur l'état d'AVANT, et classe la macro humaine en
// HORS PASSE COUPLAGE / ECARTE / rang. Le dépouillement porte donc sur les 28 parties
// gagnées DÉJÀ en banque, sans en rejouer une seule à la main.
//
// Lit la DERNIÈRE partie du journal (les `[mouv] joueur (x,y)->(x,y)`), la rejoue
// coup par coup, et signale tout état où `isPerdu()` s'arme.
//
// ⚠️ Il ne suffit pas que la partie soit dans l'espace GÉNÉRÉ par le solveur : le
// journal hybride le vérifie déjà pour les poussées choisies à la main (`⚠ ECARTE`),
// mais il ne dit rien des états traversés PENDANT une macro. C'est précisément ce
// trou que cet outil comble.
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <cstdio>
#include "game.h"
#include "jugemacro.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 3) { fprintf(stderr, "usage: rejeu <niveau.xsb> <journal.txt>\n"); return 2; }

    Level level; level.load(argv[1]);
    if (!level.isLoaded()) { fprintf(stderr, "rejeu: niveau introuvable\n"); return 2; }
    Game g(level, 0);

    QFile f(argv[2]);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { fprintf(stderr, "rejeu: journal introuvable\n"); return 2; }
    QStringList lignes;
    { QTextStream in(&f); while (!in.atEnd()) lignes << in.readLine(); }

    // La DERNIÈRE partie seulement : un journal s'écrit en AJOUT, et les parties
    // précédentes sont souvent abandonnées.
    int debut = 0;
    for (int i = 0; i < lignes.size(); i++)
        if (lignes[i].startsWith("=== niveau")) debut = i;

    // L'ORDRE SOUS LEQUEL LA PARTIE A ÉTÉ JOUÉE, relu dans l'en-tête plutôt que
    // supposé. Une macro vise butActif(), donc rejouer sous un AUTRE ordre rendrait
    // tous les verdicts faux — et sans bruit, puisque le rejeu des coups, lui,
    // marcherait très bien. C'est le piège du §7 (« charger une position recalcule
    // tout le statique ») appliqué au journal : l'UI écrit exprès la SOURCE de son
    // ordre, il suffit de la lire.
    for (int i = debut; i < lignes.size(); i++) {
        if (!lignes[i].startsWith("[hybride] ordre de remplissage")) continue;
        if (lignes[i].contains("PAR ALIGNEMENT")) {
            printf("[rejeu] partie jouee sous l'ORDRE PAR ALIGNEMENT (regime loi) — arme.\n");
            g.setOrdreAlignement(true);
        } else if (lignes[i].contains("⚠ INJECTE")) {
            // Rien à armer : Game lit `ordre_niveau_XXXX.txt` tout seul au
            // chargement. Mais si le fichier n'est plus là, le desync se verra —
            // d'où l'avertissement plutôt qu'un silence.
            printf("[rejeu] ⚠ partie jouee sous un ordre INJECTE PAR FICHIER — verifier que "
                   "le meme fichier est present, sinon les verdicts de macro seront faux.\n");
        }
        break;   // la ligne de la DERNIÈRE partie, pas celles des précédentes
    }

    const QRegularExpression re("^\\[mouv\\] joueur \\((\\d+),(\\d+)\\)->\\((\\d+),(\\d+)\\)");
    // `[macro] LANCEE caisse (x,y) -> but (bx,by) : N poussees, ...`
    const QRegularExpression reMacro(
        "^\\[macro\\] LANCEE caisse \\((\\d+),(\\d+)\\) -> but \\((\\d+),(\\d+)\\)");
    int coup = 0, fauxPositifs = 0, poussees = 0, desordre = 0;
    int macros = 0, horsPasse = 0, ecartees = 0, introuvables = 0, desyncBut = 0;
    int rang1 = 0, sommeRang = 0, pireRang = 0, dfNul = 0;
    for (int i = debut; i < lignes.size(); i++) {
        // LE JUGE DES MACROS, sur l'état d'AVANT — la ligne LANCEE précède les
        // [mouv] qu'elle produit, donc le plateau courant est bien celui d'où la
        // macro part. Aucun coup n'est joué ici : on interroge et on continue.
        const QRegularExpressionMatch mm = reMacro.match(lignes[i]);
        if (mm.hasMatch()) {
            macros++;
            const int cx = mm.captured(1).toInt(), cy = mm.captured(2).toInt();
            const int bx = mm.captured(3).toInt(), by = mm.captured(4).toInt();
            const int idxCaisse = cx + cy * g.getLargeur();
            const int but = g.butActif();
            // ⚠️ GARDE-FOU D'ORDRE, sans lequel tous les verdicts seraient faux en
            // silence. Une macro vise TOUJOURS butActif(), donc si le but que le
            // journal a écrit n'est pas celui qu'on recalcule ici, c'est que la
            // partie a été jouée sous un AUTRE ordre de remplissage (fichier
            // `ordre_niveau_XXXX.txt` présent alors et pas maintenant, ou
            // l'inverse — §7). On le dit, on ne juge pas.
            const int caseBut = but >= 0 ? g.getCaseBut(but) : -1;
            if (caseBut != bx + by * g.getLargeur()) {
                if (!desyncBut)
                    printf("⚠ DESYNC D'ORDRE au coup %d : le journal vise le but (%d,%d), "
                           "butActif() rend (%d,%d) — partie jouee sous un autre ordre "
                           "de remplissage, verdicts de macro NON FIABLES\n",
                           coup, bx, by, caseBut % g.getLargeur(), caseBut / g.getLargeur());
                desyncBut++;
                continue;
            }
            const VerdictMacro v = jugeMacro(g, idxCaisse, but);
            switch (v.type) {
            case VerdictMacro::HorsPasseCouplage:
                horsPasse++;
                printf("🔴 HORS PASSE COUPLAGE au coup %d (poussee %d) : macro (%d,%d)->but (%d,%d) "
                       "jouee, mais le couplage assigne (%d,%d) et sa macro aboutit — cette "
                       "branche n'existe dans AUCUN etat de l'arbre du solveur\n",
                       coup, poussees, cx, cy, bx, by,
                       v.voulue % g.getLargeur(), v.voulue / g.getLargeur());
                break;
            case VerdictMacro::Ecarte:
                ecartees++;
                printf("🔴 FAUX POSITIF PROUVE (macro) au coup %d (poussee %d) : macro (%d,%d)->but "
                       "(%d,%d) ECARTEE par %s dans une partie GAGNANTE\n",
                       coup, poussees, cx, cy, bx, by, v.cause);
                break;
            case VerdictMacro::Introuvable:
                introuvables++;
                printf("⚠ INTROUVABLE au coup %d : macro (%d,%d)->but (%d,%d) jouee dans la partie "
                       "et absente des %d enfilees — MIROIR EN DEFAUT\n",
                       coup, cx, cy, bx, by, v.nbEnfilees);
                break;
            case VerdictMacro::Retenue:
                sommeRang += v.rang;
                if (v.rang == 1) rang1++;
                if (v.rang > pireRang) pireRang = v.rang;
                if (v.df == 0) dfNul++;
                break;
            }
            continue;
        }

        const QRegularExpressionMatch m = re.match(lignes[i]);
        if (!m.hasMatch()) continue;
        const int ax = m.captured(1).toInt(), ay = m.captured(2).toInt();
        const int bx = m.captured(3).toInt(), by = m.captured(4).toInt();
        const QPoint p = g.getPlayerPoint();
        if (p.x() != ax || p.y() != ay) {
            printf("⚠ DESYNC au coup %d : le journal dit (%d,%d), le rejeu est en (%d,%d)\n",
                   coup, ax, ay, p.x(), p.y());
            return 1;
        }
        Game::EDirection d;
        if (bx == ax + 1)      d = Game::dDroite;
        else if (bx == ax - 1) d = Game::dGauche;
        else if (by == ay + 1) d = Game::dBas;
        else                   d = Game::dHaut;
        const int avant = g.getNbDepCaisse();
        if (!g.deplace(d)) { printf("⚠ coup %d REFUSE par le moteur\n", coup); return 1; }
        coup++;
        if (g.getNbDepCaisse() > avant) poussees++;
        // LA COUPE D'ORDRE (§10.5) : « vrai si les buts sont remplis dans l'ordre de
        // PROFONDEUR ». Elle n'est pas un théorème de géométrie mais une exigence
        // d'ordre — donc sur une partie GAGNÉE, un `false` est une contradiction de
        // la même espèce qu'un faux positif de deadlock : la coupe refuserait un
        // chemin qui gagne.
        if (!g.remplissageOrdonne()) {
            if (!desordre) printf("🔴 COUPE D'ORDRE VIOLEE au coup %d (poussee %d) : "
                                  "%d/%d posees — ce chemin GAGNANT serait coupe\n",
                                  coup, poussees, g.nbCaissesSurBut(), g.getNbButs());
            desordre++;
        }
        if (g.isPerdu()) {
            printf("🔴 FAUX POSITIF PROUVE au coup %d (poussee %d) : joueur (%d,%d), "
                   "%d/%d posees — etat declare PERDU dans une partie GAGNANTE\n",
                   coup, poussees, bx, by, g.nbCaissesSurBut(), g.getNbButs());
            fauxPositifs++;
        }
    }
    printf("%d coups rejoues, %d poussees, %d/%d posees a la fin — %s\n",
           coup, poussees, g.nbCaissesSurBut(), g.getNbButs(),
           g.isGagne() ? "PARTIE GAGNEE" : "⚠ PAS GAGNEE (journal incomplet ?)");
    printf("faux positifs de defaite : %d\n", fauxPositifs);
    printf("etats en violation de la coupe d'ordre : %d\n", desordre);

    // LE BILAN DES MACROS. 'retenues' est ce que le solveur produirait vraiment ;
    // 'hors passe' est la mesure du desaccord de GENERATION, celle qu'on cherche.
    const int retenues = macros - horsPasse - ecartees - introuvables - desyncBut;
    printf("\nmacros jugees : %d\n", macros);
    if (desyncBut)
        printf("  ⚠ %d NON JUGEES (desync d'ordre) — le reste de ce bilan est partiel\n", desyncBut);
    printf("  HORS PASSE COUPLAGE (jamais generees) : %d (%.1f %%)\n",
           horsPasse, macros ? 100.0 * horsPasse / macros : 0.0);
    printf("  ECARTEES (faux positifs prouves)      : %d\n", ecartees);
    printf("  INTROUVABLES (miroir en defaut)       : %d\n", introuvables);
    printf("  retenues                              : %d", retenues);
    if (retenues > 0)
        printf(" | rang 1 dans %d cas (%.0f %%), rang moyen %.1f, pire %d, df=0 dans %d cas",
               rang1, 100.0 * rang1 / retenues, (double)sommeRang / retenues, pireRang, dfNul);
    printf("\n");
    return 0;
}
