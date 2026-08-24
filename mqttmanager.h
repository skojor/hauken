#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <QHash>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include "config.h"
#include "mqtt.h"
#include "ppsstatusparser.h"

class MqttManager : public QObject
{
    Q_OBJECT
public:
    explicit MqttManager(QSharedPointer<Config> config, QObject *parent = nullptr);

public slots:
    void updSettings();

signals:
    void newData(const QString name, double value);
    void rawMessage(const QString &profileId, const QString &topic, const QByteArray &payload);
    void toIncidentLog(const NOTIFY::TYPE type, const QString title, const QString message);
    void triggerRecording();
    void endRecording();
    void ppsDataUpdated(const PpsData &data);
    void ppsAvailabilityChanged(bool online);
    void ppsError(const QString &error);

private:
    void addWorker(const MqttBrokerProfile &profile);
    void handleRawMessage(const QString &profileId, const QString &topic, const QByteArray &payload);

    QSharedPointer<Config> config;
    QHash<QString, Mqtt *> workers;
    PpsData lastPpsData;
};

#endif // MQTTMANAGER_H