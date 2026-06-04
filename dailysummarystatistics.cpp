#include "dailysummarystatistics.h"

#include <QTextStream>

namespace {
constexpr qint64 MsecsPerDay = 24LL * 60LL * 60LL * 1000LL;
}

void DailySummaryStatistics::recordIncident(const QDateTime &dt)
{
    if (!m_periodStart.isValid()) {
        m_periodStart = dt;
    }

    m_incidentCount++;
}

void DailySummaryStatistics::recordSignalState(const QDateTime &dt, bool signalAboveThreshold, bool l1Interference)
{
    if (!m_periodStart.isValid()) {
        m_periodStart = dt;
    }

    if (m_lastSignalStateUpdate.isValid()) {
        const qint64 elapsed = m_lastSignalStateUpdate.msecsTo(dt);
        if (elapsed > 0) {
            if (m_lastSignalAboveThreshold) {
                m_signalAboveThresholdMsecs += elapsed;
            }
            if (m_lastL1Interference) {
                m_l1InterferenceMsecs += elapsed;
            }
        }
    }

    m_lastSignalStateUpdate = dt;
    m_lastSignalAboveThreshold = signalAboveThreshold;
    m_lastL1Interference = l1Interference;
}

DailySummaryStatistics::Snapshot DailySummaryStatistics::createSnapshotAndReset(const QDateTime &periodEnd)
{
    recordSignalState(periodEnd, m_lastSignalAboveThreshold, m_lastL1Interference);

    Snapshot snapshot;
    snapshot.periodEnd = periodEnd;
    snapshot.periodStart = m_periodStart.isValid() ? m_periodStart : periodEnd.addMSecs(-MsecsPerDay);
    snapshot.incidentCount = m_incidentCount;
    snapshot.signalAboveThresholdMsecs = m_signalAboveThresholdMsecs;
    snapshot.l1InterferenceMsecs = m_l1InterferenceMsecs;

    m_periodStart = periodEnd;
    m_lastSignalStateUpdate = periodEnd;
    m_incidentCount = 0;
    m_signalAboveThresholdMsecs = 0;
    m_l1InterferenceMsecs = 0;
    return snapshot;
}

QString DailySummaryStatistics::toHtmlReport(const Snapshot &snapshot, const QString &location, const QString &instrument) const
{
    QString html;
    QTextStream ts(&html);

    ts << "<html><body>"
       << "<h2>Daily summary of incidents and L1 interference</h2>"
       << "<p>Period: " << snapshot.periodStart.toString("dd.MM.yy hh:mm:ss")
       << " - " << snapshot.periodEnd.toString("dd.MM.yy hh:mm:ss") << "</p>"
       << "<p>Location: " << location << ", instrument: " << (instrument.isEmpty() ? "unknown" : instrument) << ".</p>"
       << "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"
       << "<tr><th align=\"left\">Metric</th><th align=\"left\">Value</th></tr>"
       << "<tr><td>Registered incidents</td><td>" << snapshot.incidentCount << "</td></tr>"
       << "<tr><td>Total time above signal threshold</td><td>" << formatDuration(snapshot.signalAboveThresholdMsecs) << "</td></tr>"
       << "<tr><td>L1 interference (1575.42 MHz center frequency)</td><td>"
       << formatDuration(snapshot.l1InterferenceMsecs) << " ("
       << QString::number(percentageOfPeriod(snapshot.l1InterferenceMsecs, snapshot), 'f', 2) << " %)</td></tr>"
       << "</table>"
       << "</body></html>";

    return html;
}

QString DailySummaryStatistics::toLogLine(const Snapshot &snapshot, const QString &location, const QString &instrument) const
{
    return QString("Daily summary of incidents and L1 interference. Period: %1 - %2. Location: %3, instrument: %4. Registered incidents: %5. Total time above signal threshold: %6. L1 interference (1575.42 MHz center frequency): %7 (%8 %).")
        .arg(snapshot.periodStart.toString("dd.MM.yy hh:mm:ss"))
        .arg(snapshot.periodEnd.toString("dd.MM.yy hh:mm:ss"))
        .arg(location)
        .arg(instrument.isEmpty() ? "unknown" : instrument)
        .arg(snapshot.incidentCount)
        .arg(formatDuration(snapshot.signalAboveThresholdMsecs))
        .arg(formatDuration(snapshot.l1InterferenceMsecs))
        .arg(QString::number(percentageOfPeriod(snapshot.l1InterferenceMsecs, snapshot), 'f', 2));
}

QString DailySummaryStatistics::formatDuration(qint64 milliseconds) const
{
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    return QString("%1 seconds / %2 minutes / %3 hours (%4:%5:%6)")
        .arg(totalSeconds)
        .arg(QString::number(totalSeconds / 60.0, 'f', 2))
        .arg(QString::number(totalSeconds / 3600.0, 'f', 2))
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

double DailySummaryStatistics::percentageOfPeriod(qint64 milliseconds, const Snapshot &snapshot) const
{
    qint64 periodMsecs = snapshot.periodStart.msecsTo(snapshot.periodEnd);
    if (periodMsecs <= 0) {
        periodMsecs = MsecsPerDay;
    }

    return (static_cast<double>(milliseconds) / static_cast<double>(periodMsecs)) * 100.0;
}
