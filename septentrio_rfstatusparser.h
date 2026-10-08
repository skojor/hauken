#ifndef SEPTENTRIO_RFSTATUSPARSER_H
#define SEPTENTRIO_RFSTATUSPARSER_H

#include <QByteArray>
#include <QVector>

struct SeptentrioRfBandStatus
{
    quint32 frequencyHz = 0;
    quint16 bandwidthKHz = 0;
    quint8 mode = 0;
    quint8 antennaId = 0;
    qint8 interferencePowerDbm = 0;
};

struct SeptentrioRfStatus
{
    bool received = false;
    quint8 flags = 0;
    QVector<SeptentrioRfBandStatus> bands;
};

namespace SeptentrioRfStatusParser
{
bool parse(const QByteArray &block, SeptentrioRfStatus &status);
}

#endif