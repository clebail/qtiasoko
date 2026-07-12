QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Release (-O2) et non debug (-O0) : le solveur passe l'essentiel de son temps
# dans des boucles serrées (getZoneJoueur, getEtat, getCaissesDeplacable) que
# l'optimiseur transforme complètement. Un build debug fait tourner le solveur
# plusieurs fois moins vite, pour rien.
#
# force_debug_info garde les symboles (-g) : on reste débogable et profilable,
# on ne perd que les variables optimisées. Retire-le si tu veux poser des points
# d'arrêt fiables ligne à ligne — mais alors ne juge plus les perfs du solveur.
CONFIG += c++17 release force_debug_info
CONFIG -= debug

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    astar.cpp \
    caisse.cpp \
    game.cpp \
    goal.cpp \
    goalcaisse.cpp \
    level.cpp \
    main.cpp \
    mainwindow.cpp \
    mur.cpp \
    player.cpp \
    solveur.cpp \
    solveurbfs.cpp \
    sprite.cpp \
    wgame.cpp

HEADERS += \
    astar.h \
    caisse.h \
    game.h \
    goal.h \
    goalcaisse.h \
    level.h \
    mainwindow.h \
    mur.h \
    player.h \
    solveur.h \
    solveurbfs.h \
    sprite.h \
    wgame.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    qtiasoko.qrc

# Icône de l'application, un mécanisme par plateforme :
#  - macOS  : ICON, un .icns que qmake copie dans Contents/Resources et déclare
#             dans le CFBundleIconFile de l'Info.plist ;
#  - Windows: RC_ICONS, un .ico dont qmake tire une ressource Win32, ce qui donne
#             son icône au .exe lui-même ;
#  - Linux  : rien d'embarquable dans le binaire — l'icône de fenêtre/barre des
#             tâches vient de la propriété windowIcon de mainwindow.ui (qui
#             pointe sur :/icone.png), et l'icône du lanceur d'un .desktop.
macx:  ICON     = qtiasoko.icns
win32: RC_ICONS = qtiasoko.ico
