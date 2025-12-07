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
        readHeaders(data);
        if (sequenceNr == 0)
            sequenceNr = header.seqNumber; // Initial value

        udpPacketList.append(UdpPacket(header.seqNumber, data));
    }

    while (udpPacketList.size() > 19) { // Wait for 20 udp packets, to have some data to reorder if needed
        int idx = 0;
        for (auto && udpPacket : udpPacketList) {
            if (udpPacket.seqNumber == sequenceNr) {
                processData(udpPacket.data);
                break;
            }
            idx++;
        }
        if (idx < 20) {
            udpPacketList.removeAt(idx);
            sequenceNr++;
        }
        else {
            sequenceNr = 0;
            emit waitForPscanEndMarker(true);
            udpPacketList.clear();
        }
    }
    if (udpPacketList.size() > 500) {
        udpPacketList.clear();
        sequenceNr = 0;
        emit waitForPscanEndMarker(true);
    }
}
