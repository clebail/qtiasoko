#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMainWindow>
#include <QTimer>
#include "ui_mainwindow.h"
#include "game.h"
#include "solveur.h"
#include<iostream>

using namespace std;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private slots:
    void onNiveauChange(int index);
    void onIALance();
    void onSolutionTrouvee(QList<Game::EDirection> chemin, qint64 etatsExplores);
    void onAucuneSolution();
    // La recherche a été interrompue par un second clic sur le bouton.
    void onArretRecherche(qint64 etatsExplores);
    void onRevoir();
    void rejouerCoup();
    void onExportPassages();
    void onExportXsb();
    void onShowPassagesCaisse();
    void onShowChampButActif();
    // Clic sur une caisse (WGame::caseCliquee) : montre son trajet de macro
    // COMPLET vers le but actif, au lieu du champ par défaut (toutes les
    // caisses, un pas). Sans effet sur une autre case (mur, sol, joueur...).
    void onCaseCliquee(int idx);
    // Clic DROIT (deux doigts, ou Ctrl+clic) : consigne dans le journal un
    // désaccord avec le solveur — « ici il aurait dû y avoir une macro ». Sur une
    // caisse, dit POURQUOI elle n'en a pas ; ailleurs, pose un simple repère. Dans
    // les deux cas le plateau est joint, pour pouvoir rejouer le cas plus tard.
    void onCaseSignalee(int idx);
    // Reçoit du solveur l'état où un nouveau record de caisses posées est atteint,
    // et le chemin qui y mène (rejouable pas à pas).
    void onNouveauMax(Game etatMax, int nbRangees, QList<Game::EDirection> chemin);
    // Navigation pas à pas dans le chemin visionné. Maj enfoncée = saut à la
    // POUSSÉE voisine au lieu du coup voisin (le solveur raisonne en poussées : les
    // coups de marche entre deux poussées n'apprennent rien).
    void onPasPrec();
    void onPasSuiv();
    void onPasSlider(int valeur);
    // Bascule l'affichage entre le plateau courant et l'état-max mémorisé.
    void onToggleEtatMax(int state);
    // MODE HYBRIDE (2026-08-01) : arme/désarme les surcouches « ordre de
    // remplissage » et « macros jouables ».
    void onToggleHybride(int state);
    // Joue le coup suivant de la macro lancée au clic. Un coup toutes les
    // dureeRejeu ms, en passant par joue() comme un coup humain : l'undo, les
    // compteurs de passage et la trace [mouv] valent donc aussi pour une macro.
    void avanceMacro();
    // Fait suivre la vue au perso pendant qu'il se déplace.
    void onJoueurDeplace(QPoint centre);
private:
    // Recale la vue sur le perso sans qu'il ait à bouger (ouverture d'un niveau,
    // retour au départ avant un rejeu). Différé d'un tour de boucle : au moment
    // où on l'appelle, le QScrollArea n'a pas encore redimensionné WGame pour le
    // nouveau plateau, et le centre calculé serait celui de l'ancien.
    void centrerSurJoueur();

    // Arme ou lève le voile d'attente. Un seul endroit qui en décide : il dépend
    // de DEUX choses (un solveur tourne, et l'utilisateur ne regarde pas
    // l'état-max), et les quatre appelants oublieraient l'une ou l'autre.
    void majSpinner();

    // Joue un coup et compte le passage de caisse s'il y en a un. Unique point
    // d'entrée : le clavier (humain) et le rejeu (solution) passent tous deux par
    // ici, sinon le compteur ne verrait que la moitié des poussées.
    bool joue(Game::EDirection dir);

    // Défait le dernier coup joué À LA MAIN (touche Retour arrière) : restaure
    // l'état et les compteurs de passage empilés avant ce coup.
    void annuleCoup();

    // Remet le compteur à l'état de DÉPART : 1 sous chaque caisse, 0 ailleurs.
    // Une caisse OCCUPE déjà sa case initiale — partir de 0 ferait afficher 0 sous
    // une caisse qui ne bouge jamais, comme si elle n'existait pas.
    void initPassages();

    // Recalcule le champ de distances vers le but actif de 'g' et le pousse à
    // wGame. À rappeler à chaque fois que le plateau AFFICHÉ change d'état
    // (coup joué, changement de niveau, bascule état-max) — contrairement à
    // 'passages', ce champ n'est pas cumulatif, il ne vaut que pour l'état
    // courant. Sans effet si la checkbox est décochée (juste un peu de calcul
    // perdu, jamais faux) : plus simple que de dupliquer la garde partout.
    //
    // Depuis le mode hybride (2026-08-01) elle rafraîchit AUSSI les macros
    // jouables — d'où le nom. Un seul point d'entrée, parce que les deux
    // surcouches périment exactement au même moment (dès que l'état affiché
    // change) et que les huit appelants en oublieraient une (même argument que
    // majSpinner ci-dessus).
    void majSurcouches(const Game& g);

    // MODE HYBRIDE — le régime d'engagement du solveur (solveurastar.cpp), rejoué
    // sur l'état AFFICHÉ : pour chaque caisse déplaçable, une goal macro vers
    // butActif() est-elle jouable ? Même descente que le solveur
    // (macroVersButBacktrack) et même garde (!isPerdu), sinon l'écran montrerait
    // autre chose que ce qu'il ferait. Coût : ~nbCaisses tentatives par coup joué,
    // là où le solveur en fait 5,3 par ÉTAT développé — négligeable en UI.
    void majMacrosJouables(const Game& g);

    // Rang de remplissage (ordreButs) par case de but. STATIQUE : recalculé au
    // seul chargement du niveau, comme les tables de Game.
    void majRangsButs(const Game& g);

    // MODE HYBRIDE — LE RANG DU COUP HUMAIN (2026-08-01). À quelle place le
    // solveur classerait-il la poussée qu'on vient de jouer à la main ?
    //
    // POURQUOI. La macro pose les caisses, les poussées simples font tout le
    // démêlage (mesuré : 100 % sur 1/2/3/4, 39 % des états du niveau 7 sans
    // aucune macro) — et c'est la SEULE phase où le solveur n'a aucune
    // information directionnelle : les poussées simples ne lisent jamais
    // ordreButs, et le stock de tie-breaks est épuisé. Cette mesure transforme
    // « il n'a aucun guidage » en un NOMBRE. Rang 1-3 en permanence ⇒ le
    // démêlage n'est pas un problème de guidage et il faut chercher ailleurs ;
    // rang 80 ⇒ on connaît l'écart à combler, et le journal devient un corpus
    // étiqueté (état → bon coup) pour juger une heuristique candidate HORS
    // LIGNE, sans écrire une ligne de solveur.
    //
    // 'avant' = l'état d'où part la poussée, 'idxCaisse' = la case où la caisse
    // se tenait ALORS (pas celle où elle atterrit), 'dir' = la direction jouée.
    //
    // MIROIR EXACT de l'enfilage (solveurastar.cpp) : mêmes enfants générés
    // (getCaissesDeplacable, une poussée par direction), mêmes élagages dans le
    // même ordre (isPerdu, corral unitaire, corral-N au même CORRAL_BUDGET), même
    // clé de tri que le comparateur — f croissant, puis g décroissant, puis
    // guidage croissant. Toute divergence ferait mentir le rang.
    //
    // ⚠️ Trois limites, écrites dans le journal plutôt que tues :
    //   - c'est un rang PARMI LES FRÈRES, pas une position dans la file globale.
    //     Ce qui se transporte à la file, c'est Δf (= Δh ici, g étant commun à
    //     tous les frères) : Δf = 0 dit « même palier que le meilleur coup d'ici ».
    //   - le 'g' affiché est le compteur de poussées HUMAIN, pas celui qu'aurait
    //     le solveur sur cet état. Commun à tous les frères, donc sans effet sur
    //     le rang ni sur Δf ; il ne sert qu'à rendre f lisible.
    //   - si une macro s'engage, le solveur ne génère QUE des macros : une
    //     poussée simple jouée à la main n'est alors l'enfant de rien, et il n'y
    //     a pas de rang à donner (ligne HORS REGIME MACRO — qui est elle-même la
    //     mesure du désaccord).
    //
    // EFFET DE BORD UTILE : sur une partie GAGNÉE, tout état traversé est soluble
    // par construction — donc un élagage qui écarte le coup joué est un faux
    // positif PROUVÉ, exactement le raisonnement du juge mesures/fp. Ça étend ce
    // juge aux niveaux NON RÉSOLUS, où fp ne peut pas tourner faute de solution de
    // référence. ⚠️ La preuve ne vaut que si la partie est effectivement gagnée
    // et qu'aucun [undo] n'intervient après : le journal porte les deux, le
    // dépouillement tranche.
    void mesureRangCoup(const Game& avant, int idxCaisse, Game::EDirection dir);

    // JOURNAL DE PARTIE (2026-08-01). Le mode hybride est fait pour rejouer des
    // niveaux à la main et relire la partie APRÈS coup : une console se perd, un
    // fichier par niveau se compare. Toutes les traces de jeu ([mouv], [undo],
    // [hybride], [macro]) passent par ici.
    //
    // Journal FERMÉ (mode décoché) = qDebug, exactement comme avant : le mode ne
    // retire rien à qui joue sans lui.
    void journal(const QString& ligne);
    // Rapport de macro manquante : va dans le journal d'INTENTIONS (avec le numéro
    // de coup) pendant une annotation, dans le journal de jeu sinon. Cf. .cpp.
    void journalSignalement(const QString& ligne);
    // ANNULATION D'UNE ANNOTATION (2026-08-02) : on mémorise la TAILLE du fichier
    // avant chaque écriture, et Retour arrière tronque à la taille précédente. Une
    // pile de tailles suffit — c'est un fichier en ajout, donc tout ce qu'une
    // annotation a écrit est en queue, d'un seul tenant. Un rapport `[manque]` fait
    // quinze lignes et s'annule d'un coup, parce que la marque est posée une fois
    // pour tout le rapport et non par ligne.
    void pousseAnnulation();
    QList<qint64> pileAnnotations;
    QStringList   pileIntentionAvant;
    // (Ré)ouvre le journal du niveau 'g' en AJOUT, précédé d'un en-tête daté. En
    // ajout et pas en écrasement : on rejoue volontiers trois fois le même niveau,
    // et l'en-tête suffit à séparer les parties. Sans effet si le mode est décoché.
    void journalOuvre(const Game& g);
    QFile journalFichier;

    // Clic sur une case LIBRE : le perso s'y rend, par l'AStar de marche. Les coups
    // partent dans le même tuyau qu'une macro (timerMacro), donc animés, journalisés
    // et annulables un par un. Aucune caisse n'est poussée.
    void marcheVers(int idx);

    // Lance la goal macro de la caisse 'idxCaisse' vers le but actif : rejoue la
    // descente sur une copie pour obtenir ses poussées, les redescend en coups de
    // marche (même recette que Solveur::reconstruire), puis les joue un par un.
    // Sans effet si aucune macro n'est jouable depuis cette caisse.
    void joueMacro(int idxCaisse);

    // Les caisses dont une macro est jouable dans l'état affiché, par index de
    // case. Sert au clic : c'est la même donnée que l'overlay, il ne faut pas
    // qu'un clic puisse lancer une macro que l'écran ne montre pas.
    QVector<bool> macroCaissesJouables;
    QVector<int>  rangsButs;
    // Le plateau n'est joint qu'au PREMIER signalement d'un état : les suivants
    // ne rendent que leur cause. Remis à false à chaque changement d'état.
    bool plateauJournalise = false;

    // Cases signalées à la main dans l'état courant. Remises à zéro à chaque
    // changement d'état : un repère ne vaut que là où il a été posé.
    QVector<bool> signales;

    // Le plateau au format .xsb. Partagé par le bouton d'export et les rapports
    // [manque] du journal — un seul exemplaire, sinon les fixtures recopiées d'un
    // journal finiraient par différer de celles du bouton.
    QString plateauXsb(const Game& g) const;

    // Coups restants de la macro en cours d'exécution, et son cadenceur. Séparés
    // de timerRejeu/coupsRestants : celui-là rejoue une SOLUTION du solveur et
    // désactive les contrôles, celui-ci est un coup de pouce pendant une partie à
    // la main, qui doit rester annulable coup par coup.
    QTimer timerMacro;
    QList<Game::EDirection> coupsMacro;

    // passages[case] = nombre de fois qu'une caisse a été poussée SUR cette case
    // depuis le chargement du niveau. CUMULÉ : une caisse qui repasse au même
    // endroit incrémente à nouveau.
    //
    // Vit ici et non dans Game : le solveur clone Game des millions de fois, un
    // QVector par état ferait exploser la mémoire (cf. étape 11).
    QVector<int> passages;

    // Rôle du numéro de niveau dans cbNiveau (Qt::UserRole sert déjà au nom de fichier).
    static constexpr int RoleNumero = Qt::UserRole + 1;

    void setControlesActifs(bool actifs);

    // Recale le bouton de résolution (libellé + activation) sur l'état réel.
    // Il est le seul contrôle à ne PAS suivre setControlesActifs() : pendant une
    // résolution tout est verrouillé, mais lui doit rester cliquable — c'est le
    // seul moyen de rendre la main.
    void majBoutonResoudre();

    // Libellé de repos de pbResoudre, capté depuis mainwindow.ui : le bouton
    // bascule en « Arrêter » pendant la recherche et doit pouvoir y revenir.
    QString texteResoudre;

    // Remet cbEtatMax au repos (décochée, désactivée, libellé sans compteur).
    void resetEtatMax();

    // Libellé de cbEtatMax tel que déclaré dans mainwindow.ui, capté au démarrage :
    // unique source du texte, pour qu'un renommage dans Designer suffise.
    QString texteEtatMax;

    Game game;

    // Historique des coups joués À LA MAIN, pour l'undo (Retour arrière). On empile
    // une COPIE de 'game' (COW : les tables statiques sont partagées, seul l'état de
    // jeu diffère) + les compteurs de passage, AVANT chaque coup humain. Le rejeu
    // automatique (timerRejeu) n'y touche pas ; vidé au (re)chargement du niveau.
    struct CoupHist { Game etat; QVector<int> passages; };
    QVector<CoupHist> historique;

    // État où le solveur a rangé le plus de caisses (diagnostic §10). Mémorisé
    // pour rester valide tant que WGame le pointe.
    Game gameMax;
    int maxRangeesVu = 0;
    Solveur *solveur = nullptr;
    QTimer timerRejeu;
    QList<Game::EDirection> coupsRestants;

    // Dernière solution trouvée pour le niveau courant, conservée intacte
    // (indépendamment de coupsRestants, consommée pendant le rejeu) pour
    // permettre de la revisionner via pbRevoir.
    Game derniereSolutionDepart;
    QList<Game::EDirection> derniereSolutionCoups;
    qint64 derniereSolutionEtats = 0;

    // REJEU PAS À PAS. 'posPas' = nombre de coups de derniereSolutionCoups déjà
    // joués depuis derniereSolutionDepart. On ne défait jamais un coup : aller à la
    // position n rejoue les n premiers coups depuis le départ (allerAuPas). C'est
    // O(n) par saut — quelques centaines de coups, instantané — et surtout ça ne
    // peut pas diverger de l'état réel, contrairement à une pile d'undo à maintenir.
    //
    // Le chemin visionné n'est PAS forcément une solution : sur un run qui
    // n'aboutit pas, c'est celui du meilleur état atteint (nouveauMaxCaisses).
    // 'cheminEstSolution' ne sert qu'à l'affichage du libellé.
    int posPas = 0;
    bool cheminEstSolution = false;
    // État sur lequel le solveur a été lancé : origine de tout chemin qu'il rend.
    // Figé au lancement, car 'game' bouge dès qu'on navigue pendant le run.
    Game departSolveur;
    void allerAuPas(int n);

    // ── REJEU D'UN JOURNAL HYBRIDE + ANNOTATION D'INTENTIONS ────────────────────
    // (§6.2, 2026-08-01). Neuf parties gagnantes ont été jouées à la main ; les
    // re-jouer pour les annoter coûterait des heures. On relit donc le journal et
    // on rejoue ses coups dans le rejeu pas à pas EXISTANT — aucune mécanique de
    // navigation en double.
    //
    // Ce qu'on annote : l'INTENTION, en vocabulaire FERMÉ, et par PLAN (une frappe
    // vaut jusqu'à la suivante) et non par coup — c'est tout l'objet, le rang d'un
    // coup isolé ne voit pas un plan sur plusieurs coups (session « rang du coup
    // humain »). Sur le 25, 243 poussées choisies feraient 243 annotations ; on en
    // attend 20 à 30.
    //
    // ⚠️ OUTIL DE CHANTIER, à retirer avec la campagne.
    Game gameDepart;                  // le plateau initial du niveau, pour rejouer
    QVector<bool> coupIssuMacro;      // parallèle à derniereSolutionCoups
    QFile journalIntentions;
    QString intentionCourante;        // rappelée à l'écran : elle vaut jusqu'à la suivante
    QLabel*  lbLegende = nullptr;     // légende des touches, construite par code
    QWidget* wLegende  = nullptr;     // son conteneur, opaque, tout en bas
    void rejoueJournal();             // touche L
    // (prochainCoupChoisi retirée avec la touche N — cf. mainwindow.cpp)
    void noteIntention(const QString& code, const QString& libelle);
    // Indices, dans derniereSolutionCoups, des coups qui POUSSENT une caisse.
    // Recalculé avec le chemin ; sert aux sauts de poussée à poussée.
    QVector<int> indicesPoussees;
    void majNavigationPas();
    void chargeCheminVisionne(const Game& depart, const QList<Game::EDirection>& coups,
                              bool estSolution);
    std::chrono::time_point<std::chrono::high_resolution_clock> begin;
};
#endif // MAINWINDOW_H
