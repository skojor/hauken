#include "oauthfileuploader.h"

OAuthFileUploader::OAuthFileUploader(QSharedPointer<Config> c)
{
    config = c;

    networkAccessManager = new QNetworkAccessManager;
    authTimeoutTimer = new QTimer;
    authTimeoutTimer->setSingleShot(true);

    uploadTimeoutTimer = new QTimer;
    uploadTimeoutTimer->setSingleShot(true);

    connect(networkAccessManager, &QNetworkAccessManager::finished, this, &OAuthFileUploader::networkAccessManagerFinished);
    connect(authTimeoutTimer, &QTimer::timeout, this, &OAuthFileUploader::authTimeoutHandler);
    connect(uploadTimeoutTimer, &QTimer::timeout, this, &OAuthFileUploader::uploadTimeoutHandler);
}

void OAuthFileUploader::fileUploadRequest(const QString filename)
{
    qDebug() << "We are here" << filename;
    QFile file (filename);
    if (!file.exists()) {
        qDebug() << "File not found:" << filename;
        uploadFilename.clear();
    }
    else if (config->getOauth2Enable()) {
        //authTimeoutTimer->start(AUTH_TIMEOUT);
        //emit reqAuthToken();
        uploadFilename = filename;
        receiveAuthToken("rairai");
    }
}
void OAuthFileUploader::receiveAuthToken(const QString token) // OAuth upload requested, asked for auth and received a (hopefully) valid token. Go ahead with the upload
{
    authTimeoutTimer->stop();
    QHttpPart filePart;
    file = new QFile(uploadFilename);

    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't read from file" << uploadFilename <<", aborting";
    }
    else {
        multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        filePart.setBodyDevice(file);

        multipart->append(filePart);
        QUrl url(config->getOauth2UploadAddress());
        QNetworkRequest request;
        request.setUrl(url);

        request.setRawHeader(QByteArray("Authorization"), QByteArray("Bearer ") + token.toLatin1());

        networkReply = networkAccessManager->post(request, multipart);
        uploadTimeoutTimer->start(UPLOAD_TIMEOUT);

        connect(networkReply, &QNetworkReply::finished, this, &OAuthFileUploader::networkReplyFinishedHandler);
        connect(networkReply, &QNetworkReply::uploadProgress, this, &OAuthFileUploader::networkReplyProgressHandler);
    }

    uploadFilename.clear();
}

void OAuthFileUploader::networkAccessManagerFinished()
{
    qDebug() << "Upload finished?";
}

void OAuthFileUploader::networkReplyProgressHandler(qint64 up, qint64 total)
{
    qDebug() << "upl progress" << up << total;
}

void OAuthFileUploader::networkReplyFinishedHandler()
{
    uploadTimeoutTimer->stop();
    qDebug() << "Upload finished at" << QDateTime::currentDateTime().toString();
    networkReply->deleteLater();
    file->deleteLater();
    multipart->deleteLater();
}

void OAuthFileUploader::authTimeoutHandler()
{
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "", "Authorization request timed out, trying again later");
    QTimer::singleShot(UPLOAD_RETRY, this, [this] () {
        fileUploadRequest(file->fileName());
    });
}

void OAuthFileUploader::uploadTimeoutHandler()
{
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "", "File upload timed out, trying again later");
    QTimer::singleShot(UPLOAD_RETRY, this, [this] () {
        fileUploadRequest(file->fileName());
    });
    file->deleteLater();
    networkReply->deleteLater();
    multipart->deleteLater();
}
