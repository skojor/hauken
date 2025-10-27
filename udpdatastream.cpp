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
    //qDebug() << "UDP stream state" << state;
    if (state == QAbstractSocket::UnconnectedState)
        timeoutTimer->stop();
}

void UdpDataStream::newDataHandler()
{
    timeoutTimer->start();
    while (udpSocket->hasPendingDatagrams()) {                    // will read all pending packets and analyze, one by one
        QByteArray data = udpSocket->receiveDatagram().data();
        byteCtr += data.size();
        readHeader(data);
        if ( header.seqNumber == sequenceNr + 1 && checkHeader() ) { // Check for out-of-order packet
            if (!quarantine) {
                processData(data);
            }
            else {
                quarantine = 8;
            }
        }
        if (quarantine)
            quarantine--;

        sequenceNr = header.seqNumber;
    }
}
