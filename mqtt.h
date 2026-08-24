#ifndef MQTT_H
#define MQTT_H

#include <QWidget>
#include <QObject>
#include <QtMqtt/QtMqtt>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "config.h"
#include "typedefs.h"
#include "networkrequestsbase.h"

enum SITESTATUS {
    UNKNOWN,
    STOP,
    START,
    RUNNING,
    NORUNNING
};

#define MQTT_CONN_TIMEOUT_MS    30000
#define MQTT_DATATRANSFER_TIMEOUT_MS 120000
#define WEBSWITCH_TEMP_INTERVAL_MS 60000

class Mqtt : public NetworkRequestsBase
{
    Q_OBJECT
public:
    explicit Mqtt(QSharedPointer<Config>, const QString &profileId);
    QString profileId() const { return m_profileId; }

public slots:
    void updSettings();
    void setAdditionalSubscriptions(const QStringList &topics);

signals:
    void newData(const QString, double);
    void rawMessage(const QString &profileId, const QString &topic, const QByteArray &payload);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);
    void triggerRecording();
    void endRecording();

private slots:
    void stateChanged(QMqttClient::ClientState state);
    void checkConnection();
    void error(QMqttClient::ClientError error);
    void msgSent(qint32 id);
    void subscribe();
    void msgReceived(const QByteArray& msg, const QMqttTopicName& topic);
    void reconnect();
    void startKeepaliveTimer();
    void stopKeepaliveTimer();
    void parseMqtt(const QString& topic, const QByteArray& msg);
    void reqWebswitchData();
    void networkAccessManagerReplyHandler(QNetworkReply *reply);

private:
    QMqttClient m_mqttClient;
    QTimer *m_keepaliveTimer = nullptr;
    QTimer *m_webswitchTimer = nullptr;
    QTimer *m_connectionTimer = nullptr;
    QTimer *m_receivedDataTimer = nullptr;
    QList<double> m_subValues;
    SITESTATUS m_siteStatus = UNKNOWN;
    QSharedPointer<Config> config;
    QString m_profileId;

    // config cache
    bool enabled = false;
    QString keepaliveTopic;
    QStringList subscriptionTopics;
    QStringList additionalSubscriptions;
    QString webswitchAddress;
    bool reconnectPending = false;
};
#endif // MQTT_H
