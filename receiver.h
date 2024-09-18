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
    void connectInstrument();
    void disconnectInstrument();

signals:
    void receiverConnected();
    void receiverDisconnected();
    void warning(QString text);
    void toIncidentLog(const NOTIFY::TYPE,const QString,const QString);

private slots:
    void handleTcpSocketStateChanged(QAbstractSocket::SocketState state);
    void handleTcpSocketReadData();
    void handleTcpSocketTimeout();
    void receiverInitiate();
    void reqReceiverId();

private:
    QSharedPointer<Config> config;
    QSharedPointer<UdpDataStream> udpStream;
    QSharedPointer<TcpDataStream> tcpStream;
    QString receiverAddress;
    int receiverPort = 0;
    QTcpSocket *tcpSocket = new QTcpSocket;
    QTimer *tcpSocketTimeoutTimer = new QTimer;
    ReceiverState receiverState = ReceiverState::Disconnected;
    ReceiverInitiateOrder receiverInitiateOrder = ReceiverInitiateOrder::None;

    const int tcpSocketTimeout = 2000; // milliseconds
};

#endif // RECEIVER_H
