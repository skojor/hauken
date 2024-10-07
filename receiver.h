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
    void setUdpStreamPtr(QSharedPointer<UdpDataStream> ptr) { udpStream = ptr;}
    void setTcpStreamPtr(QSharedPointer<TcpDataStream> ptr) { tcpStream = ptr;}
    void udpStreamConnected() { receiverInitiateOrder = ReceiverInitiateOrder::UdpStreamConnected; initiateReceiver();}
    void tcpStreamConnected() { receiverInitiateOrder = ReceiverInitiateOrder::TcpStreamConnected; initiateReceiver();}
    void handleStreamTimeout();

    void setPscanStartFrequency(double freq);
    void setPscanStopFrequency(double freq);
    void abor() { writeToReceiver("abor");}
    void initImm() { writeToReceiver("init:imm");}
    void setMode(Instrument::Mode mode);

signals:
    //void receiverConnected();
    //void receiverDisconnected();
    void receiverStateChanged(bool);
    void status(QString);
    void popup(QString);
    void receiverBusy(QString);
    void receiverName(QString);
    void toIncidentLog(const NOTIFY::TYPE,const QString,const QString);
    void ipOfUser(QString);
    void modeUsed(QString);
    void receiverStreamTimeout();
    void resetBuffers();

private slots:
    void handleTcpSocketStateChanged(QAbstractSocket::SocketState state);
    void handleTcpSocketReadData();
    void handleTcpSocketTimeout();
    void handleScpiReplyTimeout();
    void writeToReceiver(QByteArray data, bool awaitReply = false);
    void initiateReceiver(); // Takes care of conn. setup, in use status, user...
    void reqReceiverId();
    void parseReceiverId(QByteArray buffer);
    void reqReceiverUdpState();
    void parseReceiverUdpState(QByteArray buffer);
    void reqReceiverTcpState();
    void parseReceiverTcpState(QByteArray buffer);
    void reqUser();
    void parseUser(QByteArray buffer);
    void issueWarning();
    void reqSocketInfo();
    void parseSocketInfo(QByteArray buffer);

    void connectUdpStream();
    void setupUdpStream();
    void connectTcpStream();
    void setupTcpStream();
    void deleteOwnStream();
    void deleteUdpStreams();
    void deleteTcpStreams();
    void disconnectTcpSocket() {tcpSocket->close();}

private:
    QSharedPointer<Config> config;
    QSharedPointer<UdpDataStream> udpStream;
    QSharedPointer<TcpDataStream> tcpStream;
    QString receiverAddress;
    int receiverPort = 0;
    QTcpSocket *tcpSocket = new QTcpSocket;
    QTimer *tcpSocketTimeoutTimer = new QTimer;
    QTimer *scpiAwaitingReplyTimer = new QTimer;
    QTimer *autoReconnectTimer = new QTimer;
    QElapsedTimer *tcpSocketWriteThrottleElapsedTimer = new QElapsedTimer;

    ReceiverState receiverState = ReceiverState::Disconnected;
    ReceiverInitiateOrder receiverInitiateOrder = ReceiverInitiateOrder::None;

    QList<QHostAddress> myOwnAddresses = QNetworkInterface::allAddresses();
    QByteArray tcpOwnAdress = "127.0.0.1";
    QByteArray tcpOwnPort = "0";
    QString receiverUser;
    bool flagWarningIssued = false;
    QSharedPointer<Device> device;

    const int tcpSocketTimeout = 20000; // milliseconds
    const int scpiAwaitingReplyTimeout = 30000;
    const int scpiThrottleTime = 5; // ms
    const int autoReconnectTimeout = 5000;
};

#endif // RECEIVER_H
