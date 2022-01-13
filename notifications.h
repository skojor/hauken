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
#include <QPixmap>
#include <QBuffer>
#include "config.h"
#include "typedefs.h"
#include "SimpleMail/SimpleMail"

/*
 * This class takes care of journalling the incident log, both shown on screen and written to the
 * incident logfile. It also takes care of notifications per email, sorting what should be sent
 * and whatnot.
 */

class NotificationsBuffer
{
public:
    NotificationsBuffer(NOTIFY::TYPE _t, QDateTime _dt, QString _id, QString _msg)
    {
        type = _t;
        timeReceived = _dt;
        id = _id;
        msg = _msg;
    }
    NOTIFY::TYPE type;
    QDateTime timeReceived;
    QString id;
    QString msg;
};

class Notifications : public Config
{
    Q_OBJECT
public:
    explicit Notifications(QObject *parent = nullptr);

public slots:
    void start();
    void updSettings();
    void toIncidentLog(const NOTIFY::TYPE type, const QString name, const QString string);
    void recTracePlot(const QPixmap *pic);
    //void recWaterfall(QPixmap *pic) { waterfall = pic;}

private slots:
    void appendIncidentLog(QDateTime dt, const QString string);
    void appendLogFile(QDateTime dt, const QString string);
    void appendEmailText(QDateTime dt, const QString string);
    void setupIncidentTable();
    bool simpleParametersCheck();
    void sendMail();
    void checkTruncate();
    void generateMsg(NOTIFY::TYPE t, const QString name, const QString string, QDateTime dt = QDateTime::currentDateTime());

signals:
    void showIncident(QString);
    void warning(QString);
    void reqTracePlot();
    void reqWaterfall();

private:
    QFile *incidentLogfile;
    QString mailtext;

    // config cache
    QString mailserverAddress, mailserverPort, smtpUser, smtpPass;
    QString recipients;
    QString fromAddress;
    QString workFolder;
    int truncateTime;
    QList<NotificationsBuffer> truncateList;
    QTimer *truncateTimer;
    QTimer *mailDelayTimer;
    QByteArray tracePlot;
    QByteArray waterfall;
};

#endif // NOTIFICATIONS_H
