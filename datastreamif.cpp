#include "datastreamif.h"


DatastreamIf::DatastreamIf(QObject *parent)
{}

bool DatastreamIf::checkHeaders()
{
    bool valid = true;
    if (m_eb200Header.magicNumber != 0x000eb200 or m_eb200Header.dataSize > 65536)
        valid = false;
    if (m_attrHeader.tag != (int)Instrument::Tags::IF)
        valid = false;
    if (m_ifOptHeader.ifMode > 2) //TODO
        valid = false;

    return valid;
}

void DatastreamIf::readData(QDataStream &ds)
{
    const int totalBytes = m_attrHeader.numItems * m_ifOptHeader.frameLength;
    QByteArray data;
    data.resize(totalBytes);
    int read = ds.readRawData(data.data(), totalBytes);

    //if (read == totalBytes)
        //emit ifDataReady(data);
}
