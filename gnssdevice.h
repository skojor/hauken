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
 *  Class connecting to and reading data from one gnss receiver.
 *  Collects data for average C/No readings, AGC levels (if available)
 *  and position/altitude/time for spoofing detection. May be used
 *  to trigger recording of spectrum.
 */


class GnssDevice : public Config
{
    Q_OBJECT
public:
    explicit GnssDevice(QObject *parent = nullptr, int val = 0);

public slots:
    void start();
    void connectToPort();
    void updSettings();

signals:
    void analyzeThisData(GnssData &);
    void toIncidentLog(QString);

private slots:
    void handleBuffer();
    bool checkChecksum(const QByteArray &val);
    bool decodeGga(const QByteArray &val);
    bool decodeGsa(const QByteArray &val);
    bool decodeRmc(const QByteArray &val);
    bool decodeGsv(const QByteArray &val);
    QDateTime convFromGnssTimeToQDateTime(const QByteArray date, const QByteArray time);
    void decodeBinary(const QByteArray &val);
    bool checkBinaryChecksum(const QByteArray &val);
    void appendToLogfile(const QByteArray &data);

private:
    QSerialPort *gnss = new QSerialPort;
    //QList<QSerialPortInfo> serialPortList;
    QByteArray gnssBuffer;
    QTimer *sendToAnalyzerTimer = new QTimer;
    QList<QByteArray> gsvSentences;
    GnssData gnssData;
    int gsvNrOfSentences;
    QFile logfile;
    bool logToFile = false;
    QDate logfileStartedDate;
};

#endif // GNSSDEVICE_H