#include "udpdatastream.h"

UdpDataStream::UdpDataStream()
{
}

void UdpDataStream::openListener()
{
    udpSocket->close();
    udpPort = 5555;
    for (int i=0; i<10; i++) {
        if (!udpSocket->bind(udpPort)) {
            udpSocket->close();
            udpPort++;
        }
        else break;
    }
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
    if (state == QAbstractSocket::ConnectedState) {
        //timeoutTimer->start(timeoutInMs); // stupid, udp conn. is stateless you fool!
    }
}

void UdpDataStream::newData()
{
    timeoutTimer->start();

    QByteArray rxData;

    while (udpSocket->hasPendingDatagrams()) {                    // will read all pending packets and analyze, one by one
        rxData.fill(0, udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(rxData.data(), rxData.size());
    }
    byteCtr += rxData.size();
    //qDebug() << byteCtr;
    processData(rxData);
}
