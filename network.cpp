#include "network.h"

Network::Network(QSharedPointer<Config> c)
{
    config = c;
    if (!tcpServer->listen(QHostAddress::LocalHost, TCPPORT)) {
        qWarning() << "Could not start TCP listener at port" << TCPPORT << ":" << tcpServer->errorString();
    }
    else {
        qInfo() << "Opened TCP listener at port" << tcpServer->serverPort();
    }
    connect(tcpServer, &QTcpServer::newConnection, this, &Network::handleNewConnection);

    /*tcpTestSocket->connectToHost(QHostAddress::LocalHost, TCPPORT);

    connect(tcpTestSocket, &QTcpSocket::readyRead, this, [this] () {
        QByteArray data = tcpTestSocket->readAll();
        qDebug() << "Client Received: " << data.size() << "bytes";

    });
    connect(tcpTestSocket, &QTcpSocket::stateChanged, this, [] (QAbstractSocket::SocketState s) {
        qDebug() << s;
    });


    connect(testTimer, &QTimer::timeout, this, [this]() {
        QByteArray barr(128000, 'a');

        for (auto && socket : tcpSockets) {
            if (socket->isOpen())
                socket->write(barr);
        }
    });
    testTimer->start(5000);*/
}

Network::~Network()
{
    tcpServer->deleteLater();
}

void Network::updSettings()
{
    useUdp = config->getSaveToTempFile();
}

void Network::newTraceline(const QList<qint16> data)
{
    bool isConnected = false;
    for (auto && socket : tcpSockets) {
        if (socket->isOpen())
            isConnected = true;
    }

    if (isConnected && useUdp) {
        QByteArray ba;
        ba.reserve(data.size() * 6);

        ba.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toLocal8Bit()).append(",")
            .append(QByteArray::number(startFreq)).append(",")
            .append(QByteArray::number(stopFreq)).append(",")
            .append(QByteArray::number(config->getInstrResolution().toDouble() * 1e3, 'f', 0)).append(",")
            .append(config->getStnLatitude().toLocal8Bit()).append(",")
            .append(config->getStnLongitude().toLocal8Bit());

        for (auto && val : data) {
            ba.append(",").append(QByteArray::number( val / 10));
        }

        for (auto && socket : tcpSockets) {
            if (socket->isOpen())
                socket->write(ba);
        }
    }
}

void Network::handleNewConnection()
{
    tcpSockets.append(tcpServer->nextPendingConnection());
    qDebug() << "New connection on TCP:" << tcpSockets.last()->peerAddress() << tcpSockets.last()->peerPort();
    //connect(tcpSockets.last(), &QTcpSocket::disconnected, tcpSockets.last(), &QTcpSocket::deleteLater);
}
