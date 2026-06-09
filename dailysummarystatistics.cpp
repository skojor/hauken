#include "dailysummarystatistics.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTextStream>
#include <QTime>
#include <QTimeZone>
#include <QtGlobal>

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

void DailySummaryStatistics::recordSignalState(const QDateTime &dt, bool signalAboveThreshold, bool l1Interference, qint64 minTriggerMsecs)
{
    if (!m_periodStart.isValid()) {
        m_periodStart = dt;
    }

    m_minTriggerMsecs = qMax<qint64>(0, minTriggerMsecs);
    recordThresholdState(m_signalAboveThreshold, dt, signalAboveThreshold);
    recordThresholdState(m_l1Interference, dt, l1Interference);
    m_lastSignalStateUpdate = dt;
}

void DailySummaryStatistics::recordThresholdState(ThresholdState &state, const QDateTime &dt, bool active)
{
    if (m_lastSignalStateUpdate.isValid() && state.lastActive) {
        const qint64 elapsed = m_lastSignalStateUpdate.msecsTo(dt);
        if (elapsed > 0) {
            if (m_minTriggerMsecs == 0) {
                state.activeMsecs += elapsed;
                state.confirmedActive = true;
            }
            else if (state.confirmedActive) {
                state.activeMsecs += elapsed;
            }
            else {
                if (!state.candidateStart.isValid()) {
                    state.candidateStart = m_lastSignalStateUpdate;
                }

                const qint64 activeDuration = state.candidateStart.msecsTo(dt);
                if (activeDuration >= m_minTriggerMsecs) {
                    state.activeMsecs += activeDuration;
                    state.confirmedActive = true;
                }
            }
        }
    }

    if (active) {
        if (!state.lastActive) {
            state.candidateStart = dt;
            state.confirmedActive = (m_minTriggerMsecs == 0);
        }
    }
    else {
        state.candidateStart = QDateTime();
        state.confirmedActive = false;
    }

    state.lastActive = active;
}

qint64 DailySummaryStatistics::thresholdMsecsAt(const ThresholdState &state, const QDateTime &periodEnd) const
{
    qint64 activeMsecs = state.activeMsecs;
    if (!m_lastSignalStateUpdate.isValid() || !state.lastActive) {
        return activeMsecs;
    }

    const qint64 elapsed = m_lastSignalStateUpdate.msecsTo(periodEnd);
    if (elapsed <= 0) {
        return activeMsecs;
    }

    if (m_minTriggerMsecs == 0 || state.confirmedActive) {
        return activeMsecs + elapsed;
    }

    if (state.candidateStart.isValid()) {
        const qint64 activeDuration = state.candidateStart.msecsTo(periodEnd);
        if (activeDuration >= m_minTriggerMsecs) {
            return activeMsecs + activeDuration;
        }
    }

    return activeMsecs;
}

void DailySummaryStatistics::resetThresholdState(ThresholdState &state, const QDateTime &periodEnd)
{
    const bool confirmedAtPeriodEnd = state.confirmedActive
        || m_minTriggerMsecs == 0
        || (state.candidateStart.isValid() && state.candidateStart.msecsTo(periodEnd) >= m_minTriggerMsecs);

    state.activeMsecs = 0;
    if (state.lastActive) {
        state.candidateStart = periodEnd;
        state.confirmedActive = confirmedAtPeriodEnd;
    }
    else {
        state.candidateStart = QDateTime();
        state.confirmedActive = false;
    }
}

DailySummaryStatistics::Snapshot DailySummaryStatistics::createSnapshot(const QDateTime &periodEnd) const
{
    Snapshot snapshot;
    snapshot.periodEnd = periodEnd;
    snapshot.periodStart = m_periodStart.isValid() ? m_periodStart : periodEnd.addMSecs(-MsecsPerDay);
    snapshot.incidentCount = m_incidentCount;
    snapshot.signalAboveThresholdMsecs = thresholdMsecsAt(m_signalAboveThreshold, periodEnd);
    snapshot.l1InterferenceMsecs = thresholdMsecsAt(m_l1Interference, periodEnd);

    return snapshot;
}

DailySummaryStatistics::Snapshot DailySummaryStatistics::createSnapshotAndReset(const QDateTime &periodEnd)
{
    const Snapshot snapshot = createSnapshot(periodEnd);

    m_periodStart = periodEnd;
    m_lastSignalStateUpdate = periodEnd;
    m_incidentCount = 0;
    resetThresholdState(m_signalAboveThreshold, periodEnd);
    resetThresholdState(m_l1Interference, periodEnd);
    return snapshot;
}

bool DailySummaryStatistics::saveToFile(const QString &filename, const QDateTime &persistedAt) const
{
    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const Snapshot snapshot = createSnapshot(persistedAt);
    QJsonObject object;
    object["version"] = 1;
    object["persistedAt"] = persistedAt.toString(Qt::ISODateWithMs);
    const QDateTime persistedPeriodStart(persistedAt.date(), QTime(0, 0), persistedAt.timeZone());
    object["periodStart"] = (m_periodStart.isValid() ? snapshot.periodStart : persistedPeriodStart).toString(Qt::ISODateWithMs);
    object["incidentCount"] = snapshot.incidentCount;
    object["signalAboveThresholdMsecs"] = QString::number(snapshot.signalAboveThresholdMsecs);
    object["l1InterferenceMsecs"] = QString::number(snapshot.l1InterferenceMsecs);
    object["lastSignalAboveThreshold"] = m_signalAboveThreshold.lastActive;
    object["lastL1Interference"] = m_l1Interference.lastActive;
    object["signalAboveThresholdConfirmed"] = m_signalAboveThreshold.confirmedActive
        || m_minTriggerMsecs == 0
        || (m_signalAboveThreshold.candidateStart.isValid() && m_signalAboveThreshold.candidateStart.msecsTo(persistedAt) >= m_minTriggerMsecs);
    object["l1InterferenceConfirmed"] = m_l1Interference.confirmedActive
        || m_minTriggerMsecs == 0
        || (m_l1Interference.candidateStart.isValid() && m_l1Interference.candidateStart.msecsTo(persistedAt) >= m_minTriggerMsecs);
    object["minTriggerMsecs"] = QString::number(m_minTriggerMsecs);

    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool DailySummaryStatistics::loadFromFile(const QString &filename, const QDateTime &now)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }

    const QJsonObject object = document.object();
    const int version = object.value("version").toInt();
    if (version != 1) {
        return false;
    }

    const QDateTime persistedAt = QDateTime::fromString(object.value("persistedAt").toString(), Qt::ISODateWithMs);
    const QDateTime periodStart = QDateTime::fromString(object.value("periodStart").toString(), Qt::ISODateWithMs);
    if (!persistedAt.isValid() || !periodStart.isValid() || !now.isValid()) {
        return false;
    }

    const QDateTime currentPeriodStart(now.date(), QTime(0, 0), now.timeZone());
    if (periodStart < currentPeriodStart || periodStart > now || persistedAt < periodStart || persistedAt > now) {
        return false;
    }

    bool signalOk = false;
    bool l1Ok = false;
    const qint64 signalAboveThresholdMsecs = object.value("signalAboveThresholdMsecs").toString().toLongLong(&signalOk);
    const qint64 l1InterferenceMsecs = object.value("l1InterferenceMsecs").toString().toLongLong(&l1Ok);
    const qint64 persistedPeriodMsecs = periodStart.msecsTo(persistedAt);
    if (!signalOk || !l1Ok || signalAboveThresholdMsecs < 0 || l1InterferenceMsecs < 0
        || signalAboveThresholdMsecs > persistedPeriodMsecs || l1InterferenceMsecs > persistedPeriodMsecs) {
        return false;
    }

    const int incidentCount = object.value("incidentCount").toInt(0);
    if (incidentCount < 0) {
        return false;
    }

    m_periodStart = periodStart;
    m_lastSignalStateUpdate = now;
    m_signalAboveThreshold.lastActive = object.value("lastSignalAboveThreshold").toBool(false);
    m_l1Interference.lastActive = object.value("lastL1Interference").toBool(false);
    m_signalAboveThreshold.confirmedActive = object.value("signalAboveThresholdConfirmed").toBool(m_signalAboveThreshold.lastActive);
    m_l1Interference.confirmedActive = object.value("l1InterferenceConfirmed").toBool(m_l1Interference.lastActive);
    m_signalAboveThreshold.candidateStart = m_signalAboveThreshold.lastActive ? now : QDateTime();
    m_l1Interference.candidateStart = m_l1Interference.lastActive ? now : QDateTime();
    m_incidentCount = incidentCount;
    m_signalAboveThreshold.activeMsecs = signalAboveThresholdMsecs;
    m_l1Interference.activeMsecs = l1InterferenceMsecs;

    bool minTriggerOk = false;
    const qint64 minTriggerMsecs = object.value("minTriggerMsecs").toString().toLongLong(&minTriggerOk);
    m_minTriggerMsecs = (minTriggerOk && minTriggerMsecs > 0) ? minTriggerMsecs : 0;

    return true;
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
       //<< "<tr><td>Total time above signal threshold</td><td>" << formatDuration(snapshot.signalAboveThresholdMsecs) << "</td></tr>"
       << "<tr><td>L1 interference (1575.42 MHz center frequency)</td><td>"
       << formatDuration(snapshot.l1InterferenceMsecs) << " ("
       << QString::number(percentageOfPeriod(snapshot.l1InterferenceMsecs, snapshot), 'f', 2) << " %)</td></tr>"
       << "</table>"
       << "</body></html>";

    return html;
}

QString DailySummaryStatistics::toLogLine(const Snapshot &snapshot, const QString &location, const QString &instrument) const
{
    return QString("Daily summary of incidents and L1 interference. Period: %1 - %2. Location: %3, instrument: %4. Registered incidents: %5. " \
                   //"Total time above signal threshold: %6. "
                   "L1 interference (1575.42 MHz center frequency): %7 (%8 %).")
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

    return QString("%1:%2:%3")
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
