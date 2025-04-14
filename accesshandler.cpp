#include "accesshandler.h"

AccessHandler::AccessHandler(QSharedPointer<Config> c)
{
    config = c;
    oauth2Flow = new QOAuth2AuthorizationCodeFlow(this);
    replyHandler = new QOAuthHttpServerReplyHandler(this);
    rewriteHeader();
    //qDebug() << "port" << replyHandler->port();

    oauth2Flow->setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256, 43);
    oauth2Flow->setReplyHandler(replyHandler);
    renewTokenTimer = new QTimer();
    renewTokenTimer->setSingleShot(true);

    connect(oauth2Flow,
            &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this,
            &QDesktopServices::openUrl);

    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::granted, this, [this]() {
        if (QDateTime::currentDateTime().msecsTo(oauth2Flow->expirationAt()) < 6e5) {
            qDebug() << "Access handler: Lib says granted, but expiry time is less than 10 "
                        "minutes. Requesting new auth";
            oauth2Flow->setToken(QString());
            reqAuthorization();
        } else {
            qDebug() << "We have a token, expires at" << oauth2Flow->expirationAt().toString();
            replyHandler->close();
            emit authorizationGranted(oauth2Flow->token());
            renewTokenTimer->start(
                QDateTime::currentDateTime().msecsTo(oauth2Flow->expirationAt().addSecs(
                    -60))); // Set a renew timer for 1 min before expiry time
            qDebug() << "Adding exp. timer in" << renewTokenTimer->remainingTime() / 1e3
                     << "seconds";
        }
    });

    connect(oauth2Flow,
            &QOAuth2AuthorizationCodeFlow::serverReportedErrorOccurred,
            this,
            [](const QString &error, const QString &errorDescription, const QUrl &uri) {
                qDebug() << "authflow error" << error << errorDescription << uri.toString();
            });
    connect(oauth2Flow,
            &QOAuth2AuthorizationCodeFlow::requestFailed,
            this,
            [](const QAbstractOAuth::Error error) { qDebug() << "Req failed" << (int) error; });

    connect(renewTokenTimer, &QTimer::timeout, this, [this] {
        qDebug() << "Asking for new token";
        oauth2Flow->refreshTokens();
    });
}

AccessHandler::~AccessHandler()
{
    delete oauth2Flow;
    delete replyHandler;
}

void AccessHandler::reqAuthorization()
{
    if (!replyHandler->isListening()) {
        replyHandler->listen(QHostAddress::Any, REDIRECT_PORT);
        rewriteHeader(); // Change 127.0.0.1 to localhost, because Azure
    }

    if (authEnabled && replyHandler->isListening() && oauth2Flow->token().isEmpty()) {
        oauth2Flow->grant();
    } else if (authEnabled && replyHandler->isListening()
               && !oauth2Flow->refreshToken().isEmpty()) { // Should already be auth'ed
        qDebug() << "Auth: We are already granted access";
        emit authorizationGranted(oauth2Flow->token());
    }
}

void AccessHandler::updSettings()
{
    setAuthorizationUrl(config->getOAuth2AuthUrl());
    setAccessTokenUrl(config->getOAuth2AccessTokenUrl());
    setClientIdentifier(config->getOAuth2ClientId());
    setScope(QSet<QByteArray>() << config->getOAuth2Scope().toLatin1()
                                << QByteArray("offline_access"));
    authEnabled = config->getOauth2Enable();

    // Parameters check
    if (authEnabled
        && (config->getOAuth2AuthUrl().isEmpty() || config->getOAuth2AccessTokenUrl().isEmpty()
            || config->getOAuth2ClientId().isEmpty() || config->getOAuth2Scope().isEmpty())) {
        //qDebug() << "OAuth2: Missing one or more parameters, disabling";
        authEnabled = false;
    }
}

void AccessHandler::rewriteHeader()
{
    oauth2Flow->setModifyParametersFunction(
        [this](QAbstractOAuth::Stage, QMultiMap<QString, QVariant> *parameters) {
            parameters->remove("redirect_uri");
            parameters->insert("redirect_uri",
                               QUrl("http://localhost:" + QByteArray::number(replyHandler->port())
                                    + "/"));
        });
}
