#include "udpdatastream.h"

UdpDataStream::UdpDataStream()
{
}

void UdpDataStream::openListener()
{
    udpSocket->close();
    udpPort = 5559;
    for (int i=0; i<10; i++) {
        if (!udpSocket->bind(udpPort)) {
            udpSocket->close();
            udpPort++;
        }
        else break;
    }
    qDebug() << "Datastream udp port:" << udpSocket->localPort();

    bytesPerSecTimer->start();
}

void UdpDataStream::closeListener()
{
    timeoutTimer->stop();
    bytesPerSecTimer->stop();
    udpSocket->close();
}

void UdpDataStream::connectionStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "UDP stream state" << state;
    if (state == QAbstractSocket::UnconnectedState)
        timeoutTimer->stop();
}

void UdpDataStream::newData()
{
    timeoutTimer->start();
    while (udpSocket->hasPendingDatagrams()) {                    // will read all pending packets and analyze, one by one
        QByteArray data = udpSocket->receiveDatagram().data();
        if (checkHeader(data))
            processData(data);
    }
}
