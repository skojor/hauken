#ifndef TRACEBUFFER_H
#define TRACEBUFFER_H

#include <QVector>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include "typedefs.h"
#include "config.h"

/*
 * Class to keep a backlog of last x(120) seconds of trace data.
 * Used for recording data before incident, and for calculating
 * maxhold values. A display container is created with reduced
 * samples to simplify calculations.
 * The 2 min average level is also calculated here, used in
 * normalized spectrum and trig level
 */

class TraceBuffer : public QObject
{
    Q_OBJECT
public:
    explicit TraceBuffer(QSharedPointer<Config> c);

public slots:
    void start();
    void addTrace(const QVector<qint16> &data);
    void emptyBuffer();
    void getSecondsOfBuffer(int secs);
    void restartCalcAvgLevel();
    void sendDispTrigline() { emit newDispTriglevel(averageDispLevel);}
    void updSettings();
    void deviceDisconnected();

signals:
    void newDispTrace(const QVector<double> &data);
    void newDispMaxhold(const QVector<double> &data);
    void newDispTriglevel(const QVector<double> &data);
    void showMaxhold(const bool);
    void toIncidentLog(QString);
    void reqReplot();
    void averageLevelReady(const QVector<qint16> &data);
    void averageLevelCalculating();
    void historicData(const QList<QDateTime>, const QList<QVector<qint16> >data);

private slots:
    void deleteOlderThan();
    void calcMaxhold();
    void addDisplayBufferTrace(const QVector<qint16> &data);
    void calcAvgLevel();
    void finishAvgLevelCalc();

private:
    QSharedPointer<Config> config;
    QList<QDateTime> datetimeBuffer;
    QList<QVector<qint16>> traceBuffer;
    QVector<qint16> averageLevel;
    QVector<double> averageDispLevel;
    QList<QVector<double>> displayBuffer;
    int maxholdTime = 10;
    const int bufferAge = 120; // always hold 120 seconds of buffer. maxhold/other output adjusted to their own settings
    QTimer *deleteOlderThanTimer;
    QTimer *averageLevelDoneTimer = new QTimer;
    QTimer *averageLevelMaintenanceTimer = new QTimer;
    QElapsedTimer *throttleTimer;
    QMutex mutex;
    int trigLevel;
    double avgFactor;
    int fftMode = -1;
    const int plotResolution = 600;
    const int throttleTime = 100; // min time in ms between screen updates
    const int calcAvgLevelTime = 45; // secs
    const int avgLevelMaintenanceTime = 60; // secs
};

#endif // TRACEBUFFER_H
