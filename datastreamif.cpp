#include "datastreamif.h"


DatastreamIf::DatastreamIf(QObject *parent)
{}

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
        m_optHeader.readData(ds);
        checkOptHeader();

        if (m_optHeader.frameLength == 4) {
            const int totalBytes = m_attrHeader.numItems * m_optHeader.frameLength;

            QList<complexInt16> iqSamples(m_attrHeader.numItems);
            int read = ds.readRawData((char *)iqSamples.data(), totalBytes);
            if (read == totalBytes && ds.atEnd()) {
                for (auto &val : iqSamples) {
                    val.imag = qToBigEndian(val.imag);
                    val.real = qToBigEndian(val.real);
                }
                emit ifDataReady(iqSamples);
            }
            else
                qDebug() << "IF datastream: Byte numbers doesn't add up!" << read << totalBytes;
        }
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
        emit headerChanged(m_frequency, m_bandwidth, m_samplerate);
    }
}
