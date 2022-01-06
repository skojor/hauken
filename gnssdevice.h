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

class GnssDevice : public Config
{
    Q_OBJECT
public:
    explicit GnssDevice(QObject *parent = nullptr);

public slots:
    void start();

signals:

private slots:

private:
    QSerialPort *serialPort = new QSerialPort;
    QList<QSerialPortInfo> serialPortList;
};

#endif // GNSSDEVICE_H
