#include "notifications.h"

Notifications::Notifications(QObject *parent)
    : Config{parent}
{
}

void Notifications::start()
{
    process = new QProcess;

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Notifications::curlCallback);
    process->setWorkingDirectory(QDir(QCoreApplication::applicationDirPath()).absolutePath());

    if (QSysInfo::kernelType().contains("win")) {
        process->setProgram("curl.exe");
    }

    else if (QSysInfo::kernelType().contains("linux")) {
        process->setProgram("curl");
    }

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

    // Housekeeping
    QDirIterator it(workFolder, {".traceplot*"});
    while (it.hasNext()){
        QFile(it.next()).remove();
    }
    QDirIterator it2(workFolder, {".json*"});
    while (it2.hasNext()){
        QFile(it2.next()).remove();
    }
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
    else if (type == NOTIFY::TYPE::GEOLIMITER) msg.prepend("Geoblocking: ");

    //else if (id.contains("traceAnalyzer", Qt::CaseInsensitive)) msg.prepend(getMeasurementDeviceName() + ": ");
    appendIncidentLog(dt, msg);
    appendLogFile(dt, msg);
    if (getEmailNotifyGnssIncidents() && type == NOTIFY::TYPE::GNSSANALYZER)
        appendEmailText(dt,  msg);
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
    if (getSdefAddPosition()) emit reqPosition(); // ask for position at this point if we are mobile, to insert into mail text later
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
            message.setSubject("Notification from " + getStationName() + " (" + getSdefStationInitals() + ")");
            message.setSender(SimpleMail::EmailAddress(getEmailFromAddress(), ""));

            for (auto &val : mailRecipients) {
                message.addTo(SimpleMail::EmailAddress(val, val.split('@').at(0)));
            }
            mimeHtml->setHtml("<table>" + mailtext +
                              (getSdefAddPosition() && positionValid?
                                                      tr("<tr><td>Current position</td><td><a href=\"https://www.google.com/maps/place/") +
                                                                                                  QString::number(latitude, 'f', 5) + "+" +
                                                                                                  QString::number(longitude, 'f', 5) + "/@" +
                                                                                                  QString::number(latitude, 'f', 5) + "," +
                                                                                                  QString::number(longitude, 'f', 5) + ",10z\">" +
                                                                           QString::number(latitude, 'f', 5) + " " +
                                                                           QString::number(longitude, 'f', 5) +
                                                                                                  tr("</a></td><td>") +
                                                                                                  "<a href=\"https://nais.kystverket.no/point/" +
                                                                                                  QString::number(longitude, 'f', 5) + "_" +
                                                                                                  QString::number(latitude, 'f', 5)  +
                                                                                                  tr("\">Link til Kystverket</td></tr>"):"") +
                              "</table><hr><img src='cid:image1' />   ");
            //qDebug() << "mail debug:" << mimeHtml->data();
            message.addPart(mimeHtml);
            htmlData = mimeHtml->html();

            auto image1 = new SimpleMail::MimeInlineFile(new QFile(lastPicFilename));

            image1->setContentId("image1");
            image1->setContentType("image/png");
            //message.addPart(&image1);
            emailPictures.append(image1);
            message.addPart(emailPictures.last());
            //qDebug() << "mail debug stuff" << mimeHtml->data() << message.sender().address() << message.toRecipients().first().address() << message.subject();
            emailBacklog.append(message);
            mailtext.clear();

            // MS Graph code in here!
            if (msGraphConfigured) {
                generateGraphEmail();
                authGraph(); // try to auth and send immediately
            }
            else {
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
            }

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
    if (!msGraphConfigured) {
        bool ok;
        int val = mailserverPort.toInt(&ok);
        if (val <= 0 || val > 65536) ok = false;

        if (!ok || mailserverAddress.isEmpty() || mailserverPort.isEmpty() || recipients.isEmpty() || fromAddress.isEmpty())
            return false;

        return true;
    }
    else {
        if (getEmailFromAddress().isEmpty()) {
            emit warning("Notification is not possible until from address is set");
            return false;
        }
        return true;
    }
}

void Notifications::setupIncidentTable()
{
    emit showIncident("<tr><th width=50 align=left>Date</th><th width=50 align=left>Time</th><th width=100 align=left>Text</th></tr>");
    appendIncidentLog(QDateTime::currentDateTime(), "Application started");
}

void Notifications::recTracePlot(const QPixmap *pic)
{
    lastPicFilename = workFolder + "/.traceplot" + QDateTime::currentDateTime().toString("ddhhmmss") + ".png";
    if (!pic->save(lastPicFilename, "png"))
        qDebug() << "Cannot save traceplot to" << lastPicFilename;
    else
        qDebug() << "Traceplot saved as" << lastPicFilename;
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
    msGraphTenantId = getEmailGraphTenantId();
    msGraphApplicationId = getEmailGraphApplicationId();
    msGraphSecret = getEmailGraphSecret();

    if (!msGraphApplicationId.isEmpty() && !msGraphTenantId.isEmpty() && !msGraphSecret.isEmpty()) {
        msGraphConfigured = true;
    }
}

void Notifications::retryEmails()
{
    qDebug() << "mail retry?" << emailBacklog.size() << graphEmailLog.size();

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

void Notifications::authGraph()
{
    simpleParametersCheck();
    QString url = "https://login.microsoftonline.com/" + getEmailGraphTenantId() + "/oauth2/v2.0/token";
    QStringList l;
    l //<< "-H" << "\"Content-Type:" << "application/x-www-form-url-encoded\""
        << "--data" << "grant_type=client_credentials"
        << "--data" << "client_id=" + getEmailGraphApplicationId()
        << "--data" << "client_secret=" + getEmailGraphSecret()
        << "--data" << "scope=https://graph.microsoft.com/.default"
        << url;

    //qDebug() << "Grant debug" << l;
    process->setArguments(l);
    process->start();
}

void Notifications::generateGraphEmail()
{
    QJsonObject mail, message, body;
    QJsonArray toRecipients, attachments;
    QJsonObject att;

    QStringList recipients = getEmailRecipients().split(';');
    for (auto &recipient : recipients) {
        QJsonObject receiver, address;
        address.insert("address", recipient.simplified());
        receiver.insert("emailAddress", address);
        toRecipients.push_back(receiver);
    }

    body.insert("contentType", "html");
    body.insert("content", htmlData);
    QFile picture(lastPicFilename);
    bool fileOk;
    if (!picture.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open traceplot file" << lastPicFilename << picture.errorString();
        fileOk = false;
    }
    else {
        fileOk = true;
        att.insert("@odata.type", "#microsoft.graph.fileAttachment");
        att.insert("name", "image1.png");
        att.insert("contentType", "image/png");
        att.insert("contentId", "image1");
        att.insert("contentBytes", QString(picture.readAll().toBase64()));
        att.insert("isInline", "true");
        attachments.append(att);
    }

    message.insert("subject", "Notification from " + getStationName() + " (" + getSdefStationInitals() + ")");
    message.insert("body", body);
    message.insert("toRecipients", toRecipients);
    if (fileOk) message.insert("attachments", attachments);

    mail.insert("message", message);
    mail.insert("saveToSentItems", "false");

    QJsonDocument json(mail);
    QString jsonFilename = workFolder + "/.json_" + QDateTime::currentDateTime().toString("ddhhmmss");
    QFile jsonFile(jsonFilename);   // turns out QProcess doesn't like super long program arguments, like ... ~40 kB :) Saving to a temporary file
    jsonFile.open(QIODevice::WriteOnly);
    jsonFile.write(json.toJson(QJsonDocument::Compact));
    jsonFile.close();
    if (!picture.remove()) // picture data is now stored in the json code, close and delete file
        qDebug() << "Couldn't remove traceplot file" << picture.fileName();

    graphEmailLog.append(jsonFilename);
    //qDebug() << toRecipients;
}

void Notifications::sendMailWithGraph()
{
    if (!graphEmailLog.isEmpty()) {
        graphMailInProgress = true;
        QStringList l;
        QString url = "https://graph.microsoft.com/v1.0/users/" + getEmailFromAddress() + "/sendMail";
        l << "-H" << "Content-Type:application/json;charset=ISO-8859-1"
          << "-s" << "-w" << "%{http_code}"
          << "-H" << graphAccessToken
          << "--data-ascii" << "@" + graphEmailLog.first()
          << url;

        process->setArguments(l);
        //qDebug() << process->arguments();
        process->start();
    }
    else qDebug() << "Trying to send an empty email?";
}

void Notifications::curlCallback(int exitCode, QProcess::ExitStatus)
{
    QByteArray output = process->readAllStandardOutput();
    //qDebug() << output << process->readAllStandardError();

    if (exitCode != 0) {
        qDebug() << "Graph exit code" << exitCode;
        if (graphMailInProgress && !graphEmailLog.isEmpty()) { // sending failed, let's retry later
            qDebug() << "Graph email failed, retrying in 15 minutes";
            QTimer::singleShot(15 * 60e3, this, &Notifications::authGraph);
        }
    }

    else if (!graphMailInProgress) {
        QJsonParseError error;
        QJsonDocument reply = QJsonDocument::fromJson(output, &error);
        QJsonObject replyObject = reply.object();
        //qDebug() << output << reply.toJson() << reply.isObject() << reply.isArray() ;

        if (reply.isNull() || replyObject.isEmpty() ||
            replyObject.find("token_type")->toString().contains("Bearer") ||
            replyObject.find("expires_in")->toInt() > 0) {

            graphAccessToken = replyObject.find("access_token")->toString().toUtf8();
            /*for (int i=0; i<output.size(); i++) {
            if (i < output.size() - 1 && output.at(i).contains("access_token"))
                graphAccessToken = output.at(i+1);
        }*/
            graphAccessToken.insert(0, "Authorization: Bearer ");

            qDebug() << "Graph authenticated"; // << graphAccessToken;
            sendMailWithGraph();
        }
        else {
            qDebug() << output << process->readAllStandardError();
            emit warning("No valid response from MS Graph authentication server");
        }
    }
    else if (output.contains("200") || output.contains("201") || output.contains("202")) {
        qDebug() << "Mail sent successfully"; // << output << process->readAllStandardError();
        if (QFile::remove(graphEmailLog.first())) graphEmailLog.removeFirst();  // delete file and name from the sendlist if successful
        graphMailInProgress = false;
    }
    else {
        qDebug() << "Server responded with code" << output;
        graphMailInProgress = false;
    }
}
