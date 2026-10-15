#ifndef MEASUREMENTRECEIVERSETTINGS_H
#define MEASUREMENTRECEIVERSETTINGS_H

#include <QString>
#include <QtGlobal>

struct MeasurementReceiverSettings
{
    quint64 startFreq = 1.56e9;
    quint64 stopFreq = 1.61e9;
    QString resolution = "1";
    quint64 ffmCenterFreq = 1560;
    QString ffmSpan = "2000";
    int measurementTime = 18;
    int manualAttenuator = 0;
    bool autoAttenuator = true;
    int antennaPort = 0;
    QString mode = "PScan";
    QString fftMode = "0";
    int gainControl = 1;
};

#endif // MEASUREMENTRECEIVERSETTINGS_H