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
#include <QSet>
//#include <QHostInfo>
#include "config.h"
#include "typedefs.h"
#include "ubloxrfbandcorrelator.h"
#include "gnssspectrumblock.h"

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

struct AGCEntry
{
    uint8_t frontEnd;
    int8_t gain;
    uint8_t ant;
};

class GnssDevice : public QObject
{
    Q_OBJECT
public:
    explicit GnssDevice(QSharedPointer<Config>, const QString &profileId);
    ~GnssDevice();

public slots:
    void start();
    void connectToPort();
    void updSettings();
    void reqPosition() { emit positionUpdate(gnssData.posValid, gnssData.latitude, gnssData.longitude);}
    GnssData sendGnssData() { return gnssData;}
    bool isValid() { return gnssData.posValid;}
    void saveBacklog(quint64 recordingId);
    void setIncidenceStartedDateTime();
    void setReplayMode(bool b) { replayMode = b; gnssData.inUse = b; }
    void injectBytes(const QByteArray &data);

signals:
    void analyzeThisData(GnssData &);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void positionUpdate(bool b, double lat, double lng);
    void gnssDisabled();
    void sendGnssPlotFilename(quint64 recordingId, QString filename);
    void spectrumReady(const GnssSpectrumBlock &block);

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
    bool checkSbfChecksum(const QByteArray &val);
    void appendToLogfile(const QByteArray &data);
    void appendToBinaryfile(const QByteArray &data);
    void checkPosValid();
    void connectTcpSocket();
    void handleTcpStateChange(QAbstractSocket::SocketState state);
    void checkTcpConnection();
    void setupUbloxDevice();
    void askUbloxVersion();
    void decodeBinary0a04(const QByteArray &val);
    void delayedReportHandler();
    QString createFilename(const QString &suffix = QString());
    void updateBacklog(QByteArray &data, bool isBinary);
    void cleanupBacklog();
    void createGnssPlot(quint64 recordingId);
    void decodeSbf4014(const QByteArray &data);
    void decodeSbf4092(const QByteArray &data);
    void decodeSbf4027(const QByteArray &data);
    void decodeSbf4013(const QByteArray &data);
    void decodeSbf4040(const QByteArray &data);

private:
    QSerialPort *gnss = new QSerialPort;
    //QList<QSerialPortInfo> serialPortList;
    QByteArray gnssBuffer;
    QTimer *sendToAnalyzerTimer = new QTimer;
    QTimer *delayedReportTimer = new QTimer;
    QList<QByteArray> gsvSentences;
    GnssData gnssData;
    int gsvNrOfSentences;
    QFile logfile, binaryFile;
    bool logToFile = false;
    QDate logfileStartedDate;
    bool posInvalidTriggered = true; // don't trigger any recording on inital startup
    //bool isTcpIpConnection = false, tcpSocketWasConnected = false;
    //QHostInfo *hostInfo = new QHostInfo;
    QHostAddress *hostAddress = new QHostAddress;
    QTcpSocket *tcpSocket = new QTcpSocket;
    QTimer *tcpWatchdogTimer = new QTimer;
    QElapsedTimer tcpLastDataTimer;
    bool tcpWasConnected = false;
    static constexpr int tcpDataTimeoutMs = 10000; // force reconnect if no data arrives within this time
    int portnumber = 0;
    QSharedPointer<Config> config;
    QList<QDateTime> backlogTimestamp;
    QList<QByteArray> backlogData;
    QList<bool> backlogIsBinary;
    QVector<double> backlogCno, backlogAgc, backlogJam;
    QMap<int, QVector<double>> backlogAgcBlocks;
    QMap<int, QString> backlogAgcBlockLabels;
    QTimer *backlogCleanupTimer = new QTimer;
    QDateTime incidenceStartedDateTime;

    // Config cache
    bool activate = false;
    QString portName, baudrate;
    QString profileLabel; // sanitized profile name, used for filenames/log titles
    bool uBloxDevice;
    QString uBloxID, uBloxFirmware;
    UbloxRfBandCorrelator rfBandCorrelator;
    quint64 binaryMessageSequence = 0;
    UBLOX uBloxState = UBLOX::NODEVICE;
    bool checkedAllSatSystems = false;

    // Septentrio: signal types (SBF 4.1.10) currently tracked on the main antenna, from MeasEpoch (4027)
    QSet<int> septentrioActiveSignalTypes;
    bool septentrioDetected = false;
    bool septentrioMeasEpochSeen = false;
    QMap<quint32, int> septentrioSpectrumBandIndexByCenterHz; // stable band index per distinct BBSamples (4040) LOFreq seen
    bool replayMode = false;
};


#endif // GNSSDEVICE_H
