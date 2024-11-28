#include "accesshandler.h"

AccessHandler::AccessHandler(QSharedPointer<Config> c) {
    config = c;
    oauth2Flow = new QOAuth2AuthorizationCodeFlow(this);
    replyHandler = new MyReplyHandler(REDIRECT_PORT, this);
    oauth2Flow->setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256, 43);
    oauth2Flow->setReplyHandler(replyHandler);

    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, &QDesktopServices::openUrl);

    connect(oauth2Flow, &QOAuth2AuthorizationCodeFlow::granted, this, [this] () {
        token = oauth2Flow->token();
        replyHandler->close();
        emit authorizationGranted(token);
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
}

AccessHandler::~AccessHandler()
{
    delete oauth2Flow;
    delete replyHandler;
}

void AccessHandler::reqAuthorization()
{
    oauth2Flow->setModifyParametersFunction([=](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant>* params)
                                            {
                                                switch (stage)
                                                {
                                                case QAbstractOAuth::Stage::RequestingAuthorization:
                                                    //params->insert("code_challenge", code_challenge);
                                                    //params->insert("code_challenge_method", "S256");
                                                    //params->insert("sso_reload", "true");
                                                    //params->replace("response_type", "code id_token");
                                                    //params->replace("response_mode", "fragment");
                                                    break;
                                                case QAbstractOAuth::Stage::RequestingAccessToken:
                                                    //params->insert("code_verifier", "Y-7bx5264Xsa_cq1Ut-DSwwQQS4l8NXjCGxTYo0_uAI");
                                                    //params->insert("scope", "User.Read%20offline_access");
                                                    //QByteArray code = params->value("code").toByteArray();
                                                    //params->replace("code", QUrl::fromPercentEncoding(code));
                                                    break;
                                                }
                                                //params->replace("redirect_uri", QUrl::toPercentEncoding("http://localhost:4343/"));

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


