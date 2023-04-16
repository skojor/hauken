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
        for (auto &val : getMqttSubTopics()) {
            QString request = val.replace("N/", "R/");
            mqttClient.publish(request, QByteArray());
        }
        //qDebug() << "Sending MQTT keepalive";
    });
    connect(webswitchTimer, &QTimer::timeout, this, &Mqtt::webswitchRequestData);

    connect(webswitchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Mqtt::webswitchParseData);
    webswitchProcess->setWorkingDirectory(QDir(QCoreApplication::applicationDirPath()).absolutePath());

    if (QSysInfo::kernelType().contains("win")) {
        webswitchProcess->setProgram("curl.exe");
    }

    else if (QSysInfo::kernelType().contains("linux")) {
        webswitchProcess->setProgram("curl");
    }
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
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

    if (getMqttWebswitchAddress() != webswitchAddress) {
        webswitchAddress = getMqttWebswitchAddress();
        if (webswitchAddress.isEmpty()) webswitchTimer->stop();
        else {
            webswitchRequestData();
            webswitchTimer->start(60 * 1e3);
        }
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

void Mqtt::checkConnection()
{

}

void Mqtt::webswitchRequestData()
{
    QStringList args;
    args << "-s" << "-w" << "%{http_code}" << getMqttWebswitchAddress();
    webswitchProcess->setArguments(args);
    webswitchProcess->start();
    //qDebug() << "req ws data" << args;
}

void Mqtt::webswitchParseData(int exitCode, QProcess::ExitStatus)
{
    QString returnText = webswitchProcess->readAllStandardOutput();
    //qDebug() << "Returned:" << returnText;

    if (exitCode != 0 || !returnText.contains("200")) {
        qDebug() << "Report failed" << webswitchProcess->readAllStandardError() << webswitchProcess->readAllStandardOutput() << exitCode;
    }
    else {
        QStringList split = returnText.split('|');
        if (split.size() > 3 && returnText.contains("OK")) {
            bool ok = false;
            double val = split[3].toDouble(&ok);
            if (ok) {
                QString name("temp");
                emit newData(name, val);
            }
        }
        //qDebug() << split << split[2];
    }
}
