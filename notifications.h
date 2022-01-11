#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QString>
#include <QTextEdit>
#include <QTextStream>
#include <QDate>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include "config.h"
#include "SimpleMail/SimpleMail"

/*
 * This class takes care of journalling the incident log, both shown on screen and written to the
 * incident logfile. It also takes care of notifications per email, sorting what should be sent
 * and whatnot.
 */

class Notifications : public Config
{
    Q_OBJECT
public:
    explicit Notifications(QObject *parent = nullptr);

public slots:
    void start();
    void updSettings();
    void toIncidentLog(const QString id, const QString name, const QString string);

private slots:
    void appendIncidentLog(const QString string);
    void appendLogFile(const QString string);
    void setupIncidentTable();

signals:
    void showIncident(QString);

private:
    QFile *incidentLogfile = new QFile;

    // config cache
    QTextEdit *incidentLogPtr;
    QString server, port;
    QString recipients;
    QString workFolder;
};

#endif // NOTIFICATIONS_H
