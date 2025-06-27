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
    qDebug() << "TCP stream state" << state;
    if (state == QAbstractSocket::UnconnectedState)
        timeoutTimer->stop();
}

void TcpDataStream::newDataHandler()
{
    timeoutTimer->start();
    QByteArray buf = tcpSocket->readAll();
    byteCtr += buf.size();
    tcpBuffer.append(buf);

    while (readHeader(tcpBuffer) && checkHeader() && tcpBuffer.size() >= (int)header.dataSize) {
        processData(tcpBuffer.first(header.dataSize));
        tcpBuffer = tcpBuffer.sliced(header.dataSize);
    }
#ifdef Q_OS_WIN
    while (readHeader(tcpBuffer) && !checkHeader()) // out of sync
        tcpBuffer.removeFirst();
#else
    tcpBuffer.clear();
#endif
}
