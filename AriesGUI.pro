TEMPLATE = app
TARGET = AriesGUI

QT = core gui
QT += multimedia
QT += bluetooth
QT += widgets
QT += multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


SOURCES += \
    main.cpp \
    window.cpp \
    camera_thread.cpp

HEADERS += \
    window.h \
    camera_thread.h

QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv

DISTFILES += \
    ../Downloads/kittens.jpeg

INCLUDEPATH += /usr/local/include/sphinxbase
INCLUDEPATH += /usr/local/include/pocketsphinx

LIBS+= -L/usr/local/lib \
    -lpocketsphinx \
    -lsphinxbase    \
    -lm                     \
    -lpthread               \
    -lavutil                \
    -lavformat              \
    -lavcodec               \
    -lswscale               \
    -lopencv_calib3d        \
    -lopencv_core           \
    -lopencv_features2d     \
    -lopencv_flann          \
    -lopencv_highgui        \
    -lopencv_imgproc        \
    -lopencv_ml             \
    -lopencv_objdetect      \
    -lopencv_video

LIBS          += /users/isabellahuang/AriesGUI/src/ardrone/ardrone.o \
                /users/isabellahuang/AriesGUI/src/ardrone/command.o \
                /users/isabellahuang/AriesGUI/src/ardrone/config.o  \
                /users/isabellahuang/AriesGUI/src/ardrone/udp.o     \
                /users/isabellahuang/AriesGUI/src/ardrone/tcp.o     \
                /users/isabellahuang/AriesGUI/src/ardrone/navdata.o \
                /users/isabellahuang/AriesGUI/src/ardrone/version.o \
                /users/isabellahuang/AriesGUI/src/ardrone/video.o

