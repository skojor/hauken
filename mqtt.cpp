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

void Mqtt::subscribe(const QString &t)
{
    QMqttTopicFilter topic(t);
    //subTopics.append(topic);
}

void Mqtt::msgReceived(const QByteArray &msg, const QMqttTopicName &topic)
{
    //emit mqttMessage(msg, topic);
}

