#ifndef DATASTREAMIFPAN_H
#define DATASTREAMIFPAN_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include <QElapsedTimer>
#include <QVector>
#include "streamparserbase.h"


class DatastreamIfPan : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamIfPan(QObject *parent = nullptr);
    void invalidateHeader() { m_ifPanCenterFreq = m_ifPanSpan = m_ifPanResolution = 0; }

signals:
    void traceReady(const QVector<qint16> &);
    void tracesPerSecond(double);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_OptHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders();
    void checkOptHeader();
    void calcTracesPerSecond();

    OptHeaderIfPanEB500 m_OptHeader;
    int m_traceCtr = 0;
    double m_ifPanCenterFreq = 0, m_ifPanSpan = 0;
    double m_ifPanResolution = 0;

};

#endif // DATASTREAMIFPAN_H
