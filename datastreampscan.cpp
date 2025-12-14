#include "datastreampscan.h"

DatastreamPScan::DatastreamPScan(QObject *parent)
{
    //connect(m_timeoutTimer, &QTimer::timeout, this, &DatastreamPScan::invalidateHeader);
    connect(m_traceTimer, &QTimer::timeout, this, &DatastreamPScan::calcTracesPerSecond);
    m_traceTimer->start(1000);
}

bool DatastreamPScan::checkHeaders()
{
    if (m_eb200Header.magicNumber != 0x000eb200 or m_eb200Header.dataSize > 65536)
        return false;
    if (m_attrHeader.tag != (int)Instrument::Tags::PSCAN and m_attrHeader.tag != (int)Instrument::Tags::ADVPSC)
        return false;
    return true;
}

void DatastreamPScan::readData(QDataStream &ds)
{
    //m_timeoutTimer->start(5000); // 5 secs without data causes changed freq/res signal to be sent. Ususally caused by changed mode

    if (checkHeaders()) {
        ds.setByteOrder(QDataStream::LittleEndian);
        m_pscanOptHeader.readData(ds, m_attrHeader.optHeaderLength);
        checkOptHeader();

        QVector<qint16> tmpBuffer(m_attrHeader.numItems);
        int readBytes = ds.readRawData((char *) tmpBuffer.data(), tmpBuffer.size() * 2);

        if (readBytes == tmpBuffer.size() * 2) {
            /*for (auto &val : tmpBuffer) // readRaw makes a mess out of byte order. Reorder manually
            val = qToBigEndian(val);*/

            m_fft.append(tmpBuffer);

            if (m_waitingForPscanEndMarker && m_fft.last() == 2000) {
                m_waitingForPscanEndMarker = false;
                m_fft.clear(); // End of pscan received, but discarding all data since we don't know if the whole trace was received correct. Next complete trace should be correct then
            }

            if (!m_waitingForPscanEndMarker and !m_fft.isEmpty() and m_fft.last() == 2000) {
                m_fft.removeLast();
                int pscanSamplesPerTrace = 1 + ( (m_pscStopFreq - m_pscStartFreq) / m_pscanOptHeader.stepFreq );

                if (m_fft.size() > pscanSamplesPerTrace)
                    m_fft.remove(pscanSamplesPerTrace - 1, m_fft.size() - pscanSamplesPerTrace);

                if (m_fft.size() == pscanSamplesPerTrace)
                    emit traceReady(m_fft);

                m_fft.clear();
                m_traceCtr++;
                if (m_traceElapsedTimer->isValid())
                    traceTime = 1e3 / m_traceElapsedTimer->restart();
                else
                    m_traceElapsedTimer->start();
            }
        }
    }
}

void DatastreamPScan::checkOptHeader()
{
    quint64 startFreq = (quint64)m_pscanOptHeader.startFreqHigh << 32 | m_pscanOptHeader.startFreqLow;
    quint64 stopFreq = (quint64)m_pscanOptHeader.stopFreqHigh << 32 | m_pscanOptHeader.stopFreqLow;
    if (startFreq != m_pscStartFreq or stopFreq != m_pscStopFreq) {
        m_pscStartFreq = startFreq;
        m_pscStopFreq = stopFreq;
        emit frequencyChanged(startFreq, stopFreq);
    }
    if (m_pscRes != m_pscanOptHeader.stepFreq) {
        m_pscRes = m_pscanOptHeader.stepFreq;
        emit resolutionChanged(m_pscRes);
    }
}

void DatastreamPScan::calcTracesPerSecond()
{
    if (m_traceCtr)
        emit tracesPerSecond(traceTime);
    m_traceCtr = 0;
}
