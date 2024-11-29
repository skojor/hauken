#include "oauthfileuploader.h"

OAuthFileUploader::OAuthFileUploader(QSharedPointer<Config> c)
{
    config = c;

    networkAccessManager = new QNetworkAccessManager;
    authTimeoutTimer = new QTimer;
    authTimeoutTimer->setSingleShot(true);

    uploadTimeoutTimer = new QTimer;
    uploadTimeoutTimer->setSingleShot(true);

    connect(authTimeoutTimer, &QTimer::timeout, this, &OAuthFileUploader::authTimeoutHandler);
    connect(uploadTimeoutTimer, &QTimer::timeout, this, &OAuthFileUploader::uploadTimeoutHandler);
}

void OAuthFileUploader::fileUploadRequest(const QString filename)
{
    qDebug() << "We are here" << filename;
    QFile file (filename);
    if (!file.exists()) {
        qDebug() << "File not found:" << filename << ", checking if it has been zipped";
        file.setFileName(filename + ".zip");
    }
    else if (!file.exists()) {
        qDebug() << "Nope, no zip either. Giving up.";
        uploadFilename.clear();
    }
    else if (config->getOauth2Enable()) {
        authTimeoutTimer->start(AUTH_TIMEOUT);
        emit reqAuthToken();
        uploadFilename = filename;
    }
}

void OAuthFileUploader::receiveAuthToken(const QString token) // OAuth upload requested, asked for auth and received a (hopefully) valid token. Go ahead with the upload
{
    accessToken = token;
    authTimeoutTimer->stop();
    QHttpPart filePart;
    if (file == nullptr) file = new QFile(uploadFilename);
    else {
        file->close();
        file->setFileName(uploadFilename);
    }

    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't read from file" << uploadFilename <<", aborting";
    }
    else {
        if (multipart == nullptr) multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        filePart.setBodyDevice(file);
        QString justFilename;
        QStringList split = uploadFilename.split('/');
        justFilename = split.last();
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"files\"; filename=\"" + justFilename + "\"");
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
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
    if (total != 0) qDebug() << "upl progress" << 100 * up / total;
}

void OAuthFileUploader::networkReplyFinishedHandler()
{
    uploadTimeoutTimer->stop();
    QByteArray reply = networkReply->readAll();
    if (QString(reply).contains("uploaded", Qt::CaseInsensitive)) {
        qDebug() << "File uploaded successfully using OAuth!";
        emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "OAuth", "File uploaded successfully");

        QStringList list = QString(reply).split(':');
        QString id;
        if (list.size() > 2) {
            if (list[0].contains("id"))
                id = list[1];
            id.remove('\"');
            id = id.split(',').first();
            //qDebug() << id << accessToken << list;
            setOperator(id, accessToken);
        }
    }
    else if (QString(reply).contains("filealreadyexists", Qt::CaseInsensitive)) {
        qDebug() << "OAuth file upload failed, already uploaded" << reply;
        emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "OAuth", "File upload failed, already uploaded");
    }

    qDebug() << "Reply finished," << QDateTime::currentDateTime().toString() << networkReply->errorString() << reply;
}

void OAuthFileUploader::authTimeoutHandler()
{
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "OAuth", "File upload failed, retrying later. Reason: Authorization timed out");
    QTimer::singleShot(UPLOAD_RETRY, this, [this] () {
        fileUploadRequest(file->fileName());
    });
}

void OAuthFileUploader::uploadTimeoutHandler()
{
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "OAuth", "File upload failed, retrying later. Reason: Upload timed out");
    QTimer::singleShot(UPLOAD_RETRY, this, [this] () {
        fileUploadRequest(file->fileName());
    });
}

void OAuthFileUploader::setOperator(QString id, QString token)
{
    QUrl url("https://api.dev.intern.nkom.no/stoydata/api/files/operators");
    QNetworkRequest request;
    request.setUrl(url);

    request.setRawHeader(QByteArray("Authorization"), QByteArray("Bearer ") + token.toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QHttpPart part;
    QJsonObject jsonObject;
    QJsonArray jsonArray;
    jsonObject.insert("id", id);
    jsonArray.append(config->getSdefStationInitals());
    jsonObject.insert("operators", jsonArray);
    QJsonDocument doc(jsonObject);

    QHttpMultiPart *multip = new QHttpMultiPart;
    part.setBody(doc.toJson());
    multip->append(part);
    QNetworkReply *reply = networkAccessManager->post(request, doc.toJson(QJsonDocument::Compact));

    qDebug() << doc.toJson(QJsonDocument::Compact);

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError) {
            QString contents = QString::fromUtf8(reply->readAll());
            qDebug() << contents;
        }
        else {
            QString err = reply->errorString();
            qDebug() << err;
        }
        reply->deleteLater();
    });
}
