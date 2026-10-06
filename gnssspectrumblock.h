#ifndef GNSSSPECTRUMBLOCK_H
#define GNSSSPECTRUMBLOCK_H

#include <QDateTime>
#include <QVector>

// Vendor-neutral GNSS receiver spectrum sweep; levels are raw/relative vendor units, not calibrated dBm/dBuV
struct GnssSpectrumBlock
{
    QVector<double> levels;
    quint64 centerHz = 0;
    quint32 spanHz = 0;
    quint32 resolutionHz = 0;
    QDateTime timestamp;
    int blockIndex = 0; // which band/front-end this came from, within the same sweep message
    int blockCount = 1; // total number of bands reported in that same sweep message
};

#endif // GNSSSPECTRUMBLOCK_H
