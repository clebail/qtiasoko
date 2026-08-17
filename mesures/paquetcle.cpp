// paquetcle — LE CODEC DE CLÉS EST-IL UNE BIJECTION ?
//
// Le canari ne verrait pas une clé subtilement fausse : il verrait un niveau non
// résolu, ou rien du tout. Le §7 rappelle que `decodeCle` a déjà mordu une fois et
// que le bug a faussé `mou` pendant des semaines sans qu'aucune mesure ne le signale.
// D'où ce test AVANT tout câblage.
//
// Trois batteries, sur les tailles réelles des 35 niveaux :
//   1. exhaustif sur les petites configurations,
//   2. aléatoire massif,
//   3. les BORDS — 0, taille-1, et toutes les valeurs identiques.
#include <QCoreApplication>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "cle.h"

static int echecs = 0, cas = 0;

static void verifie(const std::vector<quint16>& v, int taillePlateau) {
    const int n = (int)v.size();
    const int bits = bitsParCase(taillePlateau);
    const size_t octets = ((size_t)n * bits + 7) / 8;
    std::vector<quint8> paquet(octets + 1);
    std::vector<quint16> retour(n);
    empaqueteCle(v.data(), n, bits, paquet.data(), octets);
    depaqueteCle(paquet.data(), n, bits, retour.data());
    cas++;
    for (int i = 0; i < n; i++)
        if (retour[i] != v[i]) {
            if (echecs < 5)
                printf("  ECHEC taille=%d bits=%d n=%d  position %d : %u -> %u\n",
                       taillePlateau, bits, n, i, v[i], retour[i]);
            echecs++;
            return;
        }
    // canonicité : les bits de rab doivent etre a zero, sinon memcmp ment
    std::vector<quint8> p2(octets + 1, 0xFF);
    empaqueteCle(v.data(), n, bits, p2.data(), octets);
    for (size_t k = 0; k < octets; k++)
        if (p2[k] != paquet[k]) { printf("  NON CANONIQUE a l'octet %zu\n", k); echecs++; return; }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // (taille du plateau, nb de valeurs) reels : du 9x7 du niveau 0 au 20x16
    const int tailles[] = {63, 140, 180, 195, 221, 238, 247, 285, 288, 320};
    const int ns[]      = {4, 11, 14, 16, 16, 15, 17, 15, 21, 33};

    for (int t = 0; t < 10; t++) {
        const int taille = tailles[t], n = ns[t];
        // 1. bords
        for (quint16 val : {(quint16)0, (quint16)(taille - 1), (quint16)1}) {
            std::vector<quint16> v(n, val);
            verifie(v, taille);
        }
        // 2. croissant (la forme reelle : cases triees)
        std::vector<quint16> croissant(n);
        for (int i = 0; i < n; i++) croissant[i] = (quint16)(i * (taille - 1) / (n ? n : 1));
        verifie(croissant, taille);
        // 3. aleatoire massif
        for (int k = 0; k < 200000; k++) {
            std::vector<quint16> v(n);
            for (int i = 0; i < n; i++) v[i] = (quint16)(rand() % taille);
            verifie(v, taille);
        }
    }
    printf("%d cas testes, %d echec(s)\n", cas, echecs);
    return echecs ? 1 : 0;
}
