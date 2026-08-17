include(commun.pri)
QT += gui
TARGET = image
# ⚠️ Les classes de sprites viennent de l'APPLICATION, pas du solveur : cet outil
# réutilise le rendu de l'UI plutôt que d'en écrire un second (cf. l'entête de
# image.cpp). D'où les sources et la ressource ci-dessous, absentes de commun.pri.
SOURCES += image.cpp \
           $$P/sprite.cpp $$P/sol.cpp $$P/mur.cpp $$P/caisse.cpp \
           $$P/goal.cpp $$P/goalcaisse.cpp $$P/player.cpp
HEADERS += $$P/sprite.h $$P/sol.h $$P/mur.h $$P/caisse.h \
           $$P/goal.h $$P/goalcaisse.h $$P/player.h
RESOURCES += $$P/qtiasoko.qrc
