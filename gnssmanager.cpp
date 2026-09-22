#include "gnssmanager.h"
#include "measurementdevice.h"
#include <QSet>

GnssManager::GnssManager(QSharedPointer<Config> c, MeasurementDevice *m, QObject *parent)
    : QObject(parent), config(c), measurementDevice(m)
{
}

GnssData GnssManager::dataFor(const QString &profileId) const
{
    if (profileId == "instrument") return measurementDevice->sendGnssData();
    if (devices.contains(profileId)) return devices.value(profileId)->sendGnssData();
    return GnssData();
}

bool GnssManager::isValid(const QString &profileId) const
{
    if (profileId == "instrument") return measurementDevice->isPositionValid();
    if (devices.contains(profileId)) return devices.value(profileId)->isValid();
    return false;
}

void GnssManager::reqPositionForProfile(const QString &profileId)
{
    if (profileId == "instrument") measurementDevice->reqPosition();
    else if (devices.contains(profileId)) devices.value(profileId)->reqPosition();
}

void GnssManager::updSettings()
{
    QSet<QString> enabledProfileIds;
    for (const GnssReceiverProfile &profile : config->getGnssReceiverProfiles()) {
        if (!profile.enabled) continue;
        enabledProfileIds.insert(profile.id);
        if (!analyzers.contains(profile.id)) addReceiver(profile);
    }

    QStringList idsToRemove;
    for (auto it = analyzers.cbegin(); it != analyzers.cend(); ++it) {
        if (!enabledProfileIds.contains(it.key())) idsToRemove.append(it.key());
    }
    for (const QString &id : idsToRemove) removeReceiver(id);
    for (auto it = devices.cbegin(); it != devices.cend(); ++it) it.value()->updSettings();
    for (auto it = analyzers.cbegin(); it != analyzers.cend(); ++it) it.value()->updSettings();
}

void GnssManager::addReceiver(const GnssReceiverProfile &profile)
{
    GnssAnalyzer *analyzer = new GnssAnalyzer(config, profile.id);
    analyzer->setParent(this);
    analyzers.insert(profile.id, analyzer);

    connect(analyzer, &GnssAnalyzer::toIncidentLog, this, &GnssManager::toIncidentLog);
    connect(analyzer, &GnssAnalyzer::displayGnssData, this, &GnssManager::displayGnssData);
    connect(analyzer, &GnssAnalyzer::alarm, this, &GnssManager::alarm);
    connect(analyzer, &GnssAnalyzer::alarm, this, &GnssManager::markIncidenceStarted);
    connect(analyzer, &GnssAnalyzer::visualAlarm, this, &GnssManager::visualAlarm);
    connect(analyzer, &GnssAnalyzer::alarmEnded, this, &GnssManager::alarmEnded);

    if (profile.type == GnssReceiverProfile::Type::Instrument) {
        connect(measurementDevice, &MeasurementDevice::updGnssData, analyzer, &GnssAnalyzer::getData);
        return;
    }

    GnssDevice *device = new GnssDevice(config, profile.id);
    device->setParent(this);
    devices.insert(profile.id, device);

    connect(device, &GnssDevice::analyzeThisData, analyzer, &GnssAnalyzer::getData);
    connect(device, &GnssDevice::toIncidentLog, this, &GnssManager::toIncidentLog);
    connect(device, &GnssDevice::positionUpdate, this, &GnssManager::positionUpdate);
    connect(device, &GnssDevice::sendGnssPlotFilename, this, [this, profile](const QString &filename) {
        emit sendGnssPlotFilename(profile.id, config->getGnssReceiverProfile(profile.id).name, filename);
    });
    connect(device, &GnssDevice::gnssDisabled, this, [this, analyzer, profile] {
        analyzer->updStateInstrumentGnss(false);
        emit gnssDisabled(profile.id);
    });
}

void GnssManager::removeReceiver(const QString &profileId)
{
    if (GnssAnalyzer *analyzer = analyzers.take(profileId)) analyzer->deleteLater();
    if (GnssDevice *device = devices.take(profileId)) device->deleteLater();
}

void GnssManager::traceIncidentStarted()
{
    for (auto it = analyzers.cbegin(); it != analyzers.cend(); ++it) it.value()->traceIncidentStarted();
}

void GnssManager::traceIncidentEnded()
{
    for (auto it = analyzers.cbegin(); it != analyzers.cend(); ++it) it.value()->traceIncidentEnded();
}

void GnssManager::markIncidenceStarted()
{
    for (auto it = devices.cbegin(); it != devices.cend(); ++it) it.value()->setIncidenceStartedDateTime();
}

void GnssManager::saveAllBacklogs()
{
    for (auto it = devices.cbegin(); it != devices.cend(); ++it) it.value()->saveBacklog();
}

void GnssManager::setInstrumentGnssConnected(bool connected)
{
    if (GnssAnalyzer *analyzer = analyzers.value("instrument")) analyzer->updStateInstrumentGnss(connected);
}
