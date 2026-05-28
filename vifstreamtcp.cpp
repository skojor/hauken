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
    QByteArray buf = tcpSocket->readAll();
    //qDebug() << buf.size();
    byteCtr += buf.size();
    tcpBuffer.append(buf);
    HeaderType type = readHeadersSimplified(tcpBuffer);

    if (tcpBuffer.size() and
        (type == HeaderType::AMMOS or type == HeaderType::AMMOSINV)
        and
        tcpBuffer.size() >= locateAmmosHeader(tcpBuffer) + 26 * 4) // 26 words header
    {
        if (!m_sampleCtr) {
            m_sampleCtr = (quint64)ammosHeader.sampleCounterHigh << 32 | ammosHeader.sampleCounterLow;
        }
        else {
            quint64 newSamplesCtr = (quint64)ammosHeader.sampleCounterHigh << 32 | ammosHeader.sampleCounterLow;
            m_sampleCtr = newSamplesCtr;
        }

        quint64 headerFreq = (quint64)ammosHeader.freqHigh << 32 | ammosHeader.freqLow;

        if (m_freq != headerFreq or
            m_bw != ammosHeader.bandwidth or
            m_samplerate != ammosHeader.samplerate) {
            m_freq = headerFreq;
            m_bw = ammosHeader.bandwidth;
            m_samplerate = ammosHeader.samplerate;
            quint64 timestamp = (quint64)ammosHeader.startTimestampHigh | ammosHeader.startTimestampLow;
            emit headerChanged(m_freq, m_bw, m_samplerate, timestamp);
            //qDebug() << "new header" << m_freq << m_bw;
        }
        tcpBuffer.remove(locateAmmosHeader(tcpBuffer), 26 * 4); // Remove header and copy IQ data below
    }
    readIfData();

}

void VifStreamTcp::readIfData()
{
    if (tcpBuffer.size() and locateAmmosHeader(tcpBuffer) == -1) { // Copy data only if there is no header info inside
        const int16_t* src = reinterpret_cast<const int16_t*>(tcpBuffer.constData());
        int count = tcpBuffer.size() / 4;

        QVector<complexInt16> samples(count);

        for (int i = 0; i < count; ++i) {
            samples[i].real = src[2*i]; // I
            samples[i].imag = src[2*i + 1];     // Q
        }
        //iqSamples.append(samples);
        //qDebug() << "Added" << iqSamples.size() << "data points";
        emit ifDataReady(samples);
        //qDebug() << "new iq data" << samples.size();
        tcpBuffer.clear();
    }
}

int VifStreamTcp::parseDataPacket(const QByteArray &data)
{
    QDataStream ds(data);

    ds.skipRawData(28); // Skip header of data packet, 28 bytes / 7 words
    int readWords = 7;
    QVector<complexInt16> buf;
    if (nrOfWords > readWords && nrOfWords < 16000)
    {
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
    else
        return 0;
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
        && (packetClassCode == 0x0001 || packetClassCode == 0x0002)
        && nrOfWords > 0)
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
