#ifndef DAILYSUMMARYSTATISTICS_H
#define DAILYSUMMARYSTATISTICS_H

#include <QDateTime>
#include <QString>

class DailySummaryStatistics
{
public:
    struct Snapshot
    {
        QDateTime periodStart;
        QDateTime periodEnd;
        int incidentCount = 0;
        qint64 signalAboveThresholdMsecs = 0;
        qint64 l1InterferenceMsecs = 0;
    };

    void recordIncident(const QDateTime &dt);
    void recordSignalState(const QDateTime &dt, bool signalAboveThreshold, bool l1Interference, qint64 minTriggerMsecs);
    Snapshot createSnapshot(const QDateTime &periodEnd) const;
    Snapshot createSnapshotAndReset(const QDateTime &periodEnd);
    QString toHtmlReport(const Snapshot &snapshot, const QString &location, const QString &instrument) const;
    QString toLogLine(const Snapshot &snapshot, const QString &location, const QString &instrument) const;
    bool saveToFile(const QString &filename, const QDateTime &persistedAt) const;
    bool loadFromFile(const QString &filename, const QDateTime &now);

private:
    struct ThresholdState
    {
        QDateTime candidateStart;
        bool lastActive = false;
        bool confirmedActive = false;
        qint64 activeMsecs = 0;
    };

    void recordThresholdState(ThresholdState &state, const QDateTime &dt, bool active);
    qint64 thresholdMsecsAt(const ThresholdState &state, const QDateTime &periodEnd) const;
    void resetThresholdState(ThresholdState &state, const QDateTime &periodEnd);
    QString formatDuration(qint64 milliseconds) const;
    QString formatPercentage(qint64 milliseconds, const Snapshot &snapshot) const;
    double percentageOfPeriod(qint64 milliseconds, const Snapshot &snapshot) const;

    QDateTime m_periodStart;
    QDateTime m_lastSignalStateUpdate;
    ThresholdState m_signalAboveThreshold;
    ThresholdState m_l1Interference;
    int m_incidentCount = 0;
    qint64 m_minTriggerMsecs = 0;
};

#endif // DAILYSUMMARYSTATISTICS_H
