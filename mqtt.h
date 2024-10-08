#ifndef MQTT_H
#define MQTT_H

#include <QWidget>
#include <QtMqtt/QtMqtt>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include "config.h"
#include "typedefs.h"

enum SITESTATUS {
    UNKNOWN,
    STOP,
    START,
    RUNNING,
    NORUNNING
};

class Mqtt : public QObject
{
    Q_OBJECT
public:
    explicit Mqtt(QSharedPointer<Config>);

public slots:
    void updSettings();

signals:
    void newData(QString& name, double& value);
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);

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
    void webswitchRequestData();
    void webswitchParseData(int exitCode, QProcess::ExitStatus);
    void parseMqtt(QString topic, QByteArray msg);

private:
    QMqttClient mqttClient;
    QTimer *keepaliveTimer = new QTimer;
    QTimer *webswitchTimer = new QTimer;
    QList<double> subValues;
    QProcess *webswitchProcess = new QProcess;
    SITESTATUS siteStatus = UNKNOWN;
    QSharedPointer<Config> config;

    // config cache
    bool enabled = false;
    QString keepaliveTopic;
    QString webswitchAddress;
};

#endif // MQTT_H
