#ifndef GNSSANALYZER_H
#define GNSSANALYZER_H

#include <QTimer>
#include <QMutex>
#include <math.h>
#include "config.h"
#include "typedefs.h"

#define RUNTESTS false

class GnssAnalyzer : public QObject
{
    Q_OBJECT
public:
    GnssAnalyzer(QSharedPointer<Config>, int id);

public slots:
    void getData(GnssData &data);
    void updSettings();
    void updStateInstrumentGnss(bool b) { stateInstrumentGnss = b; if (!b) updDisplayTimer->stop(); }

signals:
    void alarm();
    void visualAlarm();
    void alarmEnded();
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void displayGnssData(QString, int, bool);

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
    void checkJammingIndicator(GnssData &data);
    void tests();

private:
    QMutex mutex;
    GnssData gnssData;
    QTimer *updDisplayTimer = new QTimer;
    bool posOffsetTriggered = false;
    bool altOffsetTriggered = false;
    bool timeOffsetTriggered = false;
    bool cnoLimitTriggered = false;
    bool agcLimitTriggered = false;
    bool jamIndTriggered = false;

    // config cache below here
    double cnoLimit, agcLimit;
    double posOffsetLimit, altOffsetLimit, timeOffsetLimit;
    double ownLatitude, ownLongitude, ownAltitude;
    bool checkAgc, logToFile;

    // pos. calculation constants here
    const double EARTH_RADIUS_IN_METERS = 6372797.560856;
    const double DEG_TO_RAD = 0.017453292519943295769236907684886;

    bool stateInstrumentGnss = true; // goes false if instrumentGnss is used and stream disappears

    const int jammingIndicatorTriggerValue = 98; // effectively disabled for now TBD what to use this for
    QSharedPointer<Config> config;

    QElapsedTimer posOffsetTimer, altOffsetTimer, cnoTimer, agcTimer, timeOffsetTimer, jamIndTimer;
    GnssData testData;
};

#endif // GNSSANALYZER_H
