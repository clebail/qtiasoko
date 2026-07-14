# Socle commun aux harnais de mesure. Ce ne sont PAS des cibles de l'application :
# ils compilent les sources du solveur telles quelles et les instrumentent depuis
# l'exterieur. Rien ici n'entre dans qtiasoko.pro.
QT += core
QT -= gui
CONFIG += c++17 console release force_debug_info sdk_no_version_check
CONFIG -= app_bundle debug
TEMPLATE = app

P = $$PWD/..
DEFINES += LEVELS_DIR=\\\"$$P\\\"
DEFINES += SCRATCH=\\\"$$OUT_PWD\\\"
INCLUDEPATH += $$P

SOURCES += $$P/game.cpp $$P/level.cpp $$P/astar.cpp \
           $$P/solveur.cpp $$P/solveurbfs.cpp $$P/solveurastar.cpp
HEADERS += $$P/game.h $$P/level.h $$P/astar.h \
           $$P/solveur.h $$P/solveurbfs.h $$P/solveurastar.h $$P/cle.h
