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
    qDebug() << "UDP stream state" << state;
    if (state == QAbstractSocket::UnconnectedState)
        timeoutTimer->stop();
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
    //processData(rxData);
    QDataStream ds(rxData);

    while (!ds.atEnd()) {
        uchar tmp;
        ds >> tmp;
        udpBuffer.append(tmp);

        if (!headerIsRead and udpBuffer.size() > 15) {
            if (!checkHeader(udpBuffer)) {
                qDebug() << "udp header fail" << header.dataSize;
                udpBuffer.clear();
            }
            else
                headerIsRead = true;
        }
        else if (headerIsRead && udpBuffer.size() == (int)header.dataSize) {
            processData(udpBuffer);
            headerIsRead = false;
            udpBuffer.clear();
        }
    }
}
