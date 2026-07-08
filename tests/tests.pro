QT       += core gui testlib
QT       -= widgets

CONFIG   += c++17 console debug
CONFIG   -= app_bundle release

TEMPLATE = app
TARGET   = tst_getetat

INCLUDEPATH += $$PWD/..

# Chemin vers les .xsb (racine du projet), injecté dans le binaire de test.
DEFINES += LEVELS_DIR=\\\"$$PWD/..\\\"

SOURCES += \
    tst_getetat.cpp \
    ../game.cpp \
    ../level.cpp \
    ../sprite.cpp \
    ../player.cpp \
    ../mur.cpp \
    ../caisse.cpp \
    ../goal.cpp \
    ../goalcaisse.cpp \
    ../astar.cpp

HEADERS += \
    ../game.h \
    ../level.h \
    ../sprite.h \
    ../player.h \
    ../mur.h \
    ../caisse.h \
    ../goal.h \
    ../goalcaisse.h \
    ../astar.h
