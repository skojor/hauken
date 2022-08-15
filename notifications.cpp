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
    retryEmailsTimer = new QTimer;

    updSettings();
    setupIncidentTable();

    connect(truncateTimer, &QTimer::timeout, this, &Notifications::checkTruncate);
    connect(mailDelayTimer, &QTimer::timeout, this, &Notifications::sendMail);
    connect(retryEmailsTimer, &QTimer::timeout, this, &Notifications::retryEmails);

    //truncateTimer->setSingleShot(true);
    mailDelayTimer->setSingleShot(true);

    timeBetweenEmailsTimer = new QTimer;
    timeBetweenEmailsTimer->setSingleShot(true);
    retryEmailsTimer->setSingleShot(true);

    QDirIterator it(workFolder, {".traceplot*"});
    while (it.hasNext()){
        QFile(it.next()).remove();
    }

}

void Notifications::toIncidentLog(const NOTIFY::TYPE type, const QString name, const QString string)
{
    bool truncateThis = false;

    if (truncateTime && (type == NOTIFY::TYPE::GNSSANALYZER || type == NOTIFY::TYPE::TRACEANALYZER)) { // check if certain notifications should be truncated
        if (!truncateList.isEmpty()) { // check if we recently got a notification from the same type of notifier
            for (int i=0; i < truncateList.size(); i++) {
                if (truncateList.at(i).type == type && truncateList.at(i).timeReceived.secsTo(QDateTime::currentDateTime()) < truncateTime * 1e3) { // found one, it seems we should truncate this
                    truncateThis = true;
                    truncateList.removeAt(i);
                    truncateList.append(NotificationsBuffer(type, QDateTime::currentDateTime(), name, string)); // remove the previous buffer line and insert the last one, then wait truncateTime seconds before ev. posting
                    truncateTimer->start(1000);
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
    if (getEmailNotifyGnssIncidents() && type == NOTIFY::TYPE::GNSSANALYZER)
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
            if (truncateList.at(i).timeReceived.secsTo(QDateTime::currentDateTime()) >= truncateTime * 1e3) { // found msg ready to be shown
                generateMsg(truncateList.at(i).type, truncateList.at(i).id, truncateList.at(i).msg, truncateList.at(i).timeReceived);
                truncateList.removeAt(i);
            }
        }
        if (truncateList.isEmpty())
            truncateTimer->stop();
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
    if (!mailDelayTimer->isActive()) {
        mailDelayTimer->start(truncateTime * 2e3); // wait double time of truncate time, to be sure to catch all log lines
        int delay = getEmailDelayBeforeAddingImages();
        if (delay > truncateTime * 2) delay = (truncateTime * 2) - 1; // no point in requesting an image after the mail is sent...
        QTimer::singleShot(delay * 1e3, this, [this] { emit reqTracePlot(); }); // also ask for a plot image at this time
    }
}

void Notifications::sendMail()
{
    if (!timeBetweenEmailsTimer->isActive()) {
        if (simpleParametersCheck() and !mailtext.isEmpty()) {
            auto server = new SimpleMail::Server;
            server->setHost(mailserverAddress);
            server->setPort(mailserverPort.toUInt());

            if (!smtpUser.isEmpty() || !smtpPass.isEmpty()) {
                server->setUsername(smtpUser);
                server->setPassword(smtpPass);
            }

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
            mimeHtml->setHtml("<table>" + mailtext + "</table><hr><img src='cid:image1' />   ");
            message.addPart(mimeHtml);

            auto image1 = new SimpleMail::MimeInlineFile(new QFile(lastPicFilename));

            image1->setContentId("image1");
            image1->setContentType("image/png");
            //message.addPart(&image1);
            emailPictures.append(image1);
            message.addPart(emailPictures.last());
            //qDebug() << "mail debug stuff" << mimeHtml->data() << message.sender().address() << message.toRecipients().first().address() << message.subject();
            emailBacklog.append(message);
            mailtext.clear();

            SimpleMail::ServerReply *reply = server->sendMail(message);
            connect(reply, &SimpleMail::ServerReply::finished, this, [this, reply]
            {
                qDebug() << "ServerReply finished" << reply->error() << reply->responseText();
                if (reply->error()) {
                    toIncidentLog(NOTIFY::TYPE::GENERAL, "", "Email notification failed, trying again later");
                    this->retryEmailsTimer->start(90 * 1e3); // check again in 15 min
                }
                else {
                    this->emailBacklog.removeLast();
                    this->emailPictures.removeLast();
                }
                reply->deleteLater();
            });
            timeBetweenEmailsTimer->start(delayBetweenEmails * 1e3); // start this timer to ensure emails are not spamming like crazy
        }
        else {
            emit warning("Email notifications is enabled, but one or more of the parameters are missing. Check your configuration!");
        }
    }
    else {
        mailDelayTimer->start(timeBetweenEmailsTimer->remainingTime()); // rerun timer calling this routine in x msec
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

void Notifications::recTracePlot(const QPixmap *pic)
{
    pic->save(workFolder +
              "/.traceplot" +
              QDateTime::currentDateTime().toString("ddhhmmss") +
              ".png", "png");
    lastPicFilename = workFolder + "/.traceplot" + QDateTime::currentDateTime().toString("ddhhmmss") + ".png";
}

void Notifications::updSettings()
{
    mailserverAddress = getEmailSmtpServer();
    mailserverPort = getEmailSmtpPort();
    smtpUser = getEmailSmtpUser();
    smtpPass = getEmailSmtpPassword();
    recipients = getEmailRecipients();
    fromAddress = getEmailFromAddress();
    truncateTime = getNotifyTruncateTime();
    if (!workFolder.contains(getWorkFolder())) {
        workFolder = getWorkFolder();
        if (incidentLogfile->isOpen()) incidentLogfile->close(); // in case user changes folder settings
    }
    delayBetweenEmails = getEmailMinTimeBetweenEmails();
}

void Notifications::retryEmails()
{
    qDebug() << "mail retry?" << emailBacklog.size();

    if (!emailBacklog.isEmpty()) {
        if (simpleParametersCheck()) {
            auto server = new SimpleMail::Server;
            server->setHost(mailserverAddress);
            server->setPort(mailserverPort.toUInt());

            if (!smtpUser.isEmpty() || !smtpPass.isEmpty()) {
                server->setUsername(smtpUser);
                server->setPassword(smtpPass);
            }

            QStringList mailRecipients;
            if (recipients.contains(';')) mailRecipients = recipients.split(';');
            else mailRecipients.append(recipients);

            for (int i=0; i<emailBacklog.size(); i++) {
                SimpleMail::ServerReply *reply = server->sendMail(emailBacklog.at(i));
                connect(reply, &SimpleMail::ServerReply::finished, this, [this, reply, i]
                {
                    qDebug() << "ServerReply finished" << reply->error() << reply->responseText();
                    if (reply->error()) {
                        this->retryEmailsTimer->start(900 * 1e3); // check again in 15 min
                    }
                    else {
                        this->emailBacklog.removeAt(i);
                        this->emailPictures.removeAt(i);
                    }
                    reply->deleteLater();
                });
            }
        }
        if (!emailBacklog.isEmpty()) // still emails left in the queue, retry later
            retryEmailsTimer->start(900 * 1e3);
    }
}
