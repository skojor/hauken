#include "accesshandler.h"

AccessHandler::AccessHandler(QSharedPointer<Config> c) {
    config = c;
    oauth2Flow = new QOAuth2AuthorizationCodeFlow;
    replyHandler = new MyReplyHandler(REDIRECT_PORT, this);

    //connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, &QDesktopServices::openUrl);
    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, [=](QUrl url) {
        QUrlQuery query(url);
        query.addQueryItem("prompt", "consent");      // Param required to get data everytime
        query.addQueryItem("access_type", "offline"); // Needed for Refresh Token (as AccessToken expires shortly)
        url.setQuery(query);
        QDesktopServices::openUrl(url);
    });

    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::granted, this, [this] () {
        token = oauth2Flow->token();
        replyHandler->close();
        emit authorizationGranted(token);
    });

    //connect(replyHandler, &MyReplyHandler::callbackReceived, oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizationCallbackReceived);
    connect(replyHandler, &MyReplyHandler::callbackReceived, this, [] (const QVariantMap &data){
        qDebug() << "reply callback" << data;
    });
    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::error, this, [] (const QString &error, const QString &errorDescription, const QUrl &uri) {
        qDebug() << "authflow error" << error << errorDescription << uri.toString();
    });
    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::requestFailed, this, [] (const QAbstractOAuth::Error error) {
        qDebug() << "Req failed" << (int)error;
    });
    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizationCallbackReceived, this, [] (const QVariantMap &data) {
        qDebug() << "authCallbackRec" << data;
    });

    oauth2Flow->setModifyParametersFunction([](QAbstractOAuth::Stage, QMultiMap<QString, QVariant>* parameters) {
        parameters->remove("redirect_uri");
        parameters->insert("redirect_uri", QUrl("http://localhost:4343/"));
    });
}

AccessHandler::~AccessHandler()
{
    delete oauth2Flow;
    delete replyHandler;
}

void AccessHandler::reqAuthorization()
{
    auto code_verifier = (QUuid::createUuid().toString(QUuid::WithoutBraces) +
                          QUuid::createUuid().toString(QUuid::WithoutBraces)).toLatin1(); // 43 <= length <= 128
    auto code_challenge = QCryptographicHash::hash(code_verifier, QCryptographicHash::Sha256).toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    oauth2Flow->setReplyHandler(replyHandler);
    oauth2Flow->setModifyParametersFunction([=](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant>* params)
                                            {
                                                switch (stage)
                                                {
                                                case QAbstractOAuth::Stage::RequestingAuthorization:
                                                    params->insert("code_challenge", code_challenge);
                                                    params->insert("code_challenge_method", "S256");
                                                    break;
                                                case QAbstractOAuth::Stage::RequestingAccessToken:
                                                    params->insert("code_verifier", code_verifier);
                                                    break;
                                                }
                                                params->remove("redirect_uri");
                                                params->insert("redirect_uri", QUrl("http://localhost:4343/"));
                                            });

    if (authEnabled && replyHandler->isListening())
        oauth2Flow->grant();
}

void AccessHandler::updSettings()
{
    setAuthorizationUrl(config->getOAuth2AuthUrl());
    setAccessTokenUrl(config->getOAuth2AccessTokenUrl());
    setClientIdentifier(config->getOAuth2ClientId());
    setScope(config->getOAuth2Scope());
    authEnabled = config->getOauth2Enable();
}
