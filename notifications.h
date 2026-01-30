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
#include <QFile>
#include <QDirIterator>
#include "config.h"
#include "typedefs.h"
#include "SimpleMail/SimpleMail"
#include <QProcess>
#include <QDir>
#include <QDataStream>
#include <QCoreApplication>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

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

class Notifications : public QObject
{
    Q_OBJECT
public:
    Notifications(QSharedPointer<Config>);

public slots:
    void start();
    void updSettings();
    void toIncidentLog(const NOTIFY::TYPE type, const QString name, const QString string);
    void recTracePlot(const QPixmap *pic);
    void recIqPlot(const QString filename) { iqPlotFilenames.append(filename);}
    //void recWaterfall(QPixmap *pic) { waterfall = pic;}
    void getLatitudeLongitude(bool valid, double lat, double lon) { positionValid = valid; latitude = lat; longitude = lon;}
    void recPrediction(QString pred, int prob);
    void setGnssPlotFilename(QString name) { gnssPlotFilename = name;}
    void setGnssPlotFilename2(QString name) { gnssPlotFilename2 = name;}

    void appendIncidentLog(QDateTime dt, const QString string);
    void appendLogFile(QDateTime dt, const QString string);
    void appendEmailText(QDateTime dt, const QString string);
    void setupIncidentTable();
    bool simpleParametersCheck();
    void sendMail();
    void checkTruncate();
    void generateMsg(NOTIFY::TYPE t, const QString name, const QString string, QDateTime dt = QDateTime::currentDateTime());
    void retryEmails();
    void authGraph();
    void sendMailWithGraph();
    void generateGraphEmail();
    void curlCallback(int exitCode, QProcess::ExitStatus);

signals:
    void showIncident(QString);
    void warning(QString);
    void reqTracePlot();
    void reqWaterfall();
    void reqPosition();

private:
    QFile *incidentLogfile = nullptr;
    QString mailtext;
    QTimer *timeBetweenEmailsTimer;
    QTimer *retryEmailsTimer;
    QList<SimpleMail::MimeInlineFile *> emailPictures;
    QList<SimpleMail::MimeInlineFile *> emailIqPlot;
    QList<SimpleMail::MimeMessage> emailBacklog;
    QString lastPicFilename;
    bool graphAuthenticated = false;
    QByteArray graphAccessToken;
    QByteArray mimeData;
    bool graphMailInProgress = false;
    QString htmlData;
    QList<QString> graphEmailLog;
    bool positionValid = false;
    double latitude = 0, longitude = 0;
    QSharedPointer<Config> config;

    QProcess *process;

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
    int delayBetweenEmails;
    bool msGraphConfigured = false, msGraphAuthorized = false;
    QString msGraphTenantId, msGraphApplicationId, msGraphSecret;

    QString prediction;
    int probability = 0;
    bool predictionReceived = false;
    bool notifyPriorityRecipients = false;
    QString iqPlotFilename, gnssPlotFilename, gnssPlotFilename2;
    QStringList iqPlotFilenames;
};

#endif // NOTIFICATIONS_H
