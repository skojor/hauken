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
    void startKeepaliveTimer();
    void stopKeepaliveTimer();

private:
    QMqttClient mqttClient;
    QTimer *keepaliveTimer = new QTimer;
    QList<QByteArray> subValues;

    // config cache
    bool enabled = false;
    QList< QPair<QMqttSubscription *, QString >> subs;
    QString keepaliveTopic;
};

#endif // MQTT_H
