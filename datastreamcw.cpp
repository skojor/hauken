#include "datastreamcw.h"

DatastreamCw::DatastreamCw(QObject *parent)
{

}

bool DatastreamCw::checkHeaders()
{
    if (m_eb200Header.magicNumber != 0x000eb200 or m_eb200Header.dataSize > 65536)
        return false;
    if (m_attrHeader.tag != (int)Instrument::Tags::CW and m_attrHeader.tag != (int)Instrument::Tags::ADVCW)
        return false;
    return true;
}

void DatastreamCw::readData(QDataStream &ds)
{
    if (checkHeaders()) {
        ds.setByteOrder(QDataStream::LittleEndian);
        m_optHeader.readData(ds, m_attrHeader.optHeaderLength);
        checkOptHeader();

        qint16 val;
        ds >> val;
        emit level(val);
    }
    else {
        qDebug() << "CW datastream: Header check failed";
    }
}

void DatastreamCw::checkOptHeader()
{
    quint64 freq = (quint64)m_optHeader.freqHigh << 32 | m_optHeader.freqLow;
    if (freq != m_frequency) {
        m_frequency = freq;
        emit freqChanged(m_frequency);
    }
    if (m_detector1 != m_optHeader.detector1) {
        m_detector1 = m_optHeader.detector1;
        emit detector1Changed(m_detector1);
    }
}
