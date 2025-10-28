#include "mqtt.h"

Mqtt::Mqtt(QSharedPointer<Config> c)
{
    config = c;

    connect(&m_mqttClient, &QMqttClient::stateChanged, this, &Mqtt::stateChanged);
    connect(&m_mqttClient, &QMqttClient::errorChanged, this, &Mqtt::error);
    connect(&m_mqttClient, &QMqttClient::messageSent, this, &Mqtt::msgSent);
    connect(&m_mqttClient, &QMqttClient::messageReceived, this, &Mqtt::msgReceived);
    connect(m_keepaliveTimer, &QTimer::timeout, this, [this] {
       m_mqttClient.publish(config->getMqttKeepaliveTopic(), QByteArray());
        for (auto &val : config->getMqttSubTopics()) {
            QString request = val.replace("N/", "R/");
           m_mqttClient.publish(request, QByteArray());
        }
        //qDebug() << "Sending MQTT keepalive";
    });
    connect(m_webswitchTimer, &QTimer::timeout, this, &Mqtt::reqWebswitchData);
    connect(m_connectionTimer, &QTimer::timeout, this, [this]() {
        qDebug() << "MQTT connection timed out, disconnecting";
       m_mqttClient.disconnectFromHost();
    });
    m_connectionTimer->setSingleShot(true);

    m_receivedDataTimer->setSingleShot(true);
    connect(m_receivedDataTimer, &QTimer::timeout, this, [this]() {
        qWarning() << "MQTT data transfer timeout, resetting connection";
       m_mqttClient.disconnectFromHost();
    });
    /*
     * 02.09 14:26:21:226 Debug: MQTT received QMqttTopicName("basic_status/testevent") "{\"siteid\":1,\"sitename\":\"Test Area 1\",\"name\":\"1.6.1\",\"description\":\"Jammer F8.1: 0.2 \xC2\xB5W (-37dBm) to 50 W (47dBm) with 2 dB increments PRN: L1\",\"comment\":\"Intergration test 2\",\"status\":\"start\",\"localtime\":\"2025-09-02T14:26:21+02:00\",\"utctime\":\"2025-09-02T12:26:21Z\"}"
     */

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
    QTimer::singleShot(11000, this, [this] {
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

    QTimer::singleShot(13000, this, [this] {
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

    QTimer::singleShot(15000, this, [this] {
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

    QTimer::singleShot(16000, this, [this] {
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

    QTimer::singleShot(17000, this, [this] {
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
    });*/
}

void Mqtt::stateChanged(QMqttClient::ClientState state)
{
    if (state == QMqttClient::ClientState::Disconnected) {
        qDebug() << "MQTT disconnected, trying reconnect in 1 minute";
        QTimer::singleShot(60e3, this, [this]() {
            reconnect();
        });
    }
    else if (state == QMqttClient::ClientState::Connecting) {
        qDebug() << "MQTT connecting";
        m_connectionTimer->start(MQTT_CONN_TIMEOUT_MS);
    }
    else if (state == QMqttClient::ClientState::Connected) {
        m_connectionTimer->stop();
        qDebug() << "MQTT connected to broker, requesting subscriptions";
        subscribe();
        if (!config->getMqttKeepaliveTopic().isEmpty())m_mqttClient.publish(config->getMqttKeepaliveTopic(), QByteArray());

    }
}

void Mqtt::error(QMqttClient::ClientError error)
{
    qWarning() << "MQTT error, trying to disconnect MQTT" << error;
   m_mqttClient.disconnectFromHost();
}

void Mqtt::msgSent(qint32 id)
{
    qDebug() << "MQTT message sent, id" << id;
}

void Mqtt::subscribe()
{
    for (auto &val : config->getMqttSubTopics()) {
       m_mqttClient.subscribe(val);
    }
}

void Mqtt::msgReceived(const QByteArray &msg, const QMqttTopicName &topic)
{
    m_receivedDataTimer->start(MQTT_DATATRANSFER_TIMEOUT_MS);

    QStringList subs = config->getMqttSubTopics();
    QStringList subNames = config->getMqttSubNames();
    QStringList subToIncidentlog = config->getMqttSubToIncidentlog();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonValue value = jsonObject.value("value");
    //qDebug() << "MQTT received" << topic << msg;
    if (m_subValues.size() != subs.size()) {
        m_subValues.clear();
        for (int i=0; i<subs.size();i++) m_subValues.append(0);
    }
    for (int i=0; i<subs.size(); i++) {
        if (subs[i] == topic) {
            m_subValues[i] = value.toDouble();
            emit newData(subNames[i], m_subValues[i]);
            if (subToIncidentlog.size() > i) {
                if (subToIncidentlog[i] == "1") parseMqtt(topic.name(), msg); //emit toIncidentLog(NOTIFY::TYPE::MQTT, "", msg);
            }
        }
    }
}

void Mqtt::reconnect()
{
    if (enabled && !m_mqttClient.hostname().isEmpty() &&m_mqttClient.state() == QMqttClient::ClientState::Disconnected) {
       m_mqttClient.connectToHost();
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
    bool reconnectFlag = false;
    if (config->getMqttServer() !=m_mqttClient.hostname()) {
       m_mqttClient.setHostname(config->getMqttServer());
        if (m_mqttClient.state() == QMqttClient::ClientState::Connected)m_mqttClient.disconnect();
        reconnectFlag = true;
    }
    if (config->getMqttUsername() !=m_mqttClient.username()) {
       m_mqttClient.setUsername(config->getMqttUsername());
        if (m_mqttClient.state() == QMqttClient::ClientState::Connected)m_mqttClient.disconnect();
        reconnectFlag = true;
    }
    if (config->getMqttPassword() !=m_mqttClient.password()) {
       m_mqttClient.setPassword(config->getMqttPassword());
        if (m_mqttClient.state() == QMqttClient::ClientState::Connected)m_mqttClient.disconnect();
        reconnectFlag = true;
    }
    if (config->getMqttPort() !=m_mqttClient.port()) {
       m_mqttClient.setPort(config->getMqttPort());
        if (m_mqttClient.state() == QMqttClient::ClientState::Connected)m_mqttClient.disconnect();
        reconnectFlag = true;
    }
    if (reconnectFlag)
        reconnect();

    if (config->getMqttKeepaliveTopic() != keepaliveTopic) {
        keepaliveTopic = config->getMqttKeepaliveTopic();
        if (!keepaliveTopic.isEmpty()) startKeepaliveTimer();
        else stopKeepaliveTimer();
    }

    if (config->getMqttWebswitchAddress() != webswitchAddress) {
        webswitchAddress = config->getMqttWebswitchAddress();
        if (webswitchAddress.isEmpty())m_webswitchTimer->stop();
        else {
            reqWebswitchData();
           m_webswitchTimer->start(WEBSWITCH_TEMP_INTERVAL_MS);
        }
    }
}

void Mqtt::startKeepaliveTimer()
{
    m_keepaliveTimer->start(60 * 1e3);
}

void Mqtt::stopKeepaliveTimer()
{
    m_keepaliveTimer->stop();
}

void Mqtt::checkConnection()
{

}

void Mqtt::parseMqtt(const QString &topic, const QByteArray &msg)
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

        if (config->getMqttSiteFilter() == siteid.toInt() &&
                m_siteStatus == RUNNING && status.toString().contains("stop", Qt::CaseInsensitive))
        { // test ended
            ts << "Test " << name.toString() << " ended";
            m_siteStatus = STOP;
            if (config->getMqttTestTriggersRecording())
                QTimer::singleShot(10000, this, [this] {
                    emit endRecording();
                });
        }
        else if (config->getMqttSiteFilter() == siteid.toInt() &&
                   m_siteStatus == UNKNOWN && status.toString().contains("no running test", Qt::CaseInsensitive))
        { // no test
            ts << "No test running";
            m_siteStatus = NORUNNING;
        }
        else if (config->getMqttSiteFilter() == siteid.toInt() &&
                   m_siteStatus == RUNNING && status.toString().contains("no running test", Qt::CaseInsensitive))
        { // from running to no running test, we missed sth
            ts << "No test running";
            m_siteStatus = NORUNNING;
        }
        else if (config->getMqttSiteFilter() == siteid.toInt() &&
                   m_siteStatus != RUNNING  &&
                   (status.toString() == "running" || status.toString() == "start"))
        { // from sth to running test
            ts << "Test " << name.toString() << " " << description.toString() << " started";
                //<< (config->getMqttTestTriggersRecording()?" (recording)":"");
            m_siteStatus = RUNNING;
            if (config->getMqttTestTriggersRecording()) emit triggerRecording(); // Recording triggered here, kept alive below
        }
        else if ( config->getMqttSiteFilter() == siteid.toInt() &&
            m_siteStatus == RUNNING &&
                   status.toString().contains("running") &&
                   config->getMqttTestTriggersRecording()) {
            emit triggerRecording(); // Keep triggering recording until no test running
        }

        if (!msg.isEmpty()) emit toIncidentLog(NOTIFY::TYPE::MQTT, "", msg);
    }
}

void Mqtt::reqWebswitchData()
{
    networkAccessManager->get(QNetworkRequest(QUrl(config->getMqttWebswitchAddress())));
}

void Mqtt::networkAccessManagerReplyHandler(QNetworkReply *reply)
{
    tmNetworkTimeout->stop();
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        // Webswitch specific parsing
        QList<QByteArray> list = data.split('|');
        if (list.size() == 5 && list[1].toInt() == 0 && list[2] == "OK") {
            bool ok = false;
            double val = list[3].toDouble(&ok);
            if (ok) {
                emit newData("temp", val);
            }
        }
    }
    else
        qDebug() << "Webswitch routine error:" << reply->errorString();

    reply->deleteLater();
}
