QT       += core gui serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
QMAKE_CXXFLAGS += -D_GNU_SOURCE

SOURCES += \
    main.cpp \
    ui/commSimulator.cpp \
    ui/mainwindow.cpp \
    comm/CANCommunication.cpp \
    comm/RS232Communication.cpp \

HEADERS += \
    comm/HardwareCommunication.h \
    ui/commSimulator.h \
    ui/mainwindow.h \
    comm/CANCommunication.h \
    comm/RS232Communication.h \

FORMS += \
    ui/mainwindow.ui

INCLUDEPATH += \
    $$PWD/include \
    comm \
    ui \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
