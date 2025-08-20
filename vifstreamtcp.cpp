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

void VifStreamTcp::newDataHandler()
{
    QByteArray data = tcpSocket->readAll();
    ifBufferTcp.append(data);
    if (ifBufferTcp.size() > samplesNeeded * 4 && stopIqStreamTimer->isActive()) {
        processVifData();
    }
    else if (!stopIqStreamTimer->isActive()) // Data came after we asked for stop, ignore
        ifBufferTcp.clear();
}

void VifStreamTcp::parseVifData(const QByteArray &data)
{
    QList<complexInt16> iq;
    const quint32 streamIdentifier = calcStreamIdentifier();
    QDataStream ds(data);

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
            QList<complexInt16> buf;
            buf.resize((nrOfWords - readWords));
            int readBytes = ds.readRawData((char *)buf.data(), (nrOfWords - readWords) * 4);
            if (readBytes == (nrOfWords - readWords) * 4) {
                buf.removeLast();
                for (auto & val : buf) { // readRaw makes a mess out of byte order. Reorder manually
                    val.real = qToBigEndian(val.real);
                    val.imag = qToBigEndian(val.imag);
                }
                iq += buf;
            }
        }
        else if (streamIdentifier == readStreamId) {
            // we have correct id, but wrong packet code, skip and continue
            ds.skipRawData(nrOfWords * 4  - 28);
        }
        else
            ds.skipRawData(1);
    }
    qInfo() << "Received total lines IQ:" << iq.size();

    emit newIqData(iq);
    ifBufferTcp.clear();
}

void VifStreamTcp::processVifData()
{
    emit stopIqStream();
    stopIqStreamTimer->stop();
    if (!recordingMultipleBands)
        QFuture<void> future = QtConcurrent::run(&VifStreamTcp::parseVifData, this, ifBufferTcp);
    else {
        arrIfBufferTcp.append(ifBufferTcp);
        ifBufferTcp.clear();
    }
    if (arrIfBufferTcp.size() == multipleFfmCenterFreqs.size()) { // We are done, start processing
        for (auto && iqdata : arrIfBufferTcp) {
            emit newFfmCenterFrequency(multipleFfmCenterFreqs.first());
            parseVifData(iqdata);
            QElapsedTimer timer;
            timer.start();

            while (timer.elapsed() < 5 * 1000) {
                QApplication::processEvents();      // Terrible, terrible, please do this correctly!
                QThread::msleep(10);
            }
            multipleFfmCenterFreqs.removeFirst();

        }
    }
}

quint32 VifStreamTcp::calcStreamIdentifier()
{
    //qDebug() << devicePtr->longId;
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
    return 0;
}
