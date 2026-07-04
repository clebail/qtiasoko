QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 debug
CONFIG -= release

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    caisse.cpp \
    game.cpp \
    goal.cpp \
    goalcaisse.cpp \
    level.cpp \
    main.cpp \
    mainwindow.cpp \
    mur.cpp \
    player.cpp \
    sprite.cpp \
    wgame.cpp

HEADERS += \
    caisse.h \
    game.h \
    goal.h \
    goalcaisse.h \
    level.h \
    mainwindow.h \
    mur.h \
    player.h \
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
