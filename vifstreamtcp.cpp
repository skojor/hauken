#include "vifstreamtcp.h"

VifStreamTcp::VifStreamTcp()
{
}

void VifStreamTcp::openListener(const QHostAddress host, const int port)
{
    tcpSocket->close();

    tcpSocket->connectToHost(host, port);
    tcpSocket->waitForConnected(1000);
}

void VifStreamTcp::closeListener()
{
    tcpSocket->close();
}

void VifStreamTcp::connectionStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "VIF TCP stream state" << state;
}

void VifStreamTcp::newDataHandler()
{
    timeoutTimer->start();
    const QByteArray buf = tcpSocket->readAll();
    byteCtr += buf.size();

    if (!buf.isEmpty())
        emit newAmmosData(buf);
}
