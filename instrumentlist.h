#ifndef INSTRUMENTLIST_H
#define INSTRUMENTLIST_H

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
 */


#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPair>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QSharedPointer>
#include "config.h"
#include "typedefs.h"

enum StateDataHandler {
    BEGIN,
    ASKEDFORLOGIN,
    LOGGEDIN,
    ASKEDFORSTATIONS,
    RECEIVEDSTATIONS,
    ASKEDFORINSTRUMENTS,
    RECEIVEDINSTRUMENTS,
    ASKEDFOREQUIPMENT,
    RECEIVEDEQUIPMENT,
    FAILED,
    DONE
};

class InstrumentList : public QObject
{
    Q_OBJECT
public:
    InstrumentList(QSharedPointer<Config> c);

public slots:
    void start() { loadFile(); fetchDataHandler(); }
    void updSettings() {}

private slots:
    void loadFile();
    void saveFile();
    void parseStationList(const QByteArray &reply);
    void networkAccessManagerFinished(QNetworkReply *reply);
    void tmNetworkTimeoutHandler();
    void loginRequest();
    void parseLoginReply(const QByteArray &reply);
    void stationListRequest();
    void instrumentListRequest();
    void parseInstrumentList(const QByteArray &reply);
    void equipmentListRequest();
    void parseEquipmentList(const QByteArray &reply);
    void updStationsWithEquipmentList();
    void generateLists();
    void fetchDataHandler(const QByteArray &reply = QByteArray());

private:
    QNetworkAccessManager *networkAccessManager = new QNetworkAccessManager;
    QNetworkReply *networkReply;
    QTimer *tmNetworkTimeout = new QTimer;
    QNetworkCookie networkCookie;
    QNetworkCookieJar *cookieJar = new QNetworkCookieJar;
    QList<StationInfo> stationInfo;
    QList<EquipmentInfo> equipmentInfo;
    //int nrOfStations = 0;
    int stationIndex = 0;
    StateDataHandler state = BEGIN;
    QStringList usableStnNames, usableStnIps, usableStnTypes;
    QSharedPointer<Config> config;

signals:
    void listReady(QStringList, QStringList, QStringList);
    void instrumentListDownloaded(QString);
    void instrumentListStarted(QString);
    void instrumentListFailed(QString);
};

#endif // INSTRUMENTLIST_H
