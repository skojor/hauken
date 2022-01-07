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
#include "typedefs.h"

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
    void displayGnssData(QString, int);
    void analyzeThisData(GnssData &);
    void toIncidentLog(QString);

private slots:
    void handleBuffer();
    bool checkChecksum(const QByteArray &val);
    bool decodeGga(const QByteArray &val, const int nr);
    bool decodeGsa(const QByteArray &val, const int nr);
    bool decodeRmc(const QByteArray &val, const int nr);
    bool decodeGsv(const QByteArray &val, const int nr);
    QDateTime convFromGnssTimeToQDateTime(const QByteArray date, const QByteArray time);
    void updDisplay();

private:
    QSerialPort *gnss1 = new QSerialPort;
    QSerialPort *gnss2 = new QSerialPort;
    QList<QSerialPortInfo> serialPortList;
    QByteArray gnss1Buffer, gnss2Buffer;
    QTimer *updDisplayTimer = new QTimer;
    QList<QByteArray>gsvSentences;
    QList<GnssData> gnssData;
    int display = 0;

    int gsvNrOfSentences;
};

#endif // GNSSDEVICE_H
