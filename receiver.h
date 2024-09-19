#ifndef RECEIVER_H
#define RECEIVER_H

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
#include "config.h"

using namespace Instrument;

/*
 * Class to handle all device configuration and connections, including setting up the datastreams.
 * Replacement to MeasurementDevice class, which was flawed beyond repair *
 */


class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QSharedPointer<Config> c);
    void setIpAddress(QString address) { receiverAddress = address;}
    void setPort(int port = 5555) { receiverPort = port;}
    void connectReceiver();
    void disconnectReceiver();

signals:
    void receiverConnected();
    void receiverDisconnected();
    void status(QString);
    void warning(QString);
    void receiverName(QString);
    void toIncidentLog(const NOTIFY::TYPE,const QString,const QString);

private slots:
    void handleTcpSocketStateChanged(QAbstractSocket::SocketState state);
    void handleTcpSocketReadData();
    void handleTcpSocketTimeout();
    void handleScpiReplyTimeout();
    void initiateReceiver();
    void reqReceiverId();
    void parseReceiverId(QByteArray buffer);
    void reqReceiverUdpState();
    void parseReceiverUdpState(QByteArray buffer);
    void reqReceiverTcpState();
    void parseReceiverTcpState(QByteArray buffer);

    void writeToReceiver(QByteArray data, bool awaitReply = false);

private:
    QSharedPointer<Config> config;
    QSharedPointer<UdpDataStream> udpStream;
    QSharedPointer<TcpDataStream> tcpStream;
    QString receiverAddress;
    int receiverPort = 0;
    QTcpSocket *tcpSocket = new QTcpSocket;
    QTimer *tcpSocketTimeoutTimer = new QTimer;
    QTimer *scpiAwaitingReplyTimer = new QTimer;
    QElapsedTimer *tcpSocketWriteThrottleElapsedTimer = new QElapsedTimer;

    ReceiverState receiverState = ReceiverState::Disconnected;
    ReceiverInitiateOrder receiverInitiateOrder = ReceiverInitiateOrder::None;

    Device device;

    const int tcpSocketTimeout = 2000; // milliseconds
    const int scpiAwaitingReplyTimeout = 300;
    const int scpiThrottleTime = 5; // ms
};

#endif // RECEIVER_H
