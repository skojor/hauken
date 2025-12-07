#ifndef DATASTREAMPSCAN_H
#define DATASTREAMPSCAN_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include <QElapsedTimer>
#include <QList>
#include "streamparserbase.h"

class DatastreamPScan : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamPScan(QObject *parent = nullptr);
    void updWaitForPscanEndMarker(bool b) { m_waitingForPscanEndMarker = true; }
signals:
    void traceReady(const QList<qint16> &);
    void tracesPerSecond(double);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_pscanOptHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders();
    void checkOptHeader();

    OptHeaderPScanEB500 m_pscanOptHeader;
    QList<qint16> m_fft;
    bool m_waitingForPscanEndMarker = true;
    QElapsedTimer *m_traceTimer = new QElapsedTimer;
    int m_traceCtr = 0;
    double m_pscStartFreq = 0, m_pscStopFreq = 0;
    double m_pscRes = 0;
};

#endif // DATASTREAMPSCAN_H
