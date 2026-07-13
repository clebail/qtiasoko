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
// On les range bout à bout dans un seul tableau qui double, et tout le reste du
// solveur — tables de dédup, file ouverte — ne manipule plus qu'un offset 32
// bits dedans (struct Cle).
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
class Arene {
public:
    explicit Arene(int taille) : taille(taille) {}

    int getTaille() const { return taille; }
    size_t nbCles() const { return taille ? mots.size() / (size_t)taille : 0; }

    // Réserve une clé en fin de zone et rend où l'écrire.
    //
    // ⚠️ Le pointeur rendu est invalidé par la réservation SUIVANTE (le tableau
    // peut être réalloué en doublant). Il sert à écrire la clé immédiatement,
    // jamais à être conservé — les conteneurs, eux, ne gardent que des offsets,
    // qu'une réallocation ne bouge pas.
    quint16* reserve() {
        mots.resize(mots.size() + (size_t)taille);
        return mots.data() + mots.size() - (size_t)taille;
    }

    // Offset de la clé réservée en dernier.
    quint32 dernier() const { return (quint32)(mots.size() - (size_t)taille); }

    // Reprend la dernière clé réservée. L'enfant qu'on venait de générer s'est
    // révélé être un doublon : sa clé n'a aucune raison d'occuper l'arène pour
    // toujours, alors qu'elle y figure déjà.
    void annule() { mots.resize(mots.size() - (size_t)taille); }

    const quint16* lit(quint32 offset) const { return mots.data() + offset; }

private:
    std::vector<quint16> mots;
    int taille;
};

// Référence vers une clé de l'arène : 4 octets, quel que soit le nombre de
// caisses du niveau.
struct Cle {
    quint32 offset;
};

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

#endif // CLE_H
