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
        ds.setByteOrder(QDataStream::LittleEndian);
        m_optHeader.readData(ds, m_attrHeader.optHeaderLength);
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
        emit headerChanged(m_frequency, m_bandwidth, m_samplerate);
    }
    if (m_optHeader.sampleCount - m_sampleCtr != m_attrHeader.numItems) {
        //qDebug() << "Lost I/Q samples" << m_eb200Header.seqNumber << m_seqNr << m_optHeader.sampleCount << m_sampleCtr << m_attrHeader.numItems;
        //m_frequency = m_bandwidth = m_samplerate = 0;
    }
    m_seqNr = m_eb200Header.seqNumber;
    m_sampleCtr = m_optHeader.sampleCount;
}
