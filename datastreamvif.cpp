#include "datastreamvif.h"

DatastreamVif::DatastreamVif(QObject *parent)
    : StreamParserBase{parent}
{}

void DatastreamVif::readData(QDataStream &ds)
{
    if (checkHeaders()) {
        ds.setByteOrder(QDataStream::LittleEndian);

        Vita49DataHeader header;
        ds.readRawData((char *)&header, sizeof(header));
        qDebug() << "rec data" << qToBigEndian(header.infClassCode) << qToBigEndian(header.packetSize);

        //m_optHeader.readData(ds, m_attrHeader.optHeaderLength);
        //qDebug() << "Timestamp read at" << QDateTime::fromMSecsSinceEpoch(1e-6 * m_optHeader.startTimestamp);

        //checkOptHeader();

        /*if (m_optHeader.frameLength == 4) {
            //const int totalBytes = m_attrHeader.numItems * m_optHeader.frameLength;

            QVector<complexInt16> iqSamples(m_attrHeader.numItems);
            int read = ds.readRawData((char *)iqSamples.data(), totalBytes);
            if (ds.atEnd() && read == totalBytes) {
                /*for (auto &val : iqSamples) {
                    val.imag = qToBigEndian(val.imag);
                    val.real = qToBigEndian(val.real);
                }
                emit ifDataReady(std::move(iqSamples));
                //m_byteCtr += totalBytes;
                //qDebug() << "IQ byte ctr" << m_byteCtr;
                /*qDebug() << ds.atEnd() << iqSamples.size() << m_attrHeader.numItems << m_optHeader.sampleCount << iqSamples.first().imag
                         << iqSamples.first().real << iqSamples.last().imag << iqSamples.last().real << m_attrHeader.optHeaderLength;
            }
            else
                qDebug() << "IF datastream: Byte numbers doesn't add up!" << read << totalBytes << ds.atEnd();
        }
    }
    else {
        qDebug() << "IF datastream: Header check failed";
    }*/
}
}

