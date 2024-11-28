#ifndef ACCESSHANDLER_H
#define ACCESSHANDLER_H

#include <QObject>
#include <QNetworkReply>
#include <QOAuth2AuthorizationCodeFlow>
#include <QString>
#include <QDir>
#include <QUrl>
#include <QOAuthHttpServerReplyHandler>
#include <QDesktopServices>
#include <QUuid>
#include <QUrlQuery>
#include "config.h"

#define REDIRECT_PORT 4343

class MyReplyHandler : public QOAuthHttpServerReplyHandler
{
public:
    MyReplyHandler() = default;
    using MyReplyHandler::QOAuthHttpServerReplyHandler::QOAuthHttpServerReplyHandler;

    ~MyReplyHandler() {}

    QString callback() { return "http://localhost:4343/"; }
};

class AccessHandler : public QObject
{
    Q_OBJECT
public:
    AccessHandler(QSharedPointer<Config> c);

    ~AccessHandler();

    void setAuthorizationUrl(QString s) { oauth2Flow->setAuthorizationUrl(QUrl(s));}
    void setAccessTokenUrl(QString s) { oauth2Flow->setAccessTokenUrl(QUrl(s));}
    void setClientIdentifier(QString s) { oauth2Flow->setClientIdentifier(s);}
    void setScope(QString s) { oauth2Flow->setScope(s);}

    void updSettings();

public slots:
    void reqAuthorization();

private slots:

signals:
    void authorizationGranted(const QString &token);

private:
    QOAuth2AuthorizationCodeFlow *oauth2Flow;
    QOAuthHttpServerReplyHandler *replyHandler;
    QSharedPointer<Config> config;
    bool authEnabled = false;
    QString token;
    QString codeChallenge, codeVerifer;
};

#endif // ACCESSHANDLER_H
