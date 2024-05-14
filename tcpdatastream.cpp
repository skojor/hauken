#include "tcpdatastream.h"

TcpDataStream::TcpDataStream()
{
}

void TcpDataStream::openListener(const QHostAddress host, const int port)
{
    tcpSocket->close();

    tcpSocket->connectToHost(host, port);
    tcpSocket->waitForConnected(1000);
    if (tcpSocket->isOpen()) {
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
    //qDebug() << "TCP stream state" << state;
    if (state == QAbstractSocket::UnconnectedState)
        timeoutTimer->stop();
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

        if (!headerIsRead and tcpBuffer.size() > 35) {
            if (!checkHeader(tcpBuffer)) { // out of sync here
                tcpBuffer.removeFirst();
                //tcpBuffer.clear();
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
