#ifndef TRACEANALYZER_H
#define TRACEANALYZER_H

#include <QObject>
#include <QDebug>
#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include <QtGlobal>
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
    void resetAverageLevel() { averageData.clear(); resetSignificantLevelChangeState(); }
    void clearAlarmState() { alarmEmitted = false;}
    void updTrigFrequencyTable();
    void updSettings();
    void recorderStarted() { recorderRunning = true;}
    void recorderEnded() { recorderRunning  = false;}
    double getKhzAboveLimit() { return khzAboveLimit; }
    double getKhzAboveLimitTotal() { return khzAboveLimitTotal; }
    void setAnalyzerReady() { ready = true;}
    void setAnalyzerNotReady() { ready = false; resetSignificantLevelChangeState();}
    void freqChanged(double a, double b) { startFreq = a * 1e-6; stopFreq = b * 1e-6;}
    void resChanged(double a) { resolution = a * 1e-3;}

signals:
    void alarm();
    void alarmEnded();
    void significantLevelChange();
    void toIncidentLog(const NOTIFY::TYPE, const QString,const QString);
    void trigRegistered(double centerfreq);
    void maxLevelMeasured(double);
    void signalStatisticsUpdated(bool signalAboveThreshold, bool l1Interference);

private slots:
    bool checkIfFrequencyIsInTrigArea(double freq);
    void alarmTriggered();
    void checkSignificantLevelChange(qint16 currentMaxLevel, qint16 currentTriggerLevel);
    void resetSignificantLevelChangeState();
    void pmrCheckUptime(const quint64 frequency, const bool active);

private:
    static constexpr int SignificantLevelChangeDb = 10;
    static constexpr int SignificantLevelChangeDurationMs = 10000;
    static constexpr int SignificantLevelChangeRaw = SignificantLevelChangeDb * 10;

    QSharedPointer<Config> config;
    QElapsedTimer *elapsedTimer = new QElapsedTimer;
    QElapsedTimer significantLevelChangeTimer;
    QVector<qint16> averageData;
    bool alarmEmitted = false;
    bool significantLevelReferenceValid = false;
    bool significantLevelChangePending = false;
    int significantLevelChangeDirection = 0;
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
    qint16 maxLevel;
    qint16 stableMaxLevel = 0;
    bool useDbm = false;
    //QList<PmrTable> pmrTable;
};

#endif // TRACEANALYZER_H
