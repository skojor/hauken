#include "tcpdatastream.h"
#include <QtConcurrentRun>

TcpDataStream::TcpDataStream()
{

}

void TcpDataStream::openListener(const QHostAddress host, const int port)
{
    closeListener();
    listenerExpectedClosed = false;
    timeoutTimer->start();

    tcpSocket->connectToHost(host, port);
    tcpSocket->waitForConnected(1000);
    if (tcpSocket->isOpen()) {
        tcpSocket->write("\n");
        bytesPerSecTimer->start();
    }
}

void TcpDataStream::closeListener()
{
    listenerExpectedClosed = true;
    timeoutTimer->stop();
    bytesPerSecTimer->stop();
    tcpSocket->close();
}

void TcpDataStream::connectionStateChanged(QAbstractSocket::SocketState state)
{
    //qDebug() << "TCP stream state" << state;
    if (state == QAbstractSocket::ConnectedState) {
        timeoutTimer->start();
        emit streamInfo(tcpSocket->localAddress(), tcpSocket->localPort());
    }
    else if (state == QAbstractSocket::UnconnectedState && !listenerExpectedClosed) {
        listenerExpectedClosed = true;
        timeoutTimer->stop();
        bytesPerSecTimer->stop();
        emit timeout();
    }
}

void TcpDataStream::newDataHandler()
{
    timeoutTimer->start();
    QByteArray buf = tcpSocket->readAll();
    byteCtr += buf.size();
    tcpBuffer.append(buf);
    int pos;

    while (tcpBuffer.size() and
           readHeadersSimplified(tcpBuffer) == HeaderType::EB200 and
           tcpBuffer.size() >= header.dataSize + (pos = locateEb200Header(tcpBuffer)))
    {
        if (header.seqNumber == sequenceNr + 1) {
            processData(tcpBuffer.mid(pos, header.dataSize));
        }
        else {
            //qDebug() << "ooo" << header.seqNumber << sequenceNr << attrHeader.length;
            emit waitForPscanEndMarker(true);
        }
        tcpBuffer = tcpBuffer.remove(pos, header.dataSize);
        sequenceNr = header.seqNumber;
    }

    /*#ifdef Q_OS_WIN
    while (tcpBuffer.size() > 127 && !readHeaders(tcpBuffer)) { // out of sync
        tcpBuffer.removeFirst();
        qDebug() << "Whoops";
    }
#else
    tcpBuffer.clear();
#endif*/
}
