#ifndef SOLVEURASTAR_H
#define SOLVEURASTAR_H

#include "cle.h"
#include "solveur.h"

#ifdef INSTRUM_DELTAF
#include <vector>
// Instrumentation hors-ligne du Δf DES ENFANTS ENFILÉS (harnais mesures/deltaf).
// Ne compile que dans les harnais : le code produit ne définit jamais INSTRUM_DELTAF.
//
// Question posée. À f égal, le comparateur préfère le g le plus GRAND
// (solveurastar.cpp). Une goal macro de N poussées enfile donc un enfant à g+N :
// s'il reste à f constant (h a baissé de N), il double tous les états de même f
// et « ranger une caisse » revient à passer en tête. Mais RIEN ne le garantit :
//     Δf = N + poids · Δh
// et si une seule poussée de la chaîne n'est pas productive au sens du couplage
// hongrois, Δh > −N, donc Δf > 0 — l'enfant tombe sur un palier SUPÉRIEUR et se
// retrouve RELÉGUÉ derrière toute la masse restante, au lieu d'être promu.
// C'est la distribution de ce Δf qu'on mesure ici, macro contre poussée simple.
//
// Compté au moment du push_heap, donc sur les enfants RÉELLEMENT enfilés (un
// enfant rejeté par la dédup ne concourt pas au classement).
struct StatsDeltaF {
    static const int DECALAGE = 64;    // Δf peut être négatif (h non cohérente)
    static const int TAILLE   = 256;

    std::vector<qint64> histoMacro  = std::vector<qint64>(TAILLE, 0);
    std::vector<qint64> histoSimple = std::vector<qint64>(TAILLE, 0);
    qint64 horsBornes = 0;

    // Croisement (longueur de chaîne N) × (Δf) : c'est lui qui dit si les Δf > 0
    // viennent des chaînes longues (une poussée non productive quelque part) ou
    // s'ils sont uniformes.
    std::vector<qint64> parLongueur    = std::vector<qint64>(64, 0);  // enfants macro par N
    std::vector<qint64> parLongueurNul = std::vector<qint64>(64, 0);  // ... dont Δf == 0
    std::vector<qint64> parLongueurNeg = std::vector<qint64>(64, 0);  // ... dont Δf < 0

    qint64 nMacro = 0, nSimple = 0;
    qint64 sommeDeltaMacro = 0, sommeDeltaSimple = 0;
    qint64 sommeLongMacro = 0;

    // DÉCOMPOSITION de Δh, sur les enfants de macro relégués (Δf > 0). h est
    // joueur-aware : à caisses identiques, déplacer le joueur change h. On
    // recalcule donc h(enfant) avec la position du PARENT :
    //     dhCaisses = h(enfant, joueur parent) - h(parent)   <- le travail réel
    //     dhJoueur  = h(enfant, joueur enfant) - h(enfant, joueur parent)
    // Si dhCaisses == -N, la macro a bien fait son travail et c'est le JOUEUR
    // laissé du mauvais côté qui mange le gain. Sinon, c'est le couplage qui se
    // réarrange (la caisse posée n'était pas celle que le couplage destinait à
    // ce but).
    qint64 nReleg = 0;                 // enfants de macro à Δf > 0
    qint64 sommeDhCaisses = 0, sommeDhJoueur = 0, sommeLongReleg = 0;
    qint64 relegPurJoueur = 0;         // ... dont dhCaisses == -N (macro parfaite)
    qint64 relegCouplage = 0;          // ... dont dhCaisses  > -N (couplage remanié)
    // Une MOYENNE nulle de dhJoueur pourrait masquer des valeurs opposées : on
    // compte donc séparément les cas non nuls, dans chaque sens.
    qint64 dhJoueurNonNul = 0, dhJoueurPositif = 0, dhJoueurNegatif = 0;
};
StatsDeltaF& statsDeltaF();
#endif

// Corral : ACTIF par défaut (promu, cf. §6.1) — les DEUX étages, le corral
// unitaire (motifs 1 et 2) et le corral-N (strip + A* borné). Trappe `CORRAL=0`
// pour tout couper — réservée aux OUTILS DE MESURE (`fp`, `mort`) qui doivent
// collecter/rejouer des états SANS que le corral les élague d'abord, sinon le
// juge est aveugle aux faux positifs qu'il est censé chercher. La prod ne touche
// jamais cette variable (défaut = actif).
//
// Fonction et non variable de fichier : le mode hybride en a besoin depuis
// mainwindow.cpp (cf. CORRAL_BUDGET ci-dessous). Lue une seule fois.
inline bool corralActif() {
    static const bool actif = (qgetenv("CORRAL") != "0");
    return actif;
}

// Budget du sous-solve d'enclos (corral-N). Balayage mesuré le 2026-07-27 : le
// gain d'états SATURE dès ~150, tandis que le coût des sous-solves explose (×6)
// au-delà — en « inconnus » qui ne prouvent rien et qu'on paie plein tarif.
// Figé après verdict, comme les autres réglages promus.
//
// Dans l'en-tête et non dans le .cpp : le mode hybride rejoue l'enfilage du
// solveur dans l'UI (mainwindow.cpp, mesureRangCoup) pour classer le coup joué à
// la main. Deux copies de ce 150 dériveraient sans que rien ne le signale, et le
// rang mesuré ne vaudrait plus pour le solveur réel. Exemplaire unique.
static constexpr int CORRAL_BUDGET = 150;

// ── PAQUET NON LIVRABLE — INTERRUPTEUR DE CHANTIER (2026-08-03) ───────────────
// ⚠️ Celui-ci AJOUTE un élagage, il n'en coupe pas — l'inverse de CORRAL et de
// l'ancien CORRAL_N. C'est la direction dangereuse du §7 (« un défaut coupé se
// voit tout de suite, un défaut manquant ne se voit jamais ») : mesurer avec
// PAQUET=1 en croyant lire le défaut fausserait tout. Deux garde-fous :
//   - défaut = INACTIF, donc l'app et le bench ne bougent pas d'un état ;
//   - quand il est actif il le DIT sur stderr, une fois, au premier appel.
// À retirer avec le verdict : soit promu en dur, soit supprimé.
//
// Le test : le paquet 8-connexe de caisses HORS BUT contenant la caisse qui vient
// de bouger, toutes les autres caisses retirées, tous les buts conservés. File
// vidée sous budget ⇒ ces caisses-là ne peuvent pas toutes être posées ⇒ l'état
// est MORT. Sound par relaxation (moins de caisses = joueur plus libre) + exhaustion.
//
// Pourquoi tester le seul paquet de 'arrivee' suffit : le verdict d'un paquet ne
// dépend QUE de ses caisses (les autres sont retirées). Il ne peut donc changer
// que si l'une d'elles bouge — et celle qui bouge, c'est 'arrivee'. Un paquet
// scindé par un départ ne donne que des SOUS-ensembles d'un paquet déjà jugé
// vivant, qui restent vivants (retirer une caisse ne fait que libérer).
inline bool paquetActif() {
    static const bool actif = (qgetenv("PAQUET").toInt() == 1);
    return actif;
}
// Budget du sous-solve de paquet. RÉGLABLE le temps du balayage (2026-08-03),
// exactement comme CORRAL_BUDGET l'a été avant d'être figé à 150 le 2026-07-27 :
// on ne fige un réglage qu'après l'avoir balayé, jamais sur une intuition.
// ⚠️ À REFIGER en constante dès le verdict — un défaut qui dépend d'une variable
// d'environnement est le piège du §7.
inline int paquetBudget() {
    static const int b = qgetenv("PAQUET_BUDGET").isEmpty() ? 2000
                                                            : qgetenv("PAQUET_BUDGET").toInt();
    return b;
}

// A* sur les poussées : f = g + poids * h.
//
// poids = 1 : A* classique. h est admissible ET cohérente, donc la solution est
//             OPTIMALE en nombre de poussées. Mais l'élagage est quasi nul (−3 à
//             −20 % d'états seulement) : une poussée utile fait g+1 et h−1, donc
//             f ne bouge pas, et A* doit développer tout état de f <= C*. Une
//             heuristique admissible ne peut pas élaguer ce qui n'est pas mauvais.
//
// poids > 1 : h est gonflée, donc plus admissible — l'optimalité est PERDUE, et
//             la cohérence avec elle (un état peut être re-développé après avoir
//             été atteint par un meilleur chemin ; c'est normal et géré par
//             'meilleurG'). En échange, la recherche plonge vers la solution au
//             lieu de balayer les paliers : mesuré ×30 en temps sur le niveau 1
//             pour +6 % de poussées.
class SolveurAStar : public Solveur
{
    Q_OBJECT

public:
    // NE PORTE PAS de Game. Un Game complet pèse ~700 o (72 o d'objet + le
    // tableau 'cases'), et la file ouverte d'A* compte des millions d'entrées :
    // c'était LE poste mémoire (3,4 Go sur 4,8 Go pour le niveau 2). La clé
    // détermine entièrement l'état — on reconstruit le Game au dépilement avec
    // Game::appliqueEtat().
    //
    // Et la clé elle-même n'est plus un QByteArray mais une simple référence
    // dans l'arène (4 o, cf. cle.h) : le QByteArray coûtait un malloc et un
    // en-tête QArrayData par clé, pour 22 o utiles. SElement tient maintenant en
    // 16 octets, entièrement POD — donc memcpy-able quand le tas se réalloue.
    typedef struct _SElement {
        int f;
        int g;
        int idxNoeud;
        Cle cle;
        qint64 guidage;   // départage lexicographique (§10.2) : plus PETIT = préféré
    } SElement;

    // 'macro' active la goal macro (§10.5) : rapide, optimal sur les niveaux à
    // faible congestion, approché sur les gros (le trajet solo peut y différer du
    // réel). 'false' = A* pur (optimal garanti, mais lent/inabouti sur les gros).
    //
    // 'macroCouplage' (régime d'essai, plan.md §6.3) : la macro tente D'ABORD la
    // caisse que le couplage hongrois destine au but actif. Pousser celle-là fait
    // baisser h d'exactement N, donc l'enfant reste à f CONSTANT et le tie-break
    // « g le plus grand » le fait passer en tête ; pousser une autre caisse lui
    // fait voler son but, le couplage se réarrange et h ne baisse pas (l'enfant
    // part alors DERRIÈRE tout le palier). Sans effet si la caisse assignée ne
    // peut pas faire la macro : on retombe sur les autres candidates.
    // 'plongeon' (régime d'essai, plan.md §6.0/§6.3) : dès qu'un état bat le max de
    // caisses posées ET qu'il en a assez (SEUIL_PCT), on tente de le COMPLÉTER par
    // une recherche gloutonne bornée (best-first sur h seul) avant de revenir à
    // l'A* normal. Renonce à l'optimalité — mesuré : +2 poussées sur le 4, l'optimum
    // exact sur 2/3/5/6/7/9/17.
    // 'ordreCoins' (régime d'essai, §6.2, 2026-08-03 — idée utilisateur) : une
    // caisse ne peut PAS se poser sur un but EN COIN dont le rang dépasse le but
    // actif. Fondement, énoncé par l'utilisateur : un but pas encore à son tour
    // n'est, du point de vue du solveur, que du SOL — et une caisse sur du sol en
    // coin est un corner deadlock, le tout premier élagage du projet.
    //
    // POURQUOI SEULEMENT LES COINS, et c'est ce qui sauve le niveau 1. Le §4 a
    // réfuté « interdire de remplir dans le désordre » : sur le 1, atteindre
    // (17,6) exige de faire ÉTAPE sur (16,6), qui est un but — l'interdire rend
    // le niveau insoluble. Mais **une case terminale ne peut pas être une case de
    // transit** (aucune poussée n'en sort, donc rien ne la traverse) : restreindre
    // aux coins ne peut donc JAMAIS casser un acheminement. Mesuré : 98 buts
    // terminaux sur 511 (19,2 %), dont 68 de rang > 0 ; sur le 1 ce sont (17,6)
    // rang 0 — actif d'emblée — et (17,8) rang 2, où rien ne transite.
    //
    // ⚠️ CE N'EST PAS UN ÉLAGAGE PROUVÉ. Il interdit de FINIR une caisse sur un
    // but en coin avant son tour, ce qui reste légal en Sokoban : la règle repose
    // sur la justesse de l'ORDRE, pas sur la géométrie. D'où le régime séparé —
    // le canari des résolus ne doit jamais en dépendre. Même statut que le
    // plongeon et le pondéré.
    // ⚠️ Sans effet sur 3 niveaux (0, 13, 29) : aucun but en coin.
    explicit SolveurAStar(const Game& etatDepart, int poids = 1, bool macro = false,
                          QObject* parent = nullptr, bool macroCouplage = false,
                          bool plongeon = false, bool ordreCoins = false);

protected:
    void run() override;

private:
    // PLONGEON depuis 'etatDepart' (atteint en 'gDepart' poussées, noeud
    // 'idxNoeudDepart') : best-first sur h SEUL, budget d'états, goal macro et
    // corral actifs comme dans la recherche principale. Rend l'index du noeud
    // GAGNANT dans 'noeuds' (utilisable tel quel par reconstruire()), ou -1 si le
    // budget est épuisé sans victoire — auquel cas 'noeuds' est rendu à sa taille
    // d'avant, pour que l'échec ne laisse aucune trace.
    // ⚠️ 'cachePaquet' passé comme 'cacheEnclos' et pour la MÊME raison : le
    // plongeon est un SECOND point d'enfilage, et tout élagage câblé au premier
    // seul le laisse explorer des états déjà prouvés morts. Constaté en direct le
    // 2026-08-03 — un record 7/15 du niveau 16 exporté depuis l'UI avec PAQUET=1
    // portait un paquet mort que la recherche principale aurait refusé.
    int plonge(const Game& etatDepart, int gDepart, int idxNoeudDepart,
               QHash<QByteArray,Game::VerdictEnclos>& cacheEnclos,
               QHash<QByteArray,int>& cachePaquet, int budget, qint64* etatsOut);

    const int poids;
    const bool macro;
    const bool macroCouplage;
    const bool ordreCoins;
    // Tables du régime 'ordreCoins', remplies une fois au début de run() depuis
    // l'API publique de Game : rien n'est ajouté à Game, rien n'est maintenu en
    // double. rangDeCase[cell] = rang de remplissage du but, -1 si pas un but.
    QVector<int>  rangDeCase;
    QVector<bool> coinDeCase;
    void construitTablesCoins(const Game& g);
    bool coinTropTot(const Game& e, int arrivee) const;
    const bool plongeon;
};

#endif // SOLVEURASTAR_H
