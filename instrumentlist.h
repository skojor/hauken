#ifndef INSTRUMENTLIST_H
#define INSTRUMENTLIST_H

#include <QDebug>
#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QProcess>
#include <QCoreApplication>
#include "typedefs.h"
#include "config.h"
#include <QtMqtt/QtMqtt>

/*
 * Class to read a list of stations, then read a list of instruments and IP addresses
 * from each station. The list is written to a file and updated upon every
 * successful connection.
 * Server address set in options -> general, if blank this class does nothing.
 * If server is unconnectable or fails otherwise the class will silently fail.
 * If the cache file contains valid data it will be used instead.
 *
 * When either a successful list is downloaded or a valid cache list is available
 * the class will send the signal instrumentListReady.
 *
 * The class will signal a login request if necessary, needs to be handled.
 *
 */


class InstrumentList : public Config
{
    Q_OBJECT

public:
    explicit InstrumentList(QObject *parent = nullptr);
    void start();
    void updSettings();
    void loginCompleted();

private slots:
    void checkConnection();
    void checkStationListReturn(int exitCode, QProcess::ExitStatus);
    void checkInstrumentListReturn(int exitCode, QProcess::ExitStatus);
    void checkEquipmentListReturn(int exitCode, QProcess::ExitStatus);
    void parseStationList(const QByteArray &ba);
    void parseInstrumentList(const QByteArray &ba);
    void parseEquipmentList(const QByteArray &ba);
    void loadFile();
    void saveFile();
    void askForInstruments();
    void askForEquipmentList();
    void generateInstrumentList();
    void updStationInfoWithEquipmentList();

private:
    QList<StationInfo> stationInfo;
    QList<EquipmentInfo> equipmentInfo;
    QStringList usableStnNames, usableStnIps, usableStnTypes;
    QString server;
    bool isStarted = false;
    QProcess *curlProcess = new QProcess;
    int stnIndex = 0;
    int nrOfStations = 0;
    QStringList instrumentList;

signals:
    void askForLogin();
    void instrumentListReady(QStringList, QStringList, QStringList);
};

#endif // INSTRUMENTLIST_H
