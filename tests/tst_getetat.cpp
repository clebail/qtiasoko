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

// Charge une grille (via un .xsb temporaire) et renvoie l'état normalisé.
QByteArray etatDe(const QStringList& lignes) {
    QTemporaryFile f(QDir::tempPath() + "/qtiasoko_XXXXXX.xsb");
    if (!f.open())
        return QByteArray();
    {
        QTextStream out(&f);
        for (const QString& l : lignes)
            out << l << '\n';
    }
    f.flush();
    Level lvl;
    lvl.load(f.fileName());
    Game g(lvl);
    return g.getEtat();
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

QTEST_GUILESS_MAIN(TestGetEtat)
#include "tst_getetat.moc"
