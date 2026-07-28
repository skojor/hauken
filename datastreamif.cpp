#include "datastreamif.h"

#include <algorithm>
#include <cstring>
#include <limits>


DatastreamIf::DatastreamIf(QObject *parent)
{}

void DatastreamIf::invalidateHeader()
{
    m_frequency = m_bandwidth = m_samplerate = m_sampleCtr = 0;
    m_seqNr = 0;
    m_buffer.clear();
}

void DatastreamIf::parseIfData(const QByteArray &data)
{
    m_buffer.append(data);

    constexpr qsizetype headerBytes = Eb200Header::size;
    while (m_buffer.size() >= headerBytes) {
        const int frameStart = locateHeader(m_buffer);
        if (frameStart < 0) {
            constexpr qsizetype magicBytes = 4;
            const qsizetype keepBytes = std::min<qsizetype>(m_buffer.size(), magicBytes - 1);
            if (m_buffer.size() > keepBytes)
                qDebug() << "IF parser dropped bytes without header"
                         << "dropped" << (m_buffer.size() - keepBytes)
                         << "kept" << keepBytes;
            m_buffer = m_buffer.right(keepBytes);
            break;
        }

        if (frameStart > 0) {
            m_buffer.remove(0, frameStart);
            continue;
        }

        if (!readHeaders(m_buffer.constData(), m_buffer.size())) {
            m_buffer.remove(0, 1);
            continue;
        }

        if (m_eb200Header.dataSize < quint32(Eb200Header::size + m_attrHeader.size)) {
            qDebug() << "IF parser invalid frame size"
                     << "frameBytes" << m_eb200Header.dataSize
                     << "attrBytes" << m_attrHeader.size;
            m_buffer.remove(0, 1);
            continue;
        }

        if (m_eb200Header.dataSize > quint32(std::numeric_limits<int>::max())) {
            qDebug() << "IF parser frame too large" << m_eb200Header.dataSize;
            m_buffer.clear();
            break;
        }

        const qsizetype frameBytes = qsizetype(m_eb200Header.dataSize);
        if (m_buffer.size() < frameBytes)
            break;

        readFrame(m_buffer.constData(), frameBytes);
        m_buffer.remove(0, frameBytes);
    }
}

bool DatastreamIf::checkHeaders()
{
    if (m_eb200Header.magicNumber != 0x000eb200 or m_eb200Header.dataSize > 65536)
        return false;
    if (m_attrHeader.tag != (int)Instrument::Tags::IF)
        return false;
    return true;
}

void DatastreamIf::readData(QDataStream &ds)
{
    if (checkHeaders()) {
        ds.setByteOrder(QDataStream::LittleEndian);
        m_optHeader.readData(ds, m_attrHeader.optHeaderLength);
        //qDebug() << "Timestamp read at" << QDateTime::fromMSecsSinceEpoch(1e-6 * m_optHeader.startTimestamp);

        checkOptHeader();

        if (m_optHeader.frameLength == 4) {
            const int totalBytes = m_attrHeader.numItems * m_optHeader.frameLength;

            QVector<complexInt16> iqSamples(m_attrHeader.numItems);
            int read = ds.readRawData((char *)iqSamples.data(), totalBytes);
            if (ds.atEnd() && read == totalBytes) {
                /*for (auto &val : iqSamples) {
                    val.imag = qToBigEndian(val.imag);
                    val.real = qToBigEndian(val.real);
                }*/
                emit ifDataReady(iqSamples);
                //m_byteCtr += totalBytes;
                //qDebug() << "IQ byte ctr" << m_byteCtr;
                /*qDebug() << ds.atEnd() << iqSamples.size() << m_attrHeader.numItems << m_optHeader.sampleCount << iqSamples.first().imag
                         << iqSamples.first().real << iqSamples.last().imag << iqSamples.last().real << m_attrHeader.optHeaderLength;*/
            }
            else
                qDebug() << "IF datastream: Byte numbers doesn't add up!" << read << totalBytes << ds.atEnd();
        }
    }
    else {
        qDebug() << "IF datastream: Header check failed";
    }
}

void DatastreamIf::checkOptHeader()
{
    quint64 freq = (quint64)m_optHeader.freqHigh << 32 | m_optHeader.freqLow;
    if (freq != m_frequency or
        m_bandwidth != m_optHeader.bandwidth or
        m_samplerate != m_optHeader.sampleRate)
    {
        m_frequency = freq;
        m_bandwidth = m_optHeader.bandwidth;
        m_samplerate = m_optHeader.sampleRate;
        emit headerChanged(m_frequency, m_bandwidth, m_samplerate, m_optHeader.startTimestamp);
    }
    if (m_optHeader.sampleCount - m_sampleCtr != m_attrHeader.numItems) {
        //qDebug() << "Lost I/Q samples" << m_eb200Header.seqNumber << m_seqNr << m_optHeader.sampleCount << m_sampleCtr << m_attrHeader.numItems;
        //m_frequency = m_bandwidth = m_samplerate = 0;
    }
    m_seqNr = m_eb200Header.seqNumber;
    m_sampleCtr = m_optHeader.sampleCount;
}

int DatastreamIf::locateHeader(const QByteArray &buf) const
{
    QByteArray pattern;
    pattern.append(char(0x00));
    pattern.append(char(0x0e));
    pattern.append(char(0xb2));
    pattern.append(char(0x00));
    return buf.indexOf(pattern);
}

bool DatastreamIf::readHeaders(const char *data, qsizetype size)
{
    if (size < Eb200Header::size + UdpDatagramAttribute::size)
        return false;

    m_eb200Header.magicNumber = qFromBigEndian<quint32>(data);
    m_eb200Header.versionMinor = qFromBigEndian<quint16>(data + 4);
    m_eb200Header.versionMajor = qFromBigEndian<quint16>(data + 6);
    m_eb200Header.seqNumber = qFromBigEndian<quint16>(data + 8);
    m_eb200Header.reserved = qFromBigEndian<quint16>(data + 10);
    m_eb200Header.dataSize = qFromBigEndian<quint32>(data + 12);

    if (m_eb200Header.magicNumber != 0x000eb200)
        return false;

    const char *attr = data + Eb200Header::size;
    m_attrHeader.tag = qFromBigEndian<quint16>(attr);
    if (m_attrHeader.tag < 5000) {
        m_attrHeader.size = UdpDatagramAttribute::size;
        m_attrHeader.length = qFromBigEndian<quint16>(attr + 2);
        m_attrHeader.numItems = qFromBigEndian<quint16>(attr + 4);
        m_attrHeader.channelNumber = quint8(attr[6]);
        m_attrHeader.optHeaderLength = quint8(attr[7]);
        m_attrHeader.selectorFlagsLow = qFromBigEndian<quint32>(attr + 8);
    }
    else {
        if (size < Eb200Header::size + GenAttrAdvanced::size)
            return false;
        m_attrHeader.size = GenAttrAdvanced::size;
        m_attrHeader.reserved = qFromBigEndian<quint16>(attr + 2);
        m_attrHeader.length = qFromBigEndian<quint32>(attr + 4);
        m_attrHeader.numItems = qFromBigEndian<quint32>(attr + 24);
        m_attrHeader.channelNumber = qFromBigEndian<quint32>(attr + 28);
        m_attrHeader.optHeaderLength = qFromBigEndian<quint32>(attr + 32);
        m_attrHeader.selectorFlagsLow = qFromBigEndian<quint32>(attr + 36);
        m_attrHeader.selectorFlagsHigh = qFromBigEndian<quint32>(attr + 40);
    }

    return true;
}

bool DatastreamIf::readOptHeader(const char *data, qsizetype size)
{
    if (m_attrHeader.optHeaderLength < 58 or size < 58)
        return false;

    m_optHeader.ifMode = qFromLittleEndian<qint16>(data);
    m_optHeader.frameLength = qFromLittleEndian<qint16>(data + 2);
    m_optHeader.sampleRate = qFromLittleEndian<quint32>(data + 4);
    m_optHeader.freqLow = qFromLittleEndian<quint32>(data + 8);
    m_optHeader.bandwidth = qFromLittleEndian<quint32>(data + 12);
    m_optHeader.demodulation = qFromLittleEndian<quint16>(data + 16);
    m_optHeader.rxAttenuation = qFromLittleEndian<qint16>(data + 18);
    m_optHeader.flags = qFromLittleEndian<quint16>(data + 20);
    m_optHeader.kFactor = qFromLittleEndian<qint16>(data + 22);
    std::memcpy(m_optHeader.demodString, data + 24, sizeof(m_optHeader.demodString));
    m_optHeader.sampleCount = qFromLittleEndian<quint64>(data + 32);
    m_optHeader.freqHigh = qFromLittleEndian<quint32>(data + 40);
    m_optHeader.rxGain = qFromLittleEndian<qint16>(data + 44);
    std::memcpy(m_optHeader.reserved, data + 46, sizeof(m_optHeader.reserved));
    m_optHeader.startTimestamp = qFromLittleEndian<quint64>(data + 48);
    m_optHeader.signalSource = qFromLittleEndian<qint16>(data + 56);
    return true;
}

void DatastreamIf::readFrame(const char *data, qsizetype size)
{
    if (m_eb200Header.magicNumber != 0x000eb200 or m_attrHeader.tag != (int)Instrument::Tags::IF)
        return;

    const qsizetype optOffset = Eb200Header::size + m_attrHeader.size;
    if (size < optOffset + qsizetype(m_attrHeader.optHeaderLength)) {
        qDebug() << "IF parser truncated optional header"
                 << "frameBytes" << size
                 << "optOffset" << optOffset
                 << "optHeaderBytes" << m_attrHeader.optHeaderLength;
        return;
    }

    if (!readOptHeader(data + optOffset, size - optOffset)) {
        qDebug() << "IF parser unsupported optional header"
                 << "optHeaderBytes" << m_attrHeader.optHeaderLength;
        return;
    }

    checkOptHeader();

    if (m_optHeader.frameLength != 4)
        return;

    const quint64 payloadBytes = quint64(m_attrHeader.numItems) * quint64(m_optHeader.frameLength);
    if (m_attrHeader.numItems > quint32(std::numeric_limits<int>::max()) or
        payloadBytes > quint64(std::numeric_limits<qsizetype>::max())) {
        qDebug() << "IF parser payload too large"
                 << "items" << m_attrHeader.numItems
                 << "payloadBytes" << payloadBytes;
        return;
    }

    const qsizetype payloadOffset = optOffset + qsizetype(m_attrHeader.optHeaderLength);
    if (size < payloadOffset + qsizetype(payloadBytes)) {
        qDebug() << "IF parser truncated payload"
                 << "frameBytes" << size
                 << "payloadOffset" << payloadOffset
                 << "payloadBytes" << payloadBytes;
        return;
    }

    QVector<complexInt16> iqSamples(qsizetype(m_attrHeader.numItems));
    std::memcpy(iqSamples.data(), data + payloadOffset, qsizetype(payloadBytes));
    emit ifDataReady(iqSamples);
}
