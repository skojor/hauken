#include "streamparserbase.h"

StreamParserBase::StreamParserBase(QObject *parent)
    : QObject{parent}
{}

void StreamParserBase::parseData(const QByteArray &data)
{
    QDataStream ds(data);
    if (m_eb200Header.readData(ds) and
        m_attrHeader.readData(ds))
        readData(ds);
}
