#include <QtTest>
#include <QTemporaryFile>
#include <QTextStream>
#include <QDir>
#include <QSet>
#include <QVector>
#include <QQueue>

#include "game.h"
#include "level.h"

// ---------------------------------------------------------------------------
// Outils de test : représentation canonique d'un niveau + fabrique de Game.
//
// On ne teste pas via l'API de jeu (qui ne permet pas de placer joueur/caisses
// arbitrairement) : on re-génère une grille .xsb temporaire pour chaque variante
// puis on la charge comme un vrai niveau. getEtat() ne dépend que de la grille.
// ---------------------------------------------------------------------------

namespace {

// Carte canonique : positions (en index x + y*largeur) des différents éléments.
struct LevelMap {
    int largeur = 0;
    int hauteur = 0;
    QSet<int> murs;
    QSet<int> goals;
    QSet<int> caisses;   // configuration initiale
    int joueur = -1;
};

// Découpe un .xsb en lignes, en les complétant à la largeur max (comme Level::load).
QStringList lireLignes(const QString& chemin) {
    QFile f(chemin);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QTextStream in(&f);
    QStringList lignes;
    int largeur = 0;
    while (!in.atEnd()) {
        QString ligne = in.readLine();
        largeur = qMax(largeur, ligne.size());
        lignes.append(ligne);
    }
    for (QString& l : lignes)
        l = l.leftJustified(largeur, ' ');
    return lignes;
}

LevelMap parser(const QStringList& lignes) {
    LevelMap m;
    m.hauteur = lignes.size();
    m.largeur = lignes.isEmpty() ? 0 : lignes.first().size();
    for (int y = 0; y < m.hauteur; ++y) {
        for (int x = 0; x < m.largeur; ++x) {
            const int idx = x + y * m.largeur;
            const QChar c = lignes[y][x];
            switch (c.toLatin1()) {
            case '#': m.murs.insert(idx); break;
            case '.': m.goals.insert(idx); break;
            case '$': m.caisses.insert(idx); break;
            case '*': m.caisses.insert(idx); m.goals.insert(idx); break;
            case '@': m.joueur = idx; break;
            case '+': m.joueur = idx; m.goals.insert(idx); break;
            default: break; // ' ' : sol
            }
        }
    }
    return m;
}

// Génère la grille .xsb correspondant à (joueur, caisses) sur une carte donnée.
QStringList rendre(const LevelMap& m, int joueur, const QSet<int>& caisses) {
    QStringList lignes;
    for (int y = 0; y < m.hauteur; ++y) {
        QString ligne;
        for (int x = 0; x < m.largeur; ++x) {
            const int idx = x + y * m.largeur;
            const bool goal = m.goals.contains(idx);
            char c;
            if (m.murs.contains(idx))          c = '#';
            else if (caisses.contains(idx))    c = goal ? '*' : '$';
            else if (idx == joueur)            c = goal ? '+' : '@';
            else if (goal)                     c = '.';
            else                               c = ' ';
            ligne.append(QChar(c));
        }
        lignes.append(ligne);
    }
    return lignes;
}

// Construit un Game à partir d'une grille, via un .xsb temporaire.
// (Le fichier est supprimé à la sortie ; Level::load l'a déjà copié en mémoire.)
Game makeGame(const QStringList& lignes) {
    QTemporaryFile f(QDir::tempPath() + "/qtiasoko_XXXXXX.xsb");
    f.open();
    {
        QTextStream out(&f);
        for (const QString& l : lignes)
            out << l << '\n';
    }
    f.flush();
    Level lvl;
    lvl.load(f.fileName());
    return Game(lvl);
}

// Raccourci : état normalisé d'une grille.
QByteArray etatDe(const QStringList& lignes) {
    return makeGame(lignes).getEtat();
}

// Composantes connexes des cases libres (murs ET caisses = obstacles),
// voisinage 4-directions sans wrap horizontal. comp[idx] = -1 si bloqué.
QVector<int> composantes(const LevelMap& m, const QSet<int>& caisses, int& nb) {
    QVector<int> comp(m.largeur * m.hauteur, -1);
    nb = 0;
    for (int depart = 0; depart < comp.size(); ++depart) {
        if (comp[depart] != -1) continue;
        if (m.murs.contains(depart) || caisses.contains(depart)) continue;

        QQueue<int> file;
        file.enqueue(depart);
        comp[depart] = nb;
        while (!file.isEmpty()) {
            const int idx = file.dequeue();
            const int x = idx % m.largeur;
            const int y = idx / m.largeur;
            static const int voisins[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
            for (const auto& d : voisins) {
                const int nx = x + d[0];
                const int ny = y + d[1];
                if (nx < 0 || nx >= m.largeur || ny < 0 || ny >= m.hauteur) continue;
                const int nidx = nx + ny * m.largeur;
                if (comp[nidx] != -1) continue;
                if (m.murs.contains(nidx) || caisses.contains(nidx)) continue;
                comp[nidx] = nb;
                file.enqueue(nidx);
            }
        }
        ++nb;
    }
    return comp;
}

QString cheminNiveau(int n) {
    return QString("%1/level%2.xsb").arg(LEVELS_DIR).arg(n, 4, 10, QChar('0'));
}

const int NB_NIVEAUX = 32;

} // namespace

// ---------------------------------------------------------------------------

class TestGetEtat : public QObject {
    Q_OBJECT

private slots:
    // Sanity : la grille re-générée depuis la carte canonique produit le même
    // état que le chargement direct du .xsb, et getEtat() est déterministe.
    void renduFidele_data();
    void renduFidele();

    // §1.3 — Deux positions du joueur dans la MÊME zone → même état normalisé.
    void memeZone_data();
    void memeZone();

    // §1.3 — Deux positions dans des zones DIFFÉRENTES → états différents.
    void zonesDifferentes();

    // §1.3 — Déplacer une caisse → nouvel état (même si le coup est inutile).
    void caisseDeplacee_data();
    void caisseDeplacee();

    // §2 — Corner deadlocks : caisse hors goal coincée par 2 murs perpendiculaires.
    //       checkDefaite() est privé → appel direct via l'amitié (friend).
    void cornerDeadlock_data();
    void cornerDeadlock();

    // §3 — Adjacent deadlocks : deux caisses côte à côte figées sur l'axe
    //       perpendiculaire (mur/caisse des deux côtés).
    void adjacentDeadlock_data();
    void adjacentDeadlock();

    // §3bis — Test de gel (freeze deadlock), appelé DIRECTEMENT sur caisseGelee()
    //         plutôt que via checkDefaite() : sinon casesMortes pourrait faire
    //         passer un test pour la mauvaise raison, et on ne testerait rien.
    void gel_data();
    void gel();

    // §3bis — Le même, intégré : checkDefaite() doit conclure à la défaite (ou
    //         non) sur ces plateaux.
    void gelDeadlock_data();
    void gelDeadlock();

    // §3bis — L'invariant le plus fort : l'état INITIAL d'un niveau soluble ne
    //         peut contenir aucune caisse gelée hors but. Une seule suffirait à
    //         rendre le niveau insoluble d'entrée. Balaie les 32 vrais niveaux —
    //         un faux positif de caisseGelee() n'a nulle part où se cacher.
    void gelEtatInitial_data();
    void gelEtatInitial();

    // §3bis — La règle 2 (« les deux voisins de l'axe sont des cases mortes »)
    //         n'était couverte par AUCUN test. On l'isole : la grille est bâtie
    //         pour que ce soit elle, et elle seule, qui décide.
    void gelCasesMortes_data();
    void gelCasesMortes();

    // §3bis — caisseGelee() doit être une FONCTION de l'état : même résultat quel
    //         que soit l'ordre des appels, et deux appels de suite concordent. La
    //         garde de récursion est un tampon partagé — si elle fuit, l'ordre de
    //         balayage de checkDefaite() changerait le verdict.
    void gelPurete_data();
    void gelPurete();

    // §3bis — Cohérence de groupe : dans un bloc gelé, TOUTES les caisses le sont.
    //         Le gel est une propriété du groupe, pas de la caisse interrogée.
    void gelGroupe_data();
    void gelGroupe();

    // §3bis — « Gelée » veut dire POUR TOUJOURS : aucune poussée légale d'une
    //         AUTRE caisse ne peut dégeler X. Si une le pouvait, le gel aurait
    //         confondu « bloquée maintenant » et « bloquée pour toujours » — la
    //         distinction même sur laquelle repose tout le §3bis.
    void gelStable_data();
    void gelStable();

    // §10.5 — Le motif des niveaux 8/9 : un état qui n'est PAS gelé, mais dont
    //         TOUTE poussée mène à un gel. checkDefaite() ne le voit pas — il
    //         teste un seul état. Ce test CARACTÉRISE le trou : il documente
    //         l'état actuel (non détecté) et prouve le lookahead d'un coup.
    void gelLookahead();

    // getCaissesDeplacable() : direction poussable = case d'arrivée libre +
    // point de poussée atteignable par le joueur sans traverser de caisse.
    void caissesDeplacables_data();
    void caissesDeplacables();

    // Cas limite : le joueur est déjà sur le point de poussée. AStar::getChemin
    // renvoie un chemin vide dans ce cas (start == goal) ; la direction doit
    // quand même être marquée poussable.
    void caissePousseeDejaEnPosition();
};

void TestGetEtat::renduFidele_data() {
    QTest::addColumn<int>("niveau");
    for (int n = 1; n <= NB_NIVEAUX; ++n)
        QTest::newRow(qPrintable(QString("niveau %1").arg(n))) << n;
}

void TestGetEtat::renduFidele() {
    QFETCH(int, niveau);

    const QStringList lignes = lireLignes(cheminNiveau(niveau));
    QVERIFY2(!lignes.isEmpty(), "fichier .xsb introuvable ou vide");

    const LevelMap m = parser(lignes);
    QVERIFY2(m.joueur >= 0, "aucun joueur dans le niveau");

    // Chargement direct du vrai fichier.
    Level lvl;
    lvl.load(cheminNiveau(niveau));
    const QByteArray etatFichier = Game(lvl).getEtat();

    // Grille re-générée depuis la carte canonique.
    const QByteArray etatRendu = etatDe(rendre(m, m.joueur, m.caisses));

    QCOMPARE(etatRendu, etatFichier);
    // Déterminisme.
    QCOMPARE(etatDe(rendre(m, m.joueur, m.caisses)), etatFichier);
}

void TestGetEtat::memeZone_data() {
    QTest::addColumn<int>("niveau");
    for (int n = 1; n <= NB_NIVEAUX; ++n)
        QTest::newRow(qPrintable(QString("niveau %1").arg(n))) << n;
}

void TestGetEtat::memeZone() {
    QFETCH(int, niveau);

    const LevelMap m = parser(lireLignes(cheminNiveau(niveau)));
    QVERIFY(m.joueur >= 0);

    int nbComp = 0;
    const QVector<int> comp = composantes(m, m.caisses, nbComp);
    const int zoneJoueur = comp[m.joueur];
    QVERIFY(zoneJoueur >= 0);

    // État de référence : joueur à sa position initiale.
    const QByteArray ref = etatDe(rendre(m, m.joueur, m.caisses));

    int testees = 0;
    for (int idx = 0; idx < comp.size(); ++idx) {
        if (idx == m.joueur || comp[idx] != zoneJoueur) continue;
        // Même zone, mêmes caisses → l'état DOIT être identique.
        const QByteArray variante = etatDe(rendre(m, idx, m.caisses));
        QVERIFY2(variante == ref,
                 qPrintable(QString("niveau %1 : joueur en %2 (zone %3) devrait "
                                    "donner le meme etat que la position initiale")
                            .arg(niveau).arg(idx).arg(zoneJoueur)));
        ++testees;
    }
    // Une zone réduite à une seule case (joueur coincé par murs/caisses) rend
    // l'invariance triviale : rien à comparer, on marque le cas comme ignoré.
    if (testees == 0)
        QSKIP("zone du joueur reduite a une seule case (invariance triviale)");
}

void TestGetEtat::zonesDifferentes() {
    // 1) Niveau synthétique : deux salles séparées par une colonne de mur dont
    //    l'unique ouverture est bouchée par une caisse → deux zones disjointes.
    //    Même jeu de caisses, joueur à gauche vs à droite → états différents.
    const QStringList salles = {
        "#######",
        "#  #  #",
        "#  $  #",   // colonne 3 : mur / caisse / mur → cloison étanche
        "#  #  #",
        "#######",
    };
    const LevelMap ms = parser(salles);

    int nb = 0;
    const QVector<int> comp = composantes(ms, ms.caisses, nb);
    QCOMPARE(nb, 2); // deux zones bien distinctes

    const int gauche = 1 + 1 * ms.largeur; // (1,1)
    const int droite = 5 + 1 * ms.largeur; // (5,1)
    QVERIFY(comp[gauche] != comp[droite]);

    const QByteArray etatGauche = etatDe(rendre(ms, gauche, ms.caisses));
    const QByteArray etatDroite = etatDe(rendre(ms, droite, ms.caisses));
    QVERIFY2(etatGauche != etatDroite,
             "joueur dans deux zones distinctes : les etats doivent differer");

    // 2) Niveaux réels : si des cases libres ne sont PAS dans la zone du joueur
    //    (poche inaccessible), y placer le joueur doit donner un autre état.
    for (int n = 1; n <= NB_NIVEAUX; ++n) {
        const LevelMap m = parser(lireLignes(cheminNiveau(n)));
        int nbComp = 0;
        const QVector<int> c = composantes(m, m.caisses, nbComp);
        if (nbComp < 2) continue;

        const int zoneJoueur = c[m.joueur];
        const QByteArray ref = etatDe(rendre(m, m.joueur, m.caisses));
        for (int idx = 0; idx < c.size(); ++idx) {
            if (c[idx] < 0 || c[idx] == zoneJoueur) continue;
            const QByteArray autre = etatDe(rendre(m, idx, m.caisses));
            QVERIFY2(autre != ref,
                     qPrintable(QString("niveau %1 : zone %2 != zone joueur %3, "
                                        "etats devraient differer")
                                .arg(n).arg(c[idx]).arg(zoneJoueur)));
            break; // une poche suffit pour ce niveau
        }
    }
}

void TestGetEtat::caisseDeplacee_data() {
    QTest::addColumn<int>("niveau");
    for (int n = 1; n <= NB_NIVEAUX; ++n)
        QTest::newRow(qPrintable(QString("niveau %1").arg(n))) << n;
}

void TestGetEtat::caisseDeplacee() {
    QFETCH(int, niveau);

    const LevelMap m = parser(lireLignes(cheminNiveau(niveau)));
    QVERIFY(!m.caisses.isEmpty());

    const QByteArray ref = etatDe(rendre(m, m.joueur, m.caisses));

    // Trouve une caisse et une case voisine libre où la déplacer.
    static const int voisins[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    bool trouve = false;
    for (int caisse : m.caisses) {
        const int x = caisse % m.largeur;
        const int y = caisse / m.largeur;
        for (const auto& d : voisins) {
            const int nx = x + d[0];
            const int ny = y + d[1];
            if (nx < 0 || nx >= m.largeur || ny < 0 || ny >= m.hauteur) continue;
            const int cible = nx + ny * m.largeur;
            const bool libre = !m.murs.contains(cible)
                            && !m.caisses.contains(cible)
                            && cible != m.joueur;
            if (!libre) continue;

            QSet<int> caisses = m.caisses;
            caisses.remove(caisse);
            caisses.insert(cible);
            const QByteArray variante = etatDe(rendre(m, m.joueur, caisses));
            QVERIFY2(variante != ref,
                     qPrintable(QString("niveau %1 : caisse %2 -> %3 doit changer l'etat")
                                .arg(niveau).arg(caisse).arg(cible)));
            trouve = true;
            break;
        }
        if (trouve) break;
    }
    QVERIFY2(trouve, "aucune caisse deplacable trouvee");
}

void TestGetEtat::cornerDeadlock_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<bool>("perduAttendu");

    // --- Caisse (hors goal) coincée dans un coin → défaite. Les 4 orientations.
    QTest::newRow("coin haut-gauche") << QString("####\n#$ #\n# @#\n####") << true;
    QTest::newRow("coin haut-droite") << QString("####\n# $#\n#@ #\n####") << true;
    QTest::newRow("coin bas-gauche")  << QString("####\n#@ #\n#$ #\n####") << true;
    QTest::newRow("coin bas-droite")  << QString("####\n# @#\n# $#\n####") << true;

    // Une caisse saine ne masque pas une caisse coincée ailleurs sur le plateau.
    QTest::newRow("deadlock parmi plusieurs caisses")
        << QString("######\n# $  #\n#    #\n#  @ #\n#   $#\n######") << true;

    // --- Pas de défaite.
    // Caisse dans un coin mais SUR un goal (tcGoalCaisse, exclu du test).
    QTest::newRow("coin mais sur goal")
        << QString("####\n#* #\n# @#\n####") << false;
    // Caisse contre un seul mur : encore poussable, pas de coin. Un but est
    // nécessaire sur la même colonne : une caisse collée à un mur ne peut
    // plus s'en éloigner (le joueur ne peut jamais se placer de l'autre côté),
    // donc sans but atteignable le long de ce mur, checkDefaite() la
    // considère à raison comme morte via casesMortes.
    QTest::newRow("un seul mur")
        << QString("######\n#    #\n#$ @ #\n#.   #\n######") << false;
    // Caisse au centre, aucun mur adjacent. But atteignable ajouté pour que
    // casesMortes ne la marque pas morte (aucun but = tout est mort).
    QTest::newRow("au centre")
        << QString("#####\n# . #\n# $ #\n# @ #\n#####") << false;
}

void TestGetEtat::cornerDeadlock() {
    QFETCH(QString, grille);
    QFETCH(bool, perduAttendu);

    Game g = makeGame(grille.split('\n'));
    QVERIFY2(!g.isPerdu(), "un jeu neuf ne doit pas etre perdu avant evaluation");

    g.checkDefaite(); // membre privé, accessible via friend class TestGetEtat

    QCOMPARE(g.isPerdu(), perduAttendu);
}

void TestGetEtat::adjacentDeadlock_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<bool>("perduAttendu");

    // --- Deadlocks. Caisses placées hors des coins → c'est bien la logique
    //     adjacente (et non le corner) qui déclenche la défaite.

    // Paire horizontale, mur au-dessus des deux caisses.
    QTest::newRow("horizontale, mur au-dessus")
        << QString("######\n# ## #\n# $$ #\n#  @ #\n######") << true;
    // Paire horizontale, mur en dessous des deux caisses.
    QTest::newRow("horizontale, mur en dessous")
        << QString("######\n#  @ #\n# $$ #\n# ## #\n######") << true;
    // Paire verticale, mur à gauche des deux caisses.
    QTest::newRow("verticale, mur a gauche")
        << QString("######\n#    #\n##$  #\n##$@ #\n#    #\n######") << true;
    // Bloc 2x2 : chaque paire est bloquée par les caisses voisines.
    QTest::newRow("bloc 2x2")
        << QString("######\n#    #\n# $$ #\n# $$ #\n#  @ #\n######") << true;
    // Caisse (hors goal) + caisse sur goal, mur au-dessus : la caisse hors goal
    // reste figée → deadlock (la voisine sur goal compte comme partenaire).
    QTest::newRow("caisse + caisse sur goal, mur au-dessus")
        << QString("######\n# ## #\n# $* #\n#  @ #\n######") << true;

    // --- Pas de deadlock.

    // Paire horizontale en espace ouvert : chaque caisse est poussable verticalement.
    // But ajouté au-dessus de la caisse de gauche pour que casesMortes ne
    // marque mort ni l'une ni l'autre (sans but, tout le plateau est mort).
    QTest::newRow("horizontale espace ouvert")
        << QString("######\n# .  #\n# $$ #\n#  @ #\n######") << false;
    // Paire verticale en espace ouvert : poussable horizontalement. Même
    // raison pour le but au-dessus de la caisse du haut.
    QTest::newRow("verticale espace ouvert")
        << QString("######\n# .  #\n# $  #\n# $  #\n#  @ #\n######") << false;
    // Deux caisses sur goals adossées à un mur : déjà résolues (tcGoalCaisse,
    // exclues du test) → pas de défaite.
    QTest::newRow("paire sur goals adossee au mur")
        << QString("######\n# ## #\n# ** #\n#  @ #\n######") << false;
}

void TestGetEtat::adjacentDeadlock() {
    QFETCH(QString, grille);
    QFETCH(bool, perduAttendu);

    Game g = makeGame(grille.split('\n'));
    QVERIFY2(!g.isPerdu(), "un jeu neuf ne doit pas etre perdu avant evaluation");

    g.checkDefaite();

    QCOMPARE(g.isPerdu(), perduAttendu);
}

// --- §3bis : test de gel ----------------------------------------------------
//
// Une caisse est GELÉE si elle est bloquée sur LES DEUX axes. Elle est bloquée
// sur un axe si l'un des deux voisins de cet axe est un mur, ou si les deux sont
// des cases mortes, ou si l'un est une caisse ELLE-MÊME GELÉE (récursion).
//
// Le point crucial : il faut les DEUX cases d'un axe libres pour s'y déplacer
// (une pour la destination, une pour que le joueur s'y tienne). Un seul côté
// bloqué suffit donc à condamner l'axe — mais condamner UN axe ne gèle pas la
// caisse, il en faut deux.

void TestGetEtat::gel_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<int>("xCaisse");
    QTest::addColumn<int>("yCaisse");
    QTest::addColumn<bool>("geleeAttendu");

    // --- GELÉES.

    // Coin : mur à gauche (axe X) + mur au-dessus (axe Y).
    QTest::newRow("coin")
        << QString("#####\n#$  #\n# . #\n# @ #\n#####") << 1 << 1 << true;

    // LE CAS QUE L'ANCIEN CODE RATAIT : paire verticale, mur à GAUCHE de la
    // caisse du haut et mur à DROITE de celle du bas. Aucune des deux ne peut
    // bouger, mais les murs sont de côtés OPPOSÉS.
    //   ##$    A en (2,2), mur à sa gauche
    //   # $#   B en (2,3), mur à sa droite
    QTest::newRow("diagonale, murs opposes (A)")
        << QString("######\n#  . #\n##$  #\n# $# #\n#  @ #\n######") << 2 << 2 << true;
    QTest::newRow("diagonale, murs opposes (B)")
        << QString("######\n#  . #\n##$  #\n# $# #\n#  @ #\n######") << 2 << 3 << true;

    // Bloc 2x2 : chaque caisse est bloquée par ses deux voisines, récursivement.
    QTest::newRow("bloc 2x2")
        << QString("######\n# .  #\n# $$ #\n# $$ #\n#  @ #\n######") << 2 << 2 << true;

    // Paire horizontale sous un mur : la voisine est gelée, donc elle bloque.
    QTest::newRow("paire sous un mur")
        << QString("######\n# ## #\n# $$ #\n#  @ #\n######") << 2 << 2 << true;

    // --- NON GELÉES. Ce sont les tests qui protègent des FAUX POSITIFS, et un
    //     faux positif est bien pire qu'une non-détection : il fait disparaître
    //     la solution sans le moindre signal.

    // COULOIR HORIZONTAL — la régression qui rendait le niveau 1 insoluble.
    // Murs au-dessus ET en dessous : deux bloqueurs, mais sur le MÊME axe. La
    // caisse glisse librement à gauche et à droite. Un décompte « >= 2 voisins
    // bloquants » la déclarerait gelée à tort.
    QTest::newRow("couloir horizontal")
        << QString("######\n# #  #\n#.$@ #\n# #  #\n######") << 2 << 2 << false;

    // Couloir vertical : symétrique.
    QTest::newRow("couloir vertical")
        << QString("#####\n# . #\n##$##\n# @ #\n#####") << 2 << 2 << false;

    // CONTRE-EXEMPLE EN S : une caisse voisine ne bloque PAS si elle n'est pas
    // elle-même gelée. C borde A à gauche, D borde B à droite — un décompte naïf
    // les croirait toutes coincées. Mais C peut monter ou descendre ; une fois
    // partie, A se pousse à droite et tout se libère. Rien n'est gelé.
    // Le motif est en ESPACE OUVERT à dessein : collé au mur gauche, C serait sur
    // une colonne sans but donc une case morte, et le test passerait pour une
    // tout autre raison que celle qu'on veut éprouver.
    //    $$     C=(2,3)  A=(3,3)
    //     $$    B=(3,4)  D=(4,4)
    QTest::newRow("contre-exemple S (A)")
        << QString("########\n#  ..  #\n#      #\n# $$   #\n#  $$  #\n#  @   #\n########") << 3 << 3 << false;
    QTest::newRow("contre-exemple S (C)")
        << QString("########\n#  ..  #\n#      #\n# $$   #\n#  $$  #\n#  @   #\n########") << 2 << 3 << false;
    QTest::newRow("contre-exemple S (D)")
        << QString("########\n#  ..  #\n#      #\n# $$   #\n#  $$  #\n#  @   #\n########") << 4 << 4 << false;

    // Un seul mur : l'axe X est condamné, mais l'axe Y reste libre. Le but doit
    // être SUR la colonne du mur : une caisse collée à un mur ne peut plus s'en
    // écarter (le joueur ne peut jamais se placer de l'autre côté), donc sans but
    // atteignable le long de ce mur elle est de toute façon sur une case morte.
    QTest::newRow("un seul mur")
        << QString("######\n#    #\n#$ @ #\n#.   #\n######") << 1 << 2 << false;

    // Espace ouvert.
    QTest::newRow("espace ouvert")
        << QString("#####\n# . #\n# $ #\n# @ #\n#####") << 2 << 2 << false;
}

void TestGetEtat::gel() {
    QFETCH(QString, grille);
    QFETCH(int, xCaisse);
    QFETCH(int, yCaisse);
    QFETCH(bool, geleeAttendu);

    Game g = makeGame(grille.split('\n'));
    const int idx = xCaisse + yCaisse * g.getLargeur();

    QVERIFY2(g.getCase(idx) == Level::tcCaisse || g.getCase(idx) == Level::tcGoalCaisse,
             "la grille de test ne place pas de caisse a la position indiquee");

    // Membres privés, accessibles via friend class TestGetEtat.
    QVector<bool> enCours(g.size, false);
    QCOMPARE(g.caisseGelee(idx, enCours), geleeAttendu);

    // La garde de récursion doit être rendue propre : si caisseGelee() laissait
    // des cases marquées, l'appel suivant verrait des murs fantômes.
    for (int i = 0; i < g.size; ++i) {
        QVERIFY2(!enCours[i], "caisseGelee() a laisse la garde de recursion marquee");
    }
}

void TestGetEtat::gelDeadlock_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<bool>("perduAttendu");

    // Le cas diagonal : checkDefaite() doit désormais conclure à la défaite.
    QTest::newRow("diagonale, murs opposes")
        << QString("######\n#  . #\n##$  #\n# $# #\n#  @ #\n######") << true;

    // Une caisse gelée SUR un but n'est pas une défaite : c'est la solution.
    QTest::newRow("coin mais sur goal")
        << QString("#####\n#*  #\n# . #\n# @ #\n#####") << false;

    // Régressions : ces plateaux ne doivent PAS être perdus.
    QTest::newRow("couloir horizontal")
        << QString("######\n# #  #\n#.$@ #\n# #  #\n######") << false;
    QTest::newRow("contre-exemple S")
        << QString("########\n#  ..  #\n#      #\n# $$   #\n#  $$  #\n#  @   #\n########") << false;
}

void TestGetEtat::gelDeadlock() {
    QFETCH(QString, grille);
    QFETCH(bool, perduAttendu);

    Game g = makeGame(grille.split('\n'));
    QVERIFY2(!g.isPerdu(), "un jeu neuf ne doit pas etre perdu avant evaluation");

    g.checkDefaite();

    QCOMPARE(g.isPerdu(), perduAttendu);
}

// --- §3bis : l'invariant de l'état initial ----------------------------------
//
// Le test le plus fort qu'on puisse écrire sur caisseGelee(), parce qu'il ne
// dépend d'aucune grille inventée : les 32 niveaux sont solubles (le solveur les
// résout, canari 4/97/131/134/110/213). Or une caisse gelée hors but est, par
// définition, un deadlock : s'il y en avait UNE dans un état de départ, le
// niveau serait insoluble avant le premier coup.
//
// Autrement dit : tout `true` rendu ici est un FAUX POSITIF, et un faux positif
// fait manquer l'optimum sans le moindre signal (§3bis, §8.5).

void TestGetEtat::gelEtatInitial_data() {
    QTest::addColumn<int>("niveau");
    for (int n = 1; n <= NB_NIVEAUX; ++n)
        QTest::newRow(qPrintable(QString("niveau %1").arg(n))) << n;
}

void TestGetEtat::gelEtatInitial() {
    QFETCH(int, niveau);

    Level lvl;
    lvl.load(cheminNiveau(niveau));
    Game g(lvl);

    QVector<bool> enCours(g.size, false);

    for (int idx = 0; idx < g.size; ++idx) {
        if (g.getCase(idx) != Level::tcCaisse) continue;   // hors but uniquement

        const bool gelee = g.caisseGelee(idx, enCours);
        QVERIFY2(!gelee,
                 qPrintable(QString("niveau %1 : la caisse (%2,%3) est declaree GELEE "
                                    "dans l'etat initial — le niveau serait insoluble "
                                    "d'entree. C'est un faux positif.")
                                .arg(niveau).arg(idx % g.getLargeur()).arg(idx / g.getLargeur())));
    }

    // Corollaire : aucun état initial n'est perdu.
    g.checkDefaite();
    QVERIFY2(!g.isPerdu(),
             qPrintable(QString("niveau %1 : checkDefaite() declare l'etat INITIAL perdu")
                            .arg(niveau)));
}

// --- §3bis : la règle 2 (cases mortes), isolée ------------------------------
//
// « Un axe est bloqué si ses DEUX voisins sont des cases mortes : y pousser mène
// à un deadlock de toute façon, l'axe est donc inutilisable. »
//
// Aucun test ne la couvrait. Le piège du §3bis s'applique en plein ici : il faut
// que la grille rende la règle 2 DÉCISIVE, sinon on croit l'éprouver alors que
// c'est un mur qui conclut. On vérifie donc casesMortes AVANT le verdict — sans
// cette assertion préalable, le test ne prouverait rien.

void TestGetEtat::gelCasesMortes_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<int>("xCaisse");
    QTest::addColumn<int>("yCaisse");
    QTest::addColumn<int>("xMortA");     // voisins de l'axe que la règle 2 doit condamner
    QTest::addColumn<int>("yMortA");
    QTest::addColumn<int>("xMortB");
    QTest::addColumn<int>("yMortB");
    QTest::addColumn<bool>("geleeAttendu");

    // GELÉE PAR LA RÈGLE 2, et par elle seule.
    // Caisse-sur-but en (1,2) : mur à gauche (0,2) et mur à droite (2,2) → axe
    // horizontal condamné par la règle 1. L'axe VERTICAL, lui, n'a aucun mur :
    // (1,1) et (1,3) sont du sol — mais ce sont deux coins, donc deux cases
    // mortes. Seule la règle 2 peut conclure. Sans elle, la caisse serait
    // déclarée libre.
    //   ######
    //   #    #
    //   #*#  #
    //   #@  .#
    //   ######
    QTest::newRow("regle 2 decisive (axe vertical mort)")
        << QString("######\n#    #\n#*#  #\n#@  .#\n######")
        << 1 << 2 << 1 << 1 << 1 << 3 << true;

    // NON GELÉE : UN SEUL des deux voisins de l'axe est mort.
    // Caisse en (2,2), largeur 6, hauteur 5. Les lignes y=1 et y=3 longent les
    // murs haut/bas et ne portent aucun but → mortes. L'axe vertical est donc
    // condamné par la règle 2. Mais l'axe HORIZONTAL a (3,2) vivante (la caisse
    // s'y pousse, puis vers le but en (4,2)) → un seul axe bloqué ≠ gel.
    //   ######
    //   #    #
    //   # $ .#
    //   #@   #
    //   ######
    QTest::newRow("regle 2 sur un seul axe (l'autre est vivant)")
        << QString("######\n#    #\n# $ .#\n#@   #\n######")
        << 2 << 2 << 2 << 1 << 2 << 3 << false;
}

void TestGetEtat::gelCasesMortes() {
    QFETCH(QString, grille);
    QFETCH(int, xCaisse);
    QFETCH(int, yCaisse);
    QFETCH(int, xMortA);
    QFETCH(int, yMortA);
    QFETCH(int, xMortB);
    QFETCH(int, yMortB);
    QFETCH(bool, geleeAttendu);

    Game g = makeGame(grille.split('\n'));
    const int L = g.getLargeur();
    const int idx  = xCaisse + yCaisse * L;
    const int mortA = xMortA + yMortA * L;
    const int mortB = xMortB + yMortB * L;

    QVERIFY2(g.getCase(idx) == Level::tcCaisse || g.getCase(idx) == Level::tcGoalCaisse,
             "la grille de test ne place pas de caisse a la position indiquee");

    // L'assertion qui donne son sens au test : ces deux voisins doivent bien être
    // du SOL (pas des murs — sinon c'est la règle 1 qui conclut) et bien être
    // MORTS (sinon la règle 2 ne s'applique pas et on teste autre chose).
    QVERIFY2(g.getCase(mortA) != Level::tcMur && g.getCase(mortB) != Level::tcMur,
             "grille invalide : un voisin de l'axe est un MUR, la regle 1 conclurait "
             "a la place de la regle 2 — le test ne prouverait rien");
    // Les DEUX voisins morts dans les deux scénarios : c'est ce qui arme la
    // règle 2 sur cet axe. Ce qui distingue les cas, c'est l'AUTRE axe.
    QVERIFY2(g.casesMortes.at(mortA) && g.casesMortes.at(mortB),
             "grille invalide : les deux voisins de l'axe doivent etre des cases "
             "mortes, sinon la regle 2 ne s'arme pas et on teste autre chose");

    QVector<bool> enCours(g.size, false);
    QCOMPARE(g.caisseGelee(idx, enCours), geleeAttendu);
}

// --- §3bis : caisseGelee() est-elle une FONCTION de l'état ? ----------------
//
// 'enCours' est un tampon partagé entre tous les appels d'un même checkDefaite()
// (il est alloué une fois, hors de la boucle de balayage). Si la garde fuit — un
// marquage laissé derrière, un résultat qui dépend de qui a été interrogé avant —
// alors le verdict dépendrait de l'ORDRE DE BALAYAGE, c'est-à-dire de la position
// des caisses dans la grille. Un tel bug serait parfaitement silencieux : le même
// plateau serait perdu ou non selon le chemin par lequel on l'a atteint.

void TestGetEtat::gelPurete_data() {
    QTest::addColumn<QString>("grille");

    // Des plateaux à PLUSIEURS caisses qui interagissent : c'est là que la garde
    // de récursion travaille, donc là qu'elle peut fuir.
    QTest::newRow("bloc 2x2")
        << QString("######\n# .  #\n# $$ #\n# $$ #\n#  @ #\n######");
    QTest::newRow("contre-exemple S")
        << QString("########\n#  ..  #\n#      #\n# $$   #\n#  $$  #\n#  @   #\n########");
    QTest::newRow("diagonale, murs opposes")
        << QString("######\n#  . #\n##$  #\n# $# #\n#  @ #\n######");
    QTest::newRow("paire sous un mur")
        << QString("######\n# ## #\n# $$ #\n#  @ #\n######");
}

void TestGetEtat::gelPurete() {
    QFETCH(QString, grille);

    Game g = makeGame(grille.split('\n'));

    QVector<int> caisses;
    for (int idx = 0; idx < g.size; ++idx)
        if (g.getCase(idx) == Level::tcCaisse || g.getCase(idx) == Level::tcGoalCaisse)
            caisses.append(idx);
    QVERIFY2(caisses.size() >= 2, "grille sans interaction : le test ne prouverait rien");

    // Référence : chaque caisse interrogée sur un tampon NEUF, isolément.
    QVector<bool> reference;
    for (int idx : caisses) {
        QVector<bool> enCours(g.size, false);
        reference.append(g.caisseGelee(idx, enCours));
    }

    // 1. Balayage avant, tampon partagé (ce que fait checkDefaite()).
    {
        QVector<bool> enCours(g.size, false);
        for (int i = 0; i < caisses.size(); ++i)
            QVERIFY2(g.caisseGelee(caisses[i], enCours) == reference[i],
                     "verdict different en balayage AVANT avec tampon partage : "
                     "la garde de recursion fuit");
    }

    // 2. Balayage ARRIÈRE : le verdict ne doit pas dépendre de l'ordre.
    {
        QVector<bool> enCours(g.size, false);
        for (int i = caisses.size() - 1; i >= 0; --i)
            QVERIFY2(g.caisseGelee(caisses[i], enCours) == reference[i],
                     "verdict different en balayage ARRIERE : caisseGelee() depend de "
                     "l'ordre d'interrogation, ce n'est donc pas une fonction de l'etat");
    }

    // 3. Idempotence : deux appels de suite sur la même caisse.
    for (int i = 0; i < caisses.size(); ++i) {
        QVector<bool> enCours(g.size, false);
        const bool a = g.caisseGelee(caisses[i], enCours);
        const bool b = g.caisseGelee(caisses[i], enCours);
        QVERIFY2(a == b, "caisseGelee() n'est pas idempotente");
    }

    // 4. La garde doit être rendue propre, y compris après un balayage complet.
    {
        QVector<bool> enCours(g.size, false);
        for (int idx : caisses) g.caisseGelee(idx, enCours);
        for (int i = 0; i < g.size; ++i)
            QVERIFY2(!enCours[i], "la garde de recursion reste marquee apres balayage");
    }
}

// --- §3bis : cohérence de groupe --------------------------------------------
//
// Le gel est une propriété du GROUPE de caisses, pas de celle qu'on interroge :
// dans un amas mutuellement bloqué, toutes sont gelées. Si une seule répondait
// « non », checkDefaite() rendrait un verdict dépendant de la caisse par laquelle
// il entre dans l'amas.

void TestGetEtat::gelGroupe_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<bool>("groupeGeleAttendu");

    // Bloc 2x2 : chacune est bloquée sur ses deux axes par ses deux voisines.
    // Vrai même en espace ouvert, loin de tout mur — c'est le deadlock 2x2.
    QTest::newRow("bloc 2x2 (les 4 gelees)")
        << QString("######\n# .  #\n# $$ #\n# $$ #\n#  @ #\n######") << true;

    // Le S : AUCUNE n'est gelée (C peut monter/descendre, et tout se libère).
    QTest::newRow("contre-exemple S (aucune gelee)")
        << QString("########\n#  ..  #\n#      #\n# $$   #\n#  $$  #\n#  @   #\n########") << false;
}

void TestGetEtat::gelGroupe() {
    QFETCH(QString, grille);
    QFETCH(bool, groupeGeleAttendu);

    Game g = makeGame(grille.split('\n'));

    int nbCaisses = 0;
    for (int idx = 0; idx < g.size; ++idx) {
        if (g.getCase(idx) != Level::tcCaisse && g.getCase(idx) != Level::tcGoalCaisse)
            continue;
        ++nbCaisses;
        QVector<bool> enCours(g.size, false);
        QVERIFY2(g.caisseGelee(idx, enCours) == groupeGeleAttendu,
                 qPrintable(QString("la caisse (%1,%2) contredit le groupe : le gel doit "
                                    "etre une propriete de l'amas, pas de la caisse "
                                    "interrogee").arg(idx % g.getLargeur()).arg(idx / g.getLargeur())));
    }
    QCOMPARE(nbCaisses, 4);
}

// --- §3bis : le gel est-il vraiment DÉFINITIF ? -----------------------------
//
// Toute la valeur du test de gel tient dans une distinction : « bloquée
// maintenant » vs « bloquée pour toujours ». Seule la seconde autorise à élaguer.
//
// On l'éprouve pour de vrai : on joue TOUTES les poussées légales des AUTRES
// caisses, une par une, et on revérifie. Une caisse gelée doit le rester quoi
// qu'il arrive — sinon le gel a menti, et il a fait disparaître une branche
// valide sans le moindre signal.
//
// (On ne pousse jamais X elle-même : le gel porte sur sa case actuelle.)

void TestGetEtat::gelStable_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<int>("xCaisse");
    QTest::addColumn<int>("yCaisse");

    // Toutes les grilles où gel_data() attend geleeAttendu == true.
    QTest::newRow("coin")
        << QString("#####\n#$  #\n# . #\n# @ #\n#####") << 1 << 1;
    QTest::newRow("diagonale, murs opposes (A)")
        << QString("######\n#  . #\n##$  #\n# $# #\n#  @ #\n######") << 2 << 2;
    QTest::newRow("diagonale, murs opposes (B)")
        << QString("######\n#  . #\n##$  #\n# $# #\n#  @ #\n######") << 2 << 3;
    QTest::newRow("bloc 2x2")
        << QString("######\n# .  #\n# $$ #\n# $$ #\n#  @ #\n######") << 2 << 2;
    QTest::newRow("paire sous un mur")
        << QString("######\n# ## #\n# $$ #\n#  @ #\n######") << 2 << 2;

    // Un amas gelé entouré de caisses LIBRES qui, elles, bougent : c'est le cas
    // qui compte. Si le gel de l'amas dépendait d'une voisine mobile, une de ces
    // poussées le dégèlerait. Le bloc 2x2 en (2,2)-(3,3) est gelé par lui-même ;
    // les deux caisses en (5,1) et (5,4) n'ont rien à voir avec lui.
    QTest::newRow("bloc 2x2 + caisses libres autour")
        << QString("########\n# .. $ #\n# $$   #\n# $$   #\n# ..$@ #\n########") << 2 << 2;
}

void TestGetEtat::gelStable() {
    QFETCH(QString, grille);
    QFETCH(int, xCaisse);
    QFETCH(int, yCaisse);

    const QStringList lignes = grille.split('\n');
    Game depart = makeGame(lignes);
    const int L   = depart.getLargeur();
    const int idx = xCaisse + yCaisse * L;

    {
        QVector<bool> enCours(depart.size, false);
        QVERIFY2(depart.caisseGelee(idx, enCours),
                 "grille invalide : la caisse de reference n'est pas gelee au depart");
    }

    // Toutes les poussées légales des AUTRES caisses.
    const QVector<bool> zone = depart.getZoneJoueur();
    const QVector<quint8> deplacables = depart.getCaissesDeplacable(zone);

    int nbPousseesTestees = 0;
    for (int c = 0; c < deplacables.size(); ++c) {
        if (c == idx) continue;                 // jamais X elle-même
        if (deplacables[c] == 0) continue;

        for (int d = 0; d < NB_DIRECTION; ++d) {
            if (!(deplacables[c] & (1 << d))) continue;

            Game apres(depart);
            if (!apres.pousse(c, (Game::EDirection)d)) continue;
            ++nbPousseesTestees;

            QVector<bool> enCours(apres.size, false);
            QVERIFY2(apres.caisseGelee(idx, enCours),
                     qPrintable(QString("pousser la caisse (%1,%2) en direction %3 a DEGELE "
                                        "la caisse (%4,%5) : le gel n'etait donc pas definitif")
                                    .arg(c % L).arg(c / L).arg(d).arg(xCaisse).arg(yCaisse)));
        }
    }

    // Trace : sur les grilles closes il n'y a rien à pousser, et c'est normal —
    // mais il faut le savoir, sinon le test passerait en ne testant RIEN.
    qDebug() << "poussees d'autres caisses testees :" << nbPousseesTestees;
}

// --- §10.5 : le motif 8/9 — condamné, mais pas encore gelé ------------------
//
// Plateau `plateau_niveau08.xsb` (relevé par l'utilisateur). Deux caisses en
// (8,2) et (10,2), un MUR au-dessus des deux (ligne y=1). Une caisse sous un mur
// ne peut jamais être poussée verticalement — ni vers le haut (destination murée)
// ni vers le bas (le joueur devrait se tenir DANS le mur). Elles ne glissent donc
// que sur leur ligne, et il n'y a qu'une case libre entre elles : (9,2).
//
// Conséquence : TOUTE poussée les rend adjacentes sous le mur — le cas « paire
// sous un mur », qui EST un gel. L'état est donc condamné... mais aucune caisse
// n'est gelée MAINTENANT, donc checkDefaite() ne voit rien.
//
// Pourquoi casesMortes ne sauve pas : (9,2) est VIVANTE en solo. Une caisse seule
// y glisse vers la gauche jusqu'en (4,2), où le mur du haut s'interrompt (y=1 vaut
// '  #  #####...' : les colonnes 3 et 4 sont libres), le joueur monte dans
// l'alcôve et la pousse enfin vers le bas. Le cul-de-sac n'est mortel qu'À DEUX.
//
// ⚠️ Ce test documente le comportement ACTUEL (perduAttendu == false). Il n'échoue
// pas : il fixe le trou par écrit, pour qu'une future détection le fasse basculer
// sciemment — et le canari jugera le faux positif.

void TestGetEtat::gelLookahead() {
    // Grille EN DUR, et pas un .xsb de travail : le plateau d'origine
    // (`plateau_niveau08.xsb`) etait un fichier de scratch non versionne, donc
    // susceptible de disparaitre — ce qui est arrive. Un test ne doit dependre
    // que de ce qu'il porte ou de ce qui est commite.
    // C'est le niveau 8 amene a la position litigieuse relevee par l'utilisateur.
    const QStringList lignes = {
        "  ####          ",
        "  #  ###########",
        "  #     $ $@   #",   // caisses (8,2) et (10,2), joueur (11,2)
        "  #  #   #     #",
        "  #       #    #",
        "###  # #  #### #",
        "# #       ##   #",
        "#      # #   # #",
        "#              #",
        "#####  #########",
        "   #      #     ",
        "   #      #     ",
        "   #......#     ",
        "   #......#     ",
        "   #......#     ",
        "   ########     ",
    };

    Game g = makeGame(lignes);
    const int L = g.getLargeur();

    // Les deux caisses du motif, et le mur au-dessus de chacune.
    const int c1 = 8 + 2 * L, c2 = 10 + 2 * L;
    QVERIFY2(g.getCase(c1) == Level::tcCaisse && g.getCase(c2) == Level::tcCaisse,
             "le plateau n'a plus ses caisses en (8,2) et (10,2) : test a reajuster");
    QVERIFY2(g.getCase(8 + 1 * L) == Level::tcMur && g.getCase(10 + 1 * L) == Level::tcMur,
             "le motif suppose un MUR au-dessus des deux caisses");

    // 1. Aucune des deux n'est gelée : elles peuvent encore glisser.
    {
        QVector<bool> enCours(g.size, false);
        QVERIFY2(!g.caisseGelee(c1, enCours), "(8,2) est deja gelee : ce n'est plus le motif");
        QVERIFY2(!g.caisseGelee(c2, enCours), "(10,2) est deja gelee : ce n'est plus le motif");
    }

    // 2. Le trou, constaté : l'état n'est pas déclaré perdu.
    g.checkDefaite();
    QVERIFY2(!g.isPerdu(), "checkDefaite() detecte desormais ce motif — mettre le test a jour "
                           "(et verifier le canari : 4/97/131/134/110/213)");

    // 3. La preuve : TOUTE poussée légale, de N'IMPORTE quelle caisse, mène à un
    //    état perdu. C'est ce qui rend l'état condamné — un fait qu'aucun test sur
    //    un état isolé ne peut voir.
    const QVector<bool> zone = g.getZoneJoueur();
    const QVector<quint8> deplacables = g.getCaissesDeplacable(zone);

    int nbPoussees = 0;
    for (int c = 0; c < deplacables.size(); ++c) {
        if (deplacables[c] == 0) continue;
        for (int d = 0; d < NB_DIRECTION; ++d) {
            if (!(deplacables[c] & (1 << d))) continue;

            Game apres(g);
            if (!apres.pousse(c, (Game::EDirection)d)) continue;
            ++nbPoussees;

            QVERIFY2(apres.isPerdu(),
                     qPrintable(QString("pousser (%1,%2) en direction %3 ne mene PAS a un etat "
                                        "perdu : l'etat n'est donc pas condamne, et le lookahead "
                                        "d'un coup serait un FAUX POSITIF")
                                    .arg(c % L).arg(c / L).arg(d)));

            // Piège du §3bis : vérifier que c'est bien le GEL qui conclut, et non
            // casesMortes. Sinon on croirait tenir le lookahead de gel alors que
            // le lookahead de case morte (déjà en place) suffirait — et il ne
            // suffit pas, puisque l'état passe aujourd'hui.
            bool gelTrouve = false;
            for (int i = 0; i < apres.size; ++i) {
                if (apres.getCase(i) != Level::tcCaisse) continue;
                QVector<bool> enCours(apres.size, false);
                if (apres.caisseGelee(i, enCours)) {
                    gelTrouve = true;
                    QVERIFY2(!apres.casesMortes.at(i),
                             qPrintable(QString("la caisse gelee (%1,%2) est AUSSI sur une case "
                                                "morte : casesMortes suffirait, le test ne "
                                                "prouverait pas le lookahead de gel")
                                            .arg(i % L).arg(i / L)));
                }
            }
            QVERIFY2(gelTrouve,
                     qPrintable(QString("apres la poussee de (%1,%2) en direction %3, l'etat est "
                                        "perdu mais AUCUNE caisse n'est gelee : c'est autre chose "
                                        "que le gel qui conclut")
                                    .arg(c % L).arg(c / L).arg(d)));
        }
    }

    QVERIFY2(nbPoussees > 0, "aucune poussee legale : le test ne prouve rien");
    qDebug() << "poussees legales, toutes perdantes :" << nbPoussees
             << "-> l'etat est condamne, mais checkDefaite() ne le voit pas";
}

void TestGetEtat::caissesDeplacables_data() {
    QTest::addColumn<QString>("grille");
    QTest::addColumn<int>("idxCaisse");
    QTest::addColumn<quint8>("masqueAttendu");

    // Pièce ouverte : les 4 poussées sont valides (case d'arrivée libre, et le
    // joueur peut atteindre chaque point de poussée en marchant autour de la
    // caisse — AStar::getChemin fait le vrai trajet, pas juste un test local).
    QTest::newRow("quatre directions, piece ouverte")
        << QString("#######\n#     #\n#  $  #\n#     #\n#  @  #\n#     #\n#######")
        << (3 + 2 * 7)
        << quint8((1 << Game::dHaut) | (1 << Game::dDroite) | (1 << Game::dBas) | (1 << Game::dGauche));

    // Caisse collée au mur du haut : la poussée vers le haut est bloquée (case
    // d'arrivée = mur), la poussée vers le bas aussi (le joueur devrait se
    // tenir dans le mur pour la faire). Seules les poussées parallèles au mur
    // restent valides.
    QTest::newRow("contre un mur, poussees paralleles uniquement")
        << QString("#######\n#  $  #\n#     #\n#  @  #\n#     #\n#######")
        << (3 + 1 * 7)
        << quint8((1 << Game::dDroite) | (1 << Game::dGauche));

    // Caisse sur goal (tcGoalCaisse) : traitée comme une caisse normale.
    QTest::newRow("caisse sur goal, piece ouverte")
        << QString("#######\n#     #\n#  *  #\n#     #\n#  @  #\n#     #\n#######")
        << (3 + 2 * 7)
        << quint8((1 << Game::dHaut) | (1 << Game::dDroite) | (1 << Game::dBas) | (1 << Game::dGauche));

    // Point de poussée du haut : une case au sol, mais scellée par des murs de
    // tous les côtés sauf la caisse elle-même (obstacle) → jamais atteignable.
    // La poussée vers le bas doit être exclue même si la case est "libre".
    QTest::newRow("origine hors zone : alcove scellee")
        << QString("#####\n## ##\n# $ #\n#   #\n#   #\n# @ #\n#####")
        << (2 + 2 * 5)
        << quint8((1 << Game::dHaut) | (1 << Game::dDroite) | (1 << Game::dGauche));
}

void TestGetEtat::caissesDeplacables() {
    QFETCH(QString, grille);
    QFETCH(int, idxCaisse);
    QFETCH(quint8, masqueAttendu);

    Game g = makeGame(grille.split('\n'));
    const QVector<quint8> masques = g.getCaissesDeplacable();

    QCOMPARE(masques[idxCaisse], masqueAttendu);
}

void TestGetEtat::caissePousseeDejaEnPosition() {
    // Le joueur est juste sous la caisse : c'est à la fois le point de
    // poussée pour dHaut (chemin vide, start == goal) ET la case d'arrivée
    // pour dBas (le joueur la libère en marchant vers son point de poussée
    // avant de pousser). Les 4 directions doivent être valides.
    Game g = makeGame(QString("#####\n#   #\n# $ #\n# @ #\n#####").split('\n'));
    const QVector<quint8> masques = g.getCaissesDeplacable();
    const int idxCaisse = 2 + 2 * 5;

    QCOMPARE(masques[idxCaisse],
             quint8((1 << Game::dHaut) | (1 << Game::dDroite) | (1 << Game::dBas) | (1 << Game::dGauche)));
}

QTEST_GUILESS_MAIN(TestGetEtat)
#include "tst_getetat.moc"
