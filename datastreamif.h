#ifndef DATASTREAMIF_H
#define DATASTREAMIF_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include "streamparserbase.h"

class DatastreamIf : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamIf(QObject *parent = nullptr);

signals:
    void ifDataReady(const QList<complexInt16> &);
    void headerChanged(quint64, quint32, quint32);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_optHeader.readData(ds);}
    bool checkHeaders();
    void checkOptHeader();

    IfOptHeader m_optHeader;
    quint64 m_frequency = 0;
    quint32 m_bandwidth = 0;
    quint32 m_samplerate = 0;
    quint64 m_sampleCtr = 0;

};

#endif // DATASTREAMIF_H
