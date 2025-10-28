#include "networkrequestsbase.h"

NetworkRequestsBase::NetworkRequestsBase()
{
    connect(networkAccessManager,
            &QNetworkAccessManager::finished,
            this,
            &NetworkRequestsBase::networkAccessManagerReplyHandler);
    connect(tmNetworkTimeout, &QTimer::timeout, this, &NetworkRequestsBase::tmNetworkTimeoutHandler);
    tmNetworkTimeout->setSingleShot(true);
    networkAccessManager->setTransferTimeout(5000);
}

void NetworkRequestsBase::networkAccessManagerReplyHandler(QNetworkReply *reply)
{
    tmNetworkTimeout->stop();
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        qDebug() << "Response:" << data;
    }
    else {
        qDebug() << "Error:" << reply->errorString();
    }
}

void NetworkRequestsBase::tmNetworkTimeoutHandler()
{
    tmNetworkTimeout->stop();
    qDebug() << "NetworkRequestBase timeout";
}

void NetworkRequestsBase::postRequest(QNetworkRequest request, QByteArray text)
{
    tmNetworkTimeout->start(5000);
    QNetworkReply *reply = networkAccessManager->post(request, text);
    connect(reply, &QNetworkReply::errorOccurred, this, [reply] (QNetworkReply::NetworkError error) {
        qWarning() << "Network access manager failed:" << error;
    });
    connect(reply, &QNetworkReply::finished, this, [reply] () {
        reply->deleteLater();
    });
}
