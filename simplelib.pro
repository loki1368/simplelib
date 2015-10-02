#-------------------------------------------------
#
# Project created by QtCreator 2014-12-23T01:21:45
#
#-------------------------------------------------



greaterThan(QT_MAJOR_VERSION, 4){
    QT += widgets core gui sql xmlpatterns concurrent
    DEFINES += HAVE_QT5
}

CONFIG += c++11

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

INCLUDEPATH += ./quazip
DEPENDPATH += ./quazip

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/build-quazip-Desktop-Debug/release/ -lquazip
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/build-quazip-Desktop-Debug/debug/ -lquazip

INCLUDEPATH += $$PWD/quazip
DEPENDPATH += $$PWD/quazip

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/build-quazip-Desktop-Debug/release/libquazip.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/build-quazip-Desktop-Debug/debug/libquazip.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/build-quazip-Desktop-Debug/release/quazip.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/build-quazip-Desktop-Debug/debug/quazip.lib

RESOURCES += \
    resource.qrc
