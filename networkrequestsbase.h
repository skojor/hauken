#ifndef NETWORKREQUESTSBASE_H
#define NETWORKREQUESTSBASE_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QTimer>
#include <QDebug>
#include <QList>

class NetworkRequestsBase : public QObject
{
    Q_OBJECT

public slots:
    virtual void networkAccessManagerReplyHandler(QNetworkReply *reply);
    void tmNetworkTimeoutHandler();
    virtual void createNetworkRequest(const QString host) {}
    virtual void createPost() {}
    virtual void createGet() {}
    void postRequest(QNetworkRequest request, QByteArray text);

public:
    NetworkRequestsBase();
    QNetworkAccessManager *networkAccessManager = new QNetworkAccessManager;
    QTimer *tmNetworkTimeout = new QTimer;
};

#endif // NETWORKREQUESTSBASE_H
