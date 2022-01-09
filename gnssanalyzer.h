#ifndef GNSSANALYZER_H
#define GNSSANALYZER_H

#include <QTimer>
#include <QMutex>
#include <math.h>
#include "config.h"
#include "typedefs.h"

class GnssAnalyzer : public Config
{
    Q_OBJECT
public:
    GnssAnalyzer(QObject *parent = nullptr, int i = 0);

public slots:
    void getData(GnssData &data);
    void updSettings();

signals:
    void alarm();
    void toIncidentLog(QString);
    void displayGnssData(QString, int, bool);

private slots:
    void calcAvgs(GnssData &data);
    void analyze(GnssData &data);
    double arcInRadians(GnssData &data);
    double distanceInMeters(GnssData &data);
    void updDisplay();
    void checkPosValid(GnssData &data);
    void checkPosOffset(GnssData &data);
    void checkAltOffset(GnssData &data);
    void checkTimeOffset(GnssData &data);
    void checkCnoOffset(GnssData &data);
    void checkAgcOffset(GnssData &data);


private:
    QMutex mutex;
    GnssData gnssData;
    QTimer *updDisplayTimer = new QTimer;
    bool posOffsetTriggered = false;
    bool altOffsetTriggered = false;
    bool timeOffsetTriggered = false;
    bool cnoLimitTriggered = false;
    bool agcLimitTriggered = false;
    bool posInvalidTriggered = true; // don't trigger any recording on inital startup

    // config cache below here
    double cnoLimit, agcLimit;
    double posOffsetLimit, altOffsetLimit, timeOffsetLimit;
    double ownLatitude, ownLongitude, ownAltitude;
    bool checkAgc, logToFile;

    // pos. calculation constants here
    const double EARTH_RADIUS_IN_METERS = 6372797.560856;
    const double DEG_TO_RAD = 0.017453292519943295769236907684886;

};

#endif // GNSSANALYZER_H
