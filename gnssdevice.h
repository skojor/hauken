#ifndef GNSSDEVICE_H
#define GNSSDEVICE_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <cmath>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include "config.h"

/*
 *  Class connecting to and reading data from up to two gnss receivers.
 *  Collects data for average C/No readings, AGC levels (if available)
 *  and position/altitude/time for spoofing detection. May be used
 *  to trigger recording of spectrum.
 */

class GnssDevice : public Config
{
    Q_OBJECT
public:
    explicit GnssDevice(QObject *parent = nullptr);

public slots:
    void start();
    void connectToPort(int portnr);
    void updSettings();
signals:

private slots:
    void incomingDataGnss1();
    void incomingDataGnss2();
    void handleBuffer();

private:
    QSerialPort *gnss1 = new QSerialPort;
    QSerialPort *gnss2 = new QSerialPort;
    QList<QSerialPortInfo> serialPortList;
    QByteArray gnss1Buffer, gnss2Buffer;

    // config cache below here
    double cnoLimit, agcLimit;
    double posOffsetLimit, altOffsetLimit, timeOffsetLimit;

};

#endif // GNSSDEVICE_H
