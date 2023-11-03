#-------------------------------------------------
#
# Project created by QtCreator 2017-12-31T16:16:48
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EncodeUtility
TEMPLATE = app
CONFIG += c++2a
QMAKE_CXXFLAGS += /std:c++20

TRANSLATIONS = language/lang.ja
RC_FILE = resource.rc

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Custom build step
pre_build.commands = $$PWD/build_dependencies.bat

QMAKE_EXTRA_TARGETS += pre_build
PRE_TARGETDEPS += pre_build

# Link QuaZip library
CONFIG(debug, debug|release){
    LIBS += -L$$PWD/lib -lquazip1-qt6d -lzlibstatic
} else {
    LIBS += -L$$PWD/lib -lquazip1-qt6 -lzlibstatic
}
INCLUDEPATH += quazip
INCLUDEPATH += zlib

SOURCES += \
    Encoder/AACEncoder.cpp \
    Encoder/FlacEncoder.cpp \
    Encoder/MP3Encoder.cpp \
    Undo/SetTextCommand.cpp \
    main.cpp \
    MainWindow.cpp \
    DialogAppSettings.cpp

HEADERS += \
    Encoder/AACEncoder.h \
    AudioMetaData.hpp \
    Encoder/EncoderInterface.h \
    Encoder/FlacEncoder.h \
    Encoder/MP3Encoder.h \
    MainWindow.h \
    DialogAppSettings.h \
    ProjectDefines.hpp \
    Undo/SetTextCommand.h

FORMS += \
    MainWindow.ui \
    DialogAppSettings.ui

RESOURCES += \
    resource.qrc

DISTFILES += \
    LICENSE \
    README.md \
    resource.rc
