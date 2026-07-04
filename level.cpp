#include <QFile>
#include <QTextStream>
#include "level.h"

static const char valides_char[NB_SPRITE] = {' ', '#', '@', '$', '.', '*', '+'};

Level::Level() {}

Level::Level(const Level& other)
    : loaded(other.loaded), largeur(other.largeur), hauteur(other.hauteur), cases(other.cases) {}

Level& Level::operator=(const Level& other) {
    if (this == &other) return *this;
    loaded = other.loaded;
    largeur = other.largeur;
    hauteur = other.hauteur;
    cases = other.cases;
    return *this;
}

Level::~Level() {}

void Level::load(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    QList<QString> lines;
    largeur = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        largeur = qMax(largeur, line.size());
        lines.append(line);
    }
    file.close();

    hauteur = lines.size();
    cases.clear();
    int y = 0;
    for (const QString& line : lines) {
        for (int x = 0; x < largeur; x++) {
            char car = (x < line.size()) ? line[x].toLatin1() : ' ';
            int idx = 0;
            for (int i = 0; i < NB_SPRITE; i++) {
                if (valides_char[i] == car) {
                    idx = i;
                    break;
                }
            }
            cases.append({static_cast<ETypeCase>(idx), car, idx});
        }
        y++;
    }
    loaded = true;
}

bool Level::isLoaded() const {
    return loaded;
}
