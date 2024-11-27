#ifndef OAUTHFILEUPLOADER_H
#define OAUTHFILEUPLOADER_H

#include <QObject>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>

#include "config.h"
#include "typedefs.h"

#define AUTH_TIMEOUT 10000 // millisecs
#define UPLOAD_TIMEOUT 30000
#define UPLOAD_RETRY 15 * 60e3 // millisecs

class OAuthFileUploader : public QObject
{
    Q_OBJECT
public:
    OAuthFileUploader(QSharedPointer<Config>);
    void fileUploadRequest(const QString filename);
    void receiveAuthToken(const QString);

signals:
    void reqAuthToken();
    void toIncidentLog(const NOTIFY::TYPE, const QString, const QString);

private slots:
    void networkReplyFinishedHandler();
    void networkAccessManagerFinished();
    void networkReplyProgressHandler(qint64, qint64);
    void authTimeoutHandler();
    void uploadTimeoutHandler();

private:
    QSharedPointer<Config> config;
    QNetworkAccessManager *networkAccessManager;
    QNetworkReply *networkReply;
    QString uploadFilename;
    QFile *file;
    QHttpMultiPart *multipart;
    QTimer *authTimeoutTimer, *uploadTimeoutTimer;
};

#endif // OAUTHFILEUPLOADER_H
