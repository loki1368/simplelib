#-------------------------------------------------
#
# Project created by QtCreator 2014-12-23T01:21:45
#
#-------------------------------------------------



greaterThan(QT_MAJOR_VERSION, 4){
    QT += widgets core gui sql xmlpatterns concurrent
    DEFINES += HAVE_QT5
}

TARGET = simplelib
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    settingsdialog.cpp \
    smplibdatabase.cpp \
    parsebigzip.cpp

HEADERS  += mainwindow.h \
    settingsdialog.h \
    smplibdatabase.h \
    parsebigzip.h

FORMS    += mainwindow.ui \
    settingsdialog.ui

unix:!macx: LIBS += -L$$PWD/../../../../usr/lib/

INCLUDEPATH += $$PWD/../../../../usr/include
DEPENDPATH += $$PWD/../../../../usr/include

unix:!macx: LIBS += -L$$PWD/../../../../usr/lib/ -lquazip

INCLUDEPATH += $$PWD/quazip
DEPENDPATH += $$PWD/quazip
