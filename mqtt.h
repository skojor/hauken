#ifndef MQTT_H
#define MQTT_H

#include <QWidget>
#include <QtMqtt/QtMqtt>
#include "config.h"

class Mqtt : public Config
{
public:
    explicit Mqtt(QObject *parent = nullptr);

public slots:
    void updSettings();

private slots:
    void stateChanged(QMqttClient::ClientState state);
    void checkConnection();
    void error(QMqttClient::ClientError error);
    void msgSent(qint32 id);
    //void sendMessage(const QVector<Sensor> &sensorList);
    void subscribe();
    void msgReceived(const QByteArray &msg, const QMqttTopicName &topic);
    void reconnect();

private:
    QMqttClient mqttClient;

    // config cache
    bool enabled = false;
    QString server;
    QString sub1Name, sub1Topic;
    QString sub2Name, sub2Topic;
    QString sub3Name, sub3Topic;
    QString sub4Name, sub4Topic;
    QString sub5Name, sub5Topic;
    QString keepaliveTopic;
};

#endif // MQTT_H
