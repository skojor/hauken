#ifndef GNSSANALYZER_H
#define GNSSANALYZER_H

#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <math.h>
#include <chrono>
#include "config.h"
#include "typedefs.h"

#define RUNTESTS false

class GnssIncidentState
{
public:
    using Clock = std::chrono::steady_clock;
    void setTraceIncidentActive(bool active, Clock::time_point now = Clock::now())
    {
        setActive(traceIncidentActive, active, now);
    }
    void setGnssIncidentActive(bool active, Clock::time_point now = Clock::now())
    {
        setActive(gnssIncidentActive, active, now);
    }
    bool shouldPauseSignalAverages(Clock::time_point now = Clock::now()) const
    {
        return traceIncidentActive || gnssIncidentActive
               || (graceActive && now < graceEndsAt);
    }

private:
    void setActive(bool &state, bool active, Clock::time_point now)
    {
        if (state == active) return;
        state = active;
        if (active) graceActive = false;
        else if (!traceIncidentActive && !gnssIncidentActive) {
            graceEndsAt = now + std::chrono::seconds(60);
            graceActive = true;
        }
    }

    bool traceIncidentActive = false;
    bool gnssIncidentActive = false;
    bool graceActive = false;
    Clock::time_point graceEndsAt;
};

class GnssAnalyzer : public QObject
{
    Q_OBJECT
public:
    GnssAnalyzer(QSharedPointer<Config>, const QString &profileId,
                 QSharedPointer<GnssIncidentState> incidentState = {});

public slots:
    void getData(GnssData &data);
    void updSettings();
    void updStateInstrumentGnss(bool b) { stateInstrumentGnss = b; if (!b) updDisplayTimer->stop(); }
    void traceIncidentStarted();
    void traceIncidentEnded();

signals:
    void alarm();
    void visualAlarm();
    void alarmEnded();
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void displayGnssData(QString, QString, bool);

private slots:
    void calcAvgs(GnssData &data);
    void analyze(GnssData &data);
    double arcInRadians(GnssData &data);
    double distanceInMeters(GnssData &data);
    void updDisplay();
    void checkPosOffset(GnssData &data);
    void checkAltOffset(GnssData &data);
    void checkTimeOffset(GnssData &data);
    void checkCnoOffset(GnssData &data);
    void checkAgcOffset(GnssData &data);
    void checkAgcBlockOffsets(GnssData &data);
    void checkJammingIndicator(GnssData &data);
    void tests();
    bool shouldPauseSignalAverages();

private:
    QMutex mutex;
    GnssData gnssData;
    QTimer *updDisplayTimer = new QTimer;
    bool posOffsetTriggered = false;
    bool altOffsetTriggered = false;
    bool timeOffsetTriggered = false;
    bool cnoLimitTriggered = false;
    bool agcLimitTriggered = false;
    QMap<int, bool> agcBlockLimitTriggered;
    QMap<int, QElapsedTimer> agcBlockTimers;
    QMap<int, QElapsedTimer> agcBlockFilterTimers;
    bool jamIndTriggered = false;

    // config cache below here
    double cnoLimit, agcLimit;
    double posOffsetLimit, altOffsetLimit, timeOffsetLimit;
    double ownLatitude, ownLongitude, ownAltitude;
    bool checkAgc, logToFile;
    bool isInstrumentType = false;

    // pos. calculation constants here
    const double EARTH_RADIUS_IN_METERS = 6372797.560856;
    const double DEG_TO_RAD = 0.017453292519943295769236907684886;

    bool stateInstrumentGnss = true; // goes false if instrumentGnss is used and stream disappears
    QSharedPointer<GnssIncidentState> incidentState;

    const int jammingIndicatorTriggerValue = 98; // effectively disabled for now TBD what to use this for
    QSharedPointer<Config> config;

    bool anyOffsetTriggered() const;
    void emitAlarmEndedIfNoOffsetTriggered();
    void reportRecordingTrigger(const QString &reason);

    QElapsedTimer posOffsetTimer, altOffsetTimer, cnoTimer, agcTimer, timeOffsetTimer, jamIndTimer;
    QElapsedTimer posOffsetFilterTimer, altOffsetFilterTimer, cnoFilterTimer, agcFilterTimer, timeOffsetFilterTimer, jamIndFilterTimer;
    GnssData testData;
};

#endif // GNSSANALYZER_H
