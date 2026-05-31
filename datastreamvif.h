#ifndef DATASTREAMVIF_H
#define DATASTREAMVIF_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include "streamparserbase.h"

/* Class to read Vita 49 data from R&S. NB! Never finished, using AMMOS protocol instead!
 *
 */

class DatastreamVif : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamVif(QObject *parent = nullptr);
    void invalidateHeader() { m_frequency = m_bandwidth = m_samplerate = m_sampleCtr = 0; }

signals:
    void ifDataReady(const QVector<complexInt16>);
    void headerChanged(quint64, quint32, quint32, quint64);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) {return true;} //{ return m_optHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders() {return true;}
    void checkOptHeader() {}

    quint64 m_frequency = 0;
    quint32 m_bandwidth = 0;
    quint32 m_samplerate = 0;
    quint64 m_sampleCtr = 0;
    int m_seqNr = 0;
    int m_byteCtr = 0;


};

#endif // DATASTREAMVIF_H
