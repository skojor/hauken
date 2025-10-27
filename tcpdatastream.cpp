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

void TcpDataStream::newDataHandler()
{
    timeoutTimer->start();
    QByteArray buf = tcpSocket->readAll();
    byteCtr += buf.size();
    tcpBuffer.append(buf);

    while (readHeader(tcpBuffer) && checkHeader() && tcpBuffer.size() >= (int)header.dataSize) {
        if (header.seqNumber == sequenceNr + 1) {
           if (!quarantine)
                processData(tcpBuffer.first(header.dataSize));
        }
        else {
            ///qDebug() << "OOO!" << sequenceNr << quarantine;
            quarantine = 8;
        }
        tcpBuffer = tcpBuffer.sliced(header.dataSize);

        if (quarantine)
            quarantine--;

        sequenceNr = header.seqNumber;
    }
#ifdef Q_OS_WIN
    while (tcpBuffer.size() > 127 && readHeader(tcpBuffer) && !checkHeader()) { // out of sync
        tcpBuffer.removeFirst();
    }
#else
    tcpBuffer.clear();
#endif
}
