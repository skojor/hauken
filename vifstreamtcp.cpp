#include "vifstreamtcp.h"

VifStreamTcp::VifStreamTcp()
{
    stopIqStreamTimer = new QTimer;
    connect(stopIqStreamTimer, &QTimer::timeout, this, &VifStreamTcp::processVifData);
}


void VifStreamTcp::openListener(const QHostAddress host, const int port)
{
    tcpSocket->close();

    tcpSocket->connectToHost(host, port);
    tcpSocket->waitForConnected(1000);
}

void VifStreamTcp::closeListener()
{
    timeoutTimer->stop();
    tcpSocket->close();
}

void VifStreamTcp::connectionStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "VIF TCP stream state" << state;
    //    if (state == QAbstractSocket::UnconnectedState)
    }

void VifStreamTcp::newData()
{
    QByteArray data = tcpSocket->readAll();
    ifBufferTcp.append(data);
    //qDebug() << "VIF buffer" << ifBufferTcp.size() << data.size();
    if (ifBufferTcp.size() > samplesNeeded * 4 && stopIqStreamTimer->isActive()) {
        processVifData();
    }
    else if (!stopIqStreamTimer->isActive()) // Data came after we asked for stop, ignore
        ifBufferTcp.clear();
}

void VifStreamTcp::parseVifData()
{
    const quint32 streamIdentifier = calcStreamIdentifier();
    QDataStream ds(ifBufferTcp);

    while (!ds.atEnd()) {

        qint16 nrOfWords = 0, infClassCode = 0, packetClassCode = 0;
        quint32 readStreamId = 0;

        ds.skipRawData(2);

        ds >> nrOfWords >> readStreamId;
        ds.skipRawData(4);
        ds >> infClassCode >> packetClassCode;
        ds.skipRawData(12);
        int readWords = 7;

        if (streamIdentifier == readStreamId && infClassCode == 0x0001 && packetClassCode == 0x0001) {
            // we have identied receiver and the packet code is raw IQ, continue
            while (readWords < nrOfWords) {
                readWords++;
                qint16 readI, readQ;
                ds >> readI >> readQ;
                i.append(readI);
                q.append(readQ);
            }
            i.removeLast();
            q.removeLast();
        }
        else if (streamIdentifier == readStreamId) {
            // we have correct id, but wrong packet code, skip and continue
            ds.skipRawData(nrOfWords * 4  - 28);
        }
        else
            ds.skipRawData(1);
    }
}

void VifStreamTcp::processVifData()
{
    emit stopIqStream();
    stopIqStreamTimer->stop();
    parseVifData();
    qDebug() << "Received total lines IQ:" << i.size();

    emit newIqData(i, q);
    i.clear();
    q.clear();
    ifBufferTcp.clear();
}

quint32 VifStreamTcp::calcStreamIdentifier()
{
    qDebug() << devicePtr->longId;
    QStringList split = devicePtr->longId.split(',');

    if (split.size() >= 3) {
        QString serial = split[2];
        serial.remove('.');
        serial.remove('/');
        serial.append('0');
        return serial.toULong();
    }
    else
        qDebug() << "Cannot calculate stream identifier?";
}
