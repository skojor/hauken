#include "datastreamaudio.h"

DatastreamAudio::DatastreamAudio(QObject *parent)
{
}


bool DatastreamAudio::checkHeaders()
{
    if (m_attrHeader.tag != (int)Instrument::Tags::AUDIO)
        return false;
    if (m_attrHeader.optHeaderLength && m_optHeader.audioMode != m_audioMode) {
        m_audioMode = m_optHeader.audioMode;
        reportAudioMode(m_audioMode);
    }

    return true;
}

void DatastreamAudio::readData(QDataStream &ds)
{
    if (checkHeaders()) {
        ds.setByteOrder(QDataStream::LittleEndian);
        m_optHeader.readData(ds, m_attrHeader.optHeaderLength);

        checkForHeaderChanges();
        const int totalBytes = m_attrHeader.numItems * m_optHeader.frameLength;
        QByteArray data;
        data.resize(totalBytes);
        int read = ds.readRawData(data.data(), totalBytes);

        if (read == totalBytes) {
            /*switch (m_audioOptHeader.audioMode) {
            case 1: case 2: case 5: case 6: case 9: case 10: // 16 bit modes
                qint16 *samples = reinterpret_cast<qint16*>(data.data());
                for (int i = 0; i < data.size() / 2; i++) { // Swap endianness after serialized read
                    samples[i] = qToBigEndian(samples[i]);
                }
                break;
            }*/
            emit audioDataReady(data);
        }
    }
}

void DatastreamAudio::reportAudioMode(const int mode)
{
    switch (mode) {
    case 1:
        emit audioModeChanged(32000, 2, QAudioFormat::Int16);
        break;
    case 2:
        emit audioModeChanged(32000, 1, QAudioFormat::Int16);
        break;
    case 3:
        emit audioModeChanged(32000, 2, QAudioFormat::UInt8);
        break;
    case 4:
        emit audioModeChanged(32000, 1, QAudioFormat::UInt8);
        break;
    case 5:
        emit audioModeChanged(16000, 2, QAudioFormat::Int16);
        break;
    case 6:
        emit audioModeChanged(16000, 1, QAudioFormat::Int16);
        break;
    case 7:
        emit audioModeChanged(16000, 2, QAudioFormat::UInt8);
        break;
    case 8:
        emit audioModeChanged(16000, 1, QAudioFormat::UInt8);
        break;
    case 9:
        emit audioModeChanged(8000, 2, QAudioFormat::Int16);
        break;
    case 10:
        emit audioModeChanged(8000, 1, QAudioFormat::Int16);
        break;
    case 11:
        emit audioModeChanged(8000, 2, QAudioFormat::UInt8);
        break;
    case 12:
        emit audioModeChanged(8000, 1, QAudioFormat::UInt8);
        break;
    default:
        emit audioModeChanged(0, 0, QAudioFormat::Unknown);
    }
}

void DatastreamAudio::checkForHeaderChanges()
{
    quint64 f = (quint64)m_optHeader.freqHigh << 32 | m_optHeader.freqLow;

    if (f != m_freq || m_optHeader.bandwidth != m_bw || m_optHeader.demodString != m_demodType) { // Report changes
        m_freq = f;
        m_bw = m_optHeader.bandwidth;
        m_demodType = QString(m_optHeader.demodString);
        emit headerDataChanged(m_freq, m_bw, m_demodType);
    }
}
