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
 * The average level is also calculated here, used in
 * normalized spectrum and trig level settings
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
    void getSecondsOfBuffer(int secs = 0);
    void restartCalcAvgLevel();
    void sendDispTrigline()
    { if (normalizeSpectrum) emit newDispTriglevel(averageDispLevelNormalized);
        else emit newDispTriglevel(averageDispLevel);}
    void updSettings();
    void deviceDisconnected();
    void recorderStarted() { recording = true;}
    void recorderEnded() { recording = false;}

signals:
    void newDispTrace(const QVector<double> &data);
    void newDispMaxhold(const QVector<double> &data);
    void newDispTriglevel(const QVector<double> &data);
    void showMaxhold(const bool);
    void toIncidentLog(QString);
    void reqReplot();
    void averageLevelReady(const QVector<qint16> &data);
    void stopAvgLevelFlash();
    void averageLevelCalculating();
    void historicData(const QList<QDateTime>, const QList<QVector<qint16> >data);
    void traceToAnalyzer(const QVector<qint16> &data);
    void traceToRecorder(const QVector<qint16> &data);

private slots:
    void deleteOlderThan();
    void calcMaxhold();
    void addDisplayBufferTrace(const QVector<qint16> &data);
    void calcAvgLevel(const QVector<qint16> &data = QVector<qint16>());
    void finishAvgLevelCalc();
    QVector<qint16> calcNormalizedTrace(const QVector<qint16> &data);
    void maintainAvgLevel();

private:
    QSharedPointer<Config> config;
    QList<QDateTime> datetimeBuffer;
    QList<QVector<qint16>> traceBuffer;
    QVector<qint16> averageLevel;
    QVector<double> averageDispLevel;
    QVector<double> averageDispLevelNormalized;
    QVector<double> displayBuffer;
    QList<QVector<double>> maxholdBuffer;
    int maxholdTime = 10;
    const int bufferAge = 120; // always hold 120 seconds of buffer. maxhold/other output adjusted to their own settings
    QTimer *deleteOlderThanTimer;
    QTimer *averageLevelDoneTimer = new QTimer;
    QTimer *averageLevelMaintenanceTimer = new QTimer;
    QElapsedTimer *throttleTimer;
    QMutex mutex;
    int trigLevel;
    double avgFactor;
    QString fftMode, antPort;
    bool autoAtt;
    int att;
    bool normalizeSpectrum;
    bool recording = false;
    int tracesUsedInAvg = 0;
    int plotResolution;
    QElapsedTimer *maxholdBufferElapsedTimer = new QElapsedTimer;
    QVector<double> maxholdBufferAggregate;
    const int throttleTime = 100; // min time in ms between screen updates
    const int calcAvgLevelTime = 45; // secs
    const int avgLevelMaintenanceTime = 30; // secs
    const int tracesNeededForAvg = 250;
};

#endif // TRACEBUFFER_H
