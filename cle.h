#ifndef CLE_H
#define CLE_H

#include <QtGlobal>
#include <cstddef>
#include <vector>
#include <new>
#include <cstdio>

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
// ── EMPAQUETAGE DES CLÉS (§6.5, chantier arène, 2026-08-13) ───────────────────
// Une clé est une suite de `n` indices de CASE, chacun dans [0, largeur*hauteur).
// Ils étaient rangés sur 16 bits alors que 8 ou 9 suffisent : mesuré sur les 35
// niveaux, `ceil(log2(taille du plateau))` vaut 6 à 9 bits — **46 % de l'arène**.
//
// Pourquoi le bit-packing et pas un delta+varint, qui gagnerait autant : le packing
// garde une longueur FIXE. L'arène conserve son pas fixe (`offset x octetsParCle`),
// `CleEq` reste un memcmp — sur MOINS d'octets, donc plus rapide — et le codage est
// canonique par construction, donc deux états égaux donnent deux suites d'octets
// égales. Un varint casserait les trois.
//
// ⚠️ Les bits inutilisés du dernier octet sont mis à ZÉRO : sans ça deux clés égales
// pourraient différer sur ces bits-là et le memcmp mentirait.
// ⚠️ Le tampon de destination doit avoir UN OCTET DE RAB : l'écriture d'une valeur à
// cheval touche `octet+1`, même pour la dernière valeur.
inline void empaqueteCle(const quint16* src, int n, int bits, quint8* dst, size_t octets) {
    for (size_t k = 0; k <= octets; k++) dst[k] = 0;
    size_t bit = 0;
    for (int i = 0; i < n; i++) {
        const size_t o = bit >> 3;
        const int    d = (int)(bit & 7);
        const quint32 v = (quint32)src[i] << d;
        dst[o]     |= (quint8)(v & 0xFF);
        dst[o + 1] |= (quint8)((v >> 8) & 0xFF);   // bits <= 9 et d <= 7 => 2 octets suffisent
        bit += (size_t)bits;
    }
}

inline void depaqueteCle(const quint8* src, int n, int bits, quint16* dst) {
    const quint32 masque = (1u << bits) - 1u;
    size_t bit = 0;
    for (int i = 0; i < n; i++) {
        const size_t o = bit >> 3;
        const int    d = (int)(bit & 7);
        const quint32 w = (quint32)src[o] | ((quint32)src[o + 1] << 8);
        dst[i] = (quint16)((w >> d) & masque);
        bit += (size_t)bits;
    }
}

// Nombre de bits par indice de case pour un plateau de `taillePlateau` cases.
inline int bitsParCase(int taillePlateau) {
    int b = 1;
    while ((1 << b) < taillePlateau) b++;
    return b;
}

class Arene {
public:
    // 'taille' = nombre de VALEURS par clé (nbCaisses + 1). 'taillePlateau' donne le
    // nombre de bits par valeur. ⚠️ Le pas de l'arène est désormais en OCTETS, pas en
    // shorts : confondre les deux passerait en silence, c'est la forme du piège
    // `idxCaisse` en quint8 du §7. D'où deux accesseurs distincts et nommés.
    Arene(int taille, int taillePlateau)
        : taille(taille), bits(bitsParCase(taillePlateau)),
          octetsCle(((size_t)taille * bitsParCase(taillePlateau) + 7) / 8),
          nb(0) {}

    ~Arene() { for (quint8* b : blocs) delete[] b; }

    Arene(const Arene&) = delete;              // possède ses blocs
    Arene& operator=(const Arene&) = delete;

    int getTaille() const { return taille; }          // valeurs par clé
    int getBits() const { return bits; }               // bits par valeur
    size_t getOctetsCle() const { return octetsCle; }  // octets par clé — le PAS
    size_t nbCles() const { return nb; }
    // Octets RÉELLEMENT alloués : des blocs entiers, dont le dernier est
    // partiellement rempli. Mesurer `nbCles * taille * 2` sous-estimerait.
    size_t octets() const { return blocs.size() * CLES_PAR_BLOC * (octetsCle + 1); }

    // Réserve une clé en fin d'arène et rend où l'écrire. Le pointeur reste
    // valide tant que l'arène vit.
    // Réserve une clé et l'ÉCRIT empaquetée depuis 'valeurs'. L'empaquetage est fait
    // ici plutôt que par l'appelant : un seul endroit sait que l'arène est packée.
    // ⚠️ Chaque emplacement porte UN OCTET DE RAB (d'où `octetsCle + 1` partout) :
    // empaqueteCle écrit à `octet+1` pour une valeur à cheval, y compris la dernière.
    void ecrit(const quint16* valeurs) {
        if ((nb & MASQUE_BLOC) == 0 && (nb >> BITS_BLOC) == blocs.size())
            try { blocs.push_back(new quint8[(size_t)CLES_PAR_BLOC * (octetsCle + 1)]); }
            catch (const std::bad_alloc&) {
                fprintf(stderr, "[BADALLOC] ARENE : bloc %zu de %.1f Mo REFUSE (%zu cles)\n",
                        blocs.size(), CLES_PAR_BLOC * (octetsCle + 1) / 1048576.0, nb);
                fflush(stderr); throw;
            }
        empaqueteCle(valeurs, taille, bits, adresse((quint32)nb), octetsCle);
        nb++;
    }

    // Index de la clé réservée en dernier.
    quint32 dernier() const { return (quint32)(nb - 1); }

    // Reprend la dernière clé réservée. L'enfant qu'on venait de générer s'est
    // révélé être un doublon : sa clé n'a aucune raison d'occuper l'arène pour
    // toujours, alors qu'elle y figure déjà. (Le bloc, lui, reste alloué : il
    // resservira à la clé suivante.)
    void annule() { nb--; }

    // Rend les octets EMPAQUETÉS — pour le hachage et la comparaison, qui n'ont pas
    // besoin de décoder.
    const quint8* lit(quint32 idx) const { return adresse(idx); }
    // Décode la clé 'idx' dans 'valeurs' (tailleCle() shorts). C'est la seule forme
    // que `appliqueEtat` sait lire.
    void depaquete(quint32 idx, quint16* valeurs) const {
        depaqueteCle(adresse(idx), taille, bits, valeurs);
    }

private:
    // 65536 clés par bloc : ~1,5 Mo par bloc pour une clé de 24 o. Assez gros
    // pour que le tableau de blocs reste minuscule, assez petit pour ne pas
    // gaspiller sur les niveaux qui tiennent en peu d'états.
    // ⚠️ RENOMMÉ en BITS_BLOC le 2026-08-13 : il y a désormais un membre `bits` qui
    // désigne les bits PAR VALEUR de clé. Deux « bits » de sens différents dans la
    // même classe, c'est le genre de voisinage qui produit un bug qu'on relit dix fois
    // sans le voir.
    static const int BITS_BLOC = 16;
    static const size_t CLES_PAR_BLOC = 1u << BITS_BLOC;
    static const size_t MASQUE_BLOC = CLES_PAR_BLOC - 1;

    quint8* adresse(quint32 idx) const {
        return blocs[idx >> BITS_BLOC] + ((size_t)(idx & MASQUE_BLOC) * (octetsCle + 1));
    }

    std::vector<quint8*> blocs;
    int taille;        // valeurs par clé
    int bits;          // bits par valeur
    size_t octetsCle;  // octets utiles par clé (le rab d'un octet est en plus)
    size_t nb;         // nombre de clés vivantes
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
        // FNV-1a, sur les octets EMPAQUETÉS de la clé — moins d'octets qu'avant, donc
        // moins de tours de boucle. Aucun décodage : le codage étant canonique, deux
        // états égaux ont les mêmes octets.
        const quint8* p = arene->lit(c.offset);
        size_t h = 1469598103934665603ULL;
        for (size_t i = 0; i < arene->getOctetsCle(); ++i) {
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

        const quint8* pa = arene->lit(a.offset);
        const quint8* pb = arene->lit(b.offset);
        for (size_t i = 0; i < arene->getOctetsCle(); ++i)
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
// Ici, DEUX std::vector parallèles — offsets (4 o) et g (2 o) —, soit 6 octets par
// cellule, sondage linéaire, aucune allocation par entrée. À 70 % de charge, le même
// contenu tient dans ~160 Mo.
// (C'était un seul vector de `Slot` à 8 octets jusqu'au 2026-08-11 ; la séparation
// est expliquée sur la classe elle-même.)
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
// ⚠️ Les tableaux s'appellent 'offsets' et 'gs', et surtout PAS 'slots' : Qt fait un
// #define slots (qobjectdefs.h, pour écrire « public slots: »). Un membre nommé
// slots disparaît donc à la compilation — « declaration does not declare
// anything » — et chaque slots[i] se réduit à [i], que le compilateur lit comme
// l'ouverture d'une lambda. Le déluge d'erreurs ne pointe jamais la cause.
#ifdef INSTRUM_SONDE
// Coût du SONDAGE LINÉAIRE en fonction de la charge (§6.5, chantier mur mémoire
// 2026-08-11). Question posée : `TableG` gaspille 2 662 Mo de cellules vides au mur
// (charge 35 % juste après un doublement) ; on peut la serrer en montant le seuil ou
// en réduisant le facteur de croissance — mais `cherche`/`insere` sont dans le chemin
// le PLUS chaud du solveur. Combien coûte une sonde de plus ?
// ⚠️ Ne compile QUE dans les harnais : le code produit ne définit jamais INSTRUM_SONDE.
struct StatsSonde {
    unsigned long long chercheAppels = 0, chercheSondes = 0;   // sondes = cellules visitées
    unsigned long long insereAppels  = 0, insereSondes  = 0;
};
StatsSonde& statsSonde();
#endif

// ⚠️ DEUX TABLEAUX PARALLÈLES, PAS UN TABLEAU DE STRUCTURES (§6.5, 2026-08-11).
// L'ancienne forme était `struct Slot { Cle cle; qint32 g; }`, soit 8 octets. Deux
// raisons de l'avoir cassée, et la seconde compte autant que la première :
//   1. `g` est un nombre de POUSSÉES — 639 au maximum jamais observé, sur tous les
//      niveaux. Un `quint16` suffit (65 535), mais un `struct{quint32;quint16}` est
//      repadé à 8 par l'alignement : la structure annulait le gain. Séparés, c'est
//      **6 octets par cellule au lieu de 8, soit −25 %** — 1 024 Mo sur le run du 29.
//   2. **La boucle de sondage ne compare QUE l'offset.** En tableaux séparés elle ne
//      touche que celui de 4 octets, donc **16 cellules par ligne de cache au lieu de
//      8**. `g` n'est lu qu'en cas de succès. Mesuré : le sondage coûte 1,89 sonde à
//      35 % de charge et **4,40 à 70 %** — c'est la boucle la plus chaude après le
//      flood-fill, et la diviser par deux en défauts de cache vaut autant que la RAM.
// ⚠️ `cherche` rend donc un INDEX, plus un pointeur : il n'y a plus de structure vers
// laquelle pointer. `ABSENT` marque l'échec.
class TableG {
public:
    static const size_t ABSENT = (size_t)-1;

    explicit TableG(const Arene* arene, size_t capaciteInitiale = 1024)
        : hash{arene}, eq{arene}, nb(0) {
        size_t cap = 1;
        while (cap < capaciteInitiale) cap <<= 1;   // puissance de 2 : masque au lieu d'un modulo
        offsets.assign(cap, OFFSET_VIDE);
        gs.assign(cap, 0);
    }

    Cle    cle(size_t i) const { return Cle{offsets[i]}; }
    quint16 g(size_t i) const  { return gs[i]; }
    // ⚠️ `g` est un nombre de POUSSÉES, tenu sur 16 bits depuis le 2026-08-11 (§6.5).
    // Maximum observé sur les 35 niveaux : 639 ; plafond 65 535, soit cent fois la
    // marge. La garde est là parce que le §7 collectionne les troncatures muettes —
    // `idxCaisse` en quint8 débordait sans un bruit et le canari n'y voyait rien.
    // Un dépassement rendrait un `g` FAUX, donc un chemin plus court que le réel,
    // donc une solution qui n'existe pas : ça ne planterait jamais, ça mentirait.
    void   setG(size_t i, int v) {
        Q_ASSERT_X(v >= 0 && v <= 65535, "TableG::setG", "g deborde le quint16 (§6.5)");
        gs[i] = (quint16)v;
    }

    size_t size() const { return nb; }
    // Nombre de CELLULES allouées, pas d'entrées occupées. C'est cette valeur qui
    // pèse en mémoire (chaque cellule est un Slot, occupée ou non) — `size()` ne
    // dit rien du coût. Ajouté pour le chantier MUR MÉMOIRE (§6.5, 2026-08-11).
    size_t capacite() const { return offsets.size(); }
    static constexpr size_t octetsParCellule() { return sizeof(quint32) + sizeof(quint16); }

    // Rend l'INDEX de 'c', ou ABSENT. La boucle ne lit que `offsets` — c'est tout
    // l'intérêt de la séparation.
    size_t cherche(Cle c) const {
        size_t i = hash(c) & (offsets.size() - 1);
#ifdef INSTRUM_SONDE
        StatsSonde& st = statsSonde();
        st.chercheAppels++;
        unsigned long long n = 1;
#endif
        while (offsets[i] != OFFSET_VIDE) {
            if (eq(Cle{offsets[i]}, c)) {
#ifdef INSTRUM_SONDE
                st.chercheSondes += n;
#endif
                return i;
            }
            i = (i + 1) & (offsets.size() - 1);
#ifdef INSTRUM_SONDE
            n++;
#endif
        }
#ifdef INSTRUM_SONDE
        st.chercheSondes += n;
#endif
        return ABSENT;
    }

    // Insère 'c' avec la valeur 'g'. Suppose la clé ABSENTE (le solveur ne
    // l'appelle qu'après un cherche() infructueux).
    void insere(Cle c, int g) {
        if ((nb + 1) * 10 >= offsets.size() * 7) agrandit();   // charge > 70 %

        size_t i = hash(c) & (offsets.size() - 1);
#ifdef INSTRUM_SONDE
        StatsSonde& st = statsSonde();
        st.insereAppels++;
        unsigned long long n = 1;
#endif
        while (offsets[i] != OFFSET_VIDE) {
            i = (i + 1) & (offsets.size() - 1);
#ifdef INSTRUM_SONDE
            n++;
#endif
        }
#ifdef INSTRUM_SONDE
        st.insereSondes += n;
#endif

        Q_ASSERT_X(g >= 0 && g <= 65535, "TableG::insere", "g deborde le quint16 (§6.5)");
        offsets[i] = c.offset;
        gs[i] = (quint16)g;
        nb++;
    }

    // ❌ LE DIMENSIONNEMENT UNIQUE, PROPOSÉ TROIS FOIS, JAMAIS RETENU (§6.5).
    // Idée : allouer la table une fois sur un budget dérivé de la RAM, pour supprimer
    // la pointe de réhachage. Réfuté par l'arithmétique, et il faut le garder écrit
    // parce que l'idée revient à chaque fois qu'on regarde le problème :
    //   pointe = (1 + 1/k) x la cible,  charge après croissance = 70 %/k.
    // Sauter à la MOITIÉ du budget donne donc **exactement la pointe du doublement**
    // (18 898 Mo dans les deux cas sur le 29, calculé). Pour abaisser la pointe il faut
    // sauter TÔT — à 1/8 du budget — et on sur-dimensionne alors d'un facteur 8 les
    // runs moyens : le 26, qui termine avec 1 536 Mo de table, en prendrait 6 144.
    // **Aucun facteur de croissance ne gagne sur les deux tableaux.**
    // La réponse retenue est ailleurs : survivre au refus (cf. `agrandit`).

    // Dimensionne la table pour accueillir 'nbEtats' clés sans réallouer.
    void reserve(size_t nbEtats) {
        size_t cap = 1;
        while (cap * 7 < nbEtats * 10) cap <<= 1;
        if (cap > offsets.size()) rehache(cap);
    }

private:
    // ✅ SURVIVRE AU REFUS D'ALLOCATION (§6.5, 2026-08-13). Le 29 mourait sur
    // `std::bad_alloc` DEUX RUNS DE SUITE au même dépilement : table à 3 072 Mo
    // voulant doubler à 6 144, donc 9 216 Mo de pointe et 18,9 Go au total sur une
    // machine de 18 — alors qu'il restait 5 Go libres et que le run pouvait continuer.
    //
    // Ici on refuse de mourir : si le doublement échoue, on GARDE la table actuelle.
    // La charge dépasse alors 70 %, les sondes s'allongent (mesuré : 4,40 sondes à
    // 70 %, ~22 à 85 %) — le solveur ralentit, il ne s'arrête pas. C'est le même
    // principe que le repli du régime d'engagement : dégrader plutôt que renoncer.
    //
    // ⚠️ Aucune correction n'était possible par le facteur de croissance : sauter à la
    // moitié du budget donne EXACTEMENT la pointe du doublement (mesuré, 18 898 Mo
    // dans les deux cas), et sauter plus tôt sur-dimensionne d'un facteur 4 à 8 les
    // runs moyens — le 26, qui finit à 1 536 Mo de table, aurait pris 6 144 Mo.
    // ⚠️ Le sondage linéaire à charge > 90 % s'effondre (séquences de centaines de
    // cellules). On le SIGNALE plutôt que de le subir en silence.
    void agrandit() {
        const size_t vise = offsets.size() * 2;
        try {
            rehache(vise);
        } catch (const std::bad_alloc&) {
            fprintf(stderr, "[TABLEG] agrandissement REFUSE (%zu -> %zu cellules, %.0f Mo). "
                            "On continue a charge %.1f %% — le solveur ralentit, il ne meurt pas.\n",
                    offsets.size(), vise, vise * octetsParCellule() / (1024.0*1024.0),
                    100.0 * nb / offsets.size());
            fflush(stderr);
        }
    }

    // ⚠️ Pendant le réhachage, l'ANCIENNE et la NOUVELLE table coexistent : la pointe
    // vaut 1,5 fois la taille visée. Sur le 29, le passage de 2 048 à 4 096 Mo a donc
    // demandé 6 144 Mo en un instant — c'est probablement ce bond, et non la
    // croissance graduelle, qui a tué le run (§6.5). Non corrigé ici.
    void rehache(size_t cap) {
        // ⚠️ POINTE DU RÉHACHAGE (§6.5, 2026-08-11). L'ancienne table et la nouvelle
        // coexistent le temps de la ré-insertion. On TRACE la transition au lieu de
        // la déduire : sur le 29, `footprint` rendait 14 Go résidents pour 15 Go de
        // pic, ce qui est trop peu pour une pointe à 1,5× — donc soit la coexistence
        // coûte moins que prévu, soit le pic échantillonné l'a manquée.
        {
            const double MO = 1024.0 * 1024.0;
            const size_t oAnc = offsets.size() * (sizeof(quint32) + sizeof(quint16));
            const size_t oNew = cap * (sizeof(quint32) + sizeof(quint16));
            fprintf(stderr, "[REHASH] %zu -> %zu cellules | ancienne %.0f Mo + nouvelle %.0f Mo "
                            "= POINTE %.0f Mo (%.2fx la cible) | %zu entrees\n",
                    offsets.size(), cap, oAnc / MO, oNew / MO, (oAnc + oNew) / MO,
                    oNew ? (double)(oAnc + oNew) / oNew : 0.0, nb);
            fflush(stderr);
        }
        std::vector<quint32> ancOffsets(cap, OFFSET_VIDE);
        std::vector<quint16> ancGs(cap, 0);
        offsets.swap(ancOffsets);
        gs.swap(ancGs);

        for (size_t k = 0; k < ancOffsets.size(); k++) {
            if (ancOffsets[k] == OFFSET_VIDE) continue;
            size_t i = hash(Cle{ancOffsets[k]}) & (offsets.size() - 1);
            while (offsets[i] != OFFSET_VIDE)
                i = (i + 1) & (offsets.size() - 1);
            offsets[i] = ancOffsets[k];
            gs[i] = ancGs[k];
        }
    }

    // Pas de membre 'arene' : les foncteurs la portent déjà, la dupliquer
    // n'ajouterait qu'une seconde source de vérité.
    CleHash hash;
    CleEq   eq;
    std::vector<quint32> offsets;   // OFFSET_VIDE => cellule libre
    std::vector<quint16> gs;
    size_t nb;
};

#endif // CLE_H
