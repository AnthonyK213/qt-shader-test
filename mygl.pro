######################################################################
# Automatically generated by qmake (3.1) Thu Apr 6 14:45:47 2023
######################################################################

TEMPLATE = app
TARGET = mygl
INCLUDEPATH += .
QT += widgets

# You can make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# Please consult the documentation of the deprecated API in order to know
# how to port your code away from it.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += camera.h canvas.h geometry.h model.h mygl.h tgaimage.h
FORMS += mygl.ui
SOURCES += camera.cpp \
           canvas.cpp \
           geometry.cpp \
           main.cpp \
           model.cpp \
           mygl.cpp \
           tgaimage.cpp
TRANSLATIONS += mygl_en_US.ts
