#include "mqtt.h"

Mqtt::Mqtt(QSharedPointer<Config> c, const QString &profileId)
    : m_profileId(profileId)
{
    config = c;
    m_keepaliveTimer = new QTimer(this);
    m_webswitchTimer = new QTimer(this);
    m_connectionTimer = new QTimer(this);
    m_receivedDataTimer = new QTimer(this);

    connect(&m_mqttClient, &QMqttClient::stateChanged, this, &Mqtt::stateChanged);
    connect(&m_mqttClient, &QMqttClient::errorChanged, this, &Mqtt::error);
    connect(&m_mqttClient, &QMqttClient::messageSent, this, &Mqtt::msgSent);
    connect(&m_mqttClient, &QMqttClient::messageReceived, this, &Mqtt::msgReceived);
    connect(m_keepaliveTimer, &QTimer::timeout, this, [this] {
        const MqttBrokerProfile profile = config->getMqttBrokerProfile(m_profileId);
        m_mqttClient.publish(profile.keepaliveTopic, QByteArray());
        for (const QString &topic : profile.subTopics) {
            QString val = topic;
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
        if (reconnectPending) {
            reconnectPending = false;
            updSettings();
            return;
        }
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
        qDebug() << "MQTT profile" << m_profileId << "connected, requesting subscriptions";
        subscribe();
        const QString topic = config->getMqttBrokerProfile(m_profileId).keepaliveTopic;
        if (!topic.isEmpty()) m_mqttClient.publish(topic, QByteArray());

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
    const MqttBrokerProfile profile = config->getMqttBrokerProfile(m_profileId);
    QStringList topics = profile.subTopics + additionalSubscriptions;
    topics.removeDuplicates();
    for (const QString &topic : topics) {
        m_mqttClient.subscribe(topic);
    }
}

void Mqtt::setAdditionalSubscriptions(const QStringList &topics)
{
    QStringList uniqueTopics = topics;
    uniqueTopics.removeAll(QString());
    uniqueTopics.removeDuplicates();
    if (uniqueTopics == additionalSubscriptions) return;

    additionalSubscriptions = uniqueTopics;
    if (m_mqttClient.state() != QMqttClient::ClientState::Disconnected) {
        reconnectPending = true;
        m_mqttClient.disconnectFromHost();
    }
}

void Mqtt::msgReceived(const QByteArray &msg, const QMqttTopicName &topic)
{
    m_receivedDataTimer->start(MQTT_DATATRANSFER_TIMEOUT_MS);

    emit rawMessage(m_profileId, topic.name(), msg);

    const MqttBrokerProfile profile = config->getMqttBrokerProfile(m_profileId);
    const QStringList subs = profile.subTopics;
    const QStringList subNames = profile.subNames;
    const QStringList subToIncidentlog = profile.subToIncidentlog;

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
            const QString name = subNames.size() > i ? subNames.at(i) : subs.at(i);
            emit newData(name, m_subValues[i]);
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
    const MqttBrokerProfile profile = config->getMqttBrokerProfile(m_profileId);
    enabled = profile.enabled && !profile.id.isEmpty();
    if (!enabled) {
        stopKeepaliveTimer();
        m_webswitchTimer->stop();
        if (m_mqttClient.state() != QMqttClient::ClientState::Disconnected)
            m_mqttClient.disconnectFromHost();
        return;
    }

    QStringList configuredSubscriptions = profile.subTopics + additionalSubscriptions;
    configuredSubscriptions.removeDuplicates();
    const bool reconnectRequired = profile.server != m_mqttClient.hostname()
                                || profile.username != m_mqttClient.username()
                                || profile.password != m_mqttClient.password()
                                || profile.port != m_mqttClient.port()
                                || configuredSubscriptions != subscriptionTopics;
    if (reconnectRequired && m_mqttClient.state() != QMqttClient::ClientState::Disconnected) {
        reconnectPending = true;
        m_mqttClient.disconnectFromHost();
        return;
    }

    m_mqttClient.setHostname(profile.server);
    m_mqttClient.setUsername(profile.username);
    m_mqttClient.setPassword(profile.password);
    m_mqttClient.setPort(profile.port);
    subscriptionTopics = configuredSubscriptions;
    reconnect();

    if (profile.keepaliveTopic != keepaliveTopic) {
        keepaliveTopic = profile.keepaliveTopic;
        if (!keepaliveTopic.isEmpty()) startKeepaliveTimer();
        else stopKeepaliveTimer();
    }

    const QString configuredWebswitch = profile.primary ? config->getMqttWebswitchAddress() : QString();
    if (configuredWebswitch != webswitchAddress) {
        webswitchAddress = configuredWebswitch;
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
    if (!config->getMqttBrokerProfile(m_profileId).primary) return;

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
    if (!webswitchAddress.isEmpty())
        networkAccessManager->get(QNetworkRequest(QUrl(webswitchAddress)));
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
