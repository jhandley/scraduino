#-------------------------------------------------
#
# Project created by QtCreator 2014-06-07T07:08:51
#
#-------------------------------------------------

QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = scraduino
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    ScratchWebApi.cpp \
    Scratchcc.cpp \
    ArduinoBuilder.cpp \
    ArduinoUploader.cpp \
    SettingsDialog.cpp

HEADERS  += MainWindow.h \
    ScratchWebApi.h \
    Scratchcc.h \
    ArduinoBuilder.h \
    ArduinoUploader.h \
    SettingsDialog.h

FORMS    += MainWindow.ui \
    SettingsDialog.ui

RESOURCES += \
    resource.qrc
