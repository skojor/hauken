#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QSharedPointer>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include "config.h"

#define UDPPORT 5569

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

private:
    QSharedPointer<Config> config;
    QUdpSocket *udpSocket = new QUdpSocket;
    QUdpSocket *udpTestSocket = new QUdpSocket;
    bool useUdp = false;

signals:

};

#endif // NETWORK_H
