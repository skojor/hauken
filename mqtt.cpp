#include "mqtt.h"

Mqtt::Mqtt(QObject *parent)
{
    this->setParent(parent);

    connect(&mqttClient, &QMqttClient::stateChanged, this, &Mqtt::stateChanged);
    /*connect(&mqttClient, &QMqttClient::errorChanged, this, &MQTT::error);
    connect(&mqttClient, &QMqttClient::messageSent, this, &MQTT::msgSent);
    connect(&mqttClient, &QMqttClient::messageReceived, this, &MQTT::msgReceived);*/

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
    for (int i=0; i<5; i++) {

    }
}

void Mqtt::msgReceived(const QByteArray &msg, const QMqttTopicName &topic)
{
    //emit mqttMessage(msg, topic);
}

void Mqtt::reconnect()
{
    if (enabled && !server.isEmpty() && mqttClient.state() == QMqttClient::ClientState::Disconnected) {
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
    if (getMqttServer() != server) {
        server = getMqttServer();
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }
    if (getMqttSub1Name() != sub1Name) {
        sub1Name = getMqttSub1Name();
    }
    if (getMqttSub1Topic() != sub1Topic) {
        mqttClient.unsubscribe(sub1Topic);
        sub1Topic = getMqttSub1Topic();
        subscribe();
    }
    if (getMqttSub2Name() != sub2Name) {
        sub2Name = getMqttSub2Name();
    }
    if (getMqttSub2Topic() != sub2Topic) {
        sub2Topic = getMqttSub2Topic();
    }
    if (getMqttSub3Name() != sub3Name) {
        sub3Name = getMqttSub3Name();
    }
    if (getMqttSub4Topic() != sub4Topic) {
        sub4Topic = getMqttSub4Topic();
    }
    if (getMqttSub5Name() != sub5Name) {
        sub5Name = getMqttSub5Name();
    }
    if (getMqttKeepaliveTopic() != keepaliveTopic) {
        keepaliveTopic = getMqttKeepaliveTopic();
    }
}
