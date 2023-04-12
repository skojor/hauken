#include "mqtt.h"

Mqtt::Mqtt(QObject *parent)
{
    this->setParent(parent);

    connect(&mqttClient, &QMqttClient::stateChanged, this, &Mqtt::stateChanged);
    connect(&mqttClient, &QMqttClient::errorChanged, this, &Mqtt::error);
    connect(&mqttClient, &QMqttClient::messageSent, this, &Mqtt::msgSent);
    connect(&mqttClient, &QMqttClient::messageReceived, this, &Mqtt::msgReceived);
    connect(keepaliveTimer, &QTimer::timeout, this, [this] {
        mqttClient.publish(getMqttKeepaliveTopic(), QByteArray());
        qDebug() << "Sending MQTT keepalive";
    });
}

void Mqtt::stateChanged(QMqttClient::ClientState state)
{
    if (state == QMqttClient::ClientState::Disconnected) {
        qDebug() << "MQTT disconnected";
    }
    else if (state == QMqttClient::ClientState::Connecting) {
        qDebug() << "MQTT connecting";
    }
    else if (state == QMqttClient::ClientState::Connected) {
        qDebug() << "MQTT connected to broker, requesting subscriptions";
        subscribe();
        if (!getMqttKeepaliveTopic().isEmpty()) mqttClient.publish(getMqttKeepaliveTopic(), QByteArray());

    }
}

void Mqtt::error(QMqttClient::ClientError error)
{
    qDebug() << "MQTT error:" << error;
}

void Mqtt::msgSent(qint32 id)
{
    qDebug() << "MQTT message sent, id" << id;
}

void Mqtt::subscribe()
{
    for (auto &val : getMqttSubTopics()) {
        mqttClient.subscribe(val);
    }
}

void Mqtt::msgReceived(const QByteArray &msg, const QMqttTopicName &topic)
{
    QStringList subs = getMqttSubTopics();
    QStringList subNames = getMqttSubNames();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonValue value = jsonObject.value("value");
    //qDebug() << "MQTT received" << topic << msg;
    if (subValues.size() != subs.size()) {
        subValues.clear();
        for (int i=0; i<subs.size();i++) subValues.append(0);
    }
    for (int i=0; i<subs.size(); i++) {
        if (subs[i] == topic) {
            subValues[i] = value.toDouble();
            emit newData(subNames[i], subValues[i]);
        }
    }
}

void Mqtt::reconnect()
{
    if (enabled && !mqttClient.hostname().isEmpty() && mqttClient.state() == QMqttClient::ClientState::Disconnected) {
        mqttClient.connectToHost();
    }
}

void Mqtt::updSettings()
{
    if (getMqttActivate() != enabled) {
        enabled = getMqttActivate();
        if (enabled) {
            reconnect();
        }
    }
    if (getMqttServer() != mqttClient.hostname()) {
        mqttClient.setHostname(getMqttServer());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }
    if (getMqttUsername() != mqttClient.username()) {
        mqttClient.setUsername(getMqttUsername());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }
    if (getMqttPassword() != mqttClient.password()) {
        mqttClient.setPassword(getMqttPassword());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }
    if (getMqttPort() != mqttClient.port()) {
        mqttClient.setPort(getMqttPort());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }

    if (getMqttKeepaliveTopic() != keepaliveTopic) {
        keepaliveTopic = getMqttKeepaliveTopic();
        if (!keepaliveTopic.isEmpty()) startKeepaliveTimer();
        else stopKeepaliveTimer();
    }
}

void Mqtt::startKeepaliveTimer()
{
    keepaliveTimer->start(60 * 1e3);
}

void Mqtt::stopKeepaliveTimer()
{
    keepaliveTimer->stop();
}
