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

LIBS+= -L /usr/local/lib \
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

LIBS          += /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/ardrone.o \
                /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/command.o \
                /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/config.o  \
                /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/udp.o     \
                /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/tcp.o     \
                /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/navdata.o \
                /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/version.o \
                /home/kevin/Projects/Capstone/AriesGUI/src/ardrone/video.o

