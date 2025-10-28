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
    explicit Mqtt(QSharedPointer<Config>);

public slots:
    void updSettings();

signals:
    void newData(const QString, double);
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
    QTimer *m_keepaliveTimer = new QTimer;
    QTimer *m_webswitchTimer = new QTimer;
    QTimer *m_connectionTimer = new QTimer;
    QTimer *m_receivedDataTimer = new QTimer;
    QList<double> m_subValues;
    SITESTATUS m_siteStatus = UNKNOWN;
    QSharedPointer<Config> config;

    // config cache
    bool enabled = false;
    QString keepaliveTopic;
    QString webswitchAddress;
};
#endif // MQTT_H
