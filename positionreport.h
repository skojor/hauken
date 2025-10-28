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

#define PROCESSTIMEOUT_MS   5000


class PositionReport : public QObject
{
    Q_OBJECT
public:
    explicit PositionReport(QSharedPointer<Config>);
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
    void setMeasurementDeviceConnectionStatus(bool b) { measurementDeviceConnected = b; if (b) inUse = false;}
    void setMeasurementDeviceReconnected() { measurementDeviceConnected = true; inUse = false;}
    void updSensorData(double temp, double humidity) { sensorTemp = temp; sensorHumidity = humidity; sensorDataValid = true;}
    void setInUse(QString s) { inUse = true; inUseBy = s; }
    void setInUseByIp(QString s) { inUseByIp = s.simplified(); }
    void updMqttData(const QString name, double val);
    void setModeUsed(QString s) { modeUsed = s; }
    void setFreqUsed(quint64 a, quint64 b) { startFreqUsed = a, stopFreqUsed = b; }
    void setResUsed(int a) { resUsed = a; }

private:
    QTimer *reportTimer = new QTimer;
    QTimer *gnssReqTimer = new QTimer;
    QTimer *sensorDataTimer = new QTimer;
    QTimer *processTimeoutTimer = new QTimer;

    QProcess *curlProcess = new QProcess;
    bool measurementDeviceConnected = false, inUse = false;
    QString inUseBy, inUseByIp, modeUsed;
    quint64 startFreqUsed, stopFreqUsed, resUsed;

    QSharedPointer<Device> devicePtr = nullptr;   // get from measurementDevice class, ask for the ptr in startup
    QSharedPointer<Config> config;

    double sensorTemp = -99, sensorHumidity = 0;

    // config cache
    bool posReportActive, addPosition, addCogSog, addGnssStats, addConnStats, addSensorData, addMqttData;
    QString posSource, url, id;
    int reportInterval = 0;
    GnssData gnssData;
    QStringList reportArgs;
    bool sensorDataValid = false;
    QElapsedTimer uptime;
    QStringList mqttNames;
    QList<double> mqttValues;
    QList<QDateTime> mqttLastUpdated;

signals:
    void reqPosition(QString);
    void reqMeasurementDevicePtr();
    void reqSensorData(double &t, double &h);
};

#endif // POSITIONREPORT_H
