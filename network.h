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
#include <QVector>
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
    void newTraceline(const QVector<qint16>);
    void updSettings();
    void updFrequencies(const quint64 startf, const quint64 stopf) { startFreq = startf; stopFreq = stopf;}

private slots:
    void handleNewConnection();

private:
    QSharedPointer<Config> config;
    QTcpServer *tcpServer = nullptr;
    QList<QTcpSocket *> tcpSockets;
    QTcpSocket *tcpTestSocket = nullptr;
    bool useUdp = false;
    QTimer *testTimer = nullptr;
    quint64 startFreq = 0, stopFreq = 0;

signals:

};

#endif // NETWORK_H
