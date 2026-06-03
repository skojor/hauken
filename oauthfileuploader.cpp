#include "oauthfileuploader.h"
#include "asciitranslator.h"

#include <QAuthenticator>
#include <QUrlQuery>

#include <utility>

namespace {
constexpr qint64 PROBE_BLOCK_SIZE_BYTES = 4 * 1024 * 1024;
constexpr qint64 MIN_BLOCK_SIZE_BYTES = 4 * 1024 * 1024;
constexpr qint64 MAX_BLOCK_SIZE_BYTES = 95 * 1024 * 1024;
constexpr double BLOCK_OVERHEAD_SECONDS = 0.3;
constexpr double TARGET_BLOCK_UPLOAD_SECONDS = 60;
constexpr double SLOW_BLOCK_UPLOAD_SECONDS = 150;
constexpr double ROLLING_AVERAGE_WEIGHT = 0.3;
constexpr int REQUEST_TYPE_ATTRIBUTE = QNetworkRequest::User;

QString percentEncodedPathSegment(const QString &value)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(value));
}
}

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
        Q_UNUSED(auth);
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
        m_uploadBacklog.append(tmpFilename); // In case upload fails, this list keeps filenames still to be uploaded

        if (!m_uploadInProgress) {
            m_authTimeoutTimer->start(AUTH_TIMEOUT_MS);
            emit reqAuthToken();
        }
    }
}

void OAuthFileUploader::receiveAuthToken(const QString token)
// OAuth upload requested, asked for auth and received a (hopefully) valid token. Go ahead with the upload
{
    if (m_uploadBacklog.isEmpty()) return;

    m_accessToken = token;
    m_authTimeoutTimer->stop();
    startNextUpload();
}

void OAuthFileUploader::startNextUpload()
{
    if (m_uploadBacklog.isEmpty()) return;

    m_uploadInProgress = true;
    m_currentFilePath = m_uploadBacklog.first();
    m_currentFile = new QFile(m_currentFilePath, this);

    if (!m_currentFile->open(QIODevice::ReadOnly)) {
        qDebug() << "OAuthUploader: Couldn't read from file" << m_currentFilePath << ", aborting";
        abortCurrentUpload("Could not read file");
        return;
    }

    m_currentFileSize = m_currentFile->size();
    m_uploadedBytes = 0;
    m_currentBlockSize = PROBE_BLOCK_SIZE_BYTES;
    m_currentBlockBytes = 0;
    m_averageUploadSpeedBytesPerSecond = 0;
    m_nextBlockNumber = 0;
    m_blockIds.clear();
    m_currentUploadName = currentUploadFilename();
    m_fileUploadTimer.start();

    qDebug() << "OAuthUploader: Uploading" << m_currentUploadName << "as Azure block blob. File size:"
             << formatBytes(m_currentFileSize) << "initial block size:" << formatBytes(m_currentBlockSize);
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "",
                       "OAuth: Starting upload of " + m_currentUploadName + " (" + formatBytes(m_currentFileSize) + ")");
    uploadNextBlock();
}

void OAuthFileUploader::uploadNextBlock()
{
    if (m_currentFile == nullptr || !m_currentFile->isOpen()) {
        abortCurrentUpload("File is not open");
        return;
    }

    if (m_currentFile->atEnd()) {
        finishBlockListUpload();
        return;
    }

    const qint64 bytesLeft = m_currentFileSize - m_currentFile->pos();
    const QByteArray blockData = m_currentFile->read(qMin(m_currentBlockSize, bytesLeft));
    if (blockData.isEmpty()) {
        abortCurrentUpload("Could not read next upload block");
        return;
    }

    const QString blockId = makeBlockId(m_nextBlockNumber++);
    m_blockIds.append(blockId);

    QNetworkRequest request = makeAuthorizedRequest(blockUploadUrl(m_currentUploadName, blockId));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(REQUEST_TYPE_ATTRIBUTE),
                         static_cast<int>(UploadRequestType::Block));

    m_currentBlockBytes = blockData.size();
    m_blockUploadTimer.start();

    QNetworkReply *networkReply = m_networkAccessManager->put(request, blockData);
    m_uploadTimeoutTimer->start(UPLOAD_TIMEOUT_MS);

    connect(networkReply, &QNetworkReply::finished, networkReply, &QNetworkReply::deleteLater);
    connect(networkReply, &QNetworkReply::uploadProgress, this, &OAuthFileUploader::networkReplyProgressHandler);
}

void OAuthFileUploader::finishBlockListUpload()
{
    QByteArray blockListXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><BlockList>";
    for (const QString &blockId : std::as_const(m_blockIds)) {
        blockListXml += "<Latest>";
        blockListXml += blockId.toLatin1();
        blockListXml += "</Latest>";
    }
    blockListXml += "</BlockList>";

    QNetworkRequest request = makeAuthorizedRequest(blockListUrl(m_currentUploadName));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(REQUEST_TYPE_ATTRIBUTE),
                         static_cast<int>(UploadRequestType::BlockList));

    QNetworkReply *networkReply = m_networkAccessManager->put(request, blockListXml);
    m_uploadTimeoutTimer->start(UPLOAD_TIMEOUT_MS);

    connect(networkReply, &QNetworkReply::finished, networkReply, &QNetworkReply::deleteLater);
    connect(networkReply, &QNetworkReply::uploadProgress, this, &OAuthFileUploader::networkReplyProgressHandler);
}

void OAuthFileUploader::notifyFileUploaded()
{
    QNetworkRequest request = makeAuthorizedRequest(uploadedNotificationUrl());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(REQUEST_TYPE_ATTRIBUTE),
                         static_cast<int>(UploadRequestType::UploadedNotification));

    QJsonObject jsonObject;
    jsonObject.insert("filename", m_currentUploadName);
    jsonObject.insert("showMeasurementDataOnUpload", config->getOAuth2StartDataProcessingAfterUpload());
    QJsonDocument doc(jsonObject);

    QNetworkReply *networkReply = m_networkAccessManager->post(request, doc.toJson(QJsonDocument::Compact));
    m_uploadTimeoutTimer->start(UPLOAD_TIMEOUT_MS);

    connect(networkReply, &QNetworkReply::finished, networkReply, &QNetworkReply::deleteLater);
}

void OAuthFileUploader::networkReplyProgressHandler(qint64 up, qint64 total)
{
    Q_UNUSED(total);
    m_uploadTimeoutTimer->start(UPLOAD_TIMEOUT_MS); // Restarts timer as long as data keeps flowing

    const qint64 currentBlockUploaded = qMax<qint64>(0, up);
    const qint64 sentBytes = qMin(m_currentFileSize, m_uploadedBytes + currentBlockUploaded);
    if (m_currentFileSize > 0) {
        emit uploadProgress(static_cast<int>((sentBytes * 100) / m_currentFileSize));
    }
}

void OAuthFileUploader::networkReplyFinishedHandler(QNetworkReply *networkReply)
{
    const auto requestType = static_cast<UploadRequestType>(
        networkReply->request().attribute(static_cast<QNetworkRequest::Attribute>(REQUEST_TYPE_ATTRIBUTE),
                                          static_cast<int>(UploadRequestType::Unknown)).toInt());

    if (requestType == UploadRequestType::Unknown) return;

    m_uploadTimeoutTimer->stop();
    QByteArray reply = networkReply->readAll();

    if (networkReply->error() != QNetworkReply::NoError) {
        abortCurrentUpload(networkReply->errorString());
        return;
    }

    if (requestType == UploadRequestType::Block) {
        updateBlockSize(m_blockUploadTimer.elapsed());
        m_uploadedBytes = m_currentFile != nullptr ? m_currentFile->pos() : m_uploadedBytes;
        if (m_currentFileSize > 0) {
            emit uploadProgress(static_cast<int>((m_uploadedBytes * 100) / m_currentFileSize));
        }
        uploadNextBlock();
        return;
    }

    if (requestType == UploadRequestType::BlockList) {
        notifyFileUploaded();
        return;
    }

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

    const QString status = replyStatus.toString();
    if (status.contains("uploaded", Qt::CaseInsensitive)) {
        qDebug() << "OAuthUploader: File uploaded successfully using OAuth block blob upload!";
        finalizeSuccessfulUpload(replyFilename.toString(m_currentUploadName), "uploaded successfully");
    }
    else if (status.contains("filealreadyexists", Qt::CaseInsensitive)) {
        qDebug() << "OAuth: File upload failed, already uploaded" << reply;
        finalizeSuccessfulUpload(replyFilename.toString(m_currentUploadName), "already uploaded");
    }
    else {
        abortCurrentUpload(QString("Unexpected upload status '%1'").arg(status));
    }
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
    abortCurrentUpload("Upload timed out");
}

void OAuthFileUploader::setOperator(QString id, QString token)
{
    QUrl url(config->getOauth2OperatorAddress());
    QNetworkRequest request;
    request.setUrl(url);

    request.setRawHeader(QByteArray("Authorization"), QByteArray("Bearer ") + token.toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonObject;
    QJsonArray jsonArray;
    jsonObject.insert("id", id);
    jsonArray.append(config->getSdefStationInitals());
    jsonObject.insert("operators", jsonArray);
    QJsonDocument doc(jsonObject);

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
    if (!m_uploadBacklog.isEmpty()) {
        const QString finishedFile = m_currentFilePath.isEmpty() ? m_uploadBacklog.first() : m_currentFilePath;
        qDebug() << "OAuthUploader: Deleted" << m_uploadBacklog.removeAll(finishedFile)
                 << "occurrences from the backlog, left:" << m_uploadBacklog.size();
    }
}

void OAuthFileUploader::abortCurrentUpload(const QString &reason)
{
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "", "OAuth: File upload failed, retrying later. Reason: " + reason);
    qWarning() << "OAuthUploader: Failed upload," << reason;

    if (m_currentFile != nullptr) {
        m_currentFile->deleteLater();
        m_currentFile = nullptr;
    }

    m_currentFilePath.clear();
    m_currentUploadName.clear();
    m_blockIds.clear();
    m_currentFileSize = 0;
    m_uploadedBytes = 0;
    m_currentBlockSize = 0;
    m_currentBlockBytes = 0;
    m_averageUploadSpeedBytesPerSecond = 0;
    m_nextBlockNumber = 0;
    m_uploadInProgress = false;

    if (m_uploadBacklog.size() > 1) {
        m_authTimeoutTimer->start(AUTH_TIMEOUT_MS);
        QTimer::singleShot(300000, this, &OAuthFileUploader::reqAuthToken); // Request new token in 5 mins
    }
}

void OAuthFileUploader::finalizeSuccessfulUpload(const QString &uploadedFilename, const QString &status)
{
    emit uploadProgress(100);

    const double totalSeconds = m_fileUploadTimer.isValid() ? m_fileUploadTimer.elapsed() / 1000.0 : 0;
    const double averageSpeed = totalSeconds > 0 ? m_currentFileSize / totalSeconds : m_averageUploadSpeedBytesPerSecond;
    emit toIncidentLog(NOTIFY::TYPE::OAUTHFILEUPLOAD, "",
                       "OAuth: File " + status + " (" + uploadedFilename + ", size " +
                           formatBytes(m_currentFileSize) + ", average upload speed " + formatSpeed(averageSpeed) + ")");

    if (m_currentFile != nullptr) {
        m_currentFile->deleteLater();
        m_currentFile = nullptr;
    }

    cleanUploadBacklog();

    m_currentFilePath.clear();
    m_currentUploadName.clear();
    m_blockIds.clear();
    m_currentFileSize = 0;
    m_uploadedBytes = 0;
    m_currentBlockSize = 0;
    m_currentBlockBytes = 0;
    m_averageUploadSpeedBytesPerSecond = 0;
    m_nextBlockNumber = 0;
    m_uploadInProgress = false;

    if (m_uploadBacklog.size() > 0) {
        m_authTimeoutTimer->start(AUTH_TIMEOUT_MS);
        QTimer::singleShot(5000, this, &OAuthFileUploader::reqAuthToken); // Request new upload in 5 secs if more files are waiting
    }
}

QNetworkRequest OAuthFileUploader::makeAuthorizedRequest(const QUrl &url) const
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader(QByteArray("Authorization"), QByteArray("Bearer ") + m_accessToken.toLatin1());
    return request;
}

QUrl OAuthFileUploader::blockUploadUrl(const QString &filename, const QString &blockId) const
{
    QUrl url(config->getOauth2UploadAddress());

    if (url.path().contains("/api/files", Qt::CaseInsensitive) || !url.path().contains("/stoystorage", Qt::CaseInsensitive)) {
        url.setPath("/stoystorage/file/" + percentEncodedPathSegment(filename), QUrl::TolerantMode);
    }
    else if (url.path().contains("{filename}")) {
        url.setPath(url.path().replace("{filename}", percentEncodedPathSegment(filename)), QUrl::TolerantMode);
    }
    else if (!url.path().endsWith(QString("/") + filename)) {
        QString path = url.path();
        if (!path.endsWith('/')) path += '/';
        url.setPath(path + percentEncodedPathSegment(filename), QUrl::TolerantMode);
    }

    QUrlQuery query;
    query.addQueryItem("comp", "block");
    query.addQueryItem("blockid", blockId);
    url.setQuery(query);
    return url;
}

QUrl OAuthFileUploader::blockListUrl(const QString &filename) const
{
    QUrl url = blockUploadUrl(filename, QString());
    QUrlQuery query;
    query.addQueryItem("comp", "blocklist");
    url.setQuery(query);
    return url;
}

QUrl OAuthFileUploader::uploadedNotificationUrl() const
{
    QUrl url(config->getOauth2UploadAddress());
    QString path = url.path();

    if (path.endsWith("/api/files/upload", Qt::CaseInsensitive)) {
        path.chop(QString("/upload").length());
        path += "/uploaded";
    }
    else if (!path.endsWith("/api/files/uploaded", Qt::CaseInsensitive)) {
        if (path.contains("/api/files/", Qt::CaseInsensitive)) {
            path = path.left(path.indexOf("/api/files/", 0, Qt::CaseInsensitive)) + "/api/files/uploaded";
        }
        else if (path.contains("/stoystorage", Qt::CaseInsensitive)) {
            path = "/api/files/uploaded";
        }
        else {
            if (path.endsWith('/')) path.chop(1);
            path += "/api/files/uploaded";
        }
    }

    url.setPath(path);
    url.setQuery(QString());
    return url;
}

void OAuthFileUploader::updateBlockSize(qint64 elapsedMs)
{
    const double elapsedSeconds = elapsedMs / 1000.0;
    const double effectiveSeconds = qMax(0.001, elapsedSeconds - BLOCK_OVERHEAD_SECONDS);
    const double blockSpeed = m_currentBlockBytes / effectiveSeconds;

    if (m_averageUploadSpeedBytesPerSecond <= 0) {
        m_averageUploadSpeedBytesPerSecond = blockSpeed;
    }
    else {
        m_averageUploadSpeedBytesPerSecond =
            (m_averageUploadSpeedBytesPerSecond * (1 - ROLLING_AVERAGE_WEIGHT)) + (blockSpeed * ROLLING_AVERAGE_WEIGHT);
    }

    if (elapsedSeconds > SLOW_BLOCK_UPLOAD_SECONDS) {
        m_currentBlockSize = qMax(MIN_BLOCK_SIZE_BYTES, m_currentBlockSize / 2);
    }
    else {
        m_currentBlockSize = qBound(MIN_BLOCK_SIZE_BYTES,
                                    static_cast<qint64>(m_averageUploadSpeedBytesPerSecond * TARGET_BLOCK_UPLOAD_SECONDS),
                                    MAX_BLOCK_SIZE_BYTES);
    }

    qDebug() << "OAuthUploader: Uploaded block" << m_nextBlockNumber << "of" << m_currentUploadName
             << "in" << QString::number(elapsedSeconds, 'f', 1) << "seconds. Speed:"
             << formatSpeed(blockSpeed) << "rolling average:" << formatSpeed(m_averageUploadSpeedBytesPerSecond)
             << "next block size:" << formatBytes(m_currentBlockSize);
}

QString OAuthFileUploader::formatBytes(qint64 bytes) const
{
    double value = bytes;
    QString unit = "B";

    if (value >= 1024) {
        value /= 1024;
        unit = "KiB";
    }
    if (value >= 1024) {
        value /= 1024;
        unit = "MiB";
    }
    if (value >= 1024) {
        value /= 1024;
        unit = "GiB";
    }

    return QString::number(value, 'f', unit == "B" ? 0 : 1) + " " + unit;
}

QString OAuthFileUploader::formatSpeed(double bytesPerSecond) const
{
    return formatBytes(static_cast<qint64>(bytesPerSecond)) + "/s";
}

QString OAuthFileUploader::currentUploadFilename() const
{
    QStringList split = m_currentFilePath.split('/');
    return AsciiTranslator::toAscii(split.last()); // Don't send UTF-8 encoded filename in upload URLs
}

QString OAuthFileUploader::makeBlockId(int blockNumber) const
{
    const QByteArray plainId = QByteArray::number(blockNumber).rightJustified(6, '0');
    return QString::fromLatin1(plainId.toBase64());
}
