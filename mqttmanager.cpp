#include "mqttmanager.h"

MqttManager::MqttManager(QSharedPointer<Config> c, QObject *parent)
    : QObject(parent), config(c)
{
}

void MqttManager::updSettings()
{
    QSet<QString> enabledProfileIds;
    for (const MqttBrokerProfile &profile : config->getMqttBrokerProfiles()) {
        if (!profile.enabled) continue;

        enabledProfileIds.insert(profile.id);
        if (!workers.contains(profile.id)) addWorker(profile);
    }

    const QString ppsProfileId = config->getGnssPpsBrokerProfileId();
    QStringList ppsTopics;
    if (config->getGnssPpsDisplayEnabled()) {
        const MqttBrokerProfile selectedProfile = config->getMqttBrokerProfile(ppsProfileId);
        if (selectedProfile.id.isEmpty())
            emit ppsError(tr("The selected PPS MQTT broker profile does not exist"));
        else if (!selectedProfile.enabled)
            emit ppsError(tr("The selected PPS MQTT broker profile is disabled"));
        ppsTopics = {config->getGnssPpsStatusTopic(), config->getGnssPpsAvailabilityTopic()};
    }
    for (auto it = workers.cbegin(); it != workers.cend(); ++it) {
        it.value()->setAdditionalSubscriptions(it.key() == ppsProfileId ? ppsTopics : QStringList());
        it.value()->updSettings();
    }

    for (auto it = workers.begin(); it != workers.end();) {
        if (enabledProfileIds.contains(it.key())) {
            ++it;
            continue;
        }

        Mqtt *worker = it.value();
        it = workers.erase(it);
        worker->deleteLater();
    }
}

void MqttManager::addWorker(const MqttBrokerProfile &profile)
{
    Mqtt *worker = new Mqtt(config, profile.id);
    worker->setParent(this);
    workers.insert(profile.id, worker);

    connect(worker, &Mqtt::newData, this, &MqttManager::newData);
    connect(worker, &Mqtt::rawMessage, this, &MqttManager::rawMessage);
    connect(worker, &Mqtt::rawMessage, this, &MqttManager::handleRawMessage);
    connect(worker, &Mqtt::toIncidentLog, this, &MqttManager::toIncidentLog);
    connect(worker, &Mqtt::triggerRecording, this, &MqttManager::triggerRecording);
    connect(worker, &Mqtt::endRecording, this, &MqttManager::endRecording);
}

void MqttManager::handleRawMessage(const QString &profileId,
                                   const QString &topic,
                                   const QByteArray &payload)
{
    if (!config->getGnssPpsDisplayEnabled() || profileId != config->getGnssPpsBrokerProfileId()) return;

    QString error;
    if (topic == config->getGnssPpsStatusTopic()) {
        PpsData parsed;
        if (!PpsStatusParser::parseStatus(payload, parsed, error)) {
            emit ppsError(error);
            return;
        }
        lastPpsData = parsed;
        emit ppsDataUpdated(lastPpsData);
    }
    else if (topic == config->getGnssPpsAvailabilityTopic()) {
        bool online = false;
        if (!PpsStatusParser::parseAvailability(payload, online, error)) {
            emit ppsError(error);
            return;
        }
        emit ppsAvailabilityChanged(online);
    }
}