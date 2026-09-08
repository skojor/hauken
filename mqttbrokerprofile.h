#ifndef MQTTBROKERPROFILE_H
#define MQTTBROKERPROFILE_H

#include <QString>
#include <QStringList>

struct MqttBrokerProfile
{
    QString id;
    QString name;
    bool enabled = false;
    QString server;
    QString username;
    QString password;
    int port = 1883;
    QString keepaliveTopic;
    QStringList subNames;
    QStringList subTopics;
    QStringList subToIncidentlog;
    bool primary = false;
};

#endif // MQTTBROKERPROFILE_H