#ifndef UBLOXMONSPANPARSER_H
#define UBLOXMONSPANPARSER_H

#include <QByteArray>
#include <QVector>

struct UbloxMonSpanBlock
{
    QByteArray spectrum;
    quint32 spanHz = 0;
    quint32 resolutionHz = 0;
    quint32 centerHz = 0;
    quint8 pga = 0;
};

namespace UbloxMonSpanParser {
bool parse(const QByteArray &frame, QVector<UbloxMonSpanBlock> &blocks);
}

#endif // UBLOXMONSPANPARSER_H