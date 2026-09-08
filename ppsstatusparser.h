#ifndef PPSSTATUSPARSER_H
#define PPSSTATUSPARSER_H

#include <QByteArray>
#include <QString>
#include "typedefs.h"

class PpsStatusParser
{
public:
    static bool parseStatus(const QByteArray &payload, PpsData &data, QString &error);
    static bool parseAvailability(const QByteArray &payload, bool &online, QString &error);
};

#endif // PPSSTATUSPARSER_H