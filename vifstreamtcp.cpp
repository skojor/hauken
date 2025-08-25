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
    //    if (state == QAbstractSocket::UnconnectedState)
    }

void VifStreamTcp::newDataHandler()
{
    QByteArray data = tcpSocket->readAll();
    ifBufferTcp.append(data);
    if (ifBufferTcp.size() > 127) {
        while (!readHeader(ifBufferTcp)
               && ifBufferTcp.size() > 20)
        {  // Remove one byte at a time until a valid header is found, or buffer is empty
            ifBufferTcp.removeFirst();
        }

        while (readHeader(ifBufferTcp)
               && ifBufferTcp.size() >= nrOfWords * 4)
        {
            if (packetClassCode == 0x0001) {
                int readbytes = parseDataPacket(ifBufferTcp);
                if (readbytes > 0 && ifBufferTcp.size() >= readbytes)
                    ifBufferTcp.remove(0, readbytes);
                else
                    qDebug() << "Read 0 bytes, check your code dude" << readbytes << ifBufferTcp.size();
            }
            else if (packetClassCode == 0x0002) {
                int readbytes = parseContextPacket(ifBufferTcp);

                if (readbytes > 0 && ifBufferTcp.size() >= readbytes)
                    ifBufferTcp.remove(0, readbytes);
                else
                    qDebug() << "Read 0 bytes, check your code dude"<< readbytes << ifBufferTcp.size();
            }
        }
    }
}

int VifStreamTcp::parseDataPacket(const QByteArray &data)
{
    QDataStream ds(data);

    ds.skipRawData(28); // Skip header of data packet, 28 bytes / 7 words
    int readWords = 7;
    QList<complexInt16> buf;
    buf.resize((nrOfWords - readWords));
    int readBytes = ds.readRawData((char *)buf.data(), (nrOfWords - readWords) * 4);
    if (readBytes == (nrOfWords - readWords) * 4) {
        buf.removeLast();
        for (auto & val : buf) { // readRaw makes a mess out of byte order. Reorder manually
            val.real = qToBigEndian(val.real);
            val.imag = qToBigEndian(val.imag);
        }
        if (headerValidated) emit newIqData(buf);
    }
    return nrOfWords * 4;
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

bool VifStreamTcp::readHeader(const QByteArray &data)
{
    QDataStream ds(data);
    ds.skipRawData(2);
    ds >> nrOfWords >> readStreamId;
    ds.skipRawData(4);
    ds >> infClassCode >> packetClassCode;
    //qDebug() << "Header is read:" << nrOfWords << readStreamId << infClassCode << packetClassCode << ifBufferTcp.size();
    if (calcStreamIdentifier() == readStreamId
        && infClassCode == 0x0001
        && (packetClassCode == 0x0001 || packetClassCode == 0x0002))
        return true;
    else
        return false;
}

int VifStreamTcp::parseContextPacket(const QByteArray &data)
{
    headerValidated = false; // Always assume wrong data arrives

    QDataStream ds(data);

    quint32 intTimestamp;
    quint64 fractTimestamp;
    qint64 samplerate;
    qint64 bandwidth, refFrequency;

    ds.skipRawData(16); // Skip OUI and info class/packet class, already read
    ds >> intTimestamp >> fractTimestamp;
    ds.skipRawData(4);
    ds >> bandwidth >> refFrequency;
    ds.skipRawData(4);
    ds >> samplerate;

    samplerate = samplerate >> 20;
    bandwidth = bandwidth >> 20;
    refFrequency = refFrequency >> 20;

    if (readStreamId == calcStreamIdentifier()) {
        emit iqHeaderData(refFrequency, bandwidth, samplerate); // To validate freq/bw
    }
    else {
        qWarning() << "I/Q stream identifier mismatch. This should never happen";
    }
    return nrOfWords * 4;
}
