#include "mqtt.h"

Mqtt::Mqtt(QObject *parent)
{
    this->setParent(parent);

    connect(&mqttClient, &QMqttClient::stateChanged, this, &Mqtt::stateChanged);
    connect(&mqttClient, &QMqttClient::errorChanged, this, &Mqtt::error);
    connect(&mqttClient, &QMqttClient::messageSent, this, &Mqtt::msgSent);
    connect(&mqttClient, &QMqttClient::messageReceived, this, &Mqtt::msgReceived);
    connect(keepaliveTimer, &QTimer::timeout, this, [this] {
        mqttClient.publish(keepaliveTopic, QByteArray());
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
        //for (int i=0; i<subTopics.size(); i++) client.subscribe(subTopics.at(i));
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

/*void Mqtt::sendMessage(const QVector<Sensor> &sensorList)
{
    QMqttTopicName topic;
    for (int i=0; i<sensorList.size(); i++) {
        if (sensorList[i].mqtt) {
            topic.setName("iShipWeather/" + sensorList[i].name);
            client.publish(topic, QByteArray::number(sensorList[i].value, 'f', 1));
        }
    }
}*/

void Mqtt::subscribe()
{
    for (auto &val : subs) {
        val.first = mqttClient.subscribe(val.second);
    }
}

void Mqtt::msgReceived(const QByteArray &msg, const QMqttTopicName &topic)
{
    for (int i=0; i<subs.size(); i++) {
        if (subs[i].second == topic) subValues[i] = msg;
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
    for (auto &val : getMqttSubNames()) {

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
