#include "vifstreamudp.h"

VifStreamUdp::VifStreamUdp() {}

void VifStreamUdp::openListener(const QHostAddress host, const int port)
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
    qDebug() << "VIF udp port:" << udpSocket->localPort();
    bytesPerSecTimer->start();
}

void VifStreamUdp::closeListener()
{
    timeoutTimer->stop();
    bytesPerSecTimer->stop();
    udpSocket->close();
}

void VifStreamUdp::connectionStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "VIF UDP stream state" << state;
//    if (state == QAbstractSocket::UnconnectedState)
}

void VifStreamUdp::newData()
{
    while (udpSocket->hasPendingDatagrams()) {                    // will read all pending packets and analyze, one by one
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        //rxData.fill(0, udpSocket->pendingDatagramSize());
        //udpSocket->readDatagram(rxData.data(), rxData.size());
        //qDebug() << "package size" << rxData.size();
        ifBufferUdp.append(datagram);
    }
}

bool VifStreamUdp::checkVifHeader(QNetworkDatagram datagram)
{
    QByteArray data = datagram.data();
    QDataStream ds(data);
    qint16 infClassCode, packetClassCode;
    ds.skipRawData(12);
    ds >> infClassCode >> packetClassCode;
    if (infClassCode == 1 && packetClassCode == 1)
        return true;
    else
        return false;
}

void VifStreamUdp::processVifData()
{
    for (auto && buffer : ifBufferUdp) {
        if (checkVifHeader(buffer))
            readIqData(buffer);
    }
    emit newIqData(i, q);
    i.clear();
    q.clear();
    ifBufferUdp.clear();
}

void VifStreamUdp::readIqData(QNetworkDatagram datagram)
{
    QByteArray data = datagram.data();
    QDataStream ds(data);
    ds.skipRawData(28);

    qint16 rawI, rawQ;
    while (!ds.atEnd()) {
        ds >> rawI >> rawQ;
        i.append(rawI);
        q.append(rawQ);
    }
    i.pop_back();
    q.pop_back(); // Remove last value, trailing packet
}