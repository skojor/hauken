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
    void subscribe(const QString &topic);
    void msgReceived(const QByteArray &msg, const QMqttTopicName &topic);

private:
    QMqttClient mqttClient;
};

#endif // MQTT_H
