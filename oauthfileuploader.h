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
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "config.h"
#include "typedefs.h"

#define AUTH_TIMEOUT 10000 // millisecs
#define UPLOAD_TIMEOUT 60000
#define UPLOAD_RETRY 20 * 60 * 1e3 // millisecs

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
    void uploadProgress(int);

private slots:
    void networkReplyFinishedHandler();
    void networkAccessManagerFinished();
    void networkReplyProgressHandler(qint64, qint64);
    void authTimeoutHandler();
    void uploadTimeoutHandler();
    void setOperator(QString id, QString token);

private:
    QSharedPointer<Config> config;
    QNetworkAccessManager *networkAccessManager;
    QNetworkReply *networkReply = nullptr;
    //QString uploadFilename;
    //QFile *file = nullptr;
    //QHttpMultiPart *multipart = nullptr;
    QTimer *authTimeoutTimer, *uploadTimeoutTimer;
    QString accessToken;
    QStringList uploadBacklog;
};

#endif // OAUTHFILEUPLOADER_H
