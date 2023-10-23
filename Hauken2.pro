QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport multimedia positioning

CONFIG += c++17
QMAKE_CXXFLAGS += -pthread
QMAKE_CXXFLAGS += -std=c++11
TARGET = Hauken
RC_ICONS = icons/hawk.ico #icon.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    SimpleMail/emailaddress.cpp \
    SimpleMail/mimeattachment.cpp \
    SimpleMail/mimecontentformatter.cpp \
    SimpleMail/mimefile.cpp \
    SimpleMail/mimehtml.cpp \
    SimpleMail/mimeinlinefile.cpp \
    SimpleMail/mimemessage.cpp \
    SimpleMail/mimemultipart.cpp \
    SimpleMail/mimepart.cpp \
    SimpleMail/mimetext.cpp \
    SimpleMail/quotedprintable.cpp \
    SimpleMail/sender.cpp \
    SimpleMail/server.cpp \
    SimpleMail/serverreply.cpp \
    arduino.cpp \
    arduinooptions.cpp \
    autorecorderoptions.cpp \
    cameraoptions.cpp \
    camerarecorder.cpp \
    config.cpp \
    customplotcontroller.cpp \
    datastreambaseclass.cpp \
    emailoptions.cpp \
    espenai.cpp \
    generaloptions.cpp \
    geolimit.cpp \
    geolimitoptions.cpp \
    gnssanalyzer.cpp \
    gnssdevice.cpp \
    gnssoptions.cpp \
    main.cpp \
    mainwindow.cpp \
    measurementdevice.cpp \
    mqtt.cpp \
    mqttoptions.cpp \
    notifications.cpp \
    optionsbaseclass.cpp \
    pmrtablewdg.cpp \
    positionreport.cpp \
    positionreportoptions.cpp \
    qcustomplot.cpp \
    receiveroptions.cpp \
    sdefoptions.cpp \
    sdefrecorder.cpp \
    tcpdatastream.cpp \
    traceanalyzer.cpp \
    tracebuffer.cpp \
    udpdatastream.cpp \
    waterfall.cpp

HEADERS += \
    SimpleMail/SimpleMail \
    SimpleMail/emailaddress.h \
    SimpleMail/emailaddress_p.h \
    SimpleMail/mimeattachment.h \
    SimpleMail/mimecontentformatter.h \
    SimpleMail/mimefile.h \
    SimpleMail/mimehtml.h \
    SimpleMail/mimeinlinefile.h \
    SimpleMail/mimemessage.h \
    SimpleMail/mimemessage_p.h \
    SimpleMail/mimemultipart.h \
    SimpleMail/mimemultipart_p.h \
    SimpleMail/mimepart.h \
    SimpleMail/mimepart_p.h \
    SimpleMail/mimetext.h \
    SimpleMail/quotedprintable.h \
    SimpleMail/sender.h \
    SimpleMail/sender_p.h \
    SimpleMail/server.h \
    SimpleMail/server_p.h \
    SimpleMail/serverreply.h \
    SimpleMail/serverreply_p.h \
    SimpleMail/smtpexports.h \
    arduino.h \
    arduinooptions.h \
    autorecorderoptions.h \
    cameraoptions.h \
    camerarecorder.h \
    config.h \
    customplotcontroller.h \
    datastreambaseclass.h \
    emailoptions.h \
    espenai.h \
    generaloptions.h \
    geolimit.h \
    geolimitoptions.h \
    gnssanalyzer.h \
    gnssdevice.h \
    gnssoptions.h \
    mainwindow.h \
    measurementdevice.h \
    mqtt.h \
    mqttoptions.h \
    notifications.h \
    optionsbaseclass.h \
    pmrtablewdg.h \
    positionreport.h \
    positionreportoptions.h \
    qcustomplot.h \
    receiveroptions.h \
    sdefoptions.h \
    sdefrecorder.h \
    tcpdatastream.h \
    traceanalyzer.h \
    tracebuffer.h \
    typedefs.h \
    udpdatastream.h \
    waterfall.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc
#
INCLUDEPATH += \
    $$PWD/quazip \
    $$PWD/qtmqtt/include/QtMqtt \
    $$PWD/qtmqtt/include

#win32: LIBS += -L$$PWD/quazip -lquazip1-qt5

unix: {
  LIBS += -lquazip5 -lavcodec -lavformat -lswscale -lavutil -L$$PWD/qtmqtt -lQt5Mqtt
}

win32 {
  LIBS += -lOpenGL32 -L$$PWD/quazip -lquazip1-qt5 -L$$PWD/qtmqtt -lqt5mqtt
}


#DEFINES += QCUSTOMPLOT_USE_OPENGL
DEFINES += SW_VERSION=\\\"$$system(git describe --always)\\\"
DEFINES += BUILD_DATE=\\\"$$system(git log -n 1 --format=%cd --date=short)\\\"

#DEFINES += GIT_LOG=\\\"$$system(git log --oneline)\\\"
#GIT_VERSION = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags) #$$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags  --abbrev=0) #$$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
#GIT_VERSION = \\\"$$GIT_VERSION\\\"

VERSION = 2.28.1.0
QMAKE_TARGET_COMPANY = Nkom
QMAKE_TARGET_PRODUCT = Hauken
QMAKE_TARGET_DESCRIPTION = Hauken
QMAKE_TARGET_COPYRIGHT = GPL

