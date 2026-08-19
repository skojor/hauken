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
    QTimer *reportTimer = nullptr;
    QTimer *gnssReqTimer = nullptr;
    QTimer *sensorDataTimer = nullptr;
    QTimer *processTimeoutTimer = nullptr;

    QProcess *curlProcess = nullptr;
    bool measurementDeviceConnected = false, inUse = false;
    QString inUseBy, inUseByIp, modeUsed;
    quint64 startFreqUsed = 0, stopFreqUsed = 0, resUsed = 0;

    QSharedPointer<Device> devicePtr = nullptr;   // get from measurementDevice class, ask for the ptr in startup
    QSharedPointer<Config> config;

    double sensorTemp = -99, sensorHumidity = 0;

    // config cache
    bool posReportActive = false, addPosition = false, addCogSog = false, addGnssStats = false,
        addConnStats = false, addSensorData = false, addMqttData = false;
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
