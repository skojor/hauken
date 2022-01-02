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
    void toIncidentLog(QString);
    void bytesPerSec(int);
    void tracesPerSec(double);
    void newTrace(const QVector<qint16> &);
    void resetBuffers();

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

private:
    bool connected = false;
    QHostAddress *scpiAddress = new QHostAddress;
    QList<QHostAddress> myOwnAddresses = QNetworkInterface::allAddresses();

    quint16 scpiPort = 0;
    QTcpSocket *scpiSocket = new QTcpSocket;
    InstrumentState instrumentState = InstrumentState::DISCONNECTED;
    // pointer below contains what each instr can/cannot do, and needed freq/other settings for packet inspection etc.
    // used a shared pointer just because I can, and more important because it is safer in multithread env.
    QSharedPointer<Device> devicePtr;

    int measurementTime;
    int attenuator;
    bool autoAttenuator;
    QString antPort;
    Instrument::Mode mode;
    Instrument::FftMode fftMode;
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
    double tracesPerSecValue;

    const int scpiThrottleTime = 5; // ms
};

#endif // MEASUREMENTDEVICE_H
