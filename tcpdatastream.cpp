#include "tcpdatastream.h"

TcpDataStream::TcpDataStream()
{
}

void TcpDataStream::openListener(const QHostAddress host, const int port)
{
    tcpSocket->connectToHost(host, port);
    if (tcpSocket->isOpen()) {tcpSocket->waitForConnected(1000);
        tcpSocket->write("\n");
        bytesPerSecTimer->start();
    }
}

void TcpDataStream::closeListener()
{
    timeoutTimer->stop();
    bytesPerSecTimer->stop();
    tcpSocket->close();
}

void TcpDataStream::connectionStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectedState) {
        //timeoutTimer->start();
    }
}

void TcpDataStream::newData()
{
    timeoutTimer->start();
    QByteArray data = tcpSocket->readAll();
    QDataStream ds(data);

    byteCtr += data.size();
    while (!ds.atEnd()) {
        uchar tmp;
        ds >> tmp;
        tcpBuffer.append(tmp);

        if (!headerIsRead and tcpBuffer.size() > 15) {
            if (!checkHeader(tcpBuffer)) {
                tcpBuffer.clear();
            }
            else
                headerIsRead = true;
        }
        else if (headerIsRead && tcpBuffer.size() == (int)header.dataSize) {
            processData(tcpBuffer);
            headerIsRead = false;
            tcpBuffer.clear();
        }
    }
}
