#include <cmath>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QComboBox>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDateTime>
#include <QLabel>
#include <QBoxLayout>
#include <QStatusBar>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTextStream>
#include "astar.h"
#include "mainwindow.h"
#include "solveurastar.h"   // corralActif() / CORRAL_BUDGET : le mode hybride rejoue l'enfilage
#include "ui_mainwindow.h"

// Libellé d'une direction. Exemplaire unique : le journal de macro et celui du
// rang doivent écrire le même mot, sinon un dépouillement qui cherche « Gauche »
// rate la moitié des lignes.
static const char* nomDirection(Game::EDirection d) {
    switch (d) {
    case Game::dHaut:   return "Haut";
    case Game::dDroite: return "Droite";
    case Game::dBas:    return "Bas";
    case Game::dGauche: return "Gauche";
    }
    return "?";
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi(this);
    installEventFilter(this);

    // Libellé de repos de cbEtatMax, capté depuis le .ui : c'est la seule copie du
    // texte, resetEtatMax() y revient après l'avoir suffixé du compteur (n/total).
    texteEtatMax = cbEtatMax->text();
    texteResoudre = pbResoudre->text();

    // Sélecteur de record, glissé juste après cbEtatMax dans SON layout — construit
    // par code et non déclaré dans le .ui, comme la légende : c'est un outil de
    // chantier, il doit pouvoir disparaître sans toucher au formulaire.
    if (auto* lay = qobject_cast<QBoxLayout*>(cbEtatMax->parentWidget()->layout())) {
        cbRecords = new QComboBox(cbEtatMax->parentWidget());
        cbRecords->setFocusPolicy(Qt::NoFocus);
        cbRecords->setToolTip("Chemin du solveur vers ce record de caisses posées. "
                              "Un run les garde tous ; C annote le coup affiché.");
        cbRecords->hide();                       // rien à choisir tant qu'il n'y a pas 2 records
        lay->insertWidget(lay->indexOf(cbEtatMax) + 1, cbRecords);
        connect(cbRecords, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            if (i >= 0 && i < recordsVus.size())
                chargeCheminVisionne(departSolveur, recordsVus[i].second, false);
        });
    }

    for (const Solveur::SType& t : Solveur::types()) {
        cbSolveur->addItem(t.libelle, static_cast<int>(t.type));
    }

    // Niveaux lus depuis les ressources et non depuis QDir::current() : le
    // répertoire courant n'est pas celui des sources (shadow build de Qt Creator,
    // ou '/' quand le .app est lancé depuis le Finder) et aucun niveau n'était
    // trouvé. Embarqués dans le binaire, ils sont indépendants du cwd.
    //
    // Le numéro du fichier est la seule source de vérité : il nomme l'entrée du
    // combo *et* alimente Game, sinon l'overlay de WGame affiche un autre numéro
    // que celui sélectionné.
    const QDir dossier(":/levels");
    const QStringList fichiers = dossier.entryList(QStringList() << "level????.xsb", QDir::Files, QDir::Name);
    for (const QString& fichier : fichiers) {
        bool ok = false;
        const int numero = fichier.mid(5, 4).toInt(&ok);
        if (!ok) continue;   // les ???? du filtre ne sont pas forcément des chiffres

        cbNiveau->addItem(QString("Niveau %1").arg(numero), dossier.filePath(fichier));
        cbNiveau->setItemData(cbNiveau->count() - 1, numero, RoleNumero);
    }

    connect(cbNiveau, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onNiveauChange);
    connect(pbResoudre, &QPushButton::clicked, this, &MainWindow::onIALance);
    connect(pbRevoir, &QPushButton::clicked, this, &MainWindow::onRevoir);
    connect(pbPasPrec, &QPushButton::clicked, this, &MainWindow::onPasPrec);
    connect(pbPasSuiv, &QPushButton::clicked, this, &MainWindow::onPasSuiv);
    connect(slPas, &QSlider::valueChanged, this, &MainWindow::onPasSlider);
    connect(pbExport, &QPushButton::clicked, this, &MainWindow::onExportPassages);
    connect(pbExportXsb, &QPushButton::clicked, this, &MainWindow::onExportXsb);
    connect(cbNotePassages, &QCheckBox::stateChanged, this, &MainWindow::onShowPassagesCaisse);
    connect(cbDistanceButActif, &QCheckBox::stateChanged, this, &MainWindow::onShowChampButActif);
    connect(cbEtatMax, &QCheckBox::stateChanged, this, &MainWindow::onToggleEtatMax);
    connect(cbHybride, &QCheckBox::stateChanged, this, &MainWindow::onToggleHybride);

    // LÉGENDE DES TOUCHES. Le mode hybride et le rejeu de journal n'ont ni bouton ni
    // menu — tout est au clavier, et sans légende il faudrait retenir dix raccourcis.
    // Construite par code plutôt que dans mainwindow.ui : c'est un outil de chantier,
    // il doit partir sans laisser de trace dans le Designer.
    // ⚠️ TEXTE BRUT, ASCII, SANS RichText. La version précédente était en HTML avec
    // entités et caracteres Unicode : un label qui n'affichait RIEN (« tout gris »).
    // Un texte enrichi mal interprete se rend vide sans lever la moindre erreur —
    // on ne peut donc pas distinguer « mal place » de « mal formate » a l'ecran, et
    // c'est ce qui a fait tourner cinq corrections a vide. Le jour ou c'est visible
    // et stable, on pourra remettre du gras ; pas avant.
    lbLegende = new QLabel(this);
    // ⚠️ PAS de wordWrap : le texte porte deja ses propres '\n', et un QLabel en
    // wordWrap ne sait pas calculer sa hauteur avant de connaitre sa largeur — la
    // barre d'etat lui accordait 48 px (3 lignes) et TRONQUAIT les suivantes, soit
    // exactement les lignes de signification. Constate a l'ecran, deux fois.
    lbLegende->setWordWrap(false);
    lbLegende->setTextFormat(Qt::PlainText);
    lbLegende->setText(
        "MAIN : fleches = jouer | Retour arriere = annuler | clic caisse cerclee = macro | "
        "clic case libre = marcher | clic DROIT = signaler macro manquante\n"
        "REJEU : L = charger le journal | "
        "boutons < > = coup par coup (Maj : poussee a poussee)\n"
        "   pendant un rejeu : zone du PERSO en violet (+ compte 'Zj') | caisse du coup encadree "
        "ORANGE = a toi / gris pointille = macro\n"
        "   les CHIFFRES sur le plateau = les 12 prochaines arrivees de caisse "
        "(1 = la prochaine ; orange = poussee CHOISIE, gris = macro)\n"
        "INTENTION (pendant un rejeu, vaut jusqu'a la suivante ; plusieurs touches = plusieurs "
        "intentions ; Retour arriere = ANNULER la derniere) :\n"
        "   E = ecarter cette caisse du chemin d'une AUTRE CAISSE   |   "
        "O = ouvrir un passage pour le JOUEUR\n"
        "   G = garer a un endroit temporaire, j'y reviendrai   |   "
        "T = sortir la caisse pour la REPRENDRE DANS L'AUTRE SENS (recul)\n"
        "   R = rapprocher de son but (poussee productive)   |   "
        "? = je ne sais pas ENCORE le dire");
    // Dans un widget À ELLE, placé dans la barre d'état (cf. plus bas).
    //
    // ⚠️ TROIS placements ratés avant celui-ci, tous constatés à l'écran :
    //  1. à côté des cases à cocher — c'est un QHBoxLayout, un texte de trois lignes
    //     y est écrasé à une largeur illisible ;
    //  2. dans le verticalLayout du centralWidget — lequel EST WGame, qui peint son
    //     plateau par-dessus, et dont le QScrollArea (Expanding) mange la place ;
    //  3. avec un fond forcé pour corriger (2) — mais sans forcer la couleur du
    //     texte, d'où un widget « tout gris » en thème sombre.
    wLegende = new QWidget(this);
    wLegende->setObjectName("widgetLegende");
    // ⚠️ AUCUN styleSheet de fond ici. La version précédente forçait
    // `background: palette(window)` SANS forcer la couleur du texte : en thème
    // sombre le label restait clair sur un fond clair — le widget s'affichait
    // « tout gris », contenu invisible. La barre d'état peint déjà son fond et
    // gère sa palette ; on la laisse faire.
    QVBoxLayout* layLegende = new QVBoxLayout(wLegende);
    layLegende->setContentsMargins(8, 4, 8, 4);
    layLegende->addWidget(lbLegende);

    // ⚠️ `lbPas` DESCENDU DANS LA BARRE D'ÉTAT (2026-08-02, constat utilisateur :
    // « ça déborde de l'écran, je n'ai pas un grand écran »). Il vivait dans le
    // layout HORIZONTAL de la navigation, à côté de ◀ ▶ et du slider : chaque
    // caractère ajouté y élargit la fenêtre, et le compteur « reste N à toi » l'a
    // fait sortir de l'écran. Ici il est empilé sous la légende, donc il grandit en
    // hauteur et jamais en largeur. On le retire explicitement de son ancien layout
    // — `addWidget` reparente le widget mais laisse l'ancien item en place.
    if (QWidget* p = lbPas->parentWidget())
        if (QLayout* ancien = p->layout()) ancien->removeWidget(lbPas);
    lbPas->setWordWrap(true);
    layLegende->addWidget(lbPas);
    // TOUJOURS VISIBLE. Elle etait conditionnee au mode hybride : la barre d'etat
    // restait haute (sa hauteur minimale ne depend pas de la case) mais le widget
    // etait cache, donc « il y a bien le widget, mais tout gris ». Une legende n'a
    // aucune raison d'etre conditionnelle — c'est ce qui a coute six corrections.
    wLegende->setVisible(true);

    // ⚠️ DANS LA BARRE D'ÉTAT, pas dans le layout du centralWidget. Trois tentatives
    // ont échoué avant celle-ci, toutes pour la même raison de fond : le
    // centralWidget est WGame, son verticalLayout contient un QScrollArea en
    // Expanding, et tout ce qu'on ajoute à côté se fait écraser ou recouvrir.
    // La barre d'état est gérée par QMainWindow lui-même — elle est toujours en bas,
    // toujours visible, et aucun layout de contenu ne peut la comprimer.
    // La hauteur vient du LABEL lui-meme (sizeHint), pas d'une constante : le texte
    // a grossi de 3 a 6 lignes et la constante, elle, n'avait pas suivi.
    statusBar()->addPermanentWidget(wLegende, 1);
    statusBar()->setSizeGripEnabled(false);
    // ⚠️ La hauteur couvre la légende ET `lbPas`, et pour ce dernier on RÉSERVE deux
    // lignes au lieu de lire son sizeHint : à la construction il ne contient que
    // « — », donc son sizeHint vaut une ligne et la barre tronquerait dès le premier
    // libellé long. C'est exactement le bug d'hier (hauteur figée pendant que le
    // texte passait de 3 à 6 lignes), pris dans l'autre sens.
    statusBar()->setMinimumHeight(lbLegende->sizeHint().height()
                                  + 2 * QFontMetrics(lbPas->font()).height() + 18);

    // TEST DISCRIMINANT (2026-08-01). Cinq placements de la légende ont échoué à
    // l'écran sans que rien ne le signale côté code. showMessage() est le mécanisme
    // le plus élémentaire de Qt et ne dépend d'AUCUN widget construit ici : s'il
    // n'apparaît pas non plus, le fautif n'est pas le placement mais le binaire
    // exécuté (un shadow build de Qt Creator, par exemple, n'est pas celui que
    // `make` produit ici — §7, l'app et le bench qui divergent en silence).
    qDebug().noquote()
        << "[legende] widget cree | visible=" << wLegende->isVisible()
        << "| taille label=" << lbLegende->sizeHint()
        << "| texte=" << lbLegende->text().size() << "car."
        << "| hybride coche=" << cbHybride->isChecked()
        << "|| lbPas visible=" << lbPas->isVisible()
        << "parent=" << (lbPas->parentWidget() ? lbPas->parentWidget()->objectName() : "(aucun)")
        << "| barre mini=" << statusBar()->minimumHeight();
    connect(&timerRejeu, &QTimer::timeout, this, &MainWindow::rejouerCoup);
    timerRejeu.setInterval(150);
    // Même cadence que le rejeu d'une solution : au-delà de dureeAnimation, chaque
    // glissement a le temps de finir avant le suivant (cf. WGame::dureeAnimation).
    connect(&timerMacro, &QTimer::timeout, this, &MainWindow::avanceMacro);
    timerMacro.setInterval(150);

    connect(wGame, &WGame::joueurDeplace, this, &MainWindow::onJoueurDeplace);
    connect(wGame, &WGame::caseCliquee, this, &MainWindow::onCaseCliquee);
    connect(wGame, &WGame::caseSignalee, this, &MainWindow::onCaseSignalee);

    // Les stats sont peintes au coin de la partie VISIBLE du plateau (cf.
    // WGame::paintEvent) : un défilement les déplace, il faut donc repeindre. Qt
    // ne redessine sinon que la bande découverte, et le texte resterait en double.
    connect(scrollPlateau->horizontalScrollBar(), &QScrollBar::valueChanged, wGame, QOverload<>::of(&QWidget::update));
    connect(scrollPlateau->verticalScrollBar(),   &QScrollBar::valueChanged, wGame, QOverload<>::of(&QWidget::update));

    // Game transporté par signal queued (thread solveur -> UI) pour l'état-max.
    qRegisterMetaType<Game>("Game");

    if (cbNiveau->count() > 0) {
        onNiveauChange(0);
    }
}

MainWindow::~MainWindow() {
    // Le solveur est un QThread enfant de la fenêtre : le laisser tourner
    // pendant que QObject détruit ses enfants fait avorter le programme
    // (« QThread: Destroyed while thread is still running »). On lui demande
    // l'arrêt et on attend qu'il sorte de sa boucle — quelques microsecondes,
    // le temps du dépilement en cours.
    if (solveur) {
        solveur->demanderArret();
        solveur->wait();
    }
}

void MainWindow::onNiveauChange(int index) {
    if (index < 0) return;

    Level lvl;
    lvl.load(cbNiveau->itemData(index).toString());
    game = Game(lvl, cbNiveau->itemData(index, RoleNumero).toInt());
    gameDepart = game;               // origine de tout rejeu de journal (touche L)

    historique.clear();   // nouvel état de départ : l'undo ne doit pas franchir le chargement
    derniereSolutionCoups.clear();
    timerMacro.stop();    // une macro du niveau précédent n'a plus de plateau
    coupsMacro.clear();
    indicesPoussees.clear();
    posPas = 0;
    wGame->showZoneJoueur(false);   // la zone n'est armée que par un rejeu (touche L)

    // ⚠️ FERMER LE JOURNAL D'INTENTIONS AU CHANGEMENT DE NIVEAU (2026-08-02). Il
    // n'était fermé que dans rejoueJournal(), donc il restait OUVERT sur le fichier
    // du niveau précédent après un changement. Une frappe d'intention avant d'avoir
    // pressé `L` écrivait alors dans le mauvais fichier, avec les numéros de coup
    // du niveau courant : faux, et parfaitement silencieux. C'est la forme
    // habituelle du piège — un état qui survit à ce qui le justifiait.
    if (journalIntentions.isOpen()) journalIntentions.close();
    if (journalCritique.isOpen()) journalCritique.close();   // même piège, même remède
    intentionCourante.clear();
    majNavigationPas();   // le chemin du niveau précédent n'est plus navigable
    pbRevoir->setEnabled(false);

    // Nouveau niveau : l'état-max du précédent n'a plus de sens.
    resetEtatMax();

    initPassages();

    wGame->setGame(&game);
    wGame->setEtatsExplores(0);
    wGame->setDuree(0);
    wGame->setPassages(passages);
    journalOuvre(game);    // un journal par niveau, ouvert AVANT la première trace
    majRangsButs(game);    // ordreButs est statique : une fois par niveau suffit
    majSurcouches(game);
    wGame->update();

    centrerSurJoueur();
}

void MainWindow::majSurcouches(const Game& g) {
    const int b = g.butActif();
    wGame->setChampButActif(g.champDistanceButActif(), b >= 0 ? g.getCaseBut(b) : -1);
    // L'arbre d'un clic précédent ne vaut que pour L'ÉTAT où il a été
    // calculé : périmé dès qu'un coup est joué, donc effacé à chaque
    // rafraîchissement du champ par défaut, pas seulement sur un nouveau clic.
    wGame->setArbreMacro({});
    // Les repères du clic droit non plus : ils datent de l'état d'avant.
    signales.clear();
    plateauJournalise = false;
    wGame->setSignales({});
    majMacrosJouables(g);
}

// Le régime d'engagement du solveur, rejoué sur l'état affiché (cf. mainwindow.h).
// Miroir de solveurastar.cpp:331-343 — toute divergence ici ferait mentir l'écran.
void MainWindow::majMacrosJouables(const Game& g) {
    macroCaissesJouables = QVector<bool>(g.getLargeur() * g.getHauteur(), false);
    QVector<bool> trajets(macroCaissesJouables.size(), false);

    if (!cbHybride->isChecked()) {
        wGame->setMacrosJouables({}, {});
        return;
    }

    const int but = g.butActif();
    if (but < 0) {                       // état gagné : plus rien à engager
        wGame->setMacrosJouables({}, {});
        journal("[hybride] aucun but actif (etat gagne)");
        return;
    }

    const QVector<bool>    zone    = g.getZoneJoueur();
    const QVector<quint8>  caisses = g.getCaissesDeplacable(zone);

    const int caseBut = g.getCaseBut(but);
    int rangBut = -1;
    const QVector<int>& ordre = g.getOrdreButs();
    for (int k = 0; k < ordre.size(); k++) if (ordre[k] == but) { rangBut = k; break; }

    int posees = 0;
    for (int k = 0; k < g.getNbButs(); k++)
        if (g.getCase(g.getCaseBut(k)) == Level::tcGoalCaisse) posees++;

    QStringList details;
    for (int c = 0; c < caisses.size(); c++) {
        if (caisses[c] == 0) continue;
        if (!g.macroPeutDemarrer(c, but, zone)) continue;

        Game f(g);
        QVector<QPair<int,int>> poussees;
        qint64 essais = 0;
        if (!f.macroVersButBacktrack(c, but, poussees, &essais) || f.isPerdu()) continue;

        macroCaissesJouables[c] = true;
        // Le TRAJET réellement suivi par la macro retenue : chaque poussée porte
        // la case d'où part la caisse, plus la case d'arrivée du dernier pas.
        for (const auto& p : poussees) if (p.first < trajets.size()) trajets[p.first] = true;
        if (caseBut < trajets.size()) trajets[caseBut] = true;

        details << QString("caisse (%1,%2) en %3 poussees (%4 branche%5)")
                       .arg(c % g.getLargeur()).arg(c / g.getLargeur())
                       .arg(poussees.size()).arg(essais).arg(essais > 1 ? "s" : "");
    }

    wGame->setMacrosJouables(trajets, macroCaissesJouables);

    // LOG (2026-08-01) : une ligne par état joué à la main, pour relire après coup
    // une partie entière. Le cas à zéro macro est le plus informatif — c'est là que
    // le solveur retomberait sur des poussées simples, qui ne lisent jamais l'ordre
    // (§6.2, FAIT 3).
    journal(QString("[hybride] posees %1/%2 | but actif (%3,%4) rang %5/%6 | %7 macro%8 jouable%8%9")
               .arg(posees).arg(g.getNbButs())
               .arg(caseBut % g.getLargeur()).arg(caseBut / g.getLargeur())
               .arg(rangBut).arg(g.getNbButs())
               .arg(details.size()).arg(details.size() > 1 ? "s" : "")
               .arg(details.isEmpty() ? QString(" -> POUSSEES SIMPLES")
                                      : QString(" : %1").arg(details.join(", "))));
}

// Une ligne de journal (cf. mainwindow.h). Flush à chaque ligne : une partie
// s'interrompt par un kill ou une fermeture brutale bien plus souvent que par une
// sortie propre, et un journal tronqué de ses cinquante dernières lignes ne vaut
// rien — c'est justement la fin qui intéresse.
void MainWindow::journal(const QString& ligne) {
    if (!journalFichier.isOpen()) {
        qDebug().noquote() << ligne;
        return;
    }
    journalFichier.write(ligne.toUtf8());
    journalFichier.write("\n");
    journalFichier.flush();
}

// ⚠️ UN SIGNALEMENT FAIT PENDANT UNE ANNOTATION EST UNE ANNOTATION, PAS DU JEU
// (2026-08-02). Il part donc dans le journal d'INTENTIONS, jamais dans le journal
// de jeu : celui-ci est la donnée brute qu'on relit, on n'y réécrit pas — c'est la
// règle posée à la création du rejeu. Deuxième raison, pratique : dans le journal
// d'intentions le rapport est rattachable au NUMÉRO DE COUP (cf. l'en-tête ajouté
// dans onCaseSignalee), donc au plan en cours ; dans le journal de jeu il serait
// orphelin, appendu après la dernière partie sans rien pour le situer.
// Hors annotation (partie jouée à la main en mode hybride), destination inchangée.
void MainWindow::pousseAnnulation() {
    if (!journalIntentions.isOpen()) return;
    journalIntentions.flush();                 // sinon size() ignore ce qui est en tampon
    pileAnnotations.append(journalIntentions.size());
    pileIntentionAvant.append(intentionCourante);
}

void MainWindow::journalSignalement(const QString& ligne) {
    if (journalIntentions.isOpen()) {
        journalIntentions.write(ligne.toUtf8() + "\n");
        journalIntentions.flush();
        qDebug().noquote() << ligne;
        return;
    }
    journal(ligne);
}

void MainWindow::journalOuvre(const Game& g) {
    if (journalFichier.isOpen()) journalFichier.close();
    if (!cbHybride->isChecked()) return;
    // Garde-fou : cocher la case avant qu'un niveau soit chargé ouvrirait un
    // journal au nom du niveau par défaut, sur un plateau vide que butActif()
    // déclare aussitôt « gagné ». Constaté en test, pas déduit.
    if (!g.isLoaded()) return;

    const QString nom = QString("hybride_niveau_%1.txt").arg(g.getNumNiveau(), 4, 10, QChar('0'));

    // Répertoire courant d'abord — c'est celui des sources quand on lance depuis un
    // terminal, et c'est là qu'on veut les journaux. Mais il vaut '/' quand le .app
    // part du Finder (même piège que les niveaux, cf. ctor) : d'où le repli sur
    // Documents, toujours accessible en écriture.
    journalFichier.setFileName(QDir::current().filePath(nom));
    if (!journalFichier.open(QIODevice::Append | QIODevice::Text)) {
        const QString repli = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        journalFichier.setFileName(QDir(repli).filePath(nom));
        if (!journalFichier.open(QIODevice::Append | QIODevice::Text)) {
            qDebug().noquote() << "[hybride] journal impossible a ouvrir, retour a la console";
            return;
        }
    }

    // Le chemin ABSOLU, sur la console : sans lui, savoir où le fichier a atterri
    // demande de deviner le cwd de l'app.
    qDebug().noquote() << "[hybride] journal ->" << QFileInfo(journalFichier).absoluteFilePath();

    journal(QString("\n=== niveau %1 — partie du %2 ===")
                .arg(g.getNumNiveau())
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
}

// ordreButs rendu lisible sur le plateau. Statique : ne dépend que du niveau.
void MainWindow::majRangsButs(const Game& g) {
    rangsButs = QVector<int>(g.getLargeur() * g.getHauteur(), -1);

    const QVector<int>& ordre = g.getOrdreButs();
    QStringList lisible;
    for (int k = 0; k < ordre.size(); k++) {
        const int cell = g.getCaseBut(ordre[k]);
        if (cell >= 0 && cell < rangsButs.size()) rangsButs[cell] = k;
        lisible << QString("%1:(%2,%3)").arg(k).arg(cell % g.getLargeur()).arg(cell / g.getLargeur());
    }

    wGame->setRangsButs(rangsButs);
    // ⚠️ Dire la SOURCE, pas seulement l'ordre : un `ordre_niveau_XXXX.txt` oublié
    // dans le répertoire ferait dépouiller la partie comme si l'ordre était celui du
    // solveur. Le journal se relit des jours plus tard, hors de tout contexte.
    const QString injecte = Game::cheminOrdreInjecte(g.getNumNiveau());
    journal(QString("[hybride] ordre de remplissage %1 : %2")
                .arg(injecte.isEmpty() ? QString("calcule")
                                       : QString("⚠ INJECTE depuis %1").arg(QFileInfo(injecte).fileName()))
                .arg(lisible.join(" ")));
}

// Le rang du coup humain dans le classement du solveur (cf. mainwindow.h pour le
// pourquoi, les limites et l'effet de bord « juge fp sur niveau non résolu »).
void MainWindow::mesureRangCoup(const Game& avant, int idxCaisse, Game::EDirection dir) {
    if (!cbHybride->isChecked()) return;
    // L'état-max est un instantané du solveur : les surcouches (dont
    // macroCaissesJouables, lue plus bas) le décrivent LUI et non 'game'. Même
    // garde que le clic (onCaseCliquee) — on ne mesure que sur le plateau joué.
    if (cbEtatMax->isChecked()) return;

    const int L = avant.getLargeur();
    const QString coup = QString("(%1,%2) %3").arg(idxCaisse % L).arg(idxCaisse / L)
                                              .arg(nomDirection(dir));

    // RÉGIME D'ENGAGEMENT : dès qu'UNE macro s'engage, le solveur ne génère que
    // des macros et rien d'autre (solveurastar.cpp) — une poussée simple jouée à
    // la main n'est alors l'enfant de rien, et lui donner un rang serait mentir.
    // On lit macroCaissesJouables plutôt que de refaire la passe : c'est le même
    // calcul, rempli par majMacrosJouables pour CET état (les surcouches ne sont
    // rafraîchies qu'à la fin de joue(), après cet appel).
    const int nbMacros = macroCaissesJouables.count(true);
    if (nbMacros > 0) {
        journal(QString("[rang] %1 | HORS REGIME MACRO : %2 macro%3 engagee%3, "
                        "le solveur ne genere aucune poussee simple")
                   .arg(coup).arg(nbMacros).arg(nbMacros > 1 ? "s" : ""));
        return;
    }

    struct Enfant { int caisse; int dir; int h; qint64 guidage; };
    QVector<Enfant> enfants;
    Enfant joueur{idxCaisse, (int)dir, -1, 0};
    const char* ecarte = nullptr;    // pourquoi le coup joué n'est pas un enfant

    const QVector<bool>   zone    = avant.getZoneJoueur();
    const QVector<quint8> caisses = avant.getCaissesDeplacable(zone);

    // Tampons de l'enfilage. Le cache d'enclos est LOCAL à l'appel : il n'évite
    // que de rejuger deux fois le même corral entre frères. Le faire vivre d'un
    // coup à l'autre mesurerait un cache d'historique différent de celui d'un run
    // réel — et c'est justement la réserve ouverte du §6.1 (la clé du cache).
    QVector<bool> zoneEnfant, visiteCorral;
    QHash<QByteArray, Game::VerdictEnclos> cacheEnclos;

    for (int c = 0; c < caisses.size(); c++) {
        for (int d = 0; d < NB_DIRECTION; d++) {
            if (!(caisses[c] & (1 << d))) continue;
            const bool estLeCoup = (c == idxCaisse && d == (int)dir);

            Game e(avant);
            if (!e.pousse(c, (Game::EDirection)d) || e.isPerdu()) {
                if (estLeCoup) ecarte = "perdu (checkDefaite)";
                continue;
            }
            // Case de repos de la caisse déplacée : les deux étages du corral en
            // partent, dans cet ordre (unitaire d'abord, il est en O(1)).
            const int arrivee = avant.caseApres(c, (Game::EDirection)d);
            if (corralActif() && arrivee >= 0) {
                if (e.corralUnitaireMort(arrivee)) {
                    if (estLeCoup) ecarte = "corral unitaire";
                    continue;
                }
                e.getZoneJoueur(zoneEnfant);
                const Game::EnclosInfo inf =
                    e.detecteEnclosArrivee(arrivee, zoneEnfant, visiteCorral,
                                           &cacheEnclos, CORRAL_BUDGET);
                if (inf.dursMorts > 0) {
                    if (estLeCoup) ecarte = "corral-N (mort PROUVEE)";
                    continue;
                }
            }
            qint64 guidage = 0;
            const int h = e.getHeuristique(&guidage);
            enfants.append({c, d, h, guidage});
            if (estLeCoup) joueur = enfants.last();
        }
    }

    // Le coup joué a été ÉLAGUÉ. Sur une partie qu'on finit par gagner, l'état
    // traversé est soluble par construction : c'est alors un faux positif PROUVÉ,
    // le raisonnement exact du juge mesures/fp — étendu ici aux niveaux qu'on ne
    // sait pas résoudre. Le journal porte aussi les [undo] et la victoire : c'est
    // au dépouillement de conclure, pas à cette ligne.
    if (ecarte) {
        journal(QString("[rang] %1 | ⚠ ECARTE par le solveur : %2 | %3 autre%4 enfant%4 enfile%4")
                   .arg(coup).arg(ecarte).arg(enfants.size())
                   .arg(enfants.size() > 1 ? "s" : ""));
        return;
    }
    if (joueur.h < 0) {
        // Le coup était légal (le jeu l'a accepté) mais l'enfilage ne l'a pas
        // produit : le miroir a divergé du solveur. À dire, jamais à avaler.
        journal(QString("[rang] %1 | ⚠ INTROUVABLE parmi les %2 enfants — MIROIR EN DEFAUT")
                   .arg(coup).arg(enfants.size()));
        return;
    }

    // La clé du comparateur (solveurastar.cpp) : f croissant, puis g DÉCROISSANT,
    // puis guidage croissant. Toutes les poussées simples d'un même état enfilent
    // à g+1, donc g ne départage rien ici et f ne diffère que par h.
    auto meilleurQue = [](const Enfant& a, const Enfant& b) {
        if (a.h != b.h) return a.h < b.h;
        return a.guidage < b.guidage;
    };

    int rang = 1, exAequo = -1, iBest = 0;   // exAequo part à -1 : le coup joué ne se compte pas
    for (int i = 0; i < enfants.size(); i++) {
        if (meilleurQue(enfants[i], joueur))       rang++;
        else if (!meilleurQue(joueur, enfants[i])) exAequo++;
        if (meilleurQue(enfants[i], enfants[iBest])) iBest = i;
    }

    const Enfant& best = enfants[iBest];
    const int g  = avant.getNbDepCaisse() + 1;   // les enfants sont tous à g+1
    const int df = joueur.h - best.h;            // = Δf : c'est lui qui se transporte à la file
    journal(QString("[rang] %1 | rang %2/%3%4 | h %5 f %6 | meilleur (%7,%8) %9 h %10 f %11 | df %12")
               .arg(coup).arg(rang).arg(enfants.size())
               .arg(exAequo > 0 ? QString(" (%1 ex aequo)").arg(exAequo) : QString())
               .arg(joueur.h).arg(g + joueur.h)
               .arg(best.caisse % L).arg(best.caisse / L)
               .arg(nomDirection((Game::EDirection)best.dir))
               .arg(best.h).arg(g + best.h)
               .arg(df >= 0 ? QString("+%1").arg(df) : QString::number(df)));
}

void MainWindow::onJoueurDeplace(QPoint centre) {
    // Marge d'une case et demie : la vue ne bouge que lorsque le perso approche
    // d'un bord, au lieu de défiler en permanence sous lui.
    scrollPlateau->ensureVisible(centre.x(), centre.y(),
                                 SPRITE_WIDTH * 3 / 2, SPRITE_HEIGHT * 3 / 2);
}

void MainWindow::majSpinner() {
    // L'état-max est fait pour suivre une résolution en cours : le voile qui
    // masque le plateau irait contre son seul usage.
    wGame->setResolution(solveur != nullptr && !cbEtatMax->isChecked());
}

void MainWindow::centrerSurJoueur() {
    // Après le tour de boucle en cours : WGame vient de changer de plateau, le
    // QScrollArea ne l'a pas encore redimensionné et centreJoueur() rendrait une
    // position calculée sur l'ancienne géométrie.
    QTimer::singleShot(0, this, [this]() {
        const QPoint c = wGame->centreJoueur();
        // Une demi-vue de marge : à l'ouverture on veut le perso au milieu, pas
        // collé au bord comme le fait la marge serrée du suivi.
        scrollPlateau->ensureVisible(c.x(), c.y(),
                                     scrollPlateau->viewport()->width()  / 2,
                                     scrollPlateau->viewport()->height() / 2);
    });
}

void MainWindow::initPassages() {
    const int size = game.getLargeur() * game.getHauteur();
    passages = QVector<int>(size, 0);

    // Une caisse occupe déjà sa case de départ : le compte y démarre à 1.
    for (int i = 0; i < size; i++) {
        const Level::ETypeCase c = game.getCase(i);
        if (c == Level::tcCaisse || c == Level::tcGoalCaisse) passages[i] = 1;
    }
}

// Un coup, et le comptage du passage de caisse s'il y en a un.
bool MainWindow::joue(Game::EDirection dir) {
    const QPoint avant  = game.getPlayerPoint();
    const int    caisses = game.getNbDepCaisse();

    // Undo : on mémorise l'état AVANT un coup HUMAIN (pas le rejeu auto d'une
    // solution, qui repart de son propre départ). La copie est COW, donc légère.
    const bool humain = !timerRejeu.isActive();
    CoupHist snap;
    if (humain) { snap.etat = game; snap.passages = passages; }

    if (!game.deplace(dir)) return false;
    if (humain) historique.append(std::move(snap));

    const QPoint apres = game.getPlayerPoint();
    const bool poussee = game.getNbDepCaisse() > caisses;

    if (poussee) {
        // Le joueur a avancé d'une case ; la caisse qu'il vient de pousser est
        // juste devant lui, dans le même sens.
        const QPoint delta = apres - avant;
        const QPoint caisse = apres + delta;

        const int idx = caisse.x() + caisse.y() * game.getLargeur();
        if (idx >= 0 && idx < passages.size()) passages[idx]++;

        wGame->setPassages(passages);

        // Trace brute des mouvements (§6.2, session du 2026-07-20) : rejoue le
        // niveau 11/190/191/192 à la main devant cette sortie — ça donne le trajet
        // RÉEL entre deux poses, pas juste l'ordre final. C'est elle qui a livré
        // l'ordre humain du 192 puis les 276 poussées du 13. Format INCHANGÉ depuis
        // qu'elle part au journal (2026-08-01) : les transcriptions déjà écrites
        // continuent de se relire.
        const bool posee = game.getCase(idx) == Level::tcGoalCaisse;
        journal(QString("[mouv] joueur (%1,%2)->(%3,%4) POUSSE caisse ->(%5,%6)%7")
                                  .arg(avant.x()).arg(avant.y()).arg(apres.x()).arg(apres.y())
                                  .arg(caisse.x()).arg(caisse.y())
                                  .arg(posee ? " [POSE]" : ""));

        // LE RANG DU COUP HUMAIN (cf. mainwindow.h) — uniquement pour une poussée
        // vraiment CHOISIE par le joueur : pendant une macro ou le rejeu d'une
        // solution, le coup vient du solveur et il n'y a rien à comparer.
        // L'état d'avant est celui que l'undo vient d'empiler (aucune copie de
        // plus), et la caisse poussée se tenait là où le joueur est maintenant.
        if (humain && !timerMacro.isActive() && !historique.isEmpty())
            mesureRangCoup(historique.last().etat,
                           apres.x() + apres.y() * game.getLargeur(), dir);
    } else {
        journal(QString("[mouv] joueur (%1,%2)->(%3,%4)")
                                  .arg(avant.x()).arg(avant.y()).arg(apres.x()).arg(apres.y()));
    }

    // 'game' est déjà à l'arrivée : l'affichage seul retarde, le temps que le
    // perso (et la caisse) glissent depuis la case d'où ils partent.
    wGame->animerCoup(dir, avant, poussee);
    majSurcouches(game);

    return true;
}

// Défait le dernier coup HUMAIN : on dépile l'état d'avant et on l'affiche tel
// quel (saut direct, l'animation en cours est coupée). Sans effet pendant un
// rejeu ou si l'historique est vide.
void MainWindow::annuleCoup() {
    if (timerRejeu.isActive() || historique.isEmpty()) return;

    CoupHist prec = historique.takeLast();
    game     = std::move(prec.etat);       // move-assign en place : &game inchangé, WGame le pointe toujours
    passages = std::move(prec.passages);

    wGame->arreteAnimation();
    wGame->setEtatsExplores(0);
    wGame->setPassages(passages);
    majSurcouches(game);
    wGame->update();
    centrerSurJoueur();

    journal("[undo] retour arriere");
}

// Contrôles verrouillés pendant qu'un solveur tourne ou qu'une solution se
// rejoue : changer de niveau ou de solveur sous les pieds du thread laisserait
// 'game' et le rejeu désynchronisés.
void MainWindow::setControlesActifs(bool actifs) {
    cbNiveau->setEnabled(actifs);
    cbSolveur->setEnabled(actifs);
    majBoutonResoudre();
}

// Le bouton de résolution est un BASCULE : il lance, puis il arrête. Son état ne
// se déduit donc pas de setControlesActifs() — d'où cet unique point de décision,
// pris sur l'état réel plutôt que sur ce que croit savoir l'appelant.
void MainWindow::majBoutonResoudre() {
    const bool enCours = (solveur != nullptr);
    pbResoudre->setText(enCours ? "Arrêter" : texteResoudre);
    // Pendant le rejeu d'une solution il n'y a ni recherche à lancer ni
    // recherche à arrêter.
    pbResoudre->setEnabled(enCours || !timerRejeu.isActive());
}

// Remet la case état-max à zéro : décochée, désactivée, libellé sans compteur.
// L'état-max appartient à UNE résolution d'UN niveau — changer l'un ou l'autre le
// périme, et gameMax pointerait sur un plateau qui n'a plus rien à voir.
void MainWindow::resetEtatMax() {
    maxRangeesVu = 0;
    cbEtatMax->setChecked(false);
    wGame->setMontreOrdreButs(false);
    cbEtatMax->setEnabled(false);
    cbEtatMax->setText(texteEtatMax);
    recordsVus.clear();     // les records du run précédent ne veulent plus rien dire
    majListeRecords();
}

// Le combo, reconstruit à chaque record. `blockSignals` : repeupler émet
// currentIndexChanged, qui rechargerait un chemin à contretemps.
void MainWindow::majListeRecords() {
    if (!cbRecords) return;
    const int garde = cbRecords->currentIndex();
    cbRecords->blockSignals(true);
    cbRecords->clear();
    for (int i = 0; i < recordsVus.size(); i++)
        cbRecords->addItem(QString("record %1/%2 — %3 coups")
                               .arg(recordsVus[i].first).arg(gameMax.getNbButs())
                               .arg(recordsVus[i].second.size()));
    if (garde >= 0 && garde < recordsVus.size()) cbRecords->setCurrentIndex(garde);
    cbRecords->blockSignals(false);
    cbRecords->setVisible(recordsVus.size() > 1);
}

void MainWindow::onIALance() {
    // Second clic pendant une recherche : on bascule en arrêt. Le thread ne
    // s'arrête pas dans l'instant — il finit le dépilement en cours puis rend la
    // main par rechercheArretee(). D'ici là le bouton est neutralisé, pour qu'un
    // troisième clic ne relance pas une résolution par-dessus.
    if (solveur) {
        solveur->demanderArret();
        pbResoudre->setEnabled(false);
        pbResoudre->setText("Arrêt…");
        return;
    }

    setControlesActifs(false);
    wGame->setDuree(0.0);

    // Nouvelle résolution : on repart d'un état-max vierge.
    resetEtatMax();

    const auto type = static_cast<Solveur::EType>(cbSolveur->currentData().toInt());
    // L'état sur lequel le solveur part : c'est l'origine de tout chemin qu'il
    // renverra. À figer ici, car 'game' bouge si on navigue pendant le run.
    departSolveur = game;
    solveur = Solveur::creer(type, game, this);
    connect(solveur, &Solveur::solutionTrouvee, this, &MainWindow::onSolutionTrouvee);
    connect(solveur, &Solveur::aucuneSolution, this, &MainWindow::onAucuneSolution);
    connect(solveur, &Solveur::rechercheArretee, this, &MainWindow::onArretRecherche);
    connect(solveur, &Solveur::nouveauMaxCaisses, this, &MainWindow::onNouveauMax);
    connect(solveur, &QThread::finished, solveur, &QObject::deleteLater);
    solveur->start();
    majSpinner();
    majBoutonResoudre();   // 'solveur' existe seulement maintenant : le bouton passe en « Arrêter »

    begin = chrono::high_resolution_clock::now();
}

void MainWindow::onSolutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores) {
    solveur = nullptr;
    majSpinner();
    majBoutonResoudre();   // plus rien à arrêter, y compris derrière la boîte de dialogue

    // Le point de départ du chemin est celui sur lequel le solveur a été LANCÉ, pas
    // 'game' : depuis que la navigation pas à pas existe, 'game' a pu être déplacé
    // pendant le run (on peut visionner le meilleur chemin sans attendre la fin).
    // Prendre 'game' ici rejouerait la solution depuis un état qui n'est pas le sien.
    chargeCheminVisionne(departSolveur, chemin, true);
    game = departSolveur;
    wGame->setGame(&game);
    derniereSolutionEtats = etatsExplores;
    pbRevoir->setEnabled(true);

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> diff = end - begin;

    wGame->setEtatsExplores(etatsExplores);
    wGame->setDuree(std::ceil(diff.count()));

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, "Solveur",
        QString("Solution trouvée : %1 coups (%2 états explorés).\nVoir la résolution ?")
            .arg(chemin.size())
            .arg(WGame::formaterMillier(etatsExplores)));

    if (reponse == QMessageBox::Yes) {
        coupsRestants = chemin;
        timerRejeu.start();   // laisse les contrôles désactivés le temps du rejeu
        majBoutonResoudre();  // ... le bouton de résolution compris
    } else {
        setControlesActifs(true);
    }
}

void MainWindow::onAucuneSolution() {
    solveur = nullptr;
    majSpinner();
    setControlesActifs(true);
    wGame->setEtatsExplores(0);
    QMessageBox::information(this, "Solveur", "Aucune solution trouvée pour ce niveau.");
}

// Arrêt à la demande : pas de boîte de dialogue — l'utilisateur vient de cliquer
// pour reprendre la main, la lui redemander serait absurde. On garde en revanche
// les états explorés, la durée et l'état-max atteint : c'est précisément le
// diagnostic pour lequel on interrompt une recherche qui n'aboutit pas.
void MainWindow::onArretRecherche(qint64 etatsExplores) {
    solveur = nullptr;
    majSpinner();
    setControlesActifs(true);

    const std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - begin;
    wGame->setEtatsExplores(etatsExplores);
    wGame->setDuree(std::ceil(diff.count()));
    wGame->update();
}

void MainWindow::onRevoir() {
    if (timerRejeu.isActive() || derniereSolutionCoups.isEmpty()) return;

    game = derniereSolutionDepart;
    historique.clear();   // on repart du départ de la solution : l'undo humain d'avant n'a plus de sens
    coupsRestants = derniereSolutionCoups;

    // On repart du départ : le compteur aussi (1 sous chaque caisse), sinon deux
    // visionnages cumulent leurs passages.
    initPassages();

    wGame->setGame(&game);
    wGame->setEtatsExplores(derniereSolutionEtats);
    wGame->setPassages(passages);
    majSurcouches(game);

    setControlesActifs(false);
    pbRevoir->setEnabled(false);
    centrerSurJoueur();

    posPas = 0;
    timerRejeu.start();
    majNavigationPas();   // grise la navigation manuelle pendant le rejeu auto
}

void MainWindow::rejouerCoup() {
    if (coupsRestants.isEmpty()) {
        timerRejeu.stop();
        setControlesActifs(true);
        pbRevoir->setEnabled(!derniereSolutionCoups.isEmpty());
        // Le rejeu automatique laisse le plateau à la FIN du chemin : la navigation
        // pas à pas doit repartir de là, pas d'une position périmée.
        posPas = derniereSolutionCoups.size();
        majNavigationPas();
        return;
    }

    Game::EDirection dir = coupsRestants.takeFirst();
    joue(dir);
    wGame->update();
}

// Installe un chemin à visionner : soit une solution, soit — et c'est le cas qui
// motive tout ceci — le meilleur chemin d'un run qui n'aboutit PAS. Les deux se
// rejouent exactement pareil ; seul le libellé diffère.
void MainWindow::chargeCheminVisionne(const Game& depart,
                                      const QList<Game::EDirection>& coups,
                                      bool estSolution) {
    derniereSolutionDepart = depart;
    derniereSolutionCoups  = coups;
    cheminEstSolution      = estSolution;

    // Repérage des poussées : on REJOUE le chemin sur une copie et on note les
    // coups où le compteur de caisses déplacées bouge. Le relire depuis l'état
    // courant serait faux — une case libre au coup 12 ne l'est plus au coup 200.
    indicesPoussees.clear();
    Game g(depart);
    int caisses = g.getNbDepCaisse();
    for (int i = 0; i < coups.size(); i++) {
        if (!g.deplace(coups[i])) break;   // chemin incohérent : on s'arrête là
        if (g.getNbDepCaisse() > caisses) { indicesPoussees.append(i); caisses = g.getNbDepCaisse(); }
    }

    // Le plateau affiché est celui du DÉPART (le solveur a travaillé sur sa propre
    // copie, 'game' n'a pas bougé) : la position doit dire 0, pas la fin, sinon le
    // libellé annonce un état que la grille ne montre pas.
    posPas = 0;
    majNavigationPas();
    // Rappel de la touche : sans lui, `C` n'est découvrable nulle part (pas de
    // légende hors session d'annotation d'intentions).
    qDebug().noquote() << "[chemin]" << coups.size() << "coups," << indicesPoussees.size()
                       << "poussées —" << (estSolution ? "SOLUTION" : "état max, PAS une solution")
                       << "| ◀ ▶ / slider / Maj pour naviguer, C = critiquer ce coup";
}

// Relit le journal hybride du niveau courant et installe sa DERNIÈRE partie GAGNÉE
// dans le rejeu pas à pas (cf. mainwindow.h). Rien de neuf côté navigation : on
// remplit le même chemin visionné que le solveur, les flèches ◀ ▶ font le reste.
void MainWindow::rejoueJournal() {
    const QString nom = QString("hybride_niveau_%1.txt").arg(game.getNumNiveau(), 4, 10, QChar('0'));
    QFile f(QDir::current().filePath(nom));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug().noquote() << "[rejeu] pas de journal" << nom << "dans" << QDir::currentPath();
        return;
    }
    const QStringList lignes = QString::fromUtf8(f.readAll()).split('\n');

    // On ne garde que la DERNIÈRE partie gagnée : un journal est en AJOUT, il
    // contient les tentatives abandonnées, qui n'ont pas d'intention à annoter.
    int debut = -1, fin = lignes.size();
    for (int i = 0; i < lignes.size(); i++) {
        if (!lignes[i].startsWith("=== niveau")) continue;
        int j = i + 1;
        while (j < lignes.size() && !lignes[j].startsWith("=== niveau")) j++;
        for (int k = i; k < j; k++)
            if (lignes[k].contains("etat gagne")) { debut = i; fin = j; break; }
    }
    if (debut < 0) { qDebug().noquote() << "[rejeu] aucune partie GAGNEE dans" << nom; return; }

    // Les coups, lus sur le déplacement du joueur. Un [undo] retire le dernier :
    // la partie du 13 en portait quatre, et les ignorer décalerait tout le reste.
    QList<Game::EDirection> coups;
    QVector<bool> macro;
    bool dansMacro = false;
    const QRegularExpression re("\\[mouv\\] joueur \\((\\d+),(\\d+)\\)->\\((\\d+),(\\d+)\\)");
    for (int i = debut; i < fin; i++) {
        const QString& l = lignes[i];
        if (l.startsWith("[macro] LANCEE"))   { dansMacro = true;  continue; }
        if (l.startsWith("[macro] TERMINEE")) { dansMacro = false; continue; }
        if (l.startsWith("[undo]")) {
            if (!coups.isEmpty()) { coups.removeLast(); macro.removeLast(); }
            continue;
        }
        const QRegularExpressionMatch m = re.match(l);
        if (!m.hasMatch()) continue;
        const int dx = m.captured(3).toInt() - m.captured(1).toInt();
        const int dy = m.captured(4).toInt() - m.captured(2).toInt();
        Game::EDirection d;
        if      (dy == -1 && dx == 0) d = Game::dHaut;
        else if (dy ==  1 && dx == 0) d = Game::dBas;
        else if (dx ==  1 && dy == 0) d = Game::dDroite;
        else if (dx == -1 && dy == 0) d = Game::dGauche;
        else continue;                       // saut : ligne illisible, on l'ignore
        coups.append(d);
        macro.append(dansMacro);
    }
    if (coups.isEmpty()) { qDebug().noquote() << "[rejeu] aucun coup lisible"; return; }

    coupIssuMacro = macro;
    chargeCheminVisionne(gameDepart, coups, true);
    // La zone du joueur s'arme AVEC le rejeu : c'est là, et seulement là, qu'on
    // s'en sert (trancher `O` — « le perso couvre plus de cases ou d'autres
    // cases » — contre `E`). Pas de bascule : cf. le bloc `Z` retiré dans
    // eventFilter.
    wGame->showZoneJoueur(true);

    // Le journal d'annotation est SÉPARÉ du journal de jeu : celui-ci est une
    // donnée brute qu'on relit, on n'y réécrit pas.
    if (journalIntentions.isOpen()) journalIntentions.close();
    journalIntentions.setFileName(QDir::current().filePath(
        QString("hybride_niveau_%1_intentions.txt").arg(game.getNumNiveau(), 4, 10, QChar('0'))));
    if (journalIntentions.open(QIODevice::Append | QIODevice::Text)) {
        journalIntentions.write(QString("\n=== niveau %1 — annotation du %2 — %3 coups (%4 par macro) ===\n")
                                    .arg(game.getNumNiveau())
                                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                                    .arg(coups.size()).arg(macro.count(true)).toUtf8());
        journalIntentions.flush();
    }
    intentionCourante.clear();
    pileAnnotations.clear();      // l'annulation ne franchit pas une session
    pileIntentionAvant.clear();
    qDebug().noquote() << "[rejeu]" << coups.size() << "coups," << macro.count(true)
                       << "issus d'une macro — ◀ ▶ ou le slider pour naviguer,"
                       << "E/O/G/T/R/? = intention (plusieurs touches = plusieurs intentions)";
}

// ⚠️ `prochainCoupChoisi()` SUPPRIMÉE avec la touche `N` (2026-08-02) — plus aucun
// appelant, et du code mort dans un outil de chantier ne se garde pas. Ce qu'elle
// savait, et qui reste vrai si on la ressort un jour : une décision se prend là où
// il y a une POUSSÉE **et** pas de macro, les DEUX conditions étant nécessaires —
// « hors macro » seul laisse la marche du joueur (1 079 arrêts sur le 25 au lieu de
// 243), « poussée » seule rend les 520 poussées de macro du 24, qui ne sont pas des
// choix. Mesuré à l'époque, pas déduit. Le dépouillement hors ligne applique le même
// critère, c'est lui qui le porte désormais.

// Une intention, en vocabulaire FERMÉ. Elle vaut jusqu'à la suivante — on annote un
// PLAN, pas un coup. Le numéro de coup est ce qui permet de recouper avec le
// journal d'origine ; sans lui l'annotation serait orpheline.
void MainWindow::noteIntention(const QString& code, const QString& libelle) {
    if (!journalIntentions.isOpen()) return;
    pousseAnnulation();
    int posees = 0;
    for (int k = 0; k < game.getNbButs(); k++)
        if (game.getCase(game.getCaseBut(k)) == Level::tcGoalCaisse) posees++;
    const QString ligne = QString("[intention] coup %1/%2 | %3 %4 | posees %5/%6 | joueur (%7,%8)")
                              .arg(posPas).arg(derniereSolutionCoups.size())
                              .arg(code).arg(libelle)
                              .arg(posees).arg(game.getNbButs())
                              .arg(game.getPlayerPoint().x()).arg(game.getPlayerPoint().y());
    journalIntentions.write(ligne.toUtf8() + "\n");
    journalIntentions.flush();
    qDebug().noquote() << ligne;

    intentionCourante = code;
    majNavigationPas();               // le libellé rappelle l'intention active
}

// Une CRITIQUE du chemin du solveur, en texte libre (cf. mainwindow.h pour le
// pourquoi du texte libre). Le journal s'ouvre à la première frappe : rien à armer,
// donc rien à oublier d'armer — et il ne peut pas s'ouvrir sans chemin à critiquer.
void MainWindow::noteCritique() {
    if (derniereSolutionCoups.isEmpty()) {
        statusBar()->showMessage("Aucun chemin à critiquer — lance un solve, "
                                 "puis arrête-le une fois l'état max atteint.", 4000);
        return;
    }
    bool ok = false;
    const QString texte = QInputDialog::getText(
        this, "Critique du chemin du solveur",
        QString("Coup %1/%2 — pourquoi est-ce mauvais ?")
            .arg(posPas).arg(derniereSolutionCoups.size()),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || texte.trimmed().isEmpty()) return;

    if (!journalCritique.isOpen()) {
        journalCritique.setFileName(QDir::current().filePath(
            QString("solveur_niveau_%1_critique.txt").arg(game.getNumNiveau(), 4, 10, QChar('0'))));
        if (journalCritique.open(QIODevice::Append | QIODevice::Text)) {
            journalCritique.write(
                QString("\n=== niveau %1 — critique du %2 — chemin de %3 coups (%4 poussees), %5 ===\n")
                    .arg(game.getNumNiveau())
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(derniereSolutionCoups.size()).arg(indicesPoussees.size())
                    .arg(cheminEstSolution ? "SOLUTION"
                                           : QString("etat max %1/%2 — PAS une solution")
                                                 .arg(maxRangeesVu).arg(game.getNbButs()))
                    .toUtf8());
        }
    }
    if (!journalCritique.isOpen()) return;

    int posees = 0;
    for (int k = 0; k < game.getNbButs(); k++)
        if (game.getCase(game.getCaseBut(k)) == Level::tcGoalCaisse) posees++;
    // Combien de POUSSÉES sont derrière nous : le numéro de coup mêle marche et
    // poussées, or c'est la poussée qui situe l'état.
    int pouss = 0;
    for (int i : indicesPoussees) if (i < posPas) pouss++;

    const QString ligne = QString("[critique] coup %1/%2 | poussee %3/%4 | posees %5/%6 | joueur (%7,%8)\n  %9")
                              .arg(posPas).arg(derniereSolutionCoups.size())
                              .arg(pouss).arg(indicesPoussees.size())
                              .arg(posees).arg(game.getNbButs())
                              .arg(game.getPlayerPoint().x()).arg(game.getPlayerPoint().y())
                              .arg(texte.trimmed());
    // Le PLATEAU avec la critique, comme les blocs [manque] : sans lui l'annotation
    // est illisible six mois plus tard, et surtout `mort` ne peut pas la juger. C'est
    // tout ce qui distingue cette campagne de celle des intentions — il y a un oracle,
    // encore faut-il pouvoir le lui donner à manger.
    journalCritique.write(ligne.toUtf8() + "\n" + plateauXsb(game).toUtf8());
    journalCritique.flush();
    qDebug().noquote() << ligne;
    statusBar()->showMessage(QString("Critique notée au coup %1.").arg(posPas), 3000);
}

// Rejoue les n premiers coups depuis le départ du chemin. Jamais d'undo : on
// reconstruit, donc l'état affiché ne peut pas dériver de l'état réel.
void MainWindow::allerAuPas(int n) {
    if (derniereSolutionCoups.isEmpty()) return;
    n = qBound(0, n, (int)derniereSolutionCoups.size());

    game = derniereSolutionDepart;
    historique.clear();          // l'undo humain n'a plus de sens dans un rejeu
    initPassages();

    // timerRejeu est à l'arrêt ici, donc joue() croirait à des coups HUMAINS et
    // empilerait tout l'historique. On joue directement, sans passer par lui —
    // les passages sont recalculés juste après, de toute façon.
    int caisses = game.getNbDepCaisse();
    int joues = 0;
    for (int i = 0; i < n; i++) {
        const QPoint avant = game.getPlayerPoint();
        // Un coup REFUSÉ ne peut venir que de move() : état gagné, ou état déclaré
        // perdu par checkDefaite. On s'arrête, et posPas doit dire où on s'est
        // arrêté RÉELLEMENT — sinon le libellé annonce une position que la grille
        // ne montre pas, et l'outil ment sur ce qu'il affiche.
        if (!game.deplace(derniereSolutionCoups[i])) break;
        joues++;
        if (game.getNbDepCaisse() > caisses) {
            const QPoint delta  = game.getPlayerPoint() - avant;
            const QPoint caisse = game.getPlayerPoint() + delta;
            const int idx = caisse.x() + caisse.y() * game.getLargeur();
            if (idx >= 0 && idx < passages.size()) passages[idx]++;
            caisses = game.getNbDepCaisse();
        }
    }
    posPas = joues;

    wGame->setGame(&game);
    wGame->setPassages(passages);
    majSurcouches(game);
    wGame->update();
    majNavigationPas();
}

void MainWindow::majNavigationPas() {
    const int total = derniereSolutionCoups.size();
    const bool actif = total > 0 && !timerRejeu.isActive();

    pbPasPrec->setEnabled(actif && posPas > 0);
    pbPasSuiv->setEnabled(actif && posPas < total);
    slPas->setEnabled(actif);

    // setValue rappellerait onPasSlider, qui rejouerait le chemin : on coupe le
    // signal le temps de recaler le curseur.
    slPas->blockSignals(true);
    slPas->setRange(0, total);
    slPas->setValue(posPas);
    slPas->blockSignals(false);

    // Les raccourcis ne sont nulle part ailleurs : pas de bouton, pas de menu. Sans
    // ce rappel à l'écran il faudrait les retenir, et un outil dont on oublie les
    // touches ne sert pas. Affiché seulement en mode hybride, où ils fonctionnent.
    if (!total) {
        lbPas->setText(cbHybride->isChecked() ? "—   (L = rejouer le journal hybride)" : "—");
        wGame->setPousseeCourante(-1, false);   // rien à annoter hors d'un rejeu
        return;
    }

    // Combien de poussées ont déjà été jouées à cette position.
    int poussees = 0;
    while (poussees < indicesPoussees.size() && indicesPoussees[poussees] < posPas) poussees++;

    QString txt = QString("%1 : coup %2/%3 — poussée %4/%5")
                      .arg(journalIntentions.isOpen() ? "journal"
                                                      : (cheminEstSolution ? "solution" : "meilleur état"))
                      .arg(posPas).arg(total)
                      .arg(poussees).arg(indicesPoussees.size());

    int caisseCoup = -1; bool coupChoisi = false;
    if (journalIntentions.isOpen()) {
        // Le coup qu'on VIENT de jouer : celui d'indice posPas-1. Dire s'il est un
        // choix ou une macro évite d'annoter au mauvais endroit — une intention sur
        // un coup de macro ne veut rien dire, la macro n'a rien décidé.
        const int i = posPas - 1;
        const bool estPoussee = indicesPoussees.contains(i);
        const bool estMacro   = i >= 0 && i < coupIssuMacro.size() && coupIssuMacro[i];
        if (i >= 0)
            txt += estPoussee ? (estMacro ? "   [poussée de MACRO]" : "   [poussée CHOISIE]")
                              : "   [marche]";

        // ⚠️ CE QUI EST ANNOTABLE DOIT SE VOIR SUR LE PLATEAU (2026-08-02, constat
        // utilisateur : « si une poussée n'est pas la mienne, il faut que je le voie
        // en interface, je ne peux pas le deviner »). L'information existait déjà —
        // dans CE libellé — mais noyée dans une ligne de texte, et surtout muette
        // sur la caisse concernée. On descend donc à WGame la case de la caisse
        // qui vient d'être poussée, plus le fait qu'elle soit un CHOIX ou une macro.
        // La caisse est forcément de l'autre côté du joueur dans le sens du coup :
        // l'état affiché est celui d'APRÈS, donc joueur + direction.
        if (i >= 0 && estPoussee) {
            const QPoint p = game.getPlayerPoint();
            int dx = 0, dy = 0;
            switch (derniereSolutionCoups[i]) {
                case Game::dHaut:   dy = -1; break;
                case Game::dBas:    dy =  1; break;
                case Game::dGauche: dx = -1; break;
                case Game::dDroite: dx =  1; break;
                default: break;
            }
            caisseCoup = (p.x() + dx) + (p.y() + dy) * game.getLargeur();
            coupChoisi = !estMacro;
        }

        // COMBIEN RESTE-T-IL À ANNOTER ? (2026-08-02, demande utilisateur : « est-ce
        // que tu sais quand plus aucun coup n'est à moi ? histoire de gagner du
        // temps »). Sans ça on parcourt la fin d'une partie pour rien — sur le
        // niveau 1, la dernière poussée choisie est au coup 12 sur 256, donc 95 %
        // du rejeu n'a rien à offrir. Compté ici plutôt que mémorisé : la liste des
        // poussées choisies est déjà en mémoire, et un compteur tenu à la main
        // dériverait au premier changement de chemin.
        int reste = 0, dernier = -1;
        for (int j : indicesPoussees)
            if (j < coupIssuMacro.size() && !coupIssuMacro[j]) {
                dernier = j + 1;
                if (j >= posPas) reste++;
            }
        txt += reste ? QString("   |   reste %1 à toi (dern. %2)").arg(reste).arg(dernier)
                     : QString("   |   ✔ PLUS RIEN À ANNOTER");

        // ⚠️ Le rappel du vocabulaire était RÉPÉTÉ ICI alors que la légende (lbLegende)
        // le porte déjà en entier. Retiré le 2026-08-02 : la ligne débordait de
        // l'écran, et surtout il avait fallu corriger le vocabulaire à DEUX endroits
        // le matin même (retrait de `A`, reformulation de `?`) — le signal classique
        // de la copie qui va dériver. Un seul exemplaire, dans la légende.
        if (!intentionCourante.isEmpty()) txt += "   |   en cours : " + intentionCourante;
    }
    // Hors annotation, caisseCoup vaut -1 : la marque s'efface d'elle-même. Un seul
    // appel, sur tous les chemins — une marque périmée dirait « annotable » sur un
    // coup de macro, c'est-à-dire exactement le contraire de ce qu'elle sert à dire.
    wGame->setPousseeCourante(caisseCoup, coupChoisi);

    // APERÇU DES POUSSÉES À VENIR. Simulé sur une COPIE de `game`, jamais sur l'état
    // affiché : on regarde l'avenir sans le jouer. Douze poussées suffisent — au-delà
    // le plateau devient illisible, et un plan d'annotation dépasse rarement ça (le
    // plus long du corpus, le convoyage du niveau 2, en fait quatre).
    QVector<int>  apercuOrdre;
    QVector<bool> apercuChoisi;
    if (journalIntentions.isOpen()) {
        const int taille = game.getLargeur() * game.getHauteur();
        apercuOrdre  = QVector<int>(taille, -1);
        apercuChoisi = QVector<bool>(taille, false);
        Game futur = game;
        int caisses = futur.getNbDepCaisse(), rang = 0;
        for (int i = posPas; i < derniereSolutionCoups.size() && rang < 12; i++) {
            const QPoint avant = futur.getPlayerPoint();
            if (!futur.deplace(derniereSolutionCoups[i])) break;
            if (futur.getNbDepCaisse() > caisses) {
                caisses = futur.getNbDepCaisse();
                const QPoint d = futur.getPlayerPoint() - avant;
                const QPoint c = futur.getPlayerPoint() + d;
                const int k = c.x() + c.y() * game.getLargeur();
                if (k >= 0 && k < taille && apercuOrdre[k] < 0) {
                    apercuOrdre[k]  = ++rang;
                    apercuChoisi[k] = i < coupIssuMacro.size() && !coupIssuMacro[i];
                }
            }
        }
    }
    wGame->setApercuSuite(apercuOrdre, apercuChoisi);
    lbPas->setText(txt);
}

void MainWindow::onPasSuiv() {
    // Maj = sauter à la poussée suivante : entre deux poussées, le joueur ne fait
    // que marcher, et le solveur ne raisonne qu'en poussées.
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        for (int i : indicesPoussees)
            if (i >= posPas) { allerAuPas(i + 1); return; }
        allerAuPas(derniereSolutionCoups.size());
        return;
    }
    allerAuPas(posPas + 1);
}

void MainWindow::onPasPrec() {
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        for (int k = indicesPoussees.size() - 1; k >= 0; k--)
            if (indicesPoussees[k] + 1 < posPas) { allerAuPas(indicesPoussees[k] + 1); return; }
        allerAuPas(0);
        return;
    }
    allerAuPas(posPas - 1);
}

void MainWindow::onPasSlider(int valeur) { allerAuPas(valeur); }

// Export texte de la carte des passages, à côté de la grille du niveau, pour
// pouvoir la lire et l'annoter hors de l'app.
void MainWindow::onExportPassages() {
    const QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter les passages",
        QString("passages_niveau%1.txt").arg(game.getNumNiveau(), 2, 10, QChar('0')),
        "Texte (*.txt)");
    if (chemin.isEmpty()) return;

    QFile f(chemin);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Écriture impossible.");
        return;
    }

    const int L = game.getLargeur(), H = game.getHauteur();
    QTextStream out(&f);

    out << "Niveau " << game.getNumNiveau() << "\n";
    out << "Passages de caisse par case (cumulé : une caisse qui repasse compte à nouveau).\n";
    out << "Poussées jouées : " << game.getNbDepCaisse()
        << "   Déplacements : " << game.getNbDep() << "\n\n";

    // La grille, pour situer.
    out << "-- grille --\n";
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            switch (game.getCase(x + y * L)) {
                case Level::tcMur:        out << '#'; break;
                case Level::tcCaisse:     out << '$'; break;
                case Level::tcGoalCaisse: out << '*'; break;
                case Level::tcGoal:       out << '.'; break;
                case Level::tcPlayer:     out << '@'; break;
                case Level::tcGoalPlayer: out << '+'; break;
                default:                  out << ' '; break;
            }
        }
        out << "\n";
    }

    // Les passages, alignés sur la même grille (3 colonnes par case pour que les
    // nombres à 2 chiffres restent lisibles).
    out << "\n-- passages (3 caracteres par case) --\n";
    int total = 0, pic = 0;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            const int idx = x + y * L;
            if (game.getCase(idx) == Level::tcMur) { out << "###"; continue; }

            const int n = (idx < passages.size()) ? passages[idx] : 0;
            total += n;
            if (n > pic) pic = n;

            if (n == 0) out << "  .";
            else        out << QString("%1").arg(n, 3);
        }
        out << "\n";
    }

    out << "\ntotal des passages : " << total << "   maximum sur une case : " << pic << "\n";
    f.close();

    QMessageBox::information(this, "Export",
        QString("Écrit : %1\n%2 passages, pic à %3 sur une case.")
            .arg(chemin).arg(total).arg(pic));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        if (timerRejeu.isActive()) {
            return QObject::eventFilter(obj, event);   // laisse la souris/clavier tranquille pendant le rejeu
        }

        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        // Une macro en cours joue ses coups depuis une séquence calculée d'avance :
        // un coup humain intercalé la décalerait sans qu'elle s'en aperçoive. Toute
        // touche l'ARRÊTE au lieu d'être ignorée — reprendre la main doit être
        // immédiat, la macro n'est qu'un coup de pouce.
        if (timerMacro.isActive()) {
            timerMacro.stop();
            coupsMacro.clear();
            journal("[macro] ARRETEE par une touche");
            return true;
        }

        // ⚠️ L'undo est neutralisé pendant une session d'annotation, AVANT d'être
        // traité : il défait un coup sans toucher à posPas, donc l'état affiché
        // cesse de correspondre au numéro de coup écrit dans les annotations. Même
        // défaut que les flèches (plus bas) — corrigé pour elles seules au premier
        // jet, alors que Backspace est testé ICI, plus haut, et passait toujours.
        if (keyEvent->key() == Qt::Key_Backspace) {
            // Pendant une annotation, Retour arrière ANNULE LA DERNIÈRE ANNOTATION.
            // Il était simplement neutralisé (il aurait défait un coup sans toucher
            // à posPas) : la touche était donc libre, et sa sémantique naturelle est
            // exactement celle qu'il manquait. ⚠️ Ce n'est pas du confort — sans
            // annulation, deux frappes au même coup sont AMBIGUËS (conjonction ou
            // rature ?), et le corpus porte déjà deux cas qu'on ne sait pas trancher
            // (niv 7 coup 148, niv 3 coup 211). Avec elle, une rature s'annule et
            // deux frappes veulent dire deux intentions, sans convention à retenir.
            if (journalIntentions.isOpen()) {
                if (pileAnnotations.isEmpty()) {
                    qDebug().noquote() << "[rejeu] rien à annuler dans cette session";
                    return true;
                }
                journalIntentions.resize(pileAnnotations.takeLast());
                journalIntentions.flush();
                intentionCourante = pileIntentionAvant.takeLast();
                majNavigationPas();
                qDebug().noquote() << "[rejeu] dernière annotation annulée — reste"
                                   << pileAnnotations.size() << "annulation(s) possible(s)";
                return true;
            }
            annuleCoup();
            return true;
        }

        // REJEU D'UN JOURNAL + ANNOTATION D'INTENTIONS (cf. mainwindow.h). Tout au
        // clavier, rien dans mainwindow.ui : c'est un outil de chantier, et les
        // lettres sont libres (seules les flèches et Retour arrière sont prises).
        if (keyEvent->key() == Qt::Key_L) { rejoueJournal(); wGame->update(); return true; }
        // `C` = critiquer le chemin du SOLVEUR au coup affiché (texte libre). Elle
        // ne vaut QUE hors session d'annotation d'intentions : les deux campagnes
        // écrivent dans deux fichiers et ne doivent pas se mélanger dans une même
        // session — c'est ce mélange qui rendrait les deux corpus incomparables.
        if (keyEvent->key() == Qt::Key_C && !journalIntentions.isOpen()) {
            noteCritique(); return true;
        }
        // ⚠️ `Z` (bascule de la zone du joueur) RETIRÉE le jour même de son ajout,
        // sur constat utilisateur : « aucun intérêt, mais l'overlay dès qu'on a
        // chargé un rejeu avec L ». La zone sert à trancher `O` contre `E`, donc
        // elle est utile PENDANT une annotation et à aucun autre moment — une
        // bascule ne faisait qu'ajouter une touche à retenir et un état à oublier.
        // C'est la leçon du §7 sur la légende, appliquée à une surcouche : ce qui
        // est toujours nécessaire là où on est ne se conditionne pas.
        // ⚠️ `N` (sauter à la poussée choisie suivante) RETIRÉ le 2026-08-02, sur
        // constat utilisateur : « un raccourci trop facile, qui fait rater des
        // étapes ». Un instrument qui propose une cadence l'IMPOSE — on annotait au
        // rythme des arrêts de `N`, donc à la maille de la POUSSÉE, là où l'intention
        // vit à la maille du PLAN. Le test-retest du niveau 7 (même partie annotée
        // deux fois, 0 étiquette identique sur les 7 coups communs, `RAPPROCHER` 12
        // fois dans la passe au `N` contre 0 dans l'autre) a rendu l'effet visible.
        // Navigation restante : ◀ ▶ et le slider, qui n'imposent aucun rythme.
        // Vocabulaire FERMÉ : du texte libre ne serait pas dépouillable. '?' est
        // indispensable — sans lui on étiquette par défaut, et un corpus où tout
        // porte une intention est un corpus faux. Le libellé disait « réflexe » ;
        // corrigé, car personne ne joue au hasard : la touche sert à dire « je ne
        // sais pas ENCORE le nommer », ce qui est un état parfaitement fréquent.
        // ⚠️ `A` (préparer un appui) RETIRÉ : 0 usage sur les 46 étiquettes des huit
        // premiers niveaux. Une entrée jamais prise n'est pas une réserve, c'est du
        // bruit dans la légende.
        static const QHash<int, QPair<QString,QString>> INTENTIONS = {
            { Qt::Key_E, { "ECARTER",  "(du chemin d'une autre caisse)" } },
            { Qt::Key_O, { "OUVRIR",   "(un passage pour le joueur)"    } },
            { Qt::Key_G, { "GARER",    "(pour plus tard, je reviendrai)" } },
            // RETOURNER : « je sors la caisse de la zone pour la reprendre dans le
            // bon sens » (constat utilisateur sur le 4, ou il l'avait classe GARER
            // faute de mieux). Ce n'est pas garer : la caisse revient au meme
            // endroit, approchee d'un autre cote. C'est le RECUL du §3, la seule
            // categorie qui corresponde a une grandeur deja mesuree du projet —
            // `moureel` en fait tout le mou : mou = 2 x (nombre de reculs).
            { Qt::Key_T, { "RETOURNER", "(sortir pour reprendre dans l'autre sens)" } },
            { Qt::Key_R, { "RAPPROCHER", "(du but)"                     } },
            { Qt::Key_Question, { "INCONNU", "(je ne sais pas encore le dire)" } },
        };
        if (INTENTIONS.contains(keyEvent->key()) && journalIntentions.isOpen()) {
            const auto& it = INTENTIONS[keyEvent->key()];
            noteIntention(it.first, it.second);
            return true;
        }

        // ⚠️ Pendant une session d'annotation, les flèches sont NEUTRALISÉES. Elles
        // passent par joue(), qui modifie le plateau sans toucher à posPas : l'état
        // affiché diverge alors du chemin rejoué et le numéro de coup écrit dans les
        // annotations devient faux. Constaté sur le premier corpus (deux intentions
        // au « coup 8 » avec deux positions de joueur différentes), pas déduit.
        // On navigue avec ◀ ▶ / N / le slider ; pour reprendre la main, décocher
        // le mode hybride ou recharger le niveau.
        if (journalIntentions.isOpen() &&
            (keyEvent->key() == Qt::Key_Up   || keyEvent->key() == Qt::Key_Down ||
             keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right)) {
            qDebug().noquote() << "[rejeu] flèches neutralisées pendant l'annotation "
                                  "— naviguer avec ◀ ▶, N ou le slider";
            return true;
        }

        bool moved = false;

        switch (keyEvent->key()) {
            case Qt::Key_Up:    moved = joue(Game::dHaut);   break;
            case Qt::Key_Right: moved = joue(Game::dDroite); break;
            case Qt::Key_Down:  moved = joue(Game::dBas);    break;
            case Qt::Key_Left:  moved = joue(Game::dGauche); break;
            default: break;
        }

        if (moved) {
            wGame->setEtatsExplores(0);   // l'humain joue, plus de stat IA à afficher
            wGame->update();
        }
    }

    return QObject::eventFilter(obj, event);
}

void MainWindow::onExportXsb() {
    // Exporte le plateau AFFICHÉ (état-max si la case est cochée, sinon le plateau
    // courant) au format .xsb, pour l'inspecter/le partager facilement.
    const Game& g = cbEtatMax->isChecked() ? gameMax : game;

    const QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter le plateau (.xsb)",
        QString("plateau_niveau%1.xsb").arg(g.getNumNiveau(), 2, 10, QChar('0')),
        "Sokoban (*.xsb)");
    if (chemin.isEmpty()) return;

    QFile f(chemin);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Écriture impossible.");
        return;
    }

    QTextStream out(&f);
    out << plateauXsb(g);
}

// Le plateau au format .xsb. Exemplaire UNIQUE : l'export manuel et les rapports
// [manque] du journal doivent produire exactement les mêmes octets, sinon une
// fixture recopiée depuis un journal ne se rejoue pas comme celle du bouton.
QString MainWindow::plateauXsb(const Game& g) const {
    const int L = g.getLargeur(), H = g.getHauteur();
    QString s;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            switch (g.getCase(x + y * L)) {
                case Level::tcMur:        s += '#'; break;
                case Level::tcCaisse:     s += '$'; break;
                case Level::tcGoalCaisse: s += '*'; break;
                case Level::tcGoal:       s += '.'; break;
                case Level::tcPlayer:     s += '@'; break;
                case Level::tcGoalPlayer: s += '+'; break;
                default:                  s += ' '; break;
            }
        }
        s += "\n";
    }
    return s;
}

void MainWindow::onShowPassagesCaisse() {
    wGame->showPassage(cbNotePassages->isChecked());
    pbExport->setEnabled(cbNotePassages->isChecked());
}

void MainWindow::onShowChampButActif() {
    // Le champ n'est pas tenu à jour en continu (contrairement à 'passages',
    // rafraîchi à chaque coup) : le recalculer ICI rattrape le cas où il a
    // périmé pendant que la case était décochée.
    majSurcouches(cbEtatMax->isChecked() ? gameMax : game);
    wGame->showChampButActif(cbDistanceButActif->isChecked());
}

void MainWindow::onCaseCliquee(int idx) {
    // MODE HYBRIDE : un clic sur une caisse SURLIGNÉE lance sa macro au lieu de la
    // montrer. Seulement sur le plateau courant — l'état-max est un instantané du
    // solveur, on n'y joue pas. Hors surlignage, on retombe sur le diagnostic
    // habituel : le mode ne retire rien.
    if (cbHybride->isChecked() && !cbEtatMax->isChecked()
        && idx >= 0 && idx < macroCaissesJouables.size() && macroCaissesJouables[idx]) {
        joueMacro(idx);
        return;
    }

    const Game& g = cbEtatMax->isChecked() ? gameMax : game;

    // Clic sur une case LIBRE : le perso s'y rend. Une salle de buts se traverse en
    // vingt flèches qui n'apprennent rien à personne — ni au joueur, ni au journal.
    // Rien à inventer : c'est l'AStar de marche que le solveur utilise déjà pour
    // recoller ses poussées (Solveur::reconstruire), et les coups partent dans le
    // même tuyau qu'une macro, donc annulables un par un.
    if (!cbEtatMax->isChecked() && (g.getCase(idx) == Level::tcGoal || g.getCase(idx) == Level::tcNone)) {
        marcheVers(idx);
        return;
    }

    if (g.getCase(idx) != Level::tcCaisse && g.getCase(idx) != Level::tcGoalCaisse) return;

    const QVector<int> chemin = g.cheminMacro(idx);
    if (chemin.isEmpty()) return;   // aucun but actif (état gagné) : rien à montrer

    // setChecked() déclenche onShowChampButActif (champ PAR DÉFAUT) si la case
    // n'était pas déjà cochée : le pousser AVANT de poser 'chemin', pour que ce
    // soit bien lui qui reste affiché ensuite, pas le champ par défaut qui
    // vient de l'écraser.
    if (!cbDistanceButActif->isChecked())
        cbDistanceButActif->setChecked(true);

    const int b = g.butActif();
    wGame->setChampButActif(chemin, b >= 0 ? g.getCaseBut(b) : -1);
    wGame->showChampButActif(true);

    // Toutes les branches de l'arbre de fork, pas juste celle qui gagne —
    // matérialise « les autres chemins qui marchent aussi », en aplat bleu.
    wGame->setArbreMacro(g.arbreMacro(idx));
}

// Le perso marche jusqu'à 'idx' (cf. mainwindow.h). Aucune caisse n'est poussée :
// AStar ne traverse pas les caisses, donc une case injoignable rend un chemin vide
// et on ne joue rien.
void MainWindow::marcheVers(int idx) {
    if (timerRejeu.isActive() || timerMacro.isActive()) return;

    const QPoint cible(idx % game.getLargeur(), idx / game.getLargeur());
    if (cible == game.getPlayerPoint()) return;

    coupsMacro = AStar(&game).getChemin(game.getPlayerPoint(), cible);
    if (coupsMacro.isEmpty()) {
        journal(QString("[marche] (%1,%2) INJOIGNABLE a pied").arg(cible.x()).arg(cible.y()));
        return;
    }

    journal(QString("[marche] vers (%1,%2) : %3 pas").arg(cible.x()).arg(cible.y()).arg(coupsMacro.size()));
    timerMacro.start();
}

// « Il aurait dû y avoir une macro ici » (cf. mainwindow.h). Le rapport doit se
// suffire à lui-même : dans six mois, relire le journal doit permettre de rejouer
// le cas sans avoir à se souvenir de la partie. D'où le plateau joint, et surtout
// la CAUSE — la question intéressante n'est pas « où » mais « pourquoi pas ».
void MainWindow::onCaseSignalee(int idx) {
    const Game& g = cbEtatMax->isChecked() ? gameMax : game;
    if (!g.isLoaded() || idx < 0 || idx >= g.getLargeur() * g.getHauteur()) return;
    pousseAnnulation();   // UNE marque pour tout le rapport : il s'annule d'un bloc

    if (signales.size() != g.getLargeur() * g.getHauteur())
        signales = QVector<bool>(g.getLargeur() * g.getHauteur(), false);
    signales[idx] = true;
    wGame->setSignales(signales);

    const int x = idx % g.getLargeur(), y = idx / g.getLargeur();
    const int but = g.butActif();

    if (but < 0) {
        journalSignalement(QString("[manque] (%1,%2) — aucun but actif (etat gagne)").arg(x).arg(y));
        return;
    }

    const int caseBut = g.getCaseBut(but);
    int rangBut = -1;
    const QVector<int>& ordre = g.getOrdreButs();
    for (int k = 0; k < ordre.size(); k++) if (ordre[k] == but) { rangBut = k; break; }

    // Le numéro de coup n'est ajouté que pendant une annotation — hors rejeu il ne
    // veut rien dire (posPas n'indexe aucun chemin). C'est lui qui rattache le
    // signalement au plan en cours, exactement comme pour une intention.
    journalSignalement(QString("[manque] SIGNALE (%1,%2) | but actif (%3,%4) rang %5/%6%7")
                .arg(x).arg(y)
                .arg(caseBut % g.getLargeur()).arg(caseBut / g.getLargeur())
                .arg(rangBut).arg(g.getNbButs())
                .arg(journalIntentions.isOpen()
                         ? QString(" | coup %1/%2").arg(posPas).arg(derniereSolutionCoups.size())
                         : QString()));

    const Level::ETypeCase c = g.getCase(idx);
    if (c != Level::tcCaisse && c != Level::tcGoalCaisse) {
        journalSignalement("[manque]   (pas une caisse — repere pose sur l'etat)");
    } else {
        // Le diagnostic, dans l'ordre exact où le solveur abandonne : poussable du
        // tout ? premier pas ? descente complète ? Chaque étage rend une cause
        // DIFFÉRENTE, et c'est cette distinction qui a de la valeur — « la macro
        // n'est pas là » n'apprend rien, « la caisse n'est poussable dans aucune
        // direction » ou « la descente bloque en (3,12) avec 7 restants » si.
        const QVector<bool>   zone    = g.getZoneJoueur();
        const QVector<quint8> caisses = g.getCaissesDeplacable(zone);

        if (idx >= caisses.size() || caisses[idx] == 0) {
            journalSignalement("[manque]   cause : caisse poussable dans AUCUNE direction "
                    "(zone du joueur / cases mortes)");
        } else if (!g.macroPeutDemarrer(idx, but, zone)) {
            // Les deux causes que « ECHEC AU PAS 0 » confondait (game.h) : le joueur
            // du mauvais côté, ou un détour non-monotone que la descente ne sait pas
            // faire. C'est Game qui tranche (il relâche la contrainte de zone sur la
            // condition de descente) ; ici on ne fait que nommer les directions.
            QVector<QPair<int,int>> dirsAppui;
            const int L = g.getLargeur();
            switch (g.diagnosticPas0(idx, but, zone, &dirsAppui)) {
            case Game::Pas0JoueurMauvaisCote: {
                QStringList quoi;
                for (const auto& da : dirsAppui)
                    quoi << QString("%1 (appui (%2,%3))")
                                .arg(nomDirection((Game::EDirection)da.first))
                                .arg(da.second % L).arg(da.second / L);
                journalSignalement(QString("[manque]   cause : ECHEC AU PAS 0 / LE JOUEUR EST DU MAUVAIS COTE "
                                "— %1 ferait baisser la distance, mais l'appui est HORS de sa zone")
                            .arg(quoi.join(", ")));
                break;
            }
            case Game::Pas0DetourRequis:
                journalSignalement("[manque]   cause : ECHEC AU PAS 0 / DETOUR NON-MONOTONE REQUIS — aucune "
                        "direction ne baisse la distance, meme joueur place ou l'on veut");
                break;
            case Game::Pas0ButInatteignable:
                journalSignalement("[manque]   cause : ECHEC AU PAS 0 — distance au but INFINIE depuis la "
                        "region ou se trouve le joueur");
                break;
            case Game::Pas0HorsRegion:
                journalSignalement("[manque]   cause : ECHEC AU PAS 0 — la caisse n'est dans aucune region "
                        "atteignable par le joueur");
                break;
            default:
                // Pas0Demarre / Pas0DejaSurBut ici = les deux fonctions divergent.
                journalSignalement("[manque]   ⚠ diagnosticPas0 dit que la macro demarre, "
                        "macroPeutDemarrer dit le contraire — INCOHERENCE");
                break;
            }
        } else {
            Game f(g);
            QVector<QPair<int,int>> poussees;
            qint64 essais = 0;
            const bool ok = f.macroVersButBacktrack(idx, but, poussees, &essais);

            if (ok && f.isPerdu()) {
                journalSignalement(QString("[manque]   cause : la macro ABOUTIT (%1 poussees) mais l'etat "
                                "est declare PERDU par checkDefaite").arg(poussees.size()));
            } else if (ok) {
                // Ne devrait pas arriver : l'overlay l'aurait cerclée en vert.
                journalSignalement("[manque]   ⚠ la macro est en fait JOUABLE — overlay perime ?");
            } else {
                // Où la descente meurt. cheminMacro rejoue la version GLOUTONNE (elle
                // sert aux outils de diagnostic UI, cf. game.h) : la case la plus
                // avancée est celle dont la distance restante est la plus faible.
                const QVector<int> chemin = g.cheminMacro(idx);
                int best = -1, reste = -1;
                for (int k = 0; k < chemin.size(); k++)
                    if (chemin[k] >= 0 && (reste < 0 || chemin[k] < reste)) { reste = chemin[k]; best = k; }

                if (best >= 0)
                    journalSignalement(QString("[manque]   cause : DESCENTE BLOQUEE en (%1,%2), il restait %3 "
                                    "de distance (%4 branche%5 essayee%5)")
                                .arg(best % g.getLargeur()).arg(best / g.getLargeur())
                                .arg(reste).arg(essais).arg(essais > 1 ? "s" : ""));
                else
                    journalSignalement(QString("[manque]   cause : descente echouee sans trajet lisible "
                                    "(%1 branche%2 essayee%2)").arg(essais).arg(essais > 1 ? "s" : ""));
            }
        }
    }

    // Le plateau, UNE SEULE FOIS par état : signaler cinq caisses au même endroit
    // est le cas normal (c'est bien tout un état qu'on trouve fautif), et cinq
    // copies du même plateau noieraient les cinq causes, qui sont l'information.
    if (plateauJournalise) return;
    plateauJournalise = true;

    // ⚠️ Le plateau est joint pour RELIRE le cas, pas pour le recharger comme un
    // niveau : ouvrir une position de milieu de partie recalcule tout le statique
    // (ordreButs, casesMortes, distanceParBut) sur les caisses de CET état-là, et
    // l'ordre affiché ne serait plus celui du niveau (§7 du plan).
    journalSignalement("[manque]   plateau (lecture — pas une fixture d'ordre) :");
    const QStringList lignes = plateauXsb(g).split('\n');
    for (const QString& l : lignes)
        if (!l.isEmpty()) journalSignalement("[manque]   " + l);
}

// Lance la goal macro d'une caisse (cf. mainwindow.h). Deux étages, comme
// Solveur::reconstruire : la macro rend des POUSSÉES, l'UI joue des COUPS — et le
// trajet de marche entre deux poussées dépend de la position des caisses à cet
// instant, donc il se calcule en rejouant la séquence, jamais d'avance.
void MainWindow::joueMacro(int idxCaisse) {
    if (timerRejeu.isActive() || timerMacro.isActive()) return;

    const int but = game.butActif();
    if (but < 0) return;

    Game copie(game);
    QVector<QPair<int,int>> poussees;
    qint64 essais = 0;
    if (!copie.macroVersButBacktrack(idxCaisse, but, poussees, &essais) || copie.isPerdu()) {
        // L'overlay et le clic lisent la même table : y arriver signalerait qu'elle
        // a périmé sans être rafraîchie. On le dit plutôt que de l'avaler.
        journal("[macro] REFUSEE : plus jouable depuis cet etat (overlay perime ?)");
        return;
    }

    const int caseBut = game.getCaseBut(but);
    journal(QString("[macro] LANCEE caisse (%1,%2) -> but (%3,%4) : %5 poussees, %6 branche%7 essayee%7")
               .arg(idxCaisse % game.getLargeur()).arg(idxCaisse / game.getLargeur())
               .arg(caseBut % game.getLargeur()).arg(caseBut / game.getLargeur())
               .arg(poussees.size()).arg(essais).arg(essais > 1 ? "s" : ""));

    // Descente poussées -> coups, même recette que Solveur::reconstruire : on
    // marche jusqu'à l'appui, puis on pousse. 'g' rejoue la séquence en parallèle
    // pour que chaque trajet de marche parte du bon plateau.
    coupsMacro.clear();
    Game g(game);
    for (const auto& p : poussees) {
        const Game::EDirection dir = (Game::EDirection)p.second;
        const QPoint appui(p.first % g.getLargeur() + Solveur::appuis[dir].dx,
                           p.first / g.getLargeur() + Solveur::appuis[dir].dy);

        const QList<Game::EDirection> marche = AStar(&g).getChemin(g.getPlayerPoint(), appui);
        coupsMacro += marche;
        coupsMacro.append(dir);

        journal(QString("[macro]   poussee caisse (%1,%2) vers %3 (appui (%4,%5), %6 pas de marche)")
                   .arg(p.first % g.getLargeur()).arg(p.first / g.getLargeur())
                   .arg(nomDirection(dir))
                   .arg(appui.x()).arg(appui.y()).arg(marche.size()));

        g.pousse(p.first, dir);
    }

    journal(QString("[macro] %1 coups a jouer").arg(coupsMacro.size()));
    timerMacro.start();
}

void MainWindow::avanceMacro() {
    if (coupsMacro.isEmpty()) {
        timerMacro.stop();
        journal("[macro] TERMINEE");
        majNavigationPas();
        return;
    }

    // joue() en régime HUMAIN (timerRejeu à l'arrêt) : l'undo empile coup par coup,
    // les passages comptent, la trace [mouv] sort. Une macro se défait donc à la
    // touche Retour arrière comme n'importe quelle suite de coups joués à la main.
    const Game::EDirection dir = coupsMacro.takeFirst();
    if (!joue(dir)) {
        // Un coup refusé en pleine macro veut dire que la descente et le jeu ne
        // voient pas le même plateau : on s'arrête net plutôt que de continuer sur
        // une séquence décalée.
        coupsMacro.clear();
        timerMacro.stop();
        journal("[macro] INTERROMPUE : coup refuse par le jeu");
        return;
    }
    wGame->setEtatsExplores(0);
    wGame->update();
}

void MainWindow::onToggleHybride(int state) {
    wGame->setModeHybride(state == Qt::Checked);
    majNavigationPas();      // le libellé du rejeu porte aussi le rappel de « L »
    // Ferme le journal si on décoche, l'ouvre (et réimprime l'ordre, qui est sa
    // première ligne utile) si on coche en cours de partie.
    journalOuvre(game);
    if (state == Qt::Checked) majRangsButs(game);
    majSurcouches(cbEtatMax->isChecked() ? gameMax : game);
    wGame->update();
}

void MainWindow::onNouveauMax(Game etatMax, int nbRangees, QList<Game::EDirection> chemin) {
    gameMax = etatMax;
    maxRangeesVu = nbRangees;
    cbEtatMax->setEnabled(true);
    cbEtatMax->setText(QString("%1 (%2/%3)")
                           .arg(texteEtatMax).arg(nbRangees).arg(gameMax.getNbButs()));

    // Le chemin du MEILLEUR état devient le chemin visionnable : sur un run qui
    // n'aboutira pas, c'est la seule façon de voir comment le solveur en est
    // arrivé là. 'departSolveur' et non 'game' : l'utilisateur peut avoir navigué
    // en pas à pas pendant le run, ce qui a déplacé 'game'.
    if (!chemin.isEmpty()) {
        // Suivait-on le dernier record ? Décidé AVANT d'ajouter le nouveau.
        const bool suivait = !cbRecords || recordsVus.isEmpty()
                             || cbRecords->currentIndex() == recordsVus.size() - 1;
        recordsVus.append(qMakePair(nbRangees, chemin));
        majListeRecords();
        if (suivait) {
            if (cbRecords) cbRecords->setCurrentIndex(recordsVus.size() - 1);
            chargeCheminVisionne(departSolveur, chemin, false);
        }
        pbRevoir->setEnabled(true);
    }
    // Si l'utilisateur regarde déjà l'état-max, le rafraîchir en direct.
    if (cbEtatMax->isChecked()) {
        majSurcouches(gameMax);
        wGame->update();
    }
}

void MainWindow::onToggleEtatMax(int state) {
    // L'ordre de remplissage n'a de sens à lire que sur l'état-max : c'est là qu'on
    // regarde POURQUOI un run n'aboutit pas (cf. wgame.h).
    wGame->setMontreOrdreButs(state == Qt::Checked);
    if (state == Qt::Checked) {
        timerRejeu.stop();               // fige l'affichage sur l'état-max
        wGame->setGame(&gameMax);
        majSurcouches(gameMax);
    } else {
        wGame->setGame(&game);
        majSurcouches(game);
    }
    majSpinner();
    wGame->update();
}
