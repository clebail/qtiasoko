#ifndef SOLVEURASTAR_H
#define SOLVEURASTAR_H

#include "cle.h"
#include "solveur.h"
#include "solveurdiagnostic.h"

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
    // 'loi' (régime d'essai, RESTAURÉ ISOLÉ le 2026-08-19, cf. game.h) : une caisse
    // ne peut pas se tenir sur une case morte VUE DU BUT ACTIF (`caseMorteLoi`).
    // Extrait de l'ancien régime combiné 'loiOrdre' (retiré le 2026-08-18) : celui-ci
    // testait CETTE table ET le gel hors tour sous un seul drapeau, jamais isolés.
    // Le gel, testé seul le 2026-08-19, casse LUI AUSSI le niveau 6 — donc « gel=0
    // sur le 6 » dans la mesure combinée de 2026-08-04 ne disculpait rien : cette
    // table-ci n'a jamais non plus été mesurée seule. À faire AVANT toute promotion.
    // ── RELÉGATION DES POUSSÉES SIMPLES ('relegueSimples' > 0, 2026-08-20).
    //
    // LE FAIT MESURÉ, et il est brutal : sur le niveau 16, le régime d'engagement
    // rend **AUCUNE** — espace ÉPUISÉ, pas un budget — alors qu'une partie humaine
    // GAGNANTE existe SOUS LE MÊME ORDRE (journal hybride du 2026-08-20). Les deux
    // ne peuvent pas être vrais : le solveur ne peut pas atteindre cette ligne.
    // Le journal dit pourquoi, sur 55 poussées vraiment choisies à la main, **23
    // (42 %)** portent la mention `HORS REGIME MACRO : 1 macro engagee, le solveur
    // ne genere aucune poussee simple`. Le coup humain n'est pas dans son arbre.
    //
    // Le §6.3 notait l'engagement comme une perte d'OPTIMALITÉ (« il ne génère que
    // les macros vers le but actif et abandonne le reste »). C'est en réalité une
    // perte de COMPLÉTUDE, et elle rend un niveau insoluble.
    //
    // LE CORRECTIF, dans la forme sûre du §6.4 : **dé-prioriser, jamais élaguer**.
    // Quand une macro est engagée on enfile AUSSI les poussées simples, mais avec
    // 'relegueSimples' ajouté à leur f — elles ne sont donc développées qu'une fois
    // épuisé ce qui est meilleur. La complétude revient, le guidage reste.
    //
    // ⚠️ RÉGIME SÉPARÉ, JAMAIS LE DÉFAUT. Gonfler f est un weighted-A* local : ça
    // change l'ordre de dépilement, donc le nombre d'états, donc le canari — sur
    // les canaris (1, 2, 17) les poussées macro coïncident avec C\*, il n'y a rien
    // à y gagner et tout à y perdre. L'optimalité était déjà abandonnée par
    // l'engagement lui-même (§6.3) ; on ne la dégrade pas davantage, on récupère
    // des solutions qui n'existaient pas.
    //
    // ⚠️ La valeur de 'relegueSimples' est un BUDGET À BALAYER, pas une constante à
    // figer (méthode CORRAL_BUDGET du §6.2). Le mou étant toujours PAIR (§3), les
    // paliers naturels sont 2, 4, 6 : +2 = « un recul de retard ».
    explicit SolveurAStar(const Game& etatDepart, int poids = 1, bool macro = false,
                          QObject* parent = nullptr, bool macroCouplage = false,
                          bool plongeon = false, bool loi = false,
                          int relegueSimples = 0);

protected:
    void run() override;

private:
    // PLONGEON depuis 'etatDepart' (atteint en 'gDepart' poussées, noeud
    // 'idxNoeudDepart') : best-first sur h SEUL, budget d'états, goal macro et
    // corral actifs comme dans la recherche principale. Rend l'index du noeud
    // GAGNANT dans 'noeuds' (utilisable tel quel par reconstruire()), ou -1 si le
    // budget est épuisé sans victoire — auquel cas 'noeuds' est rendu à sa taille
    // d'avant, pour que l'échec ne laisse aucune trace.
    int plonge(const Game& etatDepart, int gDepart, int idxNoeudDepart,
               QHash<QByteArray,Game::VerdictEnclos>& cacheEnclos,
               int budget, qint64* etatsOut);

    // ── DÉCOUPAGE DE run() (2026-08-18) — extraction pure, aucun changement de
    // comportement. Le corps de run() entrelaçait quatre blocs indépendants avec
    // sa boucle principale ; les isoler en méthodes NOMMÉES remplace le besoin de
    // commenter « ce que fait ce bloc » par le nom lui-même, et ne laisse aux
    // commentaires que le POURQUOI. 'enfiler' reste une lambda dans run() : elle
    // capture ~10 variables locales AU SOLVE (arène, table, corral, noeuds...),
    // et les faire remonter en membres aurait exigé de gérer leur remise à zéro
    // à la main — exactement le risque que ce fichier documente déjà ailleurs
    // (« noeuds/meilleurG doivent être réinitialisés à chaque run() », plan.md §7).
    //
    // 'tenteMacro'/'poussesSimples' prennent 'enfiler' en paramètre de PATRON
    // (template), pas en std::function : le chemin est chaud (appelé par état
    // développé), et un std::function aurait payé une dispatch virtuelle plus,
    // parfois, une allocation de tas pour rien — le patron s'inline exactement
    // comme le code d'origine.

    // La jauge de progression (stderr, tous les 1000 dépilements). Pur affichage,
    // aucune décision : 'fileAvant' est mise à jour ici (tendance de la file).
    void imprimeJauge(qint64 compteur, int& fileAvant, size_t fileSize, size_t fileCap,
                      const Arene& arene, const TableG& meilleurG,
                      int curF, int curG, int rangees, int maxRangees, int nbButs) const;

    // Réponse à un NOUVEAU RECORD de caisses posées (§6.0) : tente un plongeon
    // gloutonne borné pour le compléter tout de suite. Rend VRAI si le plongeon a
    // gagné — auquel cas TOUT est déjà émis/imprimé (solutionTrouvee compris) et
    // run() doit s'arrêter net, comme le faisait le 'return' d'origine.
    bool traiteRecord(Game& etat, int rangees, qint64& compteur, int idxNoeudCourant,
                      int gCourant, QHash<QByteArray,Game::VerdictEnclos>& cacheEnclos,
                      int& plongeons, qint64& etatsPlongeon,
                      const Arene& arene, const TableG& meilleurG, size_t fileCap);

    // GOAL MACRO — régime d'ENGAGEMENT (§10.5) : si le but actif peut être atteint
    // par au moins une caisse, n'enfile QUE les macros qui l'y envoient. Rend le
    // nombre de macros enfilées (0 = repli sur les poussées simples, à l'appelant
    // de décider). 'macroCouplage' fait tenter d'abord la caisse que le couplage
    // hongrois destine au but (§6.3) : gabarit inchangé, cf. le commentaire du
    // constructeur.
    template<typename Enfiler>
    int tenteMacro(Game& etat, const QVector<quint8>& caisses, const QVector<bool>& zone,
                   int gCur, Enfiler&& enfiler);

    // Repli : une poussée simple par direction légale, pour chaque caisse. Appelé
    // uniquement si tenteMacro() n'a rien produit (caisse coincée par la
    // congestion — la recherche doit d'abord démêler).
    // 'bonusF' relègue les enfants produits en gonflant leur f (régime 'relegue',
    // cf. le constructeur) ; 0 = enfilage normal, le comportement historique.
    template<typename Enfiler>
    void poussesSimples(Game& etat, const QVector<quint8>& caisses, int gCur,
                        Enfiler&& enfiler, int bonusF = 0);

    // LOI DE L'ORDRE (régime 'loi', cf. constructeur et game.h) : vrai si une caisse
    // se tient sur une case morte vue du but actif. Appelée aux DEUX points
    // d'enfilage (recherche principale + plonge()), même raison que le corral : un
    // élagage câblé au premier seul laisse le second explorer des états déjà
    // prouvés morts. 'arrivee' = case de repos de la caisse qui vient de bouger
    // (-1 si aucune, cf. les points d'appel).
    bool loiTropTot(const Game& e, int arrivee) const;

    const int poids;
    const bool macro;
    const bool macroCouplage;
    const bool plongeon;
    const bool loi;
    // RELÉGATION DES POUSSÉES SIMPLES (2026-08-20). 0 = régime d'engagement
    // historique. > 0 = quand une macro est engagée, on enfile AUSSI les poussées
    // simples, avec 'relegueSimples' ajouté à leur f. Cf. le constructeur.
    const int relegueSimples;
};

#endif // SOLVEURASTAR_H
