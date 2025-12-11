#ifndef DATASTREAMCW_H
#define DATASTREAMCW_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include "streamparserbase.h"

class DatastreamCw : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamCw(QObject *parent = nullptr);

signals:
    void freqChanged(quint64);
    void detector1Changed(int);
    void level(int);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_optHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders();
    void checkOptHeader();

    OptHeaderCw m_optHeader;
    quint64 m_frequency = 0;
    quint32 m_detector1 = -1;
};

#endif // DATASTREAMCW_H
