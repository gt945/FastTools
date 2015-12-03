#-------------------------------------------------
#
# Project created by QtCreator 2015-10-22T17:19:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FastTools
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    customcmddialog.cpp

HEADERS  += mainwindow.h \
    customcmddialog.h

FORMS    += mainwindow.ui \
    customcmddialog.ui
#QMAKE_LFLAGS += -Wl,-static -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-s

RESOURCES += \
    resource.qrc

