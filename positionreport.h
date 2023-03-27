#ifndef POSITIONREPORT_H
#define POSITIONREPORT_H

#include "config.h"
#include <QWidget>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include "config.h"
#include "typedefs.h"
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>


class PositionReport : public Config
{
    Q_OBJECT
public:
    explicit PositionReport(QObject *parent = nullptr);
    void start();
    void updSettings();

private slots:
    void configReportTimer();
    void generateReport();
    void sendReport();
    void checkReturnValue(int exitCode, QProcess::ExitStatus);

public slots:
    void updPosition(GnssData data) { gnssData = data;}
    void setMeasurementDevicePtr(QSharedPointer<Device> dev) { devicePtr = dev;}
    void setMeasurementDeviceConnectionStatus(bool b) { measurementDeviceConnected = b;}
    void updSensorData(double temp, double humidity) { sensorTemp = temp; sensorHumidity = humidity; sensorDataValid = true;}

private:
    QTimer *reportTimer = new QTimer;
    QTimer *gnssReqTimer = new QTimer;
    QTimer *sensorDataTimer = new QTimer;

    QProcess *curlProcess = new QProcess;
    bool measurementDeviceConnected = false;
    QSharedPointer<Device> devicePtr = nullptr;   // get from measurementDevice class, ask for the ptr in startup
    double sensorTemp = -99, sensorHumidity = 0;

    // config cache
    bool posReportActive, addPosition, addCogSog, addGnssStats, addConnStats, addSensorData;
    QString posSource, url, id;
    int reportInterval = 0;
    GnssData gnssData;
    QStringList reportArgs;
    bool sensorDataValid = false;
    QElapsedTimer uptime;

signals:
    void reqPosition(QString);
    void reqMeasurementDevicePtr();
    void reqSensorData(double &t, double &h);
};

#endif // POSITIONREPORT_H
