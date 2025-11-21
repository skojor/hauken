#include "AccessHandler.h"

AccessHandler::AccessHandler(QObject *parent, QSharedPointer<Config> c)
    : QObject(parent)
{
    if (c)
        m_config = c;
    else
        qFatal() << "AccessHandler: No config pointer set, giving up";

    connect(m_stateTimer, &QTimer::timeout, this, &AccessHandler::stateHandler);
    connect(m_timeoutTimer, &QTimer::timeout, this, [this] () {
        m_timeoutTimer->stop();
        m_stateTimer->stop();
        m_state = IDLE;
        qWarning() << "AccessHandler: Timeout while running authorization routine, retrying later";
        emit accessTokenInvalid("Authorization timed out");
        QTimer::singleShot(5 * 60 * 1000, this, &AccessHandler::login); // 5 min retry
    });

    connect(m_debugTimer, &QTimer::timeout, this, [this] () {
        qDebug() << "AccessHandler debug:" << m_ctx.token << m_ctx.expiryTime.toLocalTime().toString();
    });
    //m_debugTimer->start(60000);
}

void AccessHandler::updSettings()
{
    m_appId = m_config->getOAuth2ClientId().toStdWString();
    m_authority = m_config->getOAuth2AuthUrl().toStdWString();
    m_scope = m_config->getOAuth2Scope().toStdWString();

    if (m_loginEnabled != m_config->getOauth2Enable()) {
        m_loginEnabled = m_config->getOauth2Enable();
        if (m_loginEnabled) { // Attempt login
            login();
        }
        else {  // Was enabled, now disabled. Stop any renewal timers
            m_timeoutTimer->stop();
            m_stateTimer->stop();
            emit settingsInvalid("OAuth disabled in settings");
            m_ctx.expiryTime = QDateTime::currentDateTime(); // Expired
            m_ctx.token.clear();
        }
    }
}

void AccessHandler::login()
{
    if (m_config->getOauth2Enable()) {
        if (!m_appId.empty() && !m_authority.empty() && !m_scope.empty()) {
            // Init MSAL
            m_correlationId = uuidGen();
            MSALRUNTIME_Startup();
            MSALRUNTIME_SetIsPiiEnabled(false);
            MSALRUNTIME_CreateAuthParameters(m_appId.c_str(), m_authority.c_str(), &m_authParameters);
            MSALRUNTIME_SetRequestedScopes(m_authParameters, m_scope.c_str());
            MSALRUNTIME_SetRedirectUri(m_authParameters, L"placeholder");
            MSALRUNTIME_SetAdditionalParameter(
                m_authParameters, L"msal_gui_thread", L"true");

            m_state = IDLE;
            m_stateTimer->start(100);
            m_timeoutTimer->start(TIMEOUT_MS);
            emit reqAccessToken();
        }
        else {
            qWarning() << "Auth requested, but parameters are not set. Giving up";
            emit settingsInvalid("One or more OAuth values not set, not possible to log in");
        }
    }
    else {
        emit settingsInvalid("OAuth disabled in settings");
    }
}

void AccessHandler::stateHandler()
{
    if (m_state == IDLE) { // Start fresh
        m_state = DISCOVER_ACCOUNT;
        m_asyncHandle = nullptr;
        m_ctx = { 0 };
        MSALRUNTIME_DiscoverAccountsAsync(m_appId.c_str(), m_correlationId.c_str(), discoverCallback, &m_ctx, &m_asyncHandle);
    }
    else if (m_state == DISCOVER_ACCOUNT && m_ctx.called) { // Done, we have first acc. set in ctx
        MSALRUNTIME_ReleaseAsyncHandle(m_asyncHandle);
        m_state = ACQUIRE_TOKEN;
        m_ctx.called = false;
        MSALRUNTIME_AcquireTokenSilentlyAsync(m_authParameters, m_correlationId.c_str(), m_ctx.account, authCallback, &m_ctx, &m_asyncHandle);
    }
    else if (m_state == ACQUIRE_TOKEN && m_ctx.called) {
        m_state = FINISHED;
        m_stateTimer->stop();
        m_timeoutTimer->stop();
        MSALRUNTIME_ReleaseAsyncHandle(m_asyncHandle);
        MSALRUNTIME_ReleaseAuthParameters(m_authParameters);
        MSALRUNTIME_ReleaseLogCallbackHandle(m_logHandle);
        MSALRUNTIME_Shutdown();

        emit accessTokenValid(m_ctx.acc + ", valid until " + m_ctx.expiryTime.toLocalTime().toString("hh:mm:ss"));
        QTimer::singleShot(QDateTime::currentDateTime().msecsTo(m_ctx.expiryTime.toLocalTime().addSecs(-60)), this, &AccessHandler::login); // Renewal is handled here
    }
}

void AccessHandler::loggerCallback(const os_char *logMessage,
                                   const MSALRUNTIME_LOG_LEVEL logLevel,
                                   void *callbackData)
{
    switch (logLevel) {
    default:
        /* fall-thru */
    case Msalruntime_Log_Level_Trace:
        std::wcout << "[Trace  ]" << logMessage << std::endl;
        break;
    case Msalruntime_Log_Level_Debug:
        std::wcout << "[Debug  ]" << logMessage << std::endl;
        break;
    case Msalruntime_Log_Level_Info:
        std::wcout << "[Info   ]" << logMessage << std::endl;
        break;
    case Msalruntime_Log_Level_Warning:
        std::wcout << "[Warning]" << logMessage << std::endl;
        break;
    case Msalruntime_Log_Level_Error:
        std::wcout << "[Error  ]" << logMessage << std::endl;
        break;
    case Msalruntime_Log_Level_Fatal:
        std::wcout << "[Fatal  ]" << logMessage << std::endl;
        break;
    }
}

void AccessHandler::authCallback(MSALRUNTIME_AUTH_RESULT_HANDLE authResult, void *callbackData)
{
    discover_t *ctx = (discover_t *)callbackData;
    MSALRUNTIME_ACCOUNT_HANDLE account = nullptr;

    //MSALRUNTIME_GetAccount(authResult, &account);
    //printAccount(account);
    //printAuthResult(authResult);

    ctx->called = true;

    std::string idToken;
    MSAL_GET_STRING(authResult, MSALRUNTIME_GetIdToken, idToken);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(QByteArray::fromStdString(idToken));
    QJsonObject jsonObj = jsonDoc.object();
    QJsonValue user = jsonObj.value("preferred_username");
    ctx->acc = user.toString();

    std::string token;
    MSAL_GET_STRING(authResult, MSALRUNTIME_GetAccessToken, token);

    std::string telemetry;
    MSAL_GET_STRING(authResult, MSALRUNTIME_GetTelemetryData, telemetry);
    jsonDoc = QJsonDocument::fromJson(QByteArray::fromStdString(telemetry));
    jsonObj = jsonDoc.object();
    QJsonValue expiryTime = jsonObj.value("access_token_expiry_time");
    QJsonValue success = jsonObj.value("is_successful");

    if (success.toString() == "true") {
        ctx->expiryTime = QDateTime::fromString(expiryTime.toString(), Qt::ISODateWithMs);
        ctx->token = QString::fromStdString(token);
    }
    // free memory
    MSALRUNTIME_ReleaseAccount(account);
    MSALRUNTIME_ReleaseAuthResult(authResult);
}

void AccessHandler::discoverCallback(MSALRUNTIME_DISCOVER_ACCOUNTS_RESULT_HANDLE discoverAccountsResult, void *callbackData)
{
    discover_t *ctx = (discover_t *)callbackData;

    // fetch the first account available
    MSALRUNTIME_GetDiscoverAccountsAt(discoverAccountsResult, 0, &ctx->account);

    // free up memory
    MSALRUNTIME_ReleaseDiscoverAccountsResult(discoverAccountsResult);

    // stop waiting for this async call
    ctx->called = true;
}

MSALRUNTIME_ACCOUNT_HANDLE AccessHandler::discoverFirstAccount(std::wstring &correlationId, std::wstring &appId)
{
    discover_t ctx = { 0 };
    /*MSALRUNTIME_ASYNC_HANDLE asyncHandle = nullptr;

    MSALRUNTIME_DiscoverAccountsAsync(correlationId.c_str(),
                                      discoverCallback, &ctx, &asyncHandle);

    // this is a terrible way to wait for async, but this is
    // an example, so please use locks & signals.
    while (!ctx.called) {
        Sleep(300);
    }

    // free the async handle
    MSALRUNTIME_ReleaseAsyncHandle(asyncHandle);

    // return the first account.*/
    return ctx.account;
}

QString AccessHandler::getToken()
{
    if (QDateTime::currentDateTime().secsTo(m_ctx.expiryTime) > 0)
        return m_ctx.token;
    else
        return QString();
}
