#include "network.h"

Network::Network(QSharedPointer<Config> c)
{
    config = c;
    tcpServer->listen(QHostAddress::LocalHost, TCPPORT);

   /* udpTestSocket->bind(QHostAddress::LocalHost, UDPPORT);

    connect(udpTestSocket, &QUdpSocket::readyRead, this, [this] () {
        while (udpTestSocket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(udpTestSocket->pendingDatagramSize());
            QHostAddress senderAddress;
            quint16 senderPort;
            udpTestSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
            qDebug() << "Client Received: " << datagram << " from " << senderAddress.toString() << ":" << senderPort;
        }
    });
    connect(udpTestSocket, &QUdpSocket::stateChanged, this, [] (QAbstractSocket::SocketState s) {
        qDebug() << s;
    });*/
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
    if (useUdp) {
        QByteArray ba;
        QTextStream ts(&ba);
        ts << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toLocal8Bit() << ","
           << QByteArray::number(config->getInstrStartFreq() * 1e6, 'f', 0) << ","
           << QByteArray::number(config->getInstrStopFreq() * 1e6, 'f', 0)  << ","
           << QByteArray::number(config->getInstrResolution().toDouble() * 1e3, 'f' ,0) << ","
           << config->getStnLatitude() << "," << config->getStnLongitude();

        for (auto && val : data) {
            ts << "," << (int)(val / 10);
        }
        ts << Qt::endl;
        //tcpServer-> writeDatagram(ba.constData(), ba.size(), QHostAddress::LocalHost, UDPPORT);
    }
}
