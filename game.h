#ifndef GAME_H
#define GAME_H

#include <QByteArray>
#include <QHash>
#include <QMetaType>
#include <QPair>
#include <QPoint>
#include <QVarLengthArray>
#include <QVector>
#include "level.h"

#define NB_DIRECTION                4
#define NB_COIN_TO_CHECK            2
#define NB_MUR_TO_CHECK             2

#ifdef INSTRUM_MACRO
#include <vector>
// Instrumentation hors-ligne de la GOAL MACRO (harnais mesures/macro). Ne compile
// que dans les harnais : le code produit ne définit jamais INSTRUM_MACRO.
//
// Question posée : quand macroVersBut() échoue, est-ce parce que la caisse est
// VRAIMENT bloquée, ou parce que la descente a pris arbitrairement l'une des
// plusieurs descentes optimales possibles et s'est peinte dans un coin ? La
// boucle prend la première direction décroissante dans l'ordre de l'énumération
// et ne revient jamais dessus — 'forks' compte les pas où une ALTERNATIVE de même
// coût existait, donc les points où un backtracking aurait eu de quoi mordre.
struct StatsMacro {
    qint64 tentatives = 0, succes = 0;
    qint64 echecRegion = 0;     // le joueur n'est plus dans une région valide
    qint64 echecDistance = 0;   // but inatteignable depuis cette caisse (d < 0)
    qint64 echecBloque = 0;     // aucune poussée n'avance  <- LE CAS INTÉRESSANT
    qint64 echecPousse = 0;     // pousse() a refusé (deadlock au passage)
    qint64 echecAvecFork = 0;   // échec ALORS QU'un choix arbitraire avait eu lieu
    qint64 succesAvecFork = 0;
    qint64 forksTotal = 0;      // nombre de pas offrant >= 2 descentes optimales
    qint64 pasTotal = 0;        // pas réellement joués (succès + échecs)
    qint64 resteAuBlocage = 0;  // somme des distances restantes au moment du blocage
    std::vector<qint64> histoEchecPas;    // à quel pas l'échec survient
    std::vector<qint64> histoSuccesLong;  // longueur des chaînes réussies

    // Prototype du 2026-07-23 (§6.3, « mémoriser les forks ») —
    // macroVersButBacktrack : combien de BRANCHES (essais) une tentative a
    // réellement coûté, et combien de succès n'existaient QUE grâce au
    // retour en arrière (essais > 1 — la descente gloutonne seule aurait
    // échoué là).
    qint64 btTentatives = 0, btSucces = 0;
    qint64 btSuccesApresBacktrack = 0;   // succes ET essais > 1
    qint64 btEssaisTotal = 0;            // pour la moyenne d'essais par tentative
    qint64 btEssaisMax = 0;
};
StatsMacro& statsMacro();
#endif

class Game {
    // Accès aux membres privés pour les tests unitaires (tests/tst_getetat.cpp).
    friend class TestGetEtat;

public:
    typedef enum { dHaut, dDroite, dBas, dGauche} EDirection;
    typedef struct _SDirection {
        int dx, dy;
    }SDirection;

    Game();
    Game(const Level& level, int numNiveau = 1);
    Game(const Game& other);
    Game& operator=(const Game& other);

    // Sémantique de déplacement : vole le tableau 'cases' de 'other', puis remet
    // son pointeur à nullptr. Pas de 'const' sur le paramètre — on doit MODIFIER
    // la source pour lui prendre son pointeur ; un 'const Game&&' compilerait
    // mais forcerait une copie profonde, donc ne servirait à rien.
    //
    // 'noexcept' n'est pas décoratif : std::vector n'utilise le déplacement lors
    // d'une réallocation QUE si le move ctor est noexcept — sinon il recopie
    // silencieusement, pour conserver sa garantie forte en cas d'exception. Sans
    // ce mot-clé, le vecteur qui porte le tas d'A* recopierait profondément tous
    // ses états à chaque doublement, et le move ctor n'aurait servi à rien.
    Game(Game&& other) noexcept;
    Game& operator=(Game&& other) noexcept;

    ~Game();
    bool isLoaded() const;
    bool haut();
    bool droite();
    bool bas();
    bool gauche();
    bool deplace(EDirection dir) { return move(dir); }
    int getNbDep() const { return nbDep; }
    int getNbDepCaisse() const { return nbDepCaisse; }
    int getNbButs() const { return nbButs; }
    // Nombre de caisses posées sur un but. Recompte (O(size)) — pour le BFS, qui
    // porte des Game complets ; A* utilise le retour d'appliqueEtat, gratuit.
    int nbCaissesSurBut() const;
    int getNumNiveau() const { return numNiveau; }
    bool isGagne() const { return gagne; }
    bool isPerdu() const { return perdu; }
    int getLargeur() const { return largeur; }
    int getHauteur() const { return hauteur; }
    QPoint getPlayerPoint() const { return playerPoint; }
    Level::ETypeCase getCase(int idx) const { return cases[idx]; }
    // Zone atteignable par le joueur sans pousser de caisse (flood-fill sur
    // les cases libres). Coûteuse à calculer : à réutiliser via les surcharges
    // ci-dessous quand plusieurs requêtes portent sur le même état (le
    // solveur appelle typiquement getEtat() ET getCaissesDeplacable() par
    // état exploré).
    QVector<bool> getZoneJoueur() const { QVector<bool> v; getZoneJoueur(v); return v; }
    // Même chose dans un tampon FOURNI, réutilisable d'un appel à l'autre : à
    // taille déjà bonne, plus aucune allocation (ni le QVector rendu, ni la file
    // du parcours). C'est la forme du chemin chaud — le flood-fill est le point
    // le plus appelé du solveur. ⚠️ Le tampon doit être détenu en propre : si une
    // copie du QVector traîne ailleurs, le fill() initial détache et réalloue,
    // ce qui annule tout le bénéfice (sans nuire à la correction).
    void getZoneJoueur(QVector<bool>& visite) const;
    // Longueur de la clé d'état, en shorts : les N caisses + la case canonique du
    // joueur. CONSTANTE sur toute une résolution — aucune caisse n'apparaît ni ne
    // disparaît —, ce qui permet au solveur de ranger toutes ses clés bout à bout
    // dans une arène (cf. cle.h).
    int tailleCle() const { return nbCaisses + 1; }
    // Écrit la clé dans 'cle', qui doit pouvoir accueillir tailleCle() shorts.
    // C'est la forme qu'utilise le solveur : elle écrit directement dans l'arène,
    // sans allouer.
    void getEtat(quint16* cle) const { getEtat(cle, getZoneJoueur()); }
    void getEtat(quint16* cle, const QVector<bool>& zone) const;
    // Même clé, en QByteArray (short big-endian par case). Ne sert plus au
    // solveur — un malloc par clé était son principal poste mémoire —, mais reste
    // commode pour comparer deux états à l'unité, dans les tests.
    QByteArray getEtat() const { return getEtat(getZoneJoueur()); }
    QByteArray getEtat(const QVector<bool>& zone) const;
    QVector<quint8> getCaissesDeplacable() const { return getCaissesDeplacable(getZoneJoueur()); }
    QVector<quint8> getCaissesDeplacable(const QVector<bool>& zone) const;
    bool isLibre(const QPoint& p) const;
    // Borne inférieure du nombre de poussées restantes, par COUPLAGE de coût
    // minimal (algorithme hongrois) entre les caisses et les buts.
    //
    // Chaque coût cout[caisse][but] est la distance EXACTE d'une caisse SEULE vers
    // CE but précis, tenant compte de l'accessibilité du joueur (distanceParBut,
    // sous-produit joueur-aware : une caisse coupe le plateau, et selon le côté où
    // il se trouve elle n'est pas poussable dans les mêmes sens). On prend ensuite
    // l'affectation bijective caisses<->buts de somme minimale.
    //
    // Admissible (§7.2) : toute solution réelle réalise une bijection caisses<->buts
    // dont le coût est >= la somme de cette affectation ; retirer les autres caisses
    // ne fait que LIBÉRER le joueur. Domine strictement l'ancienne « chaque caisse
    // vise son but le plus proche » (qui relâchait la contrainte de distinction) :
    // elle corrige les COLLISIONS de buts (N caisses réclamant le même but), erreur
    // dominante des niveaux à beaucoup de caisses.
    // Coupe approchée (§10.5) : vrai si les buts sont remplis dans l'ordre de
    // PROFONDEUR (ordreButs) — un but rempli ne suit jamais un but vide plus
    // profond. Interdit les rangements « dans le désordre » de la salle de buts.
    // ⚠️ Peut rendre insoluble un niveau dont l'ordre optimal contrarie celui-ci
    // (rare, ~1/32 d'après le jeu à la main) → à utiliser en mode anytime avec un
    // repli sans coupe.
    bool remplissageOrdonne() const;
    // Index du but ACTIF (§10.5) : le plus profond (ordreButs) pas encore rempli,
    // ou -1 si tous le sont (état gagnant). C'est la cible de la goal macro.
    //
    // ORDRE DYNAMIQUE (§6.2, chantier 2026-07-31) — quand `ordreDynamique` est armé,
    // on ne rend plus aveuglément le premier but non rempli : on rend le premier but
    // non rempli **encore LIVRABLE depuis l'état courant** (`distanceLivraison`, qui
    // amorce son BFS sur les caisses RÉELLEMENT présentes). L'ordre statique reste la
    // préférence ; il cesse d'être une camisole. C'est ce qui contourne les trois
    // murages connus (13 rang 14, 18 rang 10, 22 rang 25) au lieu de les combattre :
    // la question « existe-t-il un ordre sain COMPLET, décidé avant le premier coup ? »
    // ne se pose plus, elle devient « quel but ensuite, depuis CET état ? ».
    //
    // ⚠️ CADENCE AU JALON, et c'est un choix de COÛT : `distanceLivraison` est chère
    // (BFS de poussées joueur-aware, table (case × zone)), donc on ne rechoisit que
    // lorsque le but actif vient d'être REMPLI — au plus nbButs fois par chemin, jamais
    // par état. `butCourant` est le cache qui porte ce jalon, et il se propage par copie
    // aux états enfants. Limite assumée du premier jet : si le but choisi cesse d'être
    // livrable SANS avoir été rempli, on reste dessus jusqu'au prochain jalon.
    int butActif() const;
    // Arme l'ordre dynamique ci-dessus. Posé sur l'état de départ du solveur, il se
    // propage par copie à toute la recherche. Régime d'ESSAI (§6.2) : jamais le défaut.
    void setOrdreDynamique(bool on) { ordreDynamique = on; butCourant = -1; }
    // Case (index plat) du but d'indice 'indexBut' — même indexation que
    // butActif()/ordreButs. Pour l'UI, qui a besoin d'une position à surligner.
    int getCaseBut(int indexBut) const { return goals[indexBut]; }
    // ordreButs[k] = indice du but à remplir en k-ième (cf. plus bas). Exposé en
    // LECTURE pour le harnais `mesures/ordre`, qui vérifie que cet ordre respecte
    // la précédence par approches (§6.2) — c'est la seule façon de voir POURQUOI la
    // macro se mure sur un niveau donné, sans remettre d'interrupteur d'env dans le
    // chemin chaud (piège §7).
    const QVector<int>& getOrdreButs() const { return ordreButs; }

    // ── LOI DE L'ORDRE (§6.2, 2026-08-03 — idée utilisateur) ────────────────────
    // « Vu du solveur, seul le but ACTIF existe ; les autres buts ne sont que du
    // SOL. » D'où une table de cases mortes PAR BUT au lieu d'une seule :
    //
    //   morte pour le but B  ssi  aucune caisse posée là ne peut être poussée
    //                             jusqu'à B, quelle que soit la région du joueur,
    //   SAUF si la case est ALIGNÉE avec B (même ligne ou même colonne) — auquel
    //   cas elle redevient du sol.
    //
    // ⚠️ CE N'EST PAS UN ÉLAGAGE PROUVÉ, et il ne doit jamais entrer dans
    // `checkDefaite`. Poser une caisse sur un but hors de son tour reste LÉGAL au
    // Sokoban : la règle repose sur la justesse de l'ORDRE, pas sur la géométrie.
    // Elle a été jugée sur 24 parties humaines gagnantes — 0 faux positif sur 19
    // niveaux, et les trois seuls fautifs (12, 14, 15) sont exactement ceux dont on
    // savait déjà l'ordre calculé faux, tous trois guéris par l'ordre humain injecté.
    // C'est donc un test de COHÉRENCE entre un ordre et une partie, pas un test
    // d'ordre absolu : le niveau 6 admet deux ordres valides, et la loi condamne
    // celui des deux qu'on ne lui a pas donné. Régime SÉPARÉ, comme le plongeon.
    //
    // ⚠️ La table n'est pas un sur-ensemble de `casesMortes` : l'exemption
    // d'alignement peut rendre au sol une case globalement morte. C'est sans
    // conséquence — la loi s'AJOUTE à `checkDefaite`, elle ne le remplace pas.
    //
    // Gratuit : `distanceParBut` fait déjà le BFS à rebours par but, sur les murs
    // seuls (aucune caisse, aucun autre but en obstacle) — soit exactement la vue
    // « murs seuls » sous laquelle la loi a été jugée. Il ne reste qu'une réduction
    // booléenne, calculée une fois au chargement comme `casesMortes`.
    bool caseMorteLoi(int idxBut, int cell) const {
        return mortesLoi.at((qsizetype)idxBut * size + cell);
    }
    // La tranche du but 'idxBut', pour l'affichage. Vide si 'idxBut' < 0 (état gagné).
    // ⚠️ Ne rend que le SURPLUS de la loi — les cases déjà mortes dans la table
    // ordinaire en sont retirées. C'est ce qui se lit et se dessine : le reste,
    // `checkDefaite` le coupe depuis toujours, l'afficher en gris ne dirait rien de
    // la loi et noierait le plateau sous le remplissage hors contour.
    QVector<bool> casesMortesLoi(int idxBut) const;
    // Case morte au sens ORDINAIRE (table unique, tous buts confondus) — exposée
    // pour que le juge de la loi puisse en isoler le surplus.
    bool caseMorteOrdinaire(int cell) const { return casesMortes.at(cell); }

    // ── GEL HORS TOUR (§6.2, 2026-08-04) — LA SECONDE MOITIÉ DE LA LOI ──────────
    // Même principe qu'au-dessus, poussé jusqu'au bout : si seul le but ACTIF
    // existe, une caisse posée sur un but de rang SUPÉRIEUR est une caisse sur du
    // SOL. Or une caisse gelée sur du sol est morte — c'est le tout premier élagage
    // du projet, et il n'a jamais tourné ici.
    //
    // Rien de neuf n'est calculé : `caisseGelee`/`bloqueeSurAxe` travaillent déjà
    // sur `estCaisse()`, qui couvre `tcCaisse` ET `tcGoalCaisse`. Le seul obstacle
    // était la boucle de `checkDefaite`, qui ne présente que les `tcCaisse` — pour
    // une raison juste au niveau de la CAISSE (« une caisse gelée sur un but est un
    // morceau de la solution ») et fausse au niveau de la RÉGION : quatre caisses
    // posées trop tôt, collées en carré, scellent onze buts derrière elles. C'est le
    // plateau du 2026-08-04, prouvé mort en 10 états par réduction à une caisse.
    //
    // ⚠️ CE N'EST PAS UNE PREUVE, et le sens de l'erreur est connu : une caisse
    // gelée sur un but de rang supérieur REMPLIT quand même ce but, donc la partie
    // reste gagnable dans l'absolu. On coupe des états réellement gagnables. Même
    // statut que la loi — une exigence d'ORDRE, pas un théorème de géométrie —
    // donc régime SÉPARÉ, et le canari des résolus pour juge.
    //
    // ⚠️ Les buts de rang INFÉRIEUR sont exemptés, comme dans la table : ceux-là
    // sont rangés à leur tour, une caisse gelée dessus est une caisse posée.
    bool geleHorsTour(int idxButActif) const;
    // Rang de remplissage du but 'idxBut' dans `ordreButs` (l'inverse de celui-ci).
    int rangDuBut(int idxBut) const { return rangDeBut.at(idxBut); }
    // Champ de distances vers le BUT ACTIF, SPARSE : une valeur uniquement sur
    // les caisses réellement posées (leur dCur) et sur celles de leurs cases
    // voisines vers lesquelles une poussée est LÉGALE dans l'état courant
    // (dCur-1). Toutes les autres cases valent -1 (rien à afficher).
    //
    // ⚠️ Pas un champ dense sur tout le plateau : une première version
    // affichait dpb[cell][regions[joueurRéel][cell]] pour CHAQUE case,
    // indépendamment les unes des autres. C'était FAUX pour les voisins d'une
    // caisse — regions[joueurRéel][cell] répond à « si le SEUL obstacle du
    // plateau était une caisse ici, dans quelle région tomberait le joueur
    // réel, où qu'il soit ? », pas à « si je pousse VRAIMENT cette caisse
    // jusqu'ici, quelle distance ? ». La bonne référence, après une poussée,
    // c'est la case que la caisse vient de quitter (le joueur s'y tient) — pas
    // la position réelle et figée du joueur. C'est exactement ce que fait
    // avanceVersBut (regions[c][devant], PAS regions[joueur][devant]), donc
    // cette fonction rappelle avanceVersBut lui-même plutôt que de refaire le
    // calcul : chaque valeur affichée est un coup que la macro jouerait
    // réellement, jamais une distance orpheline sur une case que rien ne peut
    // atteindre en un coup légal (mur, appui occupé par une AUTRE caisse...).
    //
    // Vide si aucun but actif (état gagné). À n'appeler que pour l'affichage
    // humain (getZoneJoueur() + un balayage de 'size' cases), jamais dans le
    // solveur.
    QVector<int> champDistanceButActif() const;
    // Trajet COMPLET de la goal macro pour la caisse 'idxCaisse' vers le but
    // actif : rejoue macroVersBut sur une COPIE (ne modifie pas *this) et
    // rend un champ sparse, une valeur (distance restante) sur CHAQUE case
    // traversée — du départ ('idxCaisse') jusqu'à l'arrivée si la macro
    // réussit, ou jusqu'à la case de BLOCAGE si elle échoue en route (auquel
    // cas la dernière valeur du chemin n'est pas 0 : c'est justement ce qui
    // montre où et pourquoi ça coince). Vide si 'idxCaisse' n'est pas une
    // caisse, ou si aucun but n'est actif.
    QVector<int> cheminMacro(int idxCaisse) const;
    // Comme cheminMacro, mais rend TOUTES les branches explorées par les
    // forks (pas juste celle qui gagne) : chaque case visitée par AU MOINS
    // UN chemin monotone (distance strictement décroissante) depuis
    // 'idxCaisse', qu'il mène au but ou se bloque. Sert à visualiser la forme
    // de l'arbre de choix — un pas sans fork n'ajoute qu'une case, un pas
    // avec fork fait bifurquer plusieurs branches à la fois. Coûte une
    // exploration bornée (budget de nœuds) sur des copies jetables ; à
    // n'appeler que pour l'affichage humain, jamais dans le solveur.
    QVector<bool> arbreMacro(int idxCaisse, qint64 budgetNoeuds = 5000) const;
    // POURQUOI LA MACRO NE DÉMARRE PAS (§6.2, 2026-08-01). macroPeutDemarrer rend
    // un booléen ; le journal du mode hybride confondait donc sous « ECHEC AU
    // PAS 0 » deux causes que rien ne permettait de départager (29 des 55 clics
    // droits de la campagne) :
    //   - le JOUEUR est du mauvais côté : une direction ferait bien baisser la
    //     distance, mais l'appui n'est pas dans sa zone ;
    //   - il faudrait un DÉTOUR non-monotone : aucune direction ne baisse la
    //     distance, où que le joueur se place. La descente est strictement
    //     décroissante (avanceVersBut), donc elle ne sait pas le faire.
    // La distinction est obtenue en RELÂCHANT la seule condition de zone — on
    // rappelle avanceVersBut avec une zone totale. Aucune logique dupliquée :
    // c'est le même exemplaire unique de la condition de descente.
    // 'dirsAppui' reçoit, pour Pas0JoueurMauvaisCote, les couples (direction,
    // case d'appui) en cause — l'appelant les nomme, Game ne le fait pas.
    // Affichage humain uniquement, jamais dans le solveur.
    enum ECausePas0 { Pas0Demarre, Pas0DejaSurBut, Pas0HorsRegion,
                      Pas0ButInatteignable, Pas0JoueurMauvaisCote, Pas0DetourRequis };
    ECausePas0 diagnosticPas0(int idxCaisse, int indexBut, const QVector<bool>& zone,
                              QVector<QPair<int,int>>* dirsAppui = nullptr) const;
    // GOAL MACRO (§10.5) : pousse la caisse en 'idxCaisse' jusqu'au but d'index
    // 'indexBut', le long de son trajet solo, en vérifiant à CHAQUE pas que la
    // poussée est réellement jouable dans l'état courant (case d'arrivée libre,
    // joueur capable d'atteindre l'appui — les autres caisses comptent). Joue les
    // poussées sur *this et les empile dans 'poussees' ((case de la caisse, dir))
    // pour la reconstruction. Rend true si le but est atteint ; false si la caisse
    // se bloque en route (l'état est alors partiellement modifié — l'appelant
    // travaille sur une copie jetable).
    // 'zoneInitiale' (optionnel) : la zone du joueur pour l'état COURANT, quand
    // l'appelant l'a déjà sous la main. Elle n'est valable que pour le premier
    // pas — dès qu'une caisse bouge, la macro la recalcule elle-même. Le solveur
    // essaie une macro par caisse candidate (~5 par état) et dispose déjà de
    // cette zone : sans ce paramètre, la moitié des flood-fills du solveur sont
    // des recalculs à l'identique.
    bool macroVersBut(int idxCaisse, int indexBut, QVector<QPair<int,int>>& poussees,
                      const QVector<bool>* zoneInitiale = nullptr);
    // PROTOTYPE (2026-07-23, §6.3) — même contrat que macroVersBut, mais
    // BACKTRACKE sur les forks au lieu de les oublier : à chaque pas où
    // plusieurs directions font baisser la distance, mémorise les autres
    // (une copie de l'état à cet instant, bon marché — une seule caisse
    // bouge pendant tout l'appel, les tables statiques sont en COW) et, si
    // la branche courante finit par se bloquer, reprend à la dernière non
    // essayée au lieu d'abandonner. N'existe qu'à côté de macroVersBut — ne
    // le remplace pas, mesuré séparément avant toute décision.
    // 'essais' (optionnel) : nombre de branches réellement tentées (1 = la
    // descente gloutonne a suffi). 'budgetBranches' borne le total, protection
    // contre un niveau à forks denses ; dépassé => échec propre (pas un crash).
    bool macroVersButBacktrack(int idxCaisse, int indexBut, QVector<QPair<int,int>>& poussees,
                                qint64* essais = nullptr, qint64 budgetBranches = 1000);
    // Filtre bon marché : la macro pourrait-elle faire AU MOINS UN pas ? Répond
    // sans copier le Game ni rien modifier — c'est le premier pas de
    // macroVersBut, dont il partage la condition exacte (avanceVersBut). Un
    // 'false' garantit que macroVersBut échouerait au pas 0.
    bool macroPeutDemarrer(int idxCaisse, int indexBut, const QVector<bool>& zone) const;
    int getHeuristique() const { return getHeuristique(nullptr); }
    // Surcharge : calcule aussi le SCORE DE GUIDAGE (§10.2) via l'appariement du
    // couplage. Ordre lexicographique des distances-restantes par but (priorité =
    // index du but) : à f et g égaux, A* préfère le score le plus PETIT, ce qui
    // impose un ordre canonique de rangement et casse la multiplicité (§9.4).
    // Pur tie-break : sans effet sur l'optimalité. scoreGuidage peut être nul.
    //
    // 'posJoueur' (index de case, -1 = la position réelle) sert UNIQUEMENT à
    // l'instrumentation : h est joueur-aware (distanceParBut est indexée par la
    // région du joueur), donc déplacer le joueur change h à caisses IDENTIQUES.
    // Recalculer h sur l'enfant avec la position du PARENT isole la part de Δh
    // due aux caisses de celle due au joueur (harnais mesures/deltaf). Le chemin
    // chaud n'en paie qu'un test sur un int, devant un hongrois en O(n³).
    //
    // 'caisseParBut' (tableau de nbButs entiers, ou nul) reçoit l'APPARIEMENT du
    // couplage : pour chaque but, l'index de CASE de la caisse que le hongrois lui
    // destine (-1 si indéterminé — matrice non carrée). C'est la même affectation
    // qui sert déjà au score de guidage : on la rend au lieu de la jeter, plutôt
    // que de refaire un hongrois ailleurs.
    int getHeuristique(qint64* scoreGuidage, int posJoueur = -1,
                       int* caisseParBut = nullptr) const;

    // CORRAL UNITAIRE (§6.1, item 4 — cas dégénéré, taille 1). Une case libre S
    // dont les 4 voisins sont murs ou caisses est inaccessible au joueur. Si TOUTE
    // poussée légale des caisses qui la bordent a son appui DANS S, ou mène à une
    // case morte, alors aucune ne bougera jamais :
    //
    //   débloquer une caisse-frontière exige un appui en S
    //   → S ne s'ouvre que si une caisse-frontière bouge
    //   → CIRCULARITÉ, donc immobilité PROUVÉE (pas seulement constatée).
    //
    // L'état est mort dès qu'une de ces caisses n'est pas sur un but.
    //
    // ⚠️ Ce qui rend ce test sûr n'est PAS que S soit inaccessible « maintenant » —
    // ce serait l'erreur qui a tué le couplage de Hall (52 % de faux positifs sur
    // le niveau 1) : l'atteignabilité instantanée n'est pas une relaxation valide.
    // C'est la circularité, elle, qui est une preuve. Ne jamais relâcher la
    // condition « appui DANS S » en « appui hors de la zone du joueur ».
    //
    // Motif 1 (le plus courant) : deux caisses en diagonale qui scellent un coin mort.
    //
    //        #                     A ne peut que monter (appui en S)
    //     A #                      B ne peut qu'aller à gauche (appui en S)
    //    B S #                     S = coin mort, scellé par A et B
    //   #####
    //
    // Motif 2 — la PINCE (ajouté 2026-07-27, idée utilisateur). Deux caisses
    // scellent S, mais chacune PEUT bouger — seulement vers S :
    //
    //   ###                        A ne peut qu'entrer dans S (descente = appui mur)
    //   A S B                      B ne peut qu'entrer dans S (idem)
    //    #                         S n'a qu'UNE place → l'une gèle hors but → MORT
    //
    // D'où la règle GÉNÉRALE (corralSMort). On classe chaque caisse-frontière :
    // LIBRE (une poussée mène hors de S → on ne conclut rien), CAPTIVE (poussées
    // possibles, mais TOUTES vers S), IMMOBILE (aucune poussée). Si aucune n'est
    // LIBRE, leurs positions finales sont figées sauf UNE captive qui peut se garer
    // dans S ; donc `capacite = 1` ssi S est un but et qu'une captive existe. Mort
    // ssi `nOffGoal > capacite`. Le motif 1 est le cas où tout est immobile
    // (capacite = 0, mort ssi une caisse hors but) : la règle générale le contient.
    //
    // Deux formes. Le balayage COMPLET teste toutes les cases — c'est le juge de
    // référence (mesures/fp) qui interroge des états quelconques. Le chemin chaud
    // du solveur, lui, appelle la forme INCRÉMENTALE : elle ne teste que les
    // voisines de la caisse qui vient d'arriver, et c'est une ÉQUIVALENCE prouvée,
    // pas une heuristique. Preuve (elle vaut sur un parent déjà jugé vivant, ce que
    // garantit l'appel à l'enfilage de chaque enfant) :
    //   - Sceller une case S = rendre caisse/mur son DERNIER voisin libre. La seule
    //     case qui a gagné une caisse depuis le parent est 'caisseArrivee' (une
    //     transition — poussée simple ou goal macro — ne déplace qu'UNE caisse et
    //     vers une seule case de repos). Donc seules les voisines de caisseArrivee
    //     peuvent passer de non-scellée à scellée.
    //   - Un S DÉJÀ scellé chez le parent (non voisin de caisseArrivee) ne peut pas
    //     devenir fatal : sa fatalité (immobilité des caisses-frontière + une hors
    //     but) ne dépend que de la géométrie STATIQUE et de l'occupation de ses 4
    //     voisines (cf. corralSMort), toutes inchangées.
    // Vérifié empiriquement : compteurs d'états identiques à l'unité (CORRAL=2).
    bool corralUnitaireMort() const;                 // balayage complet (juge)
    bool corralUnitaireMort(int caisseArrivee) const; // incrémental (chemin chaud)
    // Case voisine de 'idxCase' dans la direction 'dir' (index de CASE). Sans
    // vérification de borne : l'appelant garantit que la case existe (bordure
    // murée). Sert au solveur à retrouver la case de repos de la caisse déplacée.
    int caseApres(int idxCase, EDirection dir) const;
    // CORRAL-N (§6.1 item B, promu en défaut le 2026-07-28) — le seul élagage du
    // projet qui attaque la masse f < C* (§3). Résumé d'un appel de détection.
    //   candidats = enclos scellés avec ≥1 caisse-frontière HORS but (portail brut).
    //   durs      = ceux qui passent AUSSI le gate structurel : sous-dotés en buts
    //               (Hall) ET non-rouvrables — les seuls sur lesquels un strip + A*
    //               borné aurait à trancher.
    //   cells/frontiere/butsVides = totaux sur les DURS (pour dimensionner le strip).
    struct EnclosInfo {
        int candidats = 0, durs = 0;                 // portail / après gate
        int cells = 0, frontiere = 0, butsVides = 0; // tailles (sur durs)
        int dursMorts = 0, dursVivants = 0, dursInconnus = 0;  // verdict strip+A* (si cache)
        int cacheHits = 0;                           // durs résolus par le cache
        qint64 solveStates = 0;                      // états de sous-solve payés (miss cache)
        // ÉTAGE 0 de la mesure « clé du cache sans le joueur » (plan.md §6.1, réserve
        // du 2026-07-28). Remplis UNIQUEMENT sous CACHE_JOUEUR=1 ; aucun verdict n'en
        // dépend, le binaire rend les mêmes états à l'unité dans les deux régimes.
        int hitsTestes = 0;                          // hits dont la zone joueur a été recalculée
        int hitsZoneDiff = 0;                        // … dont la zone DIFFÈRE de celle du calcul
        int diffMort = 0, diffVivant = 0, diffInconnu = 0;   // ventilation par verdict caché
        // ÉTAGE 1 (CACHE_JOUEUR=2) — sur les SEULES collisions, on relance le
        // sous-solve pour la position de joueur RÉELLE et on croise avec le verdict
        // caché : etage1[3*cache + recalcul], index 0=MORT 1=vivant 2=inconnu.
        // La case [MORT][≠MORT] est le FAUX POSITIF prouvé ; la colonne
        // [≠MORT][MORT] est le prune MANQUÉ (le gain). Le verdict RENDU reste
        // toujours celui du cache : on mesure, on ne corrige pas.
        int etage1[9] = {0,0,0,0,0,0,0,0,0};
        qint64 etage1States = 0;                     // coût des recalculs (hors prod)
        // ARBITRAGE des seuls MORT→inconnu : « inconnu » n'est pas « vivant », c'est
        // « budget trop court ». On rejuge donc ces cas-là à budget LARGE, seul moyen
        // de savoir si le prune transféré était légitime. arbitre[0]=mort 1=vivant
        // 2=toujours inconnu. Un seul 'vivant' ⇒ faux positif PROUVÉ du corral-N.
        int arbitre[3] = {0,0,0};
        qint64 arbitreStates = 0;
    };

    // Valeur mémoïsée d'un enclos DUR. 'zoneCanon' = case canonique (min) de la zone
    // du joueur sur le board STRIPPÉ au moment où le verdict a été calculé — servait
    // de rien à la décision, sert à MESURER si le transfert du verdict à une autre
    // position de joueur est légitime (cf. EnclosInfo ci-dessus). -1 si non instrumenté.
    struct VerdictEnclos {
        int verdict = -1;      // 0 = mort, 1 = vivant, -1 = inconnu (budget)
        int zoneCanon = -1;
    };

    // INCRÉMENTALE : ne flood que les enclos au contact de 'caisseArrivee' (seuls à
    // pouvoir venir de se sceller — même argument que corralUnitaireMort ci-dessus),
    // reset O(région) sans fill O(size). Si 'cache' est fourni, chaque DUR est PROUVÉ
    // par sousSolveEnclos (strip + BFS borné 'budget'), mémoïsé par frontière triée :
    // dursMorts>0 ⇒ l'appelant peut PRUNER (mort prouvée, sound par construction).
    // ⚠️ Le GATE n'est qu'un filtre : il est FAUX POSITIF s'il tranche seul (mesuré
    // au juge fp, variante -2 — le « non-rouvrable » est un test à UN pas, il rate
    // les enclos rouvrables en plusieurs coups). Ne jamais pruner sur 'durs', seul
    // 'dursMorts' est une preuve.
    EnclosInfo detecteEnclosArrivee(int caisseArrivee, const QVector<bool>& zone,
                                    QVector<bool>& visite,
                                    QHash<QByteArray,VerdictEnclos>* cache = nullptr,
                                    int budget = 0) const;
    // GATE comme PRUNE, version FULL-SCAN (pour le juge fp, qui interroge des états
    // quelconques). Renvoie true dès qu'un enclos est DUR (scellé + ≥1 caisse hors
    // but + sous-doté en buts + non-rouvrable en un pas). ⚠️ Le non-rouvrable est un
    // test À UN PAS : possiblement faux positif si l'enclos est rouvrable en
    // plusieurs coups — c'est CE que fp doit vérifier avant tout câblage en prune.
    bool gateEnclosMort() const;
    // Mini-solveur BORNÉ pour le strip d'un enclos (§6.1 item B). 'boxesInit' = les
    // caisses-frontière (seules mobiles) ; TOUTES les autres caisses sont traitées
    // comme du SOL (le strip est une relaxation valide : moins d'obstacles = joueur
    // plus libre). Board statique (murs, buts, cases mortes) pris de *this. BFS de
    // poussées avec dédup, joueur-aware. Renvoie :
    //    0 = MORT   (espace épuisé sous budget → PREUVE, car le strip relaxe)
    //    1 = vivant (les caisses-frontière atteignent toutes un but)
    //   -1 = inconnu (budget atteint)
    // 'zoneCanonOut' rend la zone canonique du joueur AU DÉPART du sous-solve — elle
    // est calculée de toute façon (c'est la clé de l'état initial du BFS), donc gratuite.
    int sousSolveEnclos(const QVarLengthArray<int, 32>& boxesInit, int budget,
                        int* developpesOut = nullptr, int* zoneCanonOut = nullptr) const;
    // Case canonique (min) de la zone du joueur sur le board STRIPPÉ {murs + 'boxes'},
    // joueur à playerPoint. Exemplaire UNIQUE du calcul : sousSolveEnclos s'en sert
    // pour l'état initial de son BFS, et l'étage 0 de la mesure pour recalculer la
    // zone à un cache-hit. Les deux ne peuvent donc pas diverger (piège du §6.3 :
    // deux lectures indépendantes d'un même champ ne donnent pas la même chose).
    int zoneCanoniqueStrip(const QVarLengthArray<int, 32>& boxes) const;
    // Quelle caisse le couplage destine-t-il à ce but ? Index de CASE, -1 si aucune.
    // Utilisé par le régime de macro « but du couplage » (solveurastar.cpp) : pousser
    // vers un but la caisse que le couplage lui assigne garantit Δh = −N, donc un
    // enfant à f CONSTANT ; pousser une autre caisse fait que le couplage se
    // réarrange et que h ne baisse pas (mesuré : plan.md §6.3, 2026-07-24).
    int caisseAssignee(int indexBut) const;
    // Applique une poussée sans faire marcher le joueur : le TÉLÉPORTE sur la
    // case d'appui, puis pousse via move() (qui fait checkVictoire/checkDefaite).
    // Précondition, NON vérifiée : la case d'appui doit être dans la zone du
    // joueur — ce que getCaissesDeplacable() garantit déjà pour tout bit qu'elle
    // pose. Permet au solveur de ne calculer le chemin de marche (coûteux) que
    // sur les enfants réellement retenus, et non sur tous les doublons.
    // 'nbDep' ne comptera qu'un coup au lieu de la marche complète.
    bool pousse(int idxCaisse, EDirection dir);
    // Réécrit le plateau à partir d'une clé getEtat() : les caisses et le joueur
    // sont replacés, les murs et les buts ne bougent pas. Permet au solveur de ne
    // PAS transporter un Game complet (~700 o) dans sa file ouverte, mais juste la
    // clé (~22 o) — la file d'A* est le principal poste mémoire.
    //
    // Le joueur est replacé sur la case CANONIQUE de sa zone (le min des ids
    // atteignables, cf. getMinIdx), pas forcément là où il était. Sans effet :
    // getEtat() normalise déjà à cette case, pousse() téléporte, et
    // checkVictoire()/checkDefaite() ne dépendent que des caisses.
    //
    // 'cle' pointe sur tailleCle() shorts, au format de getEtat().
    // Renvoie le nombre de caisses posées sur un but (compté gratuitement pendant
    // le placement), pour la jauge de progression du diagnostic (§10).
    int appliqueEtat(const quint16* cle);
    // Deadlock de LIVRAISON (§6.1) : vrai s'il reste un but VIDE qu'aucune caisse
    // ne peut plus atteindre. 'variante' choisit la relaxation (cf. game.cpp) ;
    // 0 = celle de la variable d'environnement LIVRAISON (défaut : test COUPÉ).
    //
    // ⚠️ DÉSACTIVÉ, ET POUR CAUSE (mesuré le 2026-07-21 avec mesures/fp.cpp, qui
    // rejoue une solution GAGNANTE et interroge le test sur chacun de ses états —
    // tous solubles par construction, donc toute détection est un faux positif
    // PROUVÉ). Deux défauts indépendants :
    //   1. le BFS de livraison n'est PAS joueur-aware — il ne retient qu'UNE
    //      position de joueur par case atteinte, alors qu'une même case atteinte
    //      « par l'autre côté » ouvre d'autres poussées. C'est la faille du
    //      prototype mesures/mort.cpp, et elle rend 86 faux positifs sur le 17 ;
    //   2. tenir les caisses posées pour des obstacles fixes est faux, même
    //      restreint aux caisses GELÉES (1 faux positif sur le 2).
    // Seule la variante 3, qui lit distanceParBut (joueur-aware, elle), est sûre —
    // et elle ne capture rien de plus que staticDeadlock.
    //
    // PUBLIC parce que le point d'appel naturel, checkDefaite, est le mauvais :
    // marquer 'perdu' sur un état INTERMÉDIAIRE de goal macro fait avorter la
    // macro entière (move() refuse de jouer sur un état perdu) — niveaux 3 et 5
    // perdus. Le solveur peut donc l'appeler sur les états qu'il ENFILE.
    bool butNonLivrable(int variante = 0) const;
private:
    // Coeur du corral unitaire pour UNE case S : facteur commun du balayage
    // complet et de la forme incrémentale (cf. corralUnitaireMort ci-dessus).
    bool corralSMort(int s) const;

    int largeur = 0;
    int hauteur = 0;
    int size = 0;
    QPoint playerPoint;
    Level::ETypeCase *cases = nullptr;
    int nbDep = 0;
    int nbDepCaisse = 0;
    int numNiveau = 1;
    // ORDRE DYNAMIQUE (cf. butActif). `butCourant` est un CACHE de jalon, pas un état
    // de jeu : d'où `mutable`, pour que butActif() reste const comme tous ses appelants.
    // ⚠️ Les DEUX doivent être copiés dans les ctors de copie/déplacement (piège §7) —
    // sans `butCourant`, chaque état rechoisirait son but, et la cadence au jalon
    // (nbButs fois par chemin) redeviendrait une passe distanceLivraison PAR ÉTAT.
    bool ordreDynamique = false;
    mutable int butCourant = -1;
    int nbCaisses = 0;
    bool gagne = false;
    bool perdu = false;
    QList<int> goals;
    QVector<bool> casesMortes;

    // regions[CASE * size + CAISSE] = id de la composante connexe de CASE, sur un
    // plateau où le seul obstacle (hors murs) est une caisse posée en CAISSE.
    //
    // ⚠️ L'ordre des indices n'est PAS arbitraire : la CASE est en index majeur.
    // Le chemin chaud (getHeuristique, checkDefaite) interroge toujours avec la
    // case du JOUEUR fixe et la caisse qui varie — cet ordre rend ces lectures
    // contiguës. L'ordre inverse coûtait un défaut de cache par caisse, et
    // doublait le temps par état.
    QVector<qint16> regions, nbRegions;
    QVector<int> distancePoussee;

    // distanceParBut[(BUT * size + CASE) * maxRegions + REGION] = nombre minimal de
    // poussées pour amener une caisse SEULE de CASE vers CE but précis, le joueur
    // étant dans REGION (composante de plateau-moins-cette-caisse). Une table par
    // but, calculée par un BFS à rebours par but (cf. calculDistancePoussee).
    //
    // distancePoussee en est le min sur les buts — même valeur qu'un BFS multi-but
    // simultané, donc casesMortes/checkDefaite sont inchangés. distanceParBut sert
    // au couplage hongrois de getHeuristique() : cout[caisse][but] direct.
    QVector<int> distanceParBut;
    int nbButs = 0;
    int maxRegions = 0;

    // Ordre de REMPLISSAGE des buts (§10.5) : ordreButs[k] = indice du but à
    // remplir en k-ième. Les plus PROFONDS d'abord (coins, culs-de-sac où une
    // caisse serait gelée), en remontant vers l'entrée. Profondeur = distance de
    // poussée depuis la caisse la plus proche. Statique, partagé par COW.
    QVector<int> ordreButs;

    // LOI DE L'ORDRE (cf. caseMorteLoi) : mortesLoi[BUT * size + CASE], et
    // rangDeBut[BUT] = rang de ce but dans ordreButs (l'inverse de celui-ci).
    // ⚠️ Une seule table PLATE, pas un QVector<QVector<bool>> : le solveur copie
    // Game par candidate, et un vecteur de vecteurs coûterait nbButs incréments de
    // compteur par copie là où la table plate n'en coûte qu'un (COW).
    QVector<bool> mortesLoi;
    QVector<int>  rangDeBut;

    bool move(EDirection dir);
    bool moveCaisse(Level::ETypeCase *cases, QPoint playerPoint, QPoint caissePoint, SDirection direction);
    void checkVictoire();
    void checkDefaite();
    bool staticDeadlock(int idxCaisse, int idxJoueur,  QVector<bool>& enCours) const;
    bool dynamicDeadlock(int idxCaisse) const;
    short getMinIdx(const QVector<bool>& zone) const;
    bool isLibre(int idx) const;
    void calculCaseMorte();
    void calculCasesMortesLoi();
    // Test de gel : une caisse est gelée si elle est bloquée sur LES DEUX axes.
    // 'enCours' est la garde de récursion (cf. game.cpp).
    bool caisseGelee(int idxCaisse, QVector<bool>& enCours) const;
    bool bloqueeSurAxe(int idxCaisse, EDirection dirA, EDirection dirB, QVector<bool>& enCours) const;
    bool estCaisse(int idx) const;
    // Une poussée de la caisse en 'c' vers 'd' la fait-elle AVANCER vers le but
    // (distance dCur -> dCur-1), le joueur pouvant se mettre à l'appui ? Rend la
    // case d'arrivée, ou -1. 'dpb' = la tranche de distanceParBut du but visé.
    // Exemplaire UNIQUE de la condition de descente : macroVersBut et
    // macroPeutDemarrer s'en servent tous les deux (cf. game.cpp).
    int avanceVersBut(int c, int d, int dCur, const int* dpb,
                      const QVector<bool>& zone) const;
    void calculDistancePoussee();

// Distance de livraison d'une caisse (poussées) depuis les caisses de départ, les
// buts marqués dans `bloque` faisant obstacle. -1 = inatteignable.
QVector<int> distanceLivraison(const QVector<bool>& bloque) const;

// PRÉCÉDENCE GLOBALE (§6.2 famille B) : requis[B] = les buts qui doivent être
// remplis AVANT B, parce que sans eux plus aucune caisse n'atteint B. Statique.
QVector<QVector<int>> precedenceGlobale() const;

public:
// Le fichier `ordre_niveau_XXXX.txt` du répertoire courant s'il existe, sinon "".
// Injecte un ordre de remplissage à la main DANS L'APP (une variable d'environnement
// n'y arrive pas, §7), pour le jouer en mode hybride et voir où il coince.
// ⚠️ OUTIL DE CHANTIER, à retirer avec la campagne hybride.
static QString cheminOrdreInjecte(int numNiveau);
private:

// LES SALLES : composantes connexes des cases-buts en 4-connexité (cf. game.cpp).
// salle[b] = index de la salle du but b. Sert au groupement salle par salle du tri
// topologique — la macro ne peut pas enchaîner si l'ordre saute d'une salle à l'autre.
QVector<int> sallesDeButs() const;

// Ordre de remplissage déduit de la PRÉCÉDENCE DE LIVRAISON (§6.2, 2026-07-20) :
// glouton avant + garde anti-échouage. Rend une permutation des indices de buts.
QVector<int> ordreParPrecedence() const;
};

Q_DECLARE_METATYPE(Game::EDirection)
// Permet de transporter un Game complet par signal queued (thread solveur -> UI),
// pour l'affichage de l'état-max (§10, diagnostic). Copie profonde du plateau, mais
// les tables statiques sont en COW : le coût reste modéré et l'émission est rare.
Q_DECLARE_METATYPE(Game)

#endif // GAME_H
