#include "oauthfileuploader.h"
#include "asciitranslator.h"


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
    QString tmpFilename = filename;
    //qDebug() << "We are here" << tmpFilename;
    QFile file(tmpFilename);
    if (!file.exists()) {
        qDebug() << "File not found:" << tmpFilename << ", checking if it has been zipped";
        tmpFilename = filename + ".zip";
    }

    file.setFileName(tmpFilename);
    if (!file.exists()) {
        qDebug() << "Nope, no zip either. Giving up.";
    } else if (config->getOauth2Enable()) {
        authTimeoutTimer->start(AUTH_TIMEOUT);
        uploadBacklog.append(
            tmpFilename); // In case upload fails, this list keeps filenames still to be uploaded
        emit reqAuthToken();
    }
}

void OAuthFileUploader::receiveAuthToken(
    const QString
        token) // OAuth upload requested, asked for auth and received a (hopefully) valid token. Go ahead with the upload
{
    if (!uploadBacklog.isEmpty()) {
        accessToken = token;
        authTimeoutTimer->stop();
        QHttpPart filePart;
        QFile *file = new QFile(uploadBacklog.last());

        if (!file->open(QIODevice::ReadOnly)) {
            qDebug() << "Couldn't read from file" << uploadBacklog.last() << ", aborting";
            cleanUploadBacklog();
        } else {
            QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
            //if (multipart == nullptr) multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

            filePart.setBodyDevice(file);
            QString justFilename;
            QStringList split = uploadBacklog.last().split('/');
            justFilename = AsciiTranslator::toAscii(split.last()); // New, don't send UTF-8 encoded filename in HttpMultiPart

            filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               "form-data; name=\"files\"; filename=\"" + justFilename + "\"");
            filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            multipart->append(filePart);

            QUrl url(config->getOauth2UploadAddress());
            QNetworkRequest request;
            request.setUrl(url);

            request.setRawHeader(QByteArray("Authorization"),
                                 QByteArray("Bearer ") + token.toLatin1());

            networkReply = networkAccessManager->post(request, multipart);
            uploadTimeoutTimer->start(UPLOAD_TIMEOUT);

            connect(networkReply, &QNetworkReply::finished, this, [this]() {
                networkReplyFinishedHandler();
            });
            connect(networkReply,
                    &QNetworkReply::uploadProgress,
                    this,
                    &OAuthFileUploader::networkReplyProgressHandler);
        }
    }
}

void OAuthFileUploader::networkAccessManagerFinished()
{
    qDebug() << "Upload finished?";
}

void OAuthFileUploader::networkReplyProgressHandler(qint64 up, qint64 total)
{
    /*if (total != 0) {
        emit uploadProgress(100 * up / total);
        //qDebug() << "upl progress" << 100 * up / total;
    }*/
}

void OAuthFileUploader::networkReplyFinishedHandler()
{
    uploadTimeoutTimer->stop();
    QByteArray reply = networkReply->readAll();
    if (QString(reply).contains("uploaded", Qt::CaseInsensitive)) {
        qDebug() << "File uploaded successfully using OAuth!";
        emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "", "OAuth: File uploaded successfully");
        cleanUploadBacklog();

        QStringList list = QString(reply).split(':');
        QString id;
        if (list.size() > 2) {
            if (list[0].contains("id"))
                id = list[1];
            id.remove('\"');
            id = id.split(',').first();
            //qDebug() << id << accessToken << list;
            if (!config->getOauth2OperatorAddress().isEmpty()
                && !id.contains("null", Qt::CaseInsensitive))
                setOperator(id, accessToken); // Add operator if url is specified
        }
    } else if (QString(reply).contains("filealreadyexists", Qt::CaseInsensitive)) {
        qDebug() << "OAuth file upload failed, already uploaded" << reply;
        emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD,
                           "",
                           "OAuth: File upload failed, already uploaded");
        cleanUploadBacklog();
    }

    qDebug() << "Reply finished," << QDateTime::currentDateTime().toString()
             << networkReply->errorString() << reply;
    /*if (!uploadBacklog.isEmpty()) {
        QTimer::singleShot(UPLOAD_RETRY,
                           this,
                           [this]() { // Call uploader in x min if queue is not empty
                               if (!uploadBacklog.isEmpty())
                                   fileUploadRequest(uploadBacklog.last());
                           });
    }*/
    //networkReply->deleteLater();
}

void OAuthFileUploader::authTimeoutHandler()
{
    emit
        toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD,
                      "",
                      "OAuth: File upload failed, retrying later. Reason: Authorization timed out");
    /*QTimer::singleShot(UPLOAD_RETRY, this, [this]() {
        if (!uploadBacklog.isEmpty())
            fileUploadRequest(uploadBacklog.last());
    });*/
}

void OAuthFileUploader::uploadTimeoutHandler()
{
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD,
                       "",
                       "OAuth: File upload failed, retrying later. Reason: Upload timed out");
    /*QTimer::singleShot(UPLOAD_RETRY, this, [this]() {
        if (!uploadBacklog.isEmpty())
            fileUploadRequest(uploadBacklog.last());
    });*/
}

void OAuthFileUploader::setOperator(QString id, QString token)
{
    QUrl url(config->getOauth2OperatorAddress());
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

    //qDebug() << doc.toJson(QJsonDocument::Compact);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString contents = QString::fromUtf8(reply->readAll());
            //qDebug() << contents;
        } else {
            QString err = reply->errorString();
           // qDebug() << err;
        }
        reply->deleteLater();
    });
}

void OAuthFileUploader::cleanUploadBacklog()
{
    if (!uploadBacklog.isEmpty())
        qDebug() << "OAuth debug: Deleted" << uploadBacklog.removeAll(uploadBacklog.last())
                 << "occurrences from the backlog";
}
