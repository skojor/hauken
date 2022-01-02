QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++11
QMAKE_CXXFLAGS += -pthread
QMAKE_CXXFLAGS += -std=c++11
TARGET = Hauken
RC_ICONS = icons/icon.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    config.cpp \
    customplotcontroller.cpp \
    datastreambaseclass.cpp \
    generaloptions.cpp \
    main.cpp \
    mainwindow.cpp \
    measurementdevice.cpp \
    optionsbaseclass.cpp \
    qcustomplot.cpp \
    receiveroptions.cpp \
    sdefoptions.cpp \
    sdefrecorder.cpp \
    tcpdatastream.cpp \
    traceanalyzer.cpp \
    tracebuffer.cpp \
    udpdatastream.cpp

HEADERS += \
    config.h \
    customplotcontroller.h \
    datastreambaseclass.h \
    generaloptions.h \
    mainwindow.h \
    measurementdevice.h \
    optionsbaseclass.h \
    qcustomplot.h \
    receiveroptions.h \
    sdefoptions.h \
    sdefrecorder.h \
    tcpdatastream.h \
    traceanalyzer.h \
    tracebuffer.h \
    typedefs.h \
    udpdatastream.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc
#
win32 {
  LIBS += -lOpenGL32
}

#DEFINES += QCUSTOMPLOT_USE_OPENGL
DEFINES += SW_VERSION=\\\"$$system(git describe --always)\\\"
#DEFINES += GIT_LOG=\\\"$$system(git log --oneline)\\\"
#GIT_VERSION = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags) #$$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags  --abbrev=0) #$$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
#GIT_VERSION = \\\"$$GIT_VERSION\\\"

#win32 {
#    VERSION ~= s/-\d+-g[a-f0-9]{6,}//
#}
