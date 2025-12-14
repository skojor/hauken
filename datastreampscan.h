#ifndef DATASTREAMPSCAN_H
#define DATASTREAMPSCAN_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include <QElapsedTimer>
#include <QVector>
#include "streamparserbase.h"

class DatastreamPScan : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamPScan(QObject *parent = nullptr);
    void updWaitForPscanEndMarker(bool b) { m_waitingForPscanEndMarker = true; }
    void invalidateHeader() { m_pscStartFreq = m_pscStopFreq = m_pscRes = 0; }

signals:
    void traceReady(const QVector<qint16> &);
    void tracesPerSecond(double);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_pscanOptHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders();
    void checkOptHeader();
    void calcTracesPerSecond();

    OptHeaderPScanEB500 m_pscanOptHeader;
    QVector<qint16> m_fft;
    bool m_waitingForPscanEndMarker = true;
    QElapsedTimer *m_traceElapsedTimer = new QElapsedTimer;
    int m_traceCtr = 0;
    double m_pscStartFreq = 0, m_pscStopFreq = 0;
    double m_pscRes = 0;
    double traceTime = 0;
};

#endif // DATASTREAMPSCAN_H
