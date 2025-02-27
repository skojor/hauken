#ifndef GNSSDEVICE_H
#define GNSSDEVICE_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <cmath>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QHostAddress>
#include <QTcpSocket>
//#include <QHostInfo>
#include "config.h"
#include "typedefs.h"

/*
 *  Class connecting to and reading data from one gnss receiver.
 *  Collects data for average C/No readings, AGC levels (if available)
 *  and position/altitude/time for spoofing detection. May be used
 *  to trigger recording of spectrum.
 */

enum class UBLOX {
    NODEVICE,
    ASKEDVERSION,
    WAITFORACK,
    READY
};

class GnssDevice : public QObject
{
    Q_OBJECT
public:
    explicit GnssDevice(QSharedPointer<Config>, int id);

public slots:
    void start();
    void connectToPort();
    void updSettings();
    void reqPosition() { emit positionUpdate(gnssData.posValid, gnssData.latitude, gnssData.longitude);}
    GnssData sendGnssData() { return gnssData;}
    bool isValid() { return gnssData.posValid;}

signals:
    void analyzeThisData(GnssData &);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void positionUpdate(bool b, double lat, double lng);
    void gnssDisabled();

private slots:
    void handleBuffer();
    bool checkChecksum(const QByteArray &val);
    bool decodeGga(const QByteArray &val);
    bool decodeGsa(const QByteArray &val);
    bool decodeRmc(const QByteArray &val);
    bool decodeGsv(const QByteArray &val);
    bool decodeGns(const QByteArray & val);
    QDateTime convFromGnssTimeToQDateTime(const QByteArray date, const QByteArray time);
    void decodeBinary(const QByteArray &val);
    bool checkBinaryChecksum(const QByteArray &val);
    void appendToLogfile(const QByteArray &data);
    void checkPosValid();
    void connectTcpSocket();
    void handleTcpStateChange(QAbstractSocket::SocketState state);
    void setupUbloxDevice();
    void askUbloxVersion();
    void decodeBinary0a04(const QByteArray &val);
    void delayedReportHandler();

private:
    QSerialPort *gnss = new QSerialPort;
    //QList<QSerialPortInfo> serialPortList;
    QByteArray gnssBuffer;
    QTimer *sendToAnalyzerTimer = new QTimer;
    QTimer *delayedReportTimer = new QTimer;
    QList<QByteArray> gsvSentences;
    GnssData gnssData;
    int gsvNrOfSentences;
    QFile logfile;
    bool logToFile = false;
    QDate logfileStartedDate;
    bool posInvalidTriggered = true; // don't trigger any recording on inital startup
    //bool isTcpIpConnection = false, tcpSocketWasConnected = false;
    //QHostInfo *hostInfo = new QHostInfo;
    QHostAddress *hostAddress = new QHostAddress;
    QTcpSocket *tcpSocket = new QTcpSocket;
    int portnumber = 0;
    QSharedPointer<Config> config;

    // Config cache
    bool activate = false;
    QString portName, baudrate;
    bool uBloxDevice;
    QString uBloxID, uBloxFirmware;
    UBLOX uBloxState = UBLOX::NODEVICE;
    bool checkedAllSatSystems = false;
};


#endif // GNSSDEVICE_H
