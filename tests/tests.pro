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
    ../astar.cpp

HEADERS += \
    ../game.h \
    ../level.h \
    ../astar.h
