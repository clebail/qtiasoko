#ifndef LEVEL_H
#define LEVEL_H

#include <QList>
#include <QString>

#define NB_SPRITE           7

class Level {
public:
    typedef enum { tcNone, tcMur, tcPlayer, tcCaisse, tcGoal, tcGoalCaisse, tcGoalPlayer } ETypeCase;
    typedef struct _SCase {
        ETypeCase typeCase;
        char car;
        int idxSprite;
    }SCase;

    Level();
    Level(const Level& other);
    Level& operator=(const Level& other);
    ~Level();
    void load(const QString& fileName);
    bool isLoaded() const;
    int getLargeur() const { return largeur; }
    int getHauteur() const { return hauteur; }
    const QList<SCase>& getCases() const { return cases; }
private:
    bool loaded = false;
    int largeur;
    int hauteur;
    QList<SCase> cases;
};

#endif // LEVEL_H
