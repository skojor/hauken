#ifndef OAUTHFILEUPLOADER_H
#define OAUTHFILEUPLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "config.h"
#include "typedefs.h"


#define AUTH_TIMEOUT_MS 10000
#define UPLOAD_TIMEOUT_MS 30000
#define UPLOAD_RETRY_MS 120 * 60 * 1e3

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
    void networkReplyFinishedHandler(QNetworkReply *networkReply);
    void networkReplyProgressHandler(qint64, qint64);
    void startNextUpload();
    void uploadNextBlock();
    void finishBlockListUpload();
    void notifyFileUploaded();
    void authTimeoutHandler();
    void uploadTimeoutHandler();
    void setOperator(QString id, QString token);
    void cleanUploadBacklog();

private:
    QSharedPointer<Config> config;
    QNetworkAccessManager *m_networkAccessManager;
    QTimer *m_authTimeoutTimer, *m_uploadTimeoutTimer;
    QString m_accessToken;
    QStringList m_uploadBacklog;
    enum class UploadRequestType {
        Unknown = 0,
        Block,
        BlockList,
        UploadedNotification
    };

    void abortCurrentUpload(const QString &reason);
    void finalizeSuccessfulUpload(const QString &uploadedFilename, const QString &status);
    QNetworkRequest makeAuthorizedRequest(const QUrl &url) const;
    QUrl blockUploadUrl(const QString &filename, const QString &blockId) const;
    QUrl blockListUrl(const QString &filename) const;
    QUrl uploadedNotificationUrl() const;
    QString currentUploadFilename() const;
    QString makeBlockId(int blockNumber) const;
    void updateBlockSize(qint64 elapsedMs);
    QString formatBytes(qint64 bytes) const;
    QString formatSpeed(double bytesPerSecond) const;

    QFile *m_currentFile = nullptr;
    QString m_currentFilePath;
    QString m_currentUploadName;
    QStringList m_blockIds;
    qint64 m_currentFileSize = 0;
    qint64 m_uploadedBytes = 0;
    qint64 m_currentBlockSize = 0;
    qint64 m_currentBlockBytes = 0;
    double m_averageUploadSpeedBytesPerSecond = 0;
    QElapsedTimer m_blockUploadTimer;
    QElapsedTimer m_fileUploadTimer;
    int m_nextBlockNumber = 0;
    bool m_uploadInProgress = false;
    int m_retries = 0;
};

#endif // OAUTHFILEUPLOADER_H
