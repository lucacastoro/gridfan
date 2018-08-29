#-------------------------------------------------
#
# Project created by QtCreator 2018-08-29T17:01:12
#
#-------------------------------------------------

QT += core gui widgets

TARGET = gridfan-gui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14

SOURCES += \
	main.cpp \
	mainwindow.cpp \
    application.cpp \
    fancontrollerform.cpp

HEADERS += \
	mainwindow.h \
    application.h \
    fancontrollerform.h

FORMS += \
	mainwindow.ui \
    fancontrollerform.ui

LIBS += -L$$OUT_PWD/../libgridfan -lgridfan
INCLUDEPATH += $$PWD/../libgridfan
INCLUDEPATH += $$PWD/../../utils
DEPENDPATH += $$PWD/../libgridfan
PRE_TARGETDEPS += $$OUT_PWD/../libgridfan/libgridfan.so
