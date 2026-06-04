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
    void recordSignalState(const QDateTime &dt, bool signalAboveThreshold, bool l1Interference);
    Snapshot createSnapshot(const QDateTime &periodEnd) const;
    Snapshot createSnapshotAndReset(const QDateTime &periodEnd);
    QString toHtmlReport(const Snapshot &snapshot, const QString &location, const QString &instrument) const;
    QString toLogLine(const Snapshot &snapshot, const QString &location, const QString &instrument) const;

private:
    QString formatDuration(qint64 milliseconds) const;
    double percentageOfPeriod(qint64 milliseconds, const Snapshot &snapshot) const;

    QDateTime m_periodStart;
    QDateTime m_lastSignalStateUpdate;
    bool m_lastSignalAboveThreshold = false;
    bool m_lastL1Interference = false;
    int m_incidentCount = 0;
    qint64 m_signalAboveThresholdMsecs = 0;
    qint64 m_l1InterferenceMsecs = 0;
};

#endif // DAILYSUMMARYSTATISTICS_H
