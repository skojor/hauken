#include "tcpdatastream.h"
#include <QtConcurrentRun>

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
   /* if (state == QAbstractSocket::UnconnectedState) // Whaat, why?
        timeoutTimer->stop();*/
    if (state == QAbstractSocket::ConnectedState) // Update Meas.Dev. class to check if user is this instance
        emit streamInfo(tcpSocket->localAddress(), tcpSocket->localPort());
}

void TcpDataStream::newDataHandler()
{
    timeoutTimer->start();
    QByteArray buf = tcpSocket->readAll();
    byteCtr += buf.size();
    tcpBuffer.append(buf);

    while (!tcpBuffer.isEmpty() && readHeadersSimplified(tcpBuffer) && tcpBuffer.size() >= (int)header.dataSize) {
        if (header.seqNumber == sequenceNr + 1) {
            processData(tcpBuffer.first(header.dataSize));
        }
        else {
            ///qDebug() << "ooo" << header.seqNumber << sequenceNr << attrHeader.length;
            emit waitForPscanEndMarker(true);
        }
        tcpBuffer = tcpBuffer.sliced(header.dataSize);
        sequenceNr = header.seqNumber;
    }
    if (tcpBuffer.size() > 24 and !readHeadersSimplified(tcpBuffer))
        tcpBuffer.clear(); // Buffer contains unreadable data, clear and continue

/*#ifdef Q_OS_WIN
    while (tcpBuffer.size() > 127 && !readHeaders(tcpBuffer)) { // out of sync
        tcpBuffer.removeFirst();
        qDebug() << "Whoops";
    }
#else
    tcpBuffer.clear();
#endif*/
}
