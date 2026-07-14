// Rejoue la solution optimale de reference (sans macro) et, a chaque poussee,
// demande a pousseMacro() ce qu'ELLE aurait fait. Si la macro pousse plus loin
// que la solution optimale, l'etat intermediaire du chemin optimal devient
// INATTEIGNABLE : le solveur ne peut plus trouver 131.
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <cstdio>
#include <vector>
#include "level.h"
#include "game.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const int num = QString(argv[1]).toInt();

    std::vector<std::pair<int,int>> chemin;   // (idxCaisse, dir)
    { QFile f(argv[2]); f.open(QIODevice::ReadOnly|QIODevice::Text); QTextStream in(&f);
      while (!in.atEnd()) { int a,b; in >> a >> b; if (in.status()==QTextStream::Ok && !in.atEnd()) chemin.push_back({a,b}); } }

    Level level; level.load(QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(num,4,10,QChar('0')));
    Game g(level, num);

    printf("solution de reference : %d poussees\n\n", (int)chemin.size());

    int poussee = 0, divergences = 0;
    for (size_t i = 0; i < chemin.size(); i++) {
        const int idxC = chemin[i].first;
        const Game::EDirection d = (Game::EDirection)chemin[i].second;

        // Combien de poussees CONSECUTIVES la solution optimale fait-elle sur cette
        // caisse dans cette direction ? (la caisse avance d'une case par poussee)
        int suite = 1, pas = 0;
        {
            const int dx[4]={0,1,0,-1}, dy[4]={-1,0,1,0};
            pas = dx[(int)d] + dy[(int)d]*g.getLargeur();
            int c = idxC;
            for (size_t j = i+1; j < chemin.size(); j++) {
                if (chemin[j].first == c + pas && chemin[j].second == (int)d) { suite++; c += pas; }
                else break;
            }
        }

        // Ce que la MACRO ferait depuis cet etat.
        Game essai(g);
        const int k = essai.pousseMacro(idxC, d);

        if (k > suite) {
            divergences++;
            if (divergences == 1) {
                printf("DIVERGENCE a la poussee %d (case %d = (%d,%d), dir %d)\n",
                       poussee+1, idxC, idxC % g.getLargeur(), idxC / g.getLargeur(), (int)d);
                printf("   solution optimale : %d poussee(s)   la MACRO : %d\n\n", suite, k);

                // Etat AVANT la poussee, avec la caisse concernee en 'C'.
                printf("   AVANT (C = la caisse poussee, o = case marquee COULOIR sur l'axe de poussee)\n");
                for (int y = 0; y < g.getHauteur(); y++) {
                    printf("     ");
                    for (int x = 0; x < g.getLargeur(); x++) {
                        const int i = x + y * g.getLargeur();
                        char c;
                        switch (g.getCase(i)) {
                            case Level::tcMur: c='#'; break;
                            case Level::tcCaisse: c='$'; break;
                            case Level::tcGoalCaisse: c='*'; break;
                            case Level::tcGoal: c='.'; break;
                            case Level::tcPlayer: c='@'; break;
                            case Level::tcGoalPlayer: c='+'; break;
                            default: c=' '; break;
                        }
                        if (i == idxC) c = 'C';
                        printf("%c", c);
                    }
                    printf("\n");
                }

                Game apres(g);
                apres.pousseMacro(idxC, d);
                printf("\n   APRES LA MACRO (elle a pousse %d fois)\n", k);
                for (int y = 0; y < g.getHauteur(); y++) {
                    printf("     ");
                    for (int x = 0; x < g.getLargeur(); x++) {
                        const int i = x + y * g.getLargeur();
                        char c;
                        switch (apres.getCase(i)) {
                            case Level::tcMur: c='#'; break;
                            case Level::tcCaisse: c='$'; break;
                            case Level::tcGoalCaisse: c='*'; break;
                            case Level::tcGoal: c='.'; break;
                            case Level::tcPlayer: c='@'; break;
                            case Level::tcGoalPlayer: c='+'; break;
                            default: c=' '; break;
                        }
                        printf("%c", c);
                    }
                    printf("\n");
                }

                Game bon(g);
                bon.pousse(idxC, d);
                printf("\n   CE QUE FAIT LA SOLUTION OPTIMALE (1 poussee)\n");
                for (int y = 0; y < g.getHauteur(); y++) {
                    printf("     ");
                    for (int x = 0; x < g.getLargeur(); x++) {
                        const int i = x + y * g.getLargeur();
                        char c;
                        switch (bon.getCase(i)) {
                            case Level::tcMur: c='#'; break;
                            case Level::tcCaisse: c='$'; break;
                            case Level::tcGoalCaisse: c='*'; break;
                            case Level::tcGoal: c='.'; break;
                            case Level::tcPlayer: c='@'; break;
                            case Level::tcGoalPlayer: c='+'; break;
                            default: c=' '; break;
                        }
                        printf("%c", c);
                    }
                    printf("\n");
                }
                printf("\n");
            }
        }

        // Avancer d'UNE poussee sur le chemin de reference.
        g.pousse(idxC, d);
        poussee++;
    }

    printf("=> %d divergence(s) : autant d'endroits ou le chemin optimal devient inatteignable.\n", divergences);
    return 0;
}
