#include "notifications.h"

Notifications::Notifications(QObject *parent)
    : Config{parent}
{

}

void Notifications::start()
{
    updSettings();
    setupIncidentTable();
}

void Notifications::toIncidentLog(const QString id, const QString name, const QString string)
{
    // do sth clever here
    QString msg = string;
    if (id.contains("measurementDevice", Qt::CaseInsensitive)) msg.prepend(name + ": ");
    else if (id.contains("gnss", Qt::CaseInsensitive)) msg.prepend("GNSS" + name + ": ");
    //else if (id.contains("traceAnalyzer", Qt::CaseInsensitive)) msg.prepend(getMeasurementDeviceName() + ": ");
    appendIncidentLog(msg);
    appendLogFile(msg);
}

void Notifications::appendIncidentLog(const QString string)
{
    QString text;
    QTextStream ts(&text);
    ts << "<tr><td>" << QDate::currentDate().toString("dd.MM.yy") << "</td><td>"
       << QTime::currentTime().toString("hh:mm:ss") << "</td><td>" << string
       << "</td></tr>";

    emit showIncident(text);
}

void Notifications::appendLogFile(const QString string)
{
    if (!incidentLogfile->isOpen()) {
        incidentLogfile->setFileName(getWorkFolder() + "/incident.log");
        incidentLogfile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }

    QString text;
    QTextStream ts(&text);
    ts << QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss") << "\t" << string << "\n";
    incidentLogfile->write(text.toLocal8Bit());
    incidentLogfile->flush();
}

void Notifications::setupIncidentTable()
{
    emit showIncident("<tr><th width=50 align=left>Date</th><th width=50 align=left>Time</th><th width=100 align=left>Text</th></tr>");
    appendIncidentLog("Application started");
}

void Notifications::updSettings()
{
    server = getEmailSmtpServer();
    port = getEmailSmtpPort();
    recipients = getEmailRecipients();
    if (!workFolder.contains(getWorkFolder())) {
        workFolder = getWorkFolder();
        if (incidentLogfile->isOpen()) incidentLogfile->close(); // in case user changes folder settings
    }
}

