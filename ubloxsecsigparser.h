#ifndef UBLOXSECSIGPARSER_H
#define UBLOXSECSIGPARSER_H

#include <QByteArray>
#include <QVector>

enum class UbloxJammingState : quint8
{
    UNKNOWN,
    NOJAMMING,
    WARNING
};

enum class UbloxSpoofingState : quint8
{
    UNKNOWN,
    NOSPOOFING,
    INDICATED,
    AFFIRMED
};

struct UbloxSecSigFrequency
{
    quint32 centerFrequencyKHz = 0;
    bool jammed = false;
};

struct UbloxSecSigData
{
    bool jammingDetectionEnabled = false;
    UbloxJammingState jammingState = UbloxJammingState::UNKNOWN;
    bool spoofingDetectionEnabled = false;
    UbloxSpoofingState spoofingState = UbloxSpoofingState::UNKNOWN;
    QVector<UbloxSecSigFrequency> frequencies;
};

class UbloxSecSigParser
{
public:
    static bool parse(const QByteArray &frame, UbloxSecSigData &data);
};

#endif // UBLOXSECSIGPARSER_H