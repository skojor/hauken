#include "datastreamammos.h"

#include <QtEndian>
#include <algorithm>
#include <cstring>
#include <limits>

DatastreamAmmos::DatastreamAmmos(QObject *parent)
    : StreamParserBase{parent}
{}

void DatastreamAmmos::parseAmmosData(const QByteArray &data)
{
    m_buffer.append(data);

    constexpr qsizetype headerBytes = sizeof(AmmosHeader);

    while (m_buffer.size() >= headerBytes) {
        const int headerPos = locateHeader(m_buffer);
        const int invertedHeaderPos = locateInvertedHeader(m_buffer);
        const bool inverted = headerPos == -1 or (invertedHeaderPos > -1 and invertedHeaderPos < headerPos);
        const int frameStart = inverted ? invertedHeaderPos : headerPos;

        if (frameStart < 0) {
            constexpr qsizetype magicBytes = 4;
            const qsizetype keepBytes = std::min<qsizetype>(m_buffer.size(), magicBytes - 1);
            m_buffer = m_buffer.right(keepBytes);
            break;
        }

        if (frameStart > 0) {
            m_buffer.remove(0, frameStart);
            continue;
        }

        AmmosHeader frameHeader;
        memcpy(&frameHeader, m_buffer.constData(), sizeof(frameHeader));
        if (inverted) {
            m_header = frameHeader;
            swapHeader();
            frameHeader = m_header;
        }

        const quint64 payloadBytes = quint64(frameHeader.datablockLength) * sizeof(complexInt16);
        if (frameHeader.datablockLength > quint32(std::numeric_limits<int>::max()) or
            payloadBytes > quint64(std::numeric_limits<qsizetype>::max()) - headerBytes) {
            qDebug() << "AMMOS payload too large" << payloadBytes;
            m_buffer.clear();
            break;
        }

        const qsizetype frameBytes = headerBytes + qsizetype(payloadBytes);
        if (m_buffer.size() < frameBytes)
            break;

        m_invertedFrame = inverted;
        const QByteArray frame = m_buffer.left(frameBytes);
        QDataStream ds(frame);
        readData(ds);
        m_buffer.remove(0, frameBytes);
    }
}

bool DatastreamAmmos::checkHeaders()
{
    if (m_header.magicWord == 0xfb746572) {
        quint64 freq = (quint64)m_header.freqHigh << 32 | m_header.freqLow;
        quint64 timestamp = (quint64)m_header.startTimestampHigh << 32 | m_header.startTimestampLow;
        quint64 sampleCount = (quint64)m_header.sampleCounterHigh << 32 | m_header.sampleCounterLow;

        if (freq != m_frequency or
            m_bandwidth != m_header.bandwidth or
            m_samplerate != m_header.samplerate)
        {
            m_frequency = freq;
            m_bandwidth = m_header.bandwidth;
            m_samplerate = m_header.samplerate;
            emit headerChanged(m_frequency, m_bandwidth, m_samplerate, timestamp);
        }

        m_seqNr = m_header.frameCount;
        m_sampleCtr = sampleCount;

        return true;
    }
    return false;
}

void DatastreamAmmos::readData(QDataStream &ds)
{
    if (readOptHeader(ds) and checkHeaders()) {
        const qsizetype totalBytes = qsizetype(m_header.datablockLength) * sizeof(complexInt16);
        QVector<complexInt16> iqSamples(m_header.datablockLength);

        if (!m_invertedFrame) {
            const int read = ds.readRawData(reinterpret_cast<char *>(iqSamples.data()), totalBytes);
            if (read == totalBytes) {
                emit ifDataReady(iqSamples);
            }
            else {
                qDebug() << "AMMOS datastream: Byte numbers don't add up!" << read << totalBytes << ds.atEnd();
            }
            return;
        }

        QByteArray payload(totalBytes, Qt::Uninitialized);
        const int read = ds.readRawData(payload.data(), totalBytes);
        if (read == totalBytes) {
            const qint16 *src = reinterpret_cast<const qint16 *>(payload.constData());
            for (qsizetype i = 0; i < iqSamples.size(); ++i) {
                // AMMOSINV data is 32-bit swapped: Q bytes arrive before I bytes,
                // and each 16-bit sample is byte-swapped inside the word.
                iqSamples[i].real = qbswap(src[2 * i + 1]);
                iqSamples[i].imag = qbswap(src[2 * i]);
            }
            emit ifDataReady(iqSamples);
        }
        else {
            qDebug() << "AMMOS datastream: Byte numbers don't add up!" << read << totalBytes << ds.atEnd();
        }
    }
    else {
        qDebug() << "AMMOS datastream: Header check failed";
    }
}

void DatastreamAmmos::checkOptHeader()
{
}

bool DatastreamAmmos::readOptHeader(QDataStream &ds)
{
    if (ds.readRawData(reinterpret_cast<char *>(&m_header), sizeof(m_header)) == -1)
        return false;

    if (m_invertedFrame)
        swapHeader();

    return true;
}

int DatastreamAmmos::locateHeader(const QByteArray &buf) const
{
    QByteArray pattern;
    pattern.append(char(0x72));
    pattern.append(char(0x65));
    pattern.append(char(0x74));
    pattern.append(char(0xfb));
    return buf.indexOf(pattern);
}

int DatastreamAmmos::locateInvertedHeader(const QByteArray &buf) const
{
    QByteArray pattern;
    pattern.append(char(0xfb));
    pattern.append(char(0x74));
    pattern.append(char(0x65));
    pattern.append(char(0x72));
    return buf.indexOf(pattern);
}

void DatastreamAmmos::swapHeader()
{
    m_header.magicWord = qbswap(m_header.magicWord);
    m_header.frameLength = qbswap(m_header.frameLength);
    m_header.frameCount = qbswap(m_header.frameCount);
    m_header.frameType = qbswap(m_header.frameType);
    m_header.dataHeaderLength = qbswap(m_header.dataHeaderLength);
    m_header.reserved = qbswap(m_header.reserved);
    m_header.datablockCount = qbswap(m_header.datablockCount);
    m_header.datablockLength = qbswap(m_header.datablockLength);
    m_header.timetampLow = qbswap(m_header.timetampLow);
    m_header.timestampHigh = qbswap(m_header.timestampHigh);
    m_header.statusWord = qbswap(m_header.statusWord);
    m_header.signalSourceId = qbswap(m_header.signalSourceId);
    m_header.signalSourceState = qbswap(m_header.signalSourceState);
    m_header.freqLow = qbswap(m_header.freqLow);
    m_header.freqHigh = qbswap(m_header.freqHigh);
    m_header.bandwidth = qbswap(m_header.bandwidth);
    m_header.samplerate = qbswap(m_header.samplerate);
    m_header.interpolation = qbswap(m_header.interpolation);
    m_header.decimation = qbswap(m_header.decimation);
    m_header.intAntennaVoltageRef = qbswap(m_header.intAntennaVoltageRef);
    m_header.startTimestampLow = qbswap(m_header.startTimestampLow);
    m_header.startTimestampHigh = qbswap(m_header.startTimestampHigh);
    m_header.sampleCounterLow = qbswap(m_header.sampleCounterLow);
    m_header.sampleCounterHigh = qbswap(m_header.sampleCounterHigh);
    m_header.kFactor = qbswap(m_header.kFactor);
    m_header.datablockStatus = qbswap(m_header.datablockStatus);
}
