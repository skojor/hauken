#ifndef TRACEANALYZER_H
#define TRACEANALYZER_H

#include <QObject>
#include <QDebug>
#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include "config.h"
#include "typedefs.h"

class TraceAnalyzer : public QObject
{
    Q_OBJECT
public:
    explicit TraceAnalyzer(QSharedPointer<Config> c);

public slots:
    void setTrace(const QVector<qint16> &data);
    void setAverageTrace(const QVector<qint16> &data);
    void resetAverageLevel() {averageData.clear();}
    void clearAlarmState() { alarmEmitted = false;}
    void updTrigFrequencyTable();
    void updSettings();
    void recorderStarted() { recorderRunning = true;}
    void recorderEnded() { recorderRunning  = false;}
    double getKhzAboveLimit() { return khzAboveLimit; }
    double getKhzAboveLimitTotal() { return khzAboveLimitTotal; }
    void setAnalyzerReady() { ready = true;}
    void setAnalyzerNotReady() { ready = false;}

signals:
    void alarm();
    void alarmEnded();
    void toIncidentLog(const NOTIFY::TYPE, const QString,const QString);

private slots:
    bool checkIfFrequencyIsInTrigArea(double freq);
    void alarmTriggered();
    void pmrCheckUptime(const unsigned long frequency, const bool active);

private:
    QSharedPointer<Config> config;
    QElapsedTimer *elapsedTimer = new QElapsedTimer;
    QVector<qint16> averageData;
    bool alarmEmitted = false;
    QList<QPair<double, double>> trigFrequenciesList;
    bool recorderRunning = false;
    double khzAboveLimit = 0, khzAboveLimitTotal = 0;
    bool ready = false;

    // config cache
    double trigLevel;
    double singleTrigBandwidth, totalTrigBandwidth, singleTrigCenterFrequency;
    int trigTime;
    double startFreq, stopFreq, resolution;
    bool pmrMode;
    //QList<PmrTable> pmrTable;
};

#endif // TRACEANALYZER_H
