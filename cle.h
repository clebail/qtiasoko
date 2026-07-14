#ifndef CLE_H
#define CLE_H

#include <QtGlobal>
#include <cstddef>
#include <vector>

// Stockage compact des clés d'état du solveur.
//
// Une clé (§1.1) = [ids des N caisses triés] + [id canonique de la zone du
// joueur], soit N+1 shorts. Or N ne varie JAMAIS pendant une résolution : aucune
// caisse n'apparaît ni ne disparaît. Toutes les clés d'un même niveau ont donc
// exactement la même longueur, connue dès le chargement (Game::tailleCle()).
//
// On les range bout à bout dans une zone à blocs, et tout le reste du solveur —
// tables de dédup, file ouverte — ne manipule plus qu'un index 32 bits dedans
// (struct Cle).
//
// C'est ce qui fait tomber le mur mémoire. Avec un QByteArray par clé, on payait
// pour 22 octets utiles : un malloc, l'en-tête QArrayData (24 o), l'arrondi de
// l'allocateur, et 8 o de pointeur dans chaque conteneur qui la porte. Ici le
// coût est exactement 2*(N+1) octets dans l'arène, plus 4 par référence.
//
// Et contrairement à une clé inline de capacité fixe (quint16 v[MAX]), rien
// n'est plafonné : la taille étant fixée à l'exécution, le niveau 10 et ses 32
// caisses passent sans une ligne de plus — là où un MAX taillé pour eux aurait
// fait payer 68 o par clé aux niveaux qui n'en demandent que 24.
// Rangée en BLOCS de taille fixe, jamais réalloués — et non en un seul vector
// qui double.
//
// Un vector qui double détient, le temps de la copie, l'ancien tableau ET le
// nouveau. Sur l'arène du niveau 3 (444 Mo, le premier poste du solveur), ce pic
// transitoire coûte 1,5x — soit ~220 Mo qui ne servent qu'à déménager. Mesuré :
// 1,25 Go de régime permanent pour un pic à 1,83 Go, l'écart étant précisément
// ces déménagements.
//
// Un bloc, une fois alloué, ne bouge plus jamais. Aucun pic, et les pointeurs
// vers les clés restent valides à vie (ce qui n'était PAS le cas avant : la
// réservation suivante pouvait tout déplacer).
//
// Le nombre de clés par bloc est une puissance de 2, donc (bloc, position) se
// tire de l'index par un décalage et un masque, sans division.
class Arene {
public:
    explicit Arene(int taille) : taille(taille), nb(0) {}

    ~Arene() { for (quint16* b : blocs) delete[] b; }

    Arene(const Arene&) = delete;              // possède ses blocs
    Arene& operator=(const Arene&) = delete;

    int getTaille() const { return taille; }
    size_t nbCles() const { return nb; }

    // Réserve une clé en fin d'arène et rend où l'écrire. Le pointeur reste
    // valide tant que l'arène vit.
    quint16* reserve() {
        if ((nb & MASQUE) == 0 && (nb >> BITS) == blocs.size())
            blocs.push_back(new quint16[(size_t)CLES_PAR_BLOC * (size_t)taille]);

        quint16* p = adresse((quint32)nb);
        nb++;
        return p;
    }

    // Index de la clé réservée en dernier.
    quint32 dernier() const { return (quint32)(nb - 1); }

    // Reprend la dernière clé réservée. L'enfant qu'on venait de générer s'est
    // révélé être un doublon : sa clé n'a aucune raison d'occuper l'arène pour
    // toujours, alors qu'elle y figure déjà. (Le bloc, lui, reste alloué : il
    // resservira à la clé suivante.)
    void annule() { nb--; }

    const quint16* lit(quint32 idx) const { return adresse(idx); }

private:
    // 65536 clés par bloc : ~1,5 Mo par bloc pour une clé de 24 o. Assez gros
    // pour que le tableau de blocs reste minuscule, assez petit pour ne pas
    // gaspiller sur les niveaux qui tiennent en peu d'états.
    static const int BITS = 16;
    static const size_t CLES_PAR_BLOC = 1u << BITS;
    static const size_t MASQUE = CLES_PAR_BLOC - 1;

    quint16* adresse(quint32 idx) const {
        return blocs[idx >> BITS] + ((size_t)(idx & MASQUE) * (size_t)taille);
    }

    std::vector<quint16*> blocs;
    int taille;
    size_t nb;      // nombre de clés vivantes
};

// Référence vers une clé de l'arène : 4 octets, quel que soit le nombre de
// caisses du niveau.
struct Cle {
    quint32 offset;
};

// Offset qu'aucune clé réelle ne peut porter : marque un slot vide dans TableG.
// (L'arène ne contient jamais 2^32 - 1 shorts ; on saturerait la mémoire bien
// avant.)
static const quint32 OFFSET_VIDE = 0xFFFFFFFFu;

// Hacher ou comparer une clé demande de LIRE l'arène — une Cle seule ne se
// suffit pas. Les foncteurs la portent donc explicitement, plutôt que d'aller la
// chercher dans une variable globale ou un thread_local : la dépendance reste
// visible dans le type, et deux solveurs peuvent tourner côte à côte sans se
// marcher dessus.
//
// C'est aussi la raison du passage de QHash/QSet à std::unordered_map/set : les
// conteneurs Qt exigent un qHash(T) et un operator== GLOBAUX, qui n'ont aucun
// moyen de recevoir l'arène.
struct CleHash {
    const Arene* arene;

    size_t operator()(Cle c) const {
        // FNV-1a, sur les N+1 shorts de la clé.
        const quint16* p = arene->lit(c.offset);
        size_t h = 1469598103934665603ULL;
        for (int i = 0; i < arene->getTaille(); ++i) {
            h ^= (size_t)p[i];
            h *= 1099511628211ULL;
        }
        return h;
    }
};

struct CleEq {
    const Arene* arene;

    bool operator()(Cle a, Cle b) const {
        if (a.offset == b.offset) return true;   // même clé, physiquement

        const quint16* pa = arene->lit(a.offset);
        const quint16* pb = arene->lit(b.offset);
        for (int i = 0; i < arene->getTaille(); ++i)
            if (pa[i] != pb[i]) return false;

        return true;
    }
};

// Table clé -> meilleur g connu, à ADRESSAGE OUVERT.
//
// Remplace un std::unordered_map<Cle,int>, qui était devenu le poste mémoire
// dominant du solveur : instrumenté sur le niveau 3, 18,5 M d'entrées y
// coûtaient ~800 Mo. Une map chaînée paie, PAR ENTRÉE, un noeud alloué
// individuellement (32 o : la paire, le pointeur de chaînage, l'arrondi de
// l'allocateur) plus sa part du tableau de seaux — soit ~40 o d'infrastructure
// pour 8 o de contenu utile.
//
// Ici, un seul std::vector de cellules de 8 octets (Cle + g), sondage linéaire,
// aucune allocation par entrée. À 70 % de charge, le même contenu tient dans
// ~210 Mo.
//
// Le sondage linéaire (slot suivant, et non un double hachage) est délibéré :
// les collisions se résolvent en avançant dans la même ligne de cache, ce qui
// est bien plus rapide que de sauter au hasard dans la table — au prix d'un
// regroupement des clés qui ne pénalise qu'aux charges élevées, d'où le seuil
// à 70 %.
//
// Ne fait PAS de suppression : le solveur n'oublie jamais un état. C'est ce qui
// autorise le sondage linéaire nu, sans pierre tombale.
//
// ⚠️ Le tableau s'appelle 'cellules' et surtout PAS 'slots' : Qt fait un
// #define slots (qobjectdefs.h, pour écrire « public slots: »). Un membre nommé
// slots disparaît donc à la compilation — « declaration does not declare
// anything » — et chaque slots[i] se réduit à [i], que le compilateur lit comme
// l'ouverture d'une lambda. Le déluge d'erreurs ne pointe jamais la cause.
class TableG {
public:
    struct Slot {
        Cle  cle;   // OFFSET_VIDE => cellule libre
        qint32 g;
    };

    explicit TableG(const Arene* arene, size_t capaciteInitiale = 1024)
        : hash{arene}, eq{arene}, nb(0) {
        size_t cap = 1;
        while (cap < capaciteInitiale) cap <<= 1;   // puissance de 2 : masque au lieu d'un modulo
        cellules.assign(cap, Slot{Cle{OFFSET_VIDE}, 0});
    }

    size_t size() const { return nb; }

    // Rend le slot de 'c', ou nullptr s'il est absent.
    Slot* cherche(Cle c) {
        size_t i = hash(c) & (cellules.size() - 1);
        while (cellules[i].cle.offset != OFFSET_VIDE) {
            if (eq(cellules[i].cle, c)) return &cellules[i];
            i = (i + 1) & (cellules.size() - 1);
        }
        return nullptr;
    }

    // Insère 'c' avec la valeur 'g'. Suppose la clé ABSENTE (le solveur ne
    // l'appelle qu'après un cherche() infructueux).
    void insere(Cle c, int g) {
        if ((nb + 1) * 10 >= cellules.size() * 7) agrandit();   // charge > 70 %

        size_t i = hash(c) & (cellules.size() - 1);
        while (cellules[i].cle.offset != OFFSET_VIDE)
            i = (i + 1) & (cellules.size() - 1);

        cellules[i] = Slot{c, g};
        nb++;
    }

    // Dimensionne la table pour accueillir 'nbEtats' clés sans réallouer.
    void reserve(size_t nbEtats) {
        size_t cap = 1;
        while (cap * 7 < nbEtats * 10) cap <<= 1;
        if (cap > cellules.size()) rehache(cap);
    }

private:
    void agrandit() { rehache(cellules.size() * 2); }

    void rehache(size_t cap) {
        std::vector<Slot> anciens(cap, Slot{Cle{OFFSET_VIDE}, 0});
        cellules.swap(anciens);

        for (const Slot& s : anciens) {
            if (s.cle.offset == OFFSET_VIDE) continue;
            size_t i = hash(s.cle) & (cellules.size() - 1);
            while (cellules[i].cle.offset != OFFSET_VIDE)
                i = (i + 1) & (cellules.size() - 1);
            cellules[i] = s;
        }
    }

    // Pas de membre 'arene' : les foncteurs la portent déjà, la dupliquer
    // n'ajouterait qu'une seconde source de vérité.
    CleHash hash;
    CleEq   eq;
    std::vector<Slot> cellules;
    size_t nb;
};

#endif // CLE_H
