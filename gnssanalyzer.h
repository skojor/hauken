#ifndef GNSSANALYZER_H
#define GNSSANALYZER_H

#include "config.h"
#include "typedefs.h"

class GnssAnalyzer : public Config
{
    Q_OBJECT
public:
    GnssAnalyzer(QObject *parent = nullptr);

public slots:
    void getData(GnssData &data);
    void updSettings();

signals:
    void alarm();
    void toIncidentLog(QString);

private slots:

private:
    // config cache below here
    double cnoLimit, agcLimit;
    double posOffsetLimit, altOffsetLimit, timeOffsetLimit;
};

#endif // GNSSANALYZER_H
