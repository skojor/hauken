#ifndef TRACEANALYZER_H
#define TRACEANALYZER_H

#include <QObject>
#include <QDebug>
#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include "config.h"

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
    void recorderEnded() { recorderRunning = alarmEmitted = false;}
    double getKhzAboveLimit() { return khzAboveLimit; }

signals:
    void alarm();
    void toIncidentLog(QString);
    void toRecorder(QVector<qint16>);

private slots:
    bool checkIfFrequencyIsInTrigArea(double freq);
    void alarmTriggered();

private:
    QSharedPointer<Config> config;
    QElapsedTimer *elapsedTimer = new QElapsedTimer;
    QVector<qint16> averageData;
    double trigLevel;
    double trigBandwidth;
    int trigTime;
    double startFreq, stopFreq, resolution;
    bool alarmEmitted = false;
    QList<QPair<double, double>> trigFrequenciesList;
    bool recorderRunning = false;
    double khzAboveLimit;
};

#endif // TRACEANALYZER_H
