#ifndef GNSSMANAGER_H
#define GNSSMANAGER_H

#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include "config.h"
#include "gnssanalyzer.h"
#include "gnssdevice.h"
#include "typedefs.h"

class MeasurementDevice;

/*
 * Owns a dynamic set of GnssDevice/GnssAnalyzer pairs, one per configured
 * GnssReceiverProfile, mirroring how MqttManager owns a dynamic set of Mqtt workers.
 * The singleton "instrument" profile has no GnssDevice (its data comes from
 * MeasurementDevice instead), only a GnssAnalyzer.
 */
class GnssManager : public QObject
{
    Q_OBJECT
public:
    explicit GnssManager(QSharedPointer<Config> config, MeasurementDevice *measurementDevice, QObject *parent = nullptr);

    GnssData dataFor(const QString &profileId) const;
    bool isValid(const QString &profileId) const;

public slots:
    void updSettings();
    void traceIncidentStarted();
    void traceIncidentEnded();
    void markIncidenceStarted(); // any GNSS or RF incident marks the start point on all device backlogs
    void saveAllBacklogs();
    void setInstrumentGnssConnected(bool connected);
    void reqPositionForProfile(const QString &profileId);

signals:
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void displayGnssData(QString html, QString profileId, bool posValid);
    void alarm();
    void visualAlarm();
    void alarmEnded();
    void positionUpdate(bool valid, double lat, double lng);
    void sendGnssPlotFilename(const QString &profileId, const QString &profileName, QString filename);
    void gnssDisabled(const QString &profileId);

private:
    void addReceiver(const GnssReceiverProfile &profile);
    void removeReceiver(const QString &profileId);

    QSharedPointer<Config> config;
    MeasurementDevice *measurementDevice;
    QMap<QString, GnssDevice *> devices; // Serial-type profiles only
    QMap<QString, GnssAnalyzer *> analyzers; // Serial and Instrument profiles
};

#endif // GNSSMANAGER_H
