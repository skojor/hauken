#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QSharedPointer>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include <QTimer>
#include <QList>
#include "config.h"

#define UDPPORT 5569
#define TCPPORT 5569

class Network : public QObject
{
    Q_OBJECT
public:
    Network(QSharedPointer<Config> c);
    ~Network();

public slots:
    void newTraceline(const QList<qint16>);
    void updSettings();

private slots:
    void handleNewConnection();

private:
    QSharedPointer<Config> config;
    QTcpServer *tcpServer = new QTcpServer;
    QList<QTcpSocket *> tcpSockets;
    QTcpSocket *tcpTestSocket = new QTcpSocket;
    bool useUdp = false;
    QTimer *testTimer = new QTimer;

signals:

};

#endif // NETWORK_H
