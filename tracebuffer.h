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
#include <QBuffer>
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

#define AVGFILENAME ".avgdata"

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
    void getAiData(int secs = 0);
    void restartCalcAvgLevel();
    void sendDispTrigline()
    {
        if (normalizeSpectrum) {
            if (averageDispLevelNormalized.isEmpty() && trigLevel)
                averageDispLevelNormalized.fill(trigLevel, plotResolution);
            emit newDispTriglevel(averageDispLevelNormalized);
        }
        else {
            QVector<double> copy = averageDispLevel;
            for (auto &val : copy)
                val += trigLevel;

            emit newDispTriglevel(copy);
        }
    }
    void updSettings();
    void deviceDisconnected();
    void recorderStarted();
    void recorderEnded();
    void incidenceTriggered();
    //void updFlagInvalidateAvgLevels() { flagSavedAvgLevelsInvalidated = true; restartCalcAvgLevel(); }

signals:
    void newDispTrace(const QVector<double> &data);
    void newDispMaxhold(const QVector<double> &data);
    void newDispTriglevel(const QVector<double> &data);
    void showMaxhold(const bool);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void reqReplot();
    void averageLevelReady(const QVector<qint16> &data);
    void stopAvgLevelFlash();
    void averageLevelCalculating();
    void historicData(const QList<QDateTime>, const QList<QVector<qint16> >data);
    void aiData(const QList<QVector<qint16> >data);
    void traceToAnalyzer(const QVector<qint16> &data);
    void traceToRecorder(const QVector<qint16> &data);
    void traceData(const QVector<qint16> &data);

    void newDispMaxholdToWaterfall(const QVector<double> data);
    void nrOfDatapointsChanged(int);

private slots:
    void deleteOlderThan();
    void calcMaxhold();
    void addDisplayBufferTrace(const QVector<qint16> &data);
    void calcAvgLevel(const QVector<qint16> &data = QVector<qint16>());
    void finishAvgLevelCalc();
    QVector<qint16> calcNormalizedTrace(const QVector<qint16> &data);
    void maintainAvgLevel();
    void saveAvgLevels();
    bool restoreAvgLevels();

private:
    QSharedPointer<Config> config;
    QList<QDateTime> datetimeBuffer;
    QList<QVector<qint16>> traceBuffer;
    QList<QVector<qint16>> normTraceBuffer;
    QVector<qint16> traceCopy;
    QVector<qint16> averageLevel;
    QVector<double> averageDispLevel;
    QVector<double> averageDispLevelNormalized;
    QVector<double> displayBuffer;
    QList<QVector<double>> maxholdBuffer;
    int maxholdTime = 10;
    const int bufferAge = 120; // always hold 120 seconds of buffer. maxhold/other output adjusted to their own settings
    QTimer *deleteOlderThanTimer;
    QTimer *averageLevelMaintenanceTimer = new QTimer;
    QTimer *maintenanceRestartTimer = new QTimer;
    QElapsedTimer *throttleTimer;
    QMutex mutex;
    int trigLevel = 0;
    double avgFactor = 40;
    bool recording = false;
    int tracesUsedInAvg = 0;
    int plotResolution;
    QElapsedTimer *maxholdBufferElapsedTimer = new QElapsedTimer;
    QVector<double> maxholdBufferAggregate;
    const int throttleTime = 40; // min time in ms between screen updates
    const int avgLevelMaintenanceTime = 120000; // msecs
    int tracesNeededForAvg = 250;
    bool useDbm = false;
    int nrOfDataPoints = 0;
    bool flagSavedAvgLevelsInvalidated = true;
    bool flagAvgLevelsRestored = false;

    // Config cache
    quint64 startfreq, stopfreq, ffmCenterFreq;
    QString resolution, span;
    QString fftMode, antPort;
    bool autoAtt;
    int att;
    bool normalizeSpectrum;
    bool useSavedAvgLevels = false;
    bool init = true;
    int gainControl = 0;
    int skipTraces = 3;
};

#endif // TRACEBUFFER_H
