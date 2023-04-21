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
#include "typedefs.h"
#include "tcpdatastream.h"
#include "udpdatastream.h"
#include "qcustomplot.h"
#include "config.h"

using namespace Instrument;

class MeasurementDevice : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementDevice(QSharedPointer<Config> c);
    ~MeasurementDevice()
    {
        if (connected) instrDisconnect();
    }

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
    void reconnected();
    void newAntennaNames();

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
    QStringList getAntPorts();
    void setAddress(const QString s) { scpiAddress->setAddress(s); }
    void setPort(const int p) { scpiPort = p; }
    void setMode();
    void setFftMode();
    void setMeasurementMode();
    void setUser();
    void startDevice();
    void updSettings();

    bool isConnected() { return connected; }
    void instrConnect();
    void instrDisconnect();
    void runAfterConnected();
    QString getId() { return devicePtr->id; }
    QString getLongId() { return devicePtr->longId; }
    void restartStream();
    void setupUdpStream();
    void setupTcpStream();
    void setUseTcp(bool b) { useUdpStream = !b; }
    void setAutoReconnect(bool b) { autoReconnect = b; }
    void forwardBytesPerSec(int);
    void forwardTracesPerSec(double d) { tracesPerSecValue = d; emit tracesPerSec(d);}
    void fftDataHandler(QVector<qint16> &);
    double getTracesPerSec() { return tracesPerSecValue;}
    QSharedPointer<Device> getMeasurementDeviceData() { return devicePtr;}

    QStringList getDevicePscanResolutions() { return devicePtr->pscanResolutions;}
    QStringList getDeviceAntPorts() { return devicePtr->antPorts;}
    QStringList getDeviceFfmSpans() { return devicePtr->ffmSpans;}
    Instrument::Mode getCurrentMode() { return devicePtr->mode;}
    QStringList getDeviceFftModes() { return devicePtr->fftModes;}
    quint64 getDeviceMinFreq() { return devicePtr->minFrequency;}
    quint64 getDeviceMaxFreq() { return devicePtr->maxFrequency;}

    void reqPosition() { emit positionUpdate(devicePtr->positionValid,
                                             devicePtr->latitude,
                                             devicePtr->longitude);}
    GnssData sendGnssData();
    void updateAntennaName(const QString name);

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
    void askUser();
    void checkUser(const QByteArray buffer);
    void stateConnected();
    void delUdpStreams();
    void delTcpStreams();

    void scpiWrite(QByteArray data);
    void scpiRead();
    void tcpTimeout();
    void handleStreamTimeout();
    void autoReconnectCheckStatus();

    void resetFreqSettings();
    void abor() { scpiWrite("abor");}
    void initImm() { scpiWrite("init:imm");}
    void updGnssDisplay();
    void askForAntennaNames();
    void antennaNamesReply(QByteArray buffer);

private:
    bool connected = false;
    QHostAddress *scpiAddress = new QHostAddress;
    QList<QHostAddress> myOwnAddresses = QNetworkInterface::allAddresses();

    quint16 scpiPort = 0;
    QTcpSocket *scpiSocket = new QTcpSocket;
    InstrumentState instrumentState = InstrumentState::DISCONNECTED;
    // pointer below contains what each instr can/cannot do, and needed freq/other settings for packet inspection etc.
    // used a shared pointer just because I wanted to, and more important because it is safer in multithread env.
    QSharedPointer<Device> devicePtr;

    int measurementTime;
    int attenuator;
    bool autoAttenuator;
    QByteArray antPort;
    Instrument::Mode mode;
    QByteArray fftMode;
    QTimer *tcpTimeoutTimer = new QTimer;
    QTimer *autoReconnectTimer = new QTimer;
    QElapsedTimer *scpiThrottleTimer = new QElapsedTimer;
    bool deviceInUseWarningIssued = false;
    bool useUdpStream = true;
    bool autoReconnect;
    bool autoReconnectInProgress = false;

    UdpDataStream *udpStream = new UdpDataStream;
    TcpDataStream *tcpStream = new TcpDataStream;
    QByteArray tcpOwnAdress = "127.0.0.1";
    QByteArray tcpOwnPort = "0";
    QSharedPointer<Config> config;
    double tracesPerSecValue = 0;

    double latitude = 0, longitude = 0;
    bool posValid = false;
    bool askForPosition = false;
    const int scpiThrottleTime = 5; // ms

    bool scpiReconnect = false, discPressed = false;
    QTimer *updGnssDisplayTimer = new QTimer;
    bool firstConnection = true;
    QString inUseBy;
};

#endif // MEASUREMENTDEVICE_H
