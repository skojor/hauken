#include "accesshandler.h"

AccessHandler::AccessHandler(QSharedPointer<Config> c) {
    config = c;
    oauth2Flow = new QOAuth2AuthorizationCodeFlow(this);
    replyHandler = new QOAuthHttpServerReplyHandler(this);
    oauth2Flow->setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256, 43);
    replyHandler->listen(QHostAddress::Any, REDIRECT_PORT);
    oauth2Flow->setReplyHandler(replyHandler);
    renewTokenTimer = new QTimer();
    renewTokenTimer->setSingleShot(true);

    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, &QDesktopServices::openUrl);

    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::granted, this, [this] () {
        qDebug() << "We have a token, expires at" << oauth2Flow->expirationAt().toString();// << oauth2Flow->refreshToken();
        replyHandler->close();
        emit authorizationGranted(oauth2Flow->token());
        renewTokenTimer->start(
            QDateTime::currentDateTime().msecsTo(oauth2Flow->expirationAt().addSecs(
                -60))); // Set a renew timer for 1 min before expiry time
        qDebug() << "Adding exp. timer in" << renewTokenTimer->remainingTime() / 1e3 << "seconds";
    });

    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::error, this, [] (const QString &error, const QString &errorDescription, const QUrl &uri) {
        qDebug() << "authflow error" << error << errorDescription << uri.toString();
    });
    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::requestFailed, this, [] (const QAbstractOAuth::Error error) {
        qDebug() << "Req failed" << (int)error;
    });
    /*connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizationCallbackReceived, this, [this] (const QVariantMap &data) {
        qDebug() << "authCallbackRec" << data << oauth2Flow->refreshToken();
    });*/

    connect(renewTokenTimer, &QTimer::timeout, this, [this] {
        qDebug() << "Asking for new token";
        oauth2Flow->refreshAccessToken();
    });
}

AccessHandler::~AccessHandler()
{
    delete oauth2Flow;
    delete replyHandler;
}

void AccessHandler::reqAuthorization()
{
    if (!replyHandler->isListening())
        replyHandler->listen(QHostAddress::Any, REDIRECT_PORT);

    if (authEnabled && replyHandler->isListening() && oauth2Flow->token().isEmpty()) {
        oauth2Flow->grant();
    }
    else if (authEnabled && replyHandler->isListening() && !oauth2Flow->refreshToken().isEmpty()) { // Should already be auth'ed
        qDebug() << "Auth: We are already granted access";
        emit authorizationGranted(oauth2Flow->token());
    }
}

void AccessHandler::updSettings()
{
    setAuthorizationUrl(config->getOAuth2AuthUrl());
    setAccessTokenUrl(config->getOAuth2AccessTokenUrl());
    setClientIdentifier(config->getOAuth2ClientId());
    setScope(config->getOAuth2Scope() + " offline_access");
    authEnabled = config->getOauth2Enable();

    // Parameters check
    if (authEnabled && (config->getOAuth2AuthUrl().isEmpty() || config->getOAuth2AccessTokenUrl().isEmpty() ||
                        config->getOAuth2ClientId().isEmpty() || config->getOAuth2Scope().isEmpty())) {
        //qDebug() << "OAuth2: Missing one or more parameters, disabling";
        authEnabled = false;
    }
}


