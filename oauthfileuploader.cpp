#include "oauthfileuploader.h"
#include "asciitranslator.h"


OAuthFileUploader::OAuthFileUploader(QSharedPointer<Config> c)
{
    config = c;

    m_networkAccessManager = new QNetworkAccessManager;
    m_authTimeoutTimer = new QTimer;
    m_authTimeoutTimer->setSingleShot(true);

    m_uploadTimeoutTimer = new QTimer;
    m_uploadTimeoutTimer->setSingleShot(true);

    connect(m_authTimeoutTimer, &QTimer::timeout, this, &OAuthFileUploader::authTimeoutHandler);
    connect(m_uploadTimeoutTimer, &QTimer::timeout, this, &OAuthFileUploader::uploadTimeoutHandler);
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this, &OAuthFileUploader::networkReplyFinishedHandler);
    connect(m_networkAccessManager, &QNetworkAccessManager::authenticationRequired, this, [] (QNetworkReply *reply, QAuthenticator *auth) {
        qDebug() << "OAuthUploader: Auth required!" << reply->errorString();
    });
    //etworkAccessManager->setAutoDeleteReplies(true);
    m_networkAccessManager->setTransferTimeout(5000);
}

void OAuthFileUploader::fileUploadRequest(const QString filename)
{
    QString tmpFilename = filename;
    QFile file(tmpFilename);
    if (!file.exists()) {
        qDebug() << "OAuthUploader: File not found:" << tmpFilename << ", checking if it has been zipped";
        tmpFilename = filename + ".zip";
    }

    file.setFileName(tmpFilename);
    if (!file.exists()) {
        qDebug() << "OAuthUploader: Nope, no zip either. Giving up.";
    } else if (config->getOauth2Enable()) {
        m_authTimeoutTimer->start(AUTH_TIMEOUT_MS);
        m_uploadBacklog.append(
            tmpFilename); // In case upload fails, this list keeps filenames still to be uploaded
        emit reqAuthToken();
    }
}

void OAuthFileUploader::receiveAuthToken(const QString token)
// OAuth upload requested, asked for auth and received a (hopefully) valid token. Go ahead with the upload
{
    if (!m_uploadBacklog.isEmpty()) {
        m_accessToken = token;
        m_authTimeoutTimer->stop();
        QHttpPart filePart;
        QFile *file = new QFile(m_uploadBacklog.last());

        if (!file->open(QIODevice::ReadOnly)) {
            qDebug() << "OAuthUploader: Couldn't read from file" << m_uploadBacklog.last() << ", aborting";
            cleanUploadBacklog();
        } else {
            QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
            //if (multipart == nullptr) multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

            filePart.setBodyDevice(file);
            QString justFilename;
            QStringList split = m_uploadBacklog.last().split('/');
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

            //qDebug() << request.headers();

            QNetworkReply *networkReply = m_networkAccessManager->post(request, multipart);
            multipart->setParent(networkReply);
            file->setParent(networkReply);

            m_uploadTimeoutTimer->start(UPLOAD_TIMEOUT_MS);

            /*connect(networkReply, &QNetworkReply::socketStartedConnecting, this, [] () {
                qDebug() << "OAuthUploader: Socket connecting";
            });*/
            connect(networkReply, &QNetworkReply::finished, this, [networkReply] () {
                networkReply->deleteLater();
            });
            connect(networkReply, &QNetworkReply::uploadProgress, this, &OAuthFileUploader::networkReplyProgressHandler);
        }
    }
}

void OAuthFileUploader::networkReplyProgressHandler(qint64 up, qint64 total)
{
    m_uploadTimeoutTimer->start(UPLOAD_TIMEOUT_MS); // Restarts timer as long as data keeps flowing
}

void OAuthFileUploader::networkReplyFinishedHandler(QNetworkReply *networkReply)
{
    m_uploadTimeoutTimer->stop();
    QByteArray reply = networkReply->readAll();

    if (networkReply->error() == QNetworkReply::NoError) {
        QJsonObject jsonObject;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply);

        if (jsonDoc.isArray()) {
            QJsonArray arr = jsonDoc.array();
            if (!arr.isEmpty()) {
                jsonObject = arr.first().toObject();
            }
        }
        else {
            jsonObject = jsonDoc.object();
        }
        QJsonValue replyStatus = jsonObject.value("status");
        QJsonValue replyFilename = jsonObject.value("fileName");

        if (replyStatus.toString().contains("uploaded", Qt::CaseInsensitive)) {
            qDebug() << "OAuthUploader: File uploaded successfully using OAuth!";
            emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "", "OAuth: File uploaded successfully (" + replyFilename.toString() + ")");
            if (m_uploadBacklog.size() > 1) QTimer::singleShot(5000, this, &OAuthFileUploader::reqAuthToken); // Request new upload in 5 secs if more files are waiting
        }

        else if (replyStatus.toString().contains("filealreadyexists", Qt::CaseInsensitive)) {
            qDebug() << "OAuth: File upload failed, already uploaded" << reply;
            emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "", "OAuth: File upload failed, already uploaded (" + replyFilename.toString() + ")");
        }
        cleanUploadBacklog();
    }
    else { // Upload failed for some reason. Inform user and retry again later
        emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "", "OAuth: File upload failed, retrying later. Reason: " + networkReply->errorString());
        qWarning() << "OAuthUploader: Failed upload," << networkReply->errorString();
    }

/*    if (QString(reply).contains("uploaded", Qt::CaseInsensitive)) {
        qDebug() << "OAuthUploader: File uploaded successfully using OAuth!";
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
                setOperator(id, m_accessToken); // Add operator if url is specified
        }
    } else if (QString(reply).contains("filealreadyexists", Qt::CaseInsensitive)) {
        qDebug() << "OAuth: File upload failed, already uploaded" << reply;
        emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD,
                           "",
                           "OAuth: File upload failed, already uploaded");
        cleanUploadBacklog();
    }

    qDebug() << "OAuthUploader: Reply finished," << QDateTime::currentDateTime().toString()
             << networkReply->errorString() << reply;*/
}

void OAuthFileUploader::authTimeoutHandler()
{
    emit
        toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD,
                      "",
                      "OAuth: File upload failed, retrying later. Reason: Authorization timed out");
}

void OAuthFileUploader::uploadTimeoutHandler()
{
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD,
                       "",
                       "OAuth: File upload failed, retrying later. Reason: Upload timed out");
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
    QNetworkReply *reply = m_networkAccessManager->post(request, doc.toJson(QJsonDocument::Compact));

    //qDebug() << doc.toJson(QJsonDocument::Compact);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        /*if (reply->error() == QNetworkReply::NoError) {
            //QString contents = QString::fromUtf8(reply->readAll());
            //qDebug() << contents;
        } else {
            //QString err = reply->errorString();
           // qDebug() << err;
        }*/
        reply->deleteLater();
    });
}

void OAuthFileUploader::cleanUploadBacklog()
{
    if (!m_uploadBacklog.isEmpty())
        qDebug() << "OAuthUploader: Deleted" << m_uploadBacklog.removeAll(m_uploadBacklog.last())
                 << "occurrences from the backlog, left:" << m_uploadBacklog.size();
}
