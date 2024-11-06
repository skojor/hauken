#include "vifstream.h"

VifStream::VifStream() {}

void VifStream::openListener(const QHostAddress host, const int port)
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

void VifStream::closeListener()
{
    timeoutTimer->stop();
    bytesPerSecTimer->stop();
    udpSocket->close();
}

void VifStream::connectionStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "VIF UDP stream state" << state;
//    if (state == QAbstractSocket::UnconnectedState)
}

void VifStream::newData()
{
    while (udpSocket->hasPendingDatagrams()) {                    // will read all pending packets and analyze, one by one
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        //rxData.fill(0, udpSocket->pendingDatagramSize());
        //udpSocket->readDatagram(rxData.data(), rxData.size());
        //qDebug() << "package size" << rxData.size();
        ifBuffer.append(datagram);
    }
    qDebug() << "VIF buffer" << ifBuffer.size();
}

bool VifStream::checkVifHeader(QNetworkDatagram datagram)
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

void VifStream::processVifData()
{
    qDebug() <<"pjocess" << ifBuffer.size();
    for (auto && buffer : ifBuffer) {
        if (checkVifHeader(buffer))
            readIqData(buffer);
        else
            qDebug() << "NOPY";
    }
}

void VifStream::readIqData(QNetworkDatagram datagram)
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
    qDebug() << "got" << i.size() << q.size();
}
