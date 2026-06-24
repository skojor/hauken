#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QVector>

class Simulator : public QObject
{
    Q_OBJECT
public:
    explicit Simulator(QObject *parent = nullptr);
    bool isRunning() const { return running; }

public slots:
    void startTraceAnalyzerScenario();
    void startSignificantLevelScenario();
    void stopSimulation();

signals:
    void frequencyChanged(double startFrequencyHz, double stopFrequencyHz);
    void resolutionChanged(double resolutionHz);
    void tracesPerSecondChanged(double tracesPerSecond);
    void traceReady(const QVector<qint16> &data);
    void simulationStarted();
    void simulationFinished();

private slots:
    void sendTrace();

private:
    enum class Scenario {
        TraceAnalyzerAlarm,
        SignificantLevelIncrease
    };

    QVector<qint16> createTrace(qint64 elapsedMs) const;
    int signalIncrease(qint64 elapsedMs) const;
    double traceAnalyzerAlarmEnvelope(qint64 elapsedMs) const;
    double significantLevelEnvelope(qint64 elapsedMs) const;

    static constexpr double StartFrequencyHz = 1150e6;
    static constexpr double StopFrequencyHz = 1650e6;
    static constexpr double ResolutionHz = 6250.0;
    static constexpr double TraceRateHz = 2.0;
    static constexpr int TraceIntervalMs = 500;
    static constexpr qint64 SignalStartMs = 120000;
    static constexpr qint64 SignalRampMs = 10000;
    static constexpr qint64 SignalHoldMs = 30000;
    static constexpr qint64 SimulationEndMs = 180000;
    static constexpr qint64 SignificantSignalHoldMs = 60000;
    static constexpr qint64 SignificantIncreaseHoldMs = 60000;
    static constexpr qint64 SignificantSimulationEndMs = 270000;
    static constexpr double SignalStartFrequencyHz = 1560e6;
    static constexpr double SignalStopFrequencyHz = 1604e6;
    static constexpr int NoiseMinRaw = -300;
    static constexpr int NoiseMaxRaw = -50;
    static constexpr int SignalIncreaseRaw = 300;
    static constexpr int SignificantLevelIncreaseRaw = 120;

    QTimer traceTimer;
    QElapsedTimer elapsedTimer;
    Scenario scenario = Scenario::TraceAnalyzerAlarm;
    int sampleCount = 0;
    bool running = false;
};

#endif // SIMULATOR_H
