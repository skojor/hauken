#include "simulator.h"

#include <QRandomGenerator>
#include <QtGlobal>

Simulator::Simulator(QObject *parent)
    : QObject(parent)
{
    sampleCount = qRound((StopFrequencyHz - StartFrequencyHz) / ResolutionHz) + 1;
    traceTimer.setInterval(TraceIntervalMs);
    connect(&traceTimer, &QTimer::timeout, this, &Simulator::sendTrace);
}

void Simulator::startTraceAnalyzerScenario()
{
    if (running) return;

    scenario = Scenario::TraceAnalyzerAlarm;
    running = true;
    elapsedTimer.restart();

    emit frequencyChanged(StartFrequencyHz, StopFrequencyHz);
    emit resolutionChanged(ResolutionHz);
    emit tracesPerSecondChanged(TraceRateHz);
    emit simulationStarted();

    sendTrace();
    traceTimer.start();
}

void Simulator::startSignificantLevelScenario()
{
    if (running) return;

    scenario = Scenario::SignificantLevelIncrease;
    running = true;
    elapsedTimer.restart();

    emit frequencyChanged(StartFrequencyHz, StopFrequencyHz);
    emit resolutionChanged(ResolutionHz);
    emit tracesPerSecondChanged(TraceRateHz);
    emit simulationStarted();

    sendTrace();
    traceTimer.start();
}

void Simulator::stopSimulation()
{
    if (!running) return;

    traceTimer.stop();
    running = false;
    emit simulationFinished();
}

void Simulator::sendTrace()
{
    if (!running) return;

    const qint64 elapsedMs = elapsedTimer.elapsed();
    const qint64 simulationEndMs = scenario == Scenario::SignificantLevelIncrease
                                      ? SignificantSimulationEndMs
                                      : SimulationEndMs;
    if (elapsedMs > simulationEndMs) {
        stopSimulation();
        return;
    }

    emit traceReady(createTrace(elapsedMs));
}

QVector<qint16> Simulator::createTrace(qint64 elapsedMs) const
{
    QVector<qint16> data;
    data.reserve(sampleCount);

    const int signalIncreaseRaw = signalIncrease(elapsedMs);
    QRandomGenerator *generator = QRandomGenerator::global();

    for (int i = 0; i < sampleCount; ++i) {
        const double frequency = StartFrequencyHz + i * ResolutionHz;
        int level = generator->bounded(NoiseMinRaw, NoiseMaxRaw + 1);

        if (signalIncreaseRaw > 0 && frequency >= SignalStartFrequencyHz && frequency <= SignalStopFrequencyHz) {
            level += signalIncreaseRaw;
        }

        data.append(static_cast<qint16>(level));
    }

    return data;
}

int Simulator::signalIncrease(qint64 elapsedMs) const
{
    if (scenario == Scenario::SignificantLevelIncrease) {
        return qRound(SignalIncreaseRaw * significantLevelEnvelope(elapsedMs));
    }

    return qRound(SignalIncreaseRaw * traceAnalyzerAlarmEnvelope(elapsedMs));
}

double Simulator::traceAnalyzerAlarmEnvelope(qint64 elapsedMs) const
{
    const qint64 rampUpEndMs = SignalStartMs + SignalRampMs;
    const qint64 holdEndMs = rampUpEndMs + SignalHoldMs;
    const qint64 rampDownEndMs = holdEndMs + SignalRampMs;

    if (elapsedMs < SignalStartMs || elapsedMs > rampDownEndMs) return 0.0;
    if (elapsedMs < rampUpEndMs) return static_cast<double>(elapsedMs - SignalStartMs) / SignalRampMs;
    if (elapsedMs <= holdEndMs) return 1.0;

    return 1.0 - static_cast<double>(elapsedMs - holdEndMs) / SignalRampMs;
}

double Simulator::significantLevelEnvelope(qint64 elapsedMs) const
{
    const qint64 rampUpEndMs = SignalStartMs + SignalRampMs;
    const qint64 firstHoldEndMs = rampUpEndMs + SignificantSignalHoldMs;
    const qint64 secondHoldEndMs = firstHoldEndMs + SignificantIncreaseHoldMs;
    const qint64 rampDownEndMs = secondHoldEndMs + SignalRampMs;

    if (elapsedMs < SignalStartMs || elapsedMs > rampDownEndMs) return 0.0;
    if (elapsedMs < rampUpEndMs) return static_cast<double>(elapsedMs - SignalStartMs) / SignalRampMs;
    if (elapsedMs < firstHoldEndMs) return 1.0;
    if (elapsedMs <= secondHoldEndMs) {
        return static_cast<double>(SignalIncreaseRaw + SignificantLevelIncreaseRaw) / SignalIncreaseRaw;
    }

    const double maxEnvelope = static_cast<double>(SignalIncreaseRaw + SignificantLevelIncreaseRaw) / SignalIncreaseRaw;
    return maxEnvelope * (1.0 - static_cast<double>(elapsedMs - secondHoldEndMs) / SignalRampMs);
}
