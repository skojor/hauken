#include "mqtt.h"

Mqtt::Mqtt(QSharedPointer<Config> c)
{
    config = c;

    connect(&mqttClient, &QMqttClient::stateChanged, this, &Mqtt::stateChanged);
    connect(&mqttClient, &QMqttClient::errorChanged, this, &Mqtt::error);
    connect(&mqttClient, &QMqttClient::messageSent, this, &Mqtt::msgSent);
    connect(&mqttClient, &QMqttClient::messageReceived, this, &Mqtt::msgReceived);
    connect(keepaliveTimer, &QTimer::timeout, this, [this] {
        mqttClient.publish(config->getMqttKeepaliveTopic(), QByteArray());
        for (auto &val : config->getMqttSubTopics()) {
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


    /*QTimer::singleShot(1000, this, [this] {
        QJsonObject json;
        json["siteid"] = 1;
        json["sitename"] = "Bleik";
        json["testid"] = 1110;
        json["name"] = "";
        json["description"] = "";
        json["comment"] = "";
        json["status"] = "no running test";
        json["localtime"] = "2023-09-07T14:27:39+02:00";
        json["utctime"] = "2023-09-07T12:27:39+02:00";
        QJsonDocument jsonDoc(json);
        parseMqtt("basic_status/site1", jsonDoc.toJson());
    });
    QTimer::singleShot(3000, this, [this] {
        QJsonObject json;
        json["siteid"] = 1;
        json["sitename"] = "Bleik";
        json["testid"] = 1110;
        json["name"] = "6.1.4";
        json["description"] = "Test: 0.1 µW to 20 W, 2 dB increments PRN: L1, G1, L2, L5";
        json["comment"] = "";
        json["status"] = "running";
        json["localtime"] = "2023-09-07T14:27:39+02:00";
        json["utctime"] = "2023-09-07T12:27:39+02:00";
        QJsonDocument jsonDoc(json);
        parseMqtt("basic_status/site1", jsonDoc.toJson());
    });

    QTimer::singleShot(4000, this, [this] {
        QJsonObject json;
        json["siteid"] = 1;
        json["sitename"] = "Bleik";
        json["testid"] = 1110;
        json["name"] = "6.1.4";
        json["description"] = "Test: 0.1 µW to 20 W, 2 dB increments PRN: L1, G1, L2, L5";
        json["comment"] = "";
        json["status"] = "running";
        json["localtime"] = "2023-09-07T14:27:39+02:00";
        json["utctime"] = "2023-09-07T12:27:39+02:00";
        QJsonDocument jsonDoc(json);
        parseMqtt("basic_status/site1", jsonDoc.toJson());
    });

    QTimer::singleShot(5000, this, [this] {
        QJsonObject json;
        json["siteid"] = 1;
        json["sitename"] = "Bleik";
        json["testid"] = 1110;
        json["name"] = "6.1.4";
        json["description"] = "Test: 0.1 µW to 20 W, 2 dB increments PRN: L1, G1, L2, L5";
        json["comment"] = "";
        json["status"] = "stop";
        json["localtime"] = "2023-09-07T14:27:39+02:00";
        json["utctime"] = "2023-09-07T12:27:39+02:00";
        QJsonDocument jsonDoc(json);
        parseMqtt("basic_status/site1", jsonDoc.toJson());
    });

    QTimer::singleShot(6000, this, [this] {
        QJsonObject json;
        json["siteid"] = 1;
        json["sitename"] = "Bleik";
        json["testid"] = 1110;
        json["name"] = "6.1.4";
        json["description"] = "Test: 0.1 µW to 20 W, 2 dB increments PRN: L1, G1, L2, L5";
        json["comment"] = "";
        json["status"] = "no running test";
        json["localtime"] = "2023-09-07T14:27:39+02:00";
        json["utctime"] = "2023-09-07T12:27:39+02:00";
        QJsonDocument jsonDoc(json);
        parseMqtt("basic_status/site1", jsonDoc.toJson());
    });

    QTimer::singleShot(7000, this, [this] {
        QJsonObject json;
        json["siteid"] = 1;
        json["sitename"] = "Bleik";
        json["testid"] = 1110;
        json["name"] = "6.1.4";
        json["description"] = "Test: 0.1 µW to 20 W, 2 dB increments PRN: L1, G1, L2, L5";
        json["comment"] = "";
        json["status"] = "no running test";
        json["localtime"] = "2023-09-07T14:27:39+02:00";
        json["utctime"] = "2023-09-07T12:27:39+02:00";
        QJsonDocument jsonDoc(json);
        parseMqtt("basic_status/site1", jsonDoc.toJson());
    });
*/
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
        if (!config->getMqttKeepaliveTopic().isEmpty()) mqttClient.publish(config->getMqttKeepaliveTopic(), QByteArray());

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
    for (auto &val : config->getMqttSubTopics()) {
        mqttClient.subscribe(val);
    }
}

void Mqtt::msgReceived(const QByteArray &msg, const QMqttTopicName &topic)
{
    QStringList subs = config->getMqttSubTopics();
    QStringList subNames = config->getMqttSubNames();
    QStringList subToIncidentlog = config->getMqttSubToIncidentlog();

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
            if (subToIncidentlog.size() > i) {
                if (subToIncidentlog[i] == "1") parseMqtt(topic.name(), msg); //emit toIncidentLog(NOTIFY::TYPE::MQTT, "", msg);
            }
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
    if (config->getMqttActivate() != enabled) {
        enabled = config->getMqttActivate();
        if (enabled) {
            reconnect();
        }
    }
    if (config->getMqttServer() != mqttClient.hostname()) {
        mqttClient.setHostname(config->getMqttServer());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }
    if (config->getMqttUsername() != mqttClient.username()) {
        mqttClient.setUsername(config->getMqttUsername());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }
    if (config->getMqttPassword() != mqttClient.password()) {
        mqttClient.setPassword(config->getMqttPassword());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }
    if (config->getMqttPort() != mqttClient.port()) {
        mqttClient.setPort(config->getMqttPort());
        if (mqttClient.state() == QMqttClient::ClientState::Connected) mqttClient.disconnect();
        reconnect();
    }

    if (config->getMqttKeepaliveTopic() != keepaliveTopic) {
        keepaliveTopic = config->getMqttKeepaliveTopic();
        if (!keepaliveTopic.isEmpty()) startKeepaliveTimer();
        else stopKeepaliveTimer();
    }

    if (config->getMqttWebswitchAddress() != webswitchAddress) {
        webswitchAddress = config->getMqttWebswitchAddress();
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
    args << "-s" << "-w" << "%{http_code}" << config->getMqttWebswitchAddress();
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

void Mqtt::parseMqtt(QString topic, QByteArray msg)
{
    if (topic.contains("basic_status", Qt::CaseInsensitive)) { // special case
        QJsonDocument jsonDoc = QJsonDocument::fromJson(msg);
        QJsonObject jsonObject = jsonDoc.object();
        QJsonValue siteid = jsonObject.value("siteid");
        QJsonValue sitename = jsonObject.value("sitename");
        QJsonValue testid = jsonObject.value("testid");
        QJsonValue name = jsonObject.value("name");
        QJsonValue description = jsonObject.value("description");
        QJsonValue comment = jsonObject.value("comment");
        QJsonValue status = jsonObject.value("status");
        QJsonValue localtime = jsonObject.value("localtime");
        QJsonValue utctime = jsonObject.value("utctime");

        QString msg;
        QTextStream ts(&msg);

        if (siteStatus == RUNNING && status.toString().contains("stop", Qt::CaseInsensitive)) { // test ended
            ts << sitename.toString() << ": " << "Test " << name.toString() << " ended";
            siteStatus = STOP;
        }
        else if (siteStatus == UNKNOWN && status.toString().contains("no running test", Qt::CaseInsensitive)) { // no test
            ts << sitename.toString() << ": " << "No test running";
            siteStatus = NORUNNING;
        }
        else if (siteStatus == RUNNING && status.toString().contains("no running test", Qt::CaseInsensitive)) { // from running to no running test, we missed sth
            ts << sitename.toString() << ": " << "No test running";
            siteStatus = NORUNNING;
        }
        else if (siteStatus != RUNNING  && status.toString() == "running") { // from sth to running test
            ts << sitename.toString() << ": " << "Test " << name.toString() << " " << description.toString() << " started";
            siteStatus = RUNNING;
        }

        if (!msg.isEmpty()) emit toIncidentLog(NOTIFY::TYPE::MQTT, "", msg);
    }
}
