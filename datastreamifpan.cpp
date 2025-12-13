#include "datastreamifpan.h"

DatastreamIfPan::DatastreamIfPan(QObject *parent)
{
    connect(m_traceTimer, &QTimer::timeout, this, &DatastreamIfPan::calcTracesPerSecond);
    m_traceTimer->start(1000);
}

bool DatastreamIfPan::checkHeaders()
{
    if (m_eb200Header.magicNumber != 0x000eb200 or m_eb200Header.dataSize > 65536)
        return false;
    if (m_attrHeader.tag != (int)Instrument::Tags::IFPAN and m_attrHeader.tag != (int)Instrument::Tags::ADVIFP)
        return false;
    return true;
}

void DatastreamIfPan::readData(QDataStream &ds)
{
    if (checkHeaders()) {
        ds.setByteOrder(QDataStream::LittleEndian);

        m_OptHeader.readData(ds, m_attrHeader.optHeaderLength);
        checkOptHeader();

        QVector<qint16> tmpBuffer(m_attrHeader.numItems);
        int readBytes = ds.readRawData((char *) tmpBuffer.data(), tmpBuffer.size() * 2);

        if (readBytes == tmpBuffer.size() * 2) {
            /* for (auto &val : tmpBuffer) // readRaw makes a mess out of byte order. Reorder manually
            val = qToBigEndian(val);*/

            emit traceReady(tmpBuffer);

            m_traceCtr++;
        }
    }
}

void DatastreamIfPan::checkOptHeader()
{
    quint64 centerFreq = (quint64)m_OptHeader.freqHigh << 32 | m_OptHeader.freqLow;

    if ((quint64)m_ifPanCenterFreq != centerFreq or m_ifPanSpan != m_OptHeader.freqSpan) {
        m_ifPanCenterFreq = centerFreq;
        m_ifPanSpan = m_OptHeader.freqSpan;
        emit frequencyChanged(m_ifPanCenterFreq - m_ifPanSpan / 2, m_ifPanCenterFreq + m_ifPanSpan / 2);
    }
    double resolution = 0;
    if (m_attrHeader.numItems) resolution = m_OptHeader.freqSpan / (m_attrHeader.numItems - 1);
    if ((int)resolution != (int)m_ifPanResolution) {
        m_ifPanResolution = resolution;
        emit resolutionChanged(m_ifPanResolution);
    }
}

void DatastreamIfPan::calcTracesPerSecond()
{
    if (m_traceCtr) {
        emit tracesPerSecond(m_traceCtr);
        m_traceCtr = 0;
    }
}
