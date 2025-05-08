#ifndef MEASUREMENTDEVICE_H
#define MEASUREMENTDEVICE_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>
#include <QNetworkInterface>
#include <QList>
#include <QSharedPointer>
#include <QThread>
#include <QCoreApplication>
#include "typedefs.h"
#include "tcpdatastream.h"
#include "udpdatastream.h"
#include "vifstreamudp.h"
#include "vifstreamtcp.h"
#include "config.h"

using namespace Instrument;

class MeasurementDevice : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementDevice(QSharedPointer<Config> c);
    ~MeasurementDevice();

protected:

signals:
    void connectedStateChanged(bool);
    void popup(QString);
    void status(QString);
    void instrId(QString);
    void initiateDatastream();
    void toIncidentLog(const NOTIFY::TYPE,const QString,const QString);
    void bytesPerSec(int);
    void tracesPerSec(double);
    void newTrace(const QVector<qint16> &);
    void resetBuffers();
    void positionUpdate(bool b, double lat, double lng);
    void deviceStreamTimeout(); // signal to stop eventual recording
    void displayGnssData(QString, int, bool);
    void updGnssData(GnssData &);
    void deviceBusy(QString); // For HTTP status updates
    void ipOfUser(QString);
    void reconnected();
    void newAntennaNames();
    void modeUsed(QString);
    void freqRangeUsed(unsigned long, unsigned long);
    void resUsed(int);
    //void freqChanged(double, double);
    //void resChanged(double);
    void iqFfmFreqChanged(double);

public slots:
    void start();
    void setPscanFrequency();
    void setPscanResolution();
    void setFfmCenterFrequency();
    void setFfmFrequencySpan();
    void setMeasurementTime();
    void setAttenuator();
    void setAutoAttenuator();
    void setAntPort();
    //QStringList antPorts();
    void setAddress(const QString s) { scpiAddress->setAddress(s); }
    void setPort(const int p) { scpiPort = p; }
    void setMode();
    void setFftMode();
    void setMeasurementMode();
    void setUser();
    void startDevice();
    void updSettings();

    bool isConnected();
    void instrConnect();
    void instrDisconnect();
    void runAfterConnected();
    QString id() { return devicePtr->id; }
    QString longId() { return devicePtr->longId; }
    void restartStream(bool withDisconnect = true);
    void setupUdpStream();
    void setupTcpStream();
    void setUseTcp(bool b) { useUdpStream = !b; }
    void setAutoReconnect(bool b) { autoReconnect = b; }
    QSharedPointer<Device> measurementDeviceData() { return devicePtr;}

    QStringList devicePscanResolutions() { return devicePtr->pscanResolutions;}
    QStringList deviceAntPorts() { return devicePtr->antPorts;}
    QStringList deviceFfmSpans() { return devicePtr->ffmSpans;}
    Instrument::Mode currentMode() { return devicePtr->mode;}
    QStringList deviceFftModes() { return devicePtr->fftModes;}
    quint64 deviceMinFreq() { return devicePtr->minFrequency;}
    quint64 deviceMaxFreq() { return devicePtr->maxFrequency;}

    void reqPosition() { emit positionUpdate(devicePtr->positionValid,
                                             devicePtr->latitude,
                                             devicePtr->longitude);}
    GnssData sendGnssData();
    bool isPositionValid() { return devicePtr->positionValid;}
    void updateAntennaName(const int index, const QString name);
    void setUdpStreamPtr(QSharedPointer<UdpDataStream> ptr) { udpStream = ptr;}
    void setTcpStreamPtr(QSharedPointer<TcpDataStream> ptr) { tcpStream = ptr;}
    void setVifStreamUdpPtr(QSharedPointer<VifStreamUdp> ptr) { vifStreamUdp = ptr;}
    void setVifStreamTcpPtr(QSharedPointer<VifStreamTcp> ptr) { vifStreamTcp = ptr;}
    void handleStreamTimeout();
    void handleNetworkError();
    void autoReconnectCheckStatus();
    void resetFreqSettings();
    void collectIqData(int nrOfSamplesNeeded);
    void setTrigCenterFrequency(double d) {trigFrequency = d;}
    void deleteIfStream();
    void setGainControl(int index);
    bool deviceHasGainControl() { return devicePtr->hasGainControl; }

private slots:
    void scpiConnected();
    void scpiDisconnected();
    void scpiError(QAbstractSocket::SocketError error);
    void scpiStateChanged(QAbstractSocket::SocketState state);
    void askId();
    void checkId(const QByteArray buffer);
    void askUdp();
    void checkUdp(const QByteArray buffer);
    void askTcp();
    void checkTcp(const QByteArray buffer);
    void askUser(bool flagCheckUserOnly = false);
    void checkUser(const QByteArray buffer);
    void checkUserOnly(const QByteArray buffer);
    void stateConnected();
    void delUdpStreams();
    void delTcpStreams();
    void delOwnStream();

    void scpiWrite(QByteArray data);
    void scpiRead();
    void tcpTimeout();

    void abor() { scpiWrite("abor");}
    void initImm() { scpiWrite("init:imm");}
    void updGnssDisplay();
    void askForAntennaNames();
    void antennaNamesReply(QByteArray buffer);

    void askFrequencies();
    void askPscanStartFreq();
    void askPscanStopFreq();
    void askPscanResolution();
    void askFfmFreq();
    void askFfmSpan();
    void checkPscanStartFreq(const QByteArray buffer);
    void checkPscanStopFreq(const QByteArray buffer);
    void checkPscanResolution(const QByteArray buffer);
    void checkFfmFreq(const QByteArray buffer);
    void checkFfmSpan(const QByteArray buffer);
    void setIfMode();
    void setupIfStream();

private:
    bool connected = false;
    QHostAddress *scpiAddress = new QHostAddress;
    QList<QHostAddress> myOwnAddresses = QNetworkInterface::allAddresses();

    quint16 scpiPort = 0;
    QTcpSocket *scpiSocket = new QTcpSocket;
    InstrumentState instrumentState = InstrumentState::DISCONNECTED;
    // pointer below contains what each instr can/cannot do, and needed freq/other settings for packet inspection etc.
    // used a shared pointer because it is safer in a multithread env.
    QSharedPointer<Device> devicePtr;

    int measurementTime;
    int attenuator;
    bool autoAttenuator;
    QByteArray antPort;
    //Instrument::Mode mode;
    QByteArray fftMode;
    QTimer *tcpTimeoutTimer = new QTimer;
    QTimer *autoReconnectTimer = new QTimer;
    QTimer *timedReconnectTimer = new QTimer;
    QElapsedTimer *scpiThrottleTimer = new QElapsedTimer;
    bool deviceInUseWarningIssued = false;
    bool useUdpStream = true;
    bool autoReconnect;
    bool autoReconnectInProgress = false;
    bool muteNotification = false;

    /*UdpDataStream *udpStream = new UdpDataStream;
    TcpDataStream *tcpStream = new TcpDataStream;*/
    QByteArray tcpOwnAdress = "127.0.0.1";
    QByteArray tcpOwnPort = "0";
    QSharedPointer<Config> config;
    QSharedPointer<UdpDataStream> udpStream;
    QSharedPointer<TcpDataStream> tcpStream;
    QSharedPointer<VifStreamUdp> vifStreamUdp;
    QSharedPointer<VifStreamTcp> vifStreamTcp;

    double tracesPerSecValue = 0;

    double latitude = 0, longitude = 0;
    bool posValid = false;
    bool askForPosition = false;
    const int scpiThrottleTime = 5; // ms

    bool scpiReconnect = false, discPressed = false;
    QTimer *updGnssDisplayTimer = new QTimer, *updFrequencyData = new QTimer, *updStreamsTimer = new QTimer;
    bool firstConnection = true;
    QString inUseBy, inUseByIp, inUseMode;
    unsigned long inUseStart = 0, inUseStop = 0, inUseRes = 0;
    bool waitingForReply = false;
    const int tcpTimeoutInMs = 10000;

    bool modeChangeInProgress = false;
    double trigFrequency = 0;
    bool modeChanged = false;
};

#endif // MEASUREMENTDEVICE_H
