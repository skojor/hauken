#ifndef GNSSRECEIVERPROFILE_H
#define GNSSRECEIVERPROFILE_H

#include <QString>

struct GnssReceiverProfile
{
    enum class Type { Serial, Instrument };

    QString id;
    QString name;
    Type type = Type::Serial;
    bool enabled = false;
    QString portName;
    QString baudrate;
    bool ublox = false; // uBlox/Septentrio binary protocol enabled; also gates AGC monitoring in GnssAnalyzer
    bool logToFile = false;
    bool triggerRecording = false;
    bool nonRemovable = false; // true for the singleton instrument-GNSS entry
};

#endif // GNSSRECEIVERPROFILE_H
