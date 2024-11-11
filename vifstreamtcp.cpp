#include "vifstreamtcp.h"

VifStreamTcp::VifStreamTcp() {}


void VifStreamTcp::openListener(const QHostAddress host, const int port)
{
    tcpSocket->close();

    tcpSocket->connectToHost(host, port);
    tcpSocket->waitForConnected(1000);
    if (tcpSocket->isOpen()) {
        tcpSocket->write("\n");
        bytesPerSecTimer->start();
    }
}

void VifStreamTcp::closeListener()
{
    timeoutTimer->stop();
    bytesPerSecTimer->stop();
    udpSocket->close();
}

void VifStreamTcp::connectionStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "VIF UDP stream state" << state;
    //    if (state == QAbstractSocket::UnconnectedState)
    }

void VifStreamTcp::newData()
{
    QByteArray data = tcpSocket->readAll();
    ifBufferTcp.append(data);
    qDebug() << "VIF buffer" << ifBufferTcp.size();
}

bool VifStreamTcp::checkVifHeader(QByteArray data)
{
    QDataStream ds(data);
    qint16 infClassCode, packetClassCode;
    ds.skipRawData(12);
    ds >> infClassCode >> packetClassCode;
    if (infClassCode == 1 && packetClassCode == 1)
        return true;
    else
        return false;
}

void VifStreamTcp::processVifData()
{
    qDebug() <<"pjocess" << ifBufferTcp.size();
    for (auto && buffer : ifBufferTcp) {
        if (checkVifHeader(buffer))
            readIqData(buffer);
        else
            qDebug() << "NOPY";
    }
}

void VifStreamTcp::readIqData(QByteArray data)
{
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
