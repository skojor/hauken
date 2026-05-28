#include "datastreamammos.h"

DatastreamAmmos::DatastreamAmmos(QObject *parent)
    : StreamParserBase{parent}
{}

bool DatastreamAmmos::checkHeaders()
{
    if (m_header.magicWord == 0xfb746572) {
        quint64 freq = (quint64)m_header.freqHigh << 32 | m_header.freqLow;
        quint64 timestamp = (quint64)m_header.timestampHigh << 32 | m_header.timetampLow;
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
        /*if (sampleCount - m_sampleCtr != m_header.frameLength) {
            qDebug() << "Lost I/Q samples" << m_header.frameCount << m_seqNr <<sampleCount << m_sampleCtr << m_header.frameLength;
            //m_frequency = m_bandwidth = m_samplerate = 0;
        }*/
        m_seqNr = m_header.frameCount;
        m_sampleCtr = sampleCount;

        return true;
    }
    return false;
}

void DatastreamAmmos::readData(QDataStream &ds)
{
    if (readOptHeader(ds) and checkHeaders()) {

        //ds.setByteOrder(QDataStream::LittleEndian);

        const int totalBytes = m_header.datablockLength * 4;

        QVector<complexInt16> iqSamples(m_header.datablockLength); // NB! Assumes 16 bit IQ pairs. Will fail miserably if we try sth else

        int read = ds.readRawData((char *)iqSamples.data(), totalBytes);
        if (read == totalBytes) {
            for (auto &val : iqSamples) { // Rearrange i/q, sent in opposite order
                    const qint16 tmp = val.imag;
                    val.imag = val.real;
                    val.real = tmp;
            }
            emit ifDataReady(iqSamples); //std::move(iqSamples));
            //m_byteCtr += totalBytes;
            //qDebug() << "IQ byte ctr" << m_byteCtr;
            /*qDebug() << ds.atEnd() << iqSamples.size() << m_attrHeader.numItems << m_optHeader.sampleCount << iqSamples.first().imag
                         << iqSamples.first().real << iqSamples.last().imag << iqSamples.last().real << m_attrHeader.optHeaderLength;*/
        }
        else
            qDebug() << "IF datastream: Byte numbers doesn't add up!" << read << totalBytes << ds.atEnd();
    }
    else {
        qDebug() << "IF datastream: Header check failed";
    }
}

void DatastreamAmmos::checkOptHeader()  // Not used in AMMOS
{
}

bool DatastreamAmmos::readOptHeader(QDataStream &ds)
{
    if (ds.readRawData((char *)&m_header, sizeof(m_header)) == -1)
        return false;
    return true;
}
