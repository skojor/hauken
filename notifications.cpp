#include "notifications.h"

Notifications::Notifications(QObject *parent)
    : Config{parent}
{

}

void Notifications::start()
{
    incidentLogfile = new QFile;
    truncateTimer = new QTimer;
    mailDelayTimer = new QTimer;

    updSettings();
    setupIncidentTable();

    connect(truncateTimer, &QTimer::timeout, this, &Notifications::checkTruncate);
    connect(mailDelayTimer, &QTimer::timeout, this, &Notifications::sendMail);
    truncateTimer->setSingleShot(true);
    mailDelayTimer->setSingleShot(true);
}

void Notifications::toIncidentLog(const NOTIFY::TYPE type, const QString name, const QString string)
{
    bool truncateThis = false;

    if (truncateTime && (type == NOTIFY::TYPE::GNSSANALYZER || type == NOTIFY::TYPE::TRACEANALYZER)) { // check if certain notifications should be truncated
        if (!truncateList.isEmpty()) { // check if we recently got a notification from the same type of notifier
            for (int i=0; i < truncateList.size(); i++) {
                if (truncateList.at(i).type == type && truncateList.at(i).timeReceived.secsTo(QDateTime::currentDateTime()) < truncateTime) { // found one, it seems we should truncate this
                    truncateThis = true;
                    truncateList.removeAt(i);
                    truncateList.append(NotificationsBuffer(type, QDateTime::currentDateTime(), name, string)); // remove the previous buffer line and insert the last one, then wait truncateTime seconds before ev. posting
                    truncateTimer->start(truncateTime * 1e3);
                }
            }
        }
        else { // not been questioned for truncating before, adding this one
            truncateList.append(NotificationsBuffer(type, QDateTime::currentDateTime(), name, string));
        }
    }

    if (!truncateThis) {
        generateMsg(type, name, string, QDateTime::currentDateTime());
    }
}

void Notifications::generateMsg(NOTIFY::TYPE type, const QString name, const QString string, QDateTime dt)
{
    QString msg = string;
    if (type == NOTIFY::TYPE::MEASUREMENTDEVICE) msg.prepend(name + ": ");
    else if (type == NOTIFY::TYPE::GNSSDEVICE || type == NOTIFY::TYPE::GNSSANALYZER) msg.prepend("GNSS" + name + ": ");
    //else if (id.contains("traceAnalyzer", Qt::CaseInsensitive)) msg.prepend(getMeasurementDeviceName() + ": ");
    appendIncidentLog(dt, msg);
    appendLogFile(dt, msg);
    if (getEmailNotifyGnssIncidents() && (type == NOTIFY::TYPE::GNSSANALYZER || type == NOTIFY::TYPE::GNSSDEVICE))
        appendEmailText(dt, msg);
    else if (getEmailNotifyMeasurementDeviceHighLevel() && type == NOTIFY::TYPE::TRACEANALYZER)
        appendEmailText(dt, msg);
    else if (getEmailNotifyMeasurementDeviceDisconnected() && msg.contains("disconnected", Qt::CaseInsensitive))
        appendEmailText(dt, msg);
}

void Notifications::checkTruncate()
{
    if (!truncateList.isEmpty()) {
        for (int i=0; i<truncateList.size(); i++) {
            if (truncateList.at(i).timeReceived.secsTo(QDateTime::currentDateTime()) >= truncateTime) { // found msg ready to be shown
                generateMsg(truncateList.at(i).type, truncateList.at(i).id, truncateList.at(i).msg, truncateList.at(i).timeReceived);
                truncateList.removeAt(i);
            }
        }
        if (!truncateList.isEmpty()) // could happen if two truncateable classes reports at almost the same time, let's rerun the timer just to check
            truncateTimer->start(truncateTime * 1e3);
    }
}

void Notifications::appendIncidentLog(QDateTime dt, const QString string)
{
    QString text;
    QTextStream ts(&text);
    ts << "<tr><td>" << dt.toString("dd.MM.yy") << "</td><td>"
       << dt.toString("hh:mm:ss") << "</td><td>" << string
       << "</td></tr>";

    emit showIncident(text);
}

void Notifications::appendLogFile(QDateTime dt, const QString string)
{
    if (!incidentLogfile->isOpen()) {
        incidentLogfile->setFileName(getWorkFolder() + "/incident.log");
        incidentLogfile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }

    QString text;
    QTextStream ts(&text);
    ts << dt.toString("dd.MM.yy hh:mm:ss") << "\t" << string << "\n";
    incidentLogfile->write(text.toLocal8Bit());
    incidentLogfile->flush();
}

void Notifications::appendEmailText(QDateTime dt, const QString string)
{
    QString text;
    QTextStream ts(&text);
    ts << "<tr><td>" << dt.toString("dd.MM.yy hh:mm:ss") << "</td><td>" << string << "</td></tr>";
    mailtext.append(text);
    if (!mailDelayTimer->isActive()) mailDelayTimer->start(30 * 1e3);
}

void Notifications::sendMail()
{
    if (simpleParametersCheck()) {
        auto server = new SimpleMail::Server;
        server->setHost(mailserverAddress);
        server->setPort(mailserverPort.toUInt());

        QStringList mailRecipients;
        if (recipients.contains(';')) mailRecipients = recipients.split(';');
        else mailRecipients.append(recipients);

        auto mimeHtml = new SimpleMail::MimeHtml;
        SimpleMail::MimeMessage message;
        message.setSubject("Notification " + getStationName());
        message.setSender(SimpleMail::EmailAddress(getEmailFromAddress(), getStationName()));

        for (auto &val : mailRecipients) {
            message.addTo(SimpleMail::EmailAddress(val, val.split('@').at(0)));
        }
        mimeHtml->setHtml(mailtext);
        message.addPart(mimeHtml);

        SimpleMail::ServerReply *reply = server->sendMail(message);
        connect(reply, &SimpleMail::ServerReply::finished, this, [this, reply]
        {
            qDebug() << "ServerReply finished" << reply->error() << reply->responseText();
            if (reply->error()) emit this->warning("Email SMTP error: " + reply->responseText());
            reply->deleteLater();
        });
    }
    else {
        emit warning("Email notifications is enabled, but one or more of the parameters are missing. Check your configuration!");
    }
}

bool Notifications::simpleParametersCheck()
{
    bool ok;
    int val = mailserverPort.toInt(&ok);
    if (val <= 0 || val > 65536) ok = false;

    if (!ok || mailserverAddress.isEmpty() || mailserverPort.isEmpty() || recipients.isEmpty() || fromAddress.isEmpty())
        return false;

    return true;
}

void Notifications::setupIncidentTable()
{
    emit showIncident("<tr><th width=50 align=left>Date</th><th width=50 align=left>Time</th><th width=100 align=left>Text</th></tr>");
    appendIncidentLog(QDateTime::currentDateTime(), "Application started");
}

void Notifications::updSettings()
{
    mailserverAddress = getEmailSmtpServer();
    mailserverPort = getEmailSmtpPort();
    recipients = getEmailRecipients();
    fromAddress = getEmailFromAddress();
    truncateTime = getNotifyTruncateTime();
    if (!workFolder.contains(getWorkFolder())) {
        workFolder = getWorkFolder();
        if (incidentLogfile->isOpen()) incidentLogfile->close(); // in case user changes folder settings
    }
}

