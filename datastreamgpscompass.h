#ifndef DATASTREAMGPSCOMPASS_H
#define DATASTREAMGPSCOMPASS_H

#include <QObject>
#include <QDebug>
#include <QtEndian>
#include <QElapsedTimer>
#include <QVector>
#include "streamparserbase.h"

class DatastreamGpsCompass : public StreamParserBase
{
    Q_OBJECT
public:
    explicit DatastreamGpsCompass(QObject *parent = nullptr);

signals:
    void traceReady(const QVector<qint16> &);
    void tracesPerSecond(double);
    void gpsdataReady(GpsData &);

private:
    void readData(QDataStream &ds);
    bool readOptHeader(QDataStream &ds) { return m_optHeader.readData(ds, m_attrHeader.optHeaderLength);}
    bool checkHeaders();
    void checkOptHeader();

    GpsCompassOptHeader m_optHeader;
    GpsCompassData m_gpsCompassData;
};

#endif // DATASTREAMGPSCOMPASS_H
