#include "accesshandler.h"

AccessHandler::AccessHandler(QObject *parent, QSharedPointer<Config> c)
    : QObject(parent)
{
    m_stateTimer = new QTimer(this);
    m_timeoutTimer = new QTimer(this);
    m_debugTimer = new QTimer(this);

    if (c)
        m_config = c;
    else
        qFatal() << "AccessHandler: No config pointer set, giving up";


    connect(m_stateTimer, &QTimer::timeout, this, &AccessHandler::stateHandler);
    connect(m_timeoutTimer, &QTimer::timeout, this, [this] () {
        m_timeoutTimer->stop();
        m_stateTimer->stop();
        
        // Critical fix: Don't call MSALRUNTIME_Shutdown() while async operations may be active.
        // This can cause crashes or hangs if the MSAL background thread is still processing.
        // Instead, only release the async handle and cleanup non-runtime resources.
        // Keep MSAL runtime alive until app exit to avoid re-initialization issues.
        
        m_ctx.called.store(false, std::memory_order_release);
        cleanupMsalResources(false);

        // Don't call MSALRUNTIME_Shutdown() here - it can hang if async op is still in progress
        // Shutdown will be called in destructor when app is exiting.
        
        m_state = StateHandler::Idle;
        qWarning() << "AccessHandler: Timeout while running authorization routine, retrying later";
        emit accessTokenInvalid("Authorization timed out");
        QTimer::singleShot(5 * 60 * 1000, this, &AccessHandler::login); // 5 min retry
    });

    connect(m_debugTimer, &QTimer::timeout, this, [this] () {
        qDebug() << "AccessHandler debug:" << m_ctx.token << m_ctx.expiryTime.toLocalTime().toString();
    });
    //m_debugTimer->start(60000);
}

AccessHandler::~AccessHandler()
{
    m_timeoutTimer->stop();
    m_stateTimer->stop();
    m_debugTimer->stop();
    cleanupMsalResources(true);
}

// Keep local OAuth values synchronized with Config and react to enable/disable transitions.
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
            cleanupMsalResources(true);
            emit settingsInvalid("OAuth disabled in settings");
            m_ctx.expiryTime = QDateTime::currentDateTime(); // Expired
            m_ctx.token.clear();
        }
    }
}

// Initialize MSAL and kick off async token flow guarded by timeout + state polling.
void AccessHandler::login()
{
    if (m_state != StateHandler::Idle && m_state != StateHandler::Finished)
        return;

    if (m_config->getOauth2Enable()) {
        if (!m_appId.empty() && !m_authority.empty() && !m_scope.empty()) {
            // Init MSAL
            cleanupMsalResources(true);
            m_correlationId = uuidGen();
            MSALRUNTIME_ERROR_HANDLE error = MSALRUNTIME_Startup();
            if (error) {
                MSALRUNTIME_ReleaseError(error);
                emit accessTokenInvalid("Could not initialize authorization runtime");
                return;
            }
            m_msalStarted = true;
            MSALRUNTIME_SetIsPiiEnabled(false);
            //MSALRUNTIME_RegisterLogCallback(loggerCallback, nullptr, &m_logHandle);
            error = MSALRUNTIME_CreateAuthParameters(m_appId.c_str(), m_authority.c_str(), &m_authParameters);
            if (error || !m_authParameters) {
                if (error)
                    MSALRUNTIME_ReleaseError(error);
                cleanupMsalResources(true);
                emit accessTokenInvalid("Could not create authorization parameters");
                return;
            }
            MSALRUNTIME_SetRequestedScopes(m_authParameters, m_scope.c_str());
            MSALRUNTIME_SetRedirectUri(m_authParameters, L"placeholder");
            MSALRUNTIME_SetAdditionalParameter(
                m_authParameters, L"msal_gui_thread", L"true");

            m_state = StateHandler::Idle;
            m_stateTimer->start(100);
            m_timeoutTimer->start(kTimeoutMs);
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

// Drive async MSAL operations as a simple polling state machine.
void AccessHandler::stateHandler()
{
    if (m_state == StateHandler::Idle) { // Start fresh
        m_state = StateHandler::DiscoverAccount;
        m_asyncHandle = nullptr;
        m_ctx.called.store(false, std::memory_order_relaxed);
        m_ctx.account = nullptr;
        m_ctx.expiryTime = {};
        m_ctx.token.clear();
        m_ctx.acc.clear();
        qDebug() << "AccessHandler: Starting account discovery...";
        MSALRUNTIME_ERROR_HANDLE error = MSALRUNTIME_DiscoverAccountsAsync(
            m_appId.c_str(), m_correlationId.c_str(), discoverCallback, &m_ctx, &m_asyncHandle);
        if (error) {
            MSALRUNTIME_ReleaseError(error);
            m_ctx.called.store(true, std::memory_order_release);
        }
    }
    else if (m_state == StateHandler::DiscoverAccount && m_ctx.called.load(std::memory_order_acquire)) { // Done, we have first acc. set in ctx
        qDebug() << "AccessHandler: Account discovered, acquiring token...";
        if (m_asyncHandle) {
            MSALRUNTIME_ReleaseAsyncHandle(m_asyncHandle);
            m_asyncHandle = nullptr;
        }
        if (!m_ctx.account) {
            m_stateTimer->stop();
            m_timeoutTimer->stop();
            cleanupMsalResources(true);
            m_state = StateHandler::Finished;
            emit accessTokenInvalid("No cached account available for silent authorization");
            return;
        }
        m_state = StateHandler::AcquireToken;
        m_ctx.called.store(false, std::memory_order_relaxed);
        MSALRUNTIME_ERROR_HANDLE error = MSALRUNTIME_AcquireTokenSilentlyAsync(
            m_authParameters, m_correlationId.c_str(), m_ctx.account, authCallback, &m_ctx, &m_asyncHandle);
        if (error) {
            MSALRUNTIME_ReleaseError(error);
            m_ctx.called.store(true, std::memory_order_release);
        }
    }
    else if (m_state == StateHandler::AcquireToken && m_ctx.called.load(std::memory_order_acquire)) {
        m_state = StateHandler::Finished;
        m_stateTimer->stop();
        m_timeoutTimer->stop();
        if (m_asyncHandle) {
            MSALRUNTIME_ReleaseAsyncHandle(m_asyncHandle);
            m_asyncHandle = nullptr;
        }
        cleanupMsalResources(true);
        qDebug() << "AccessHandler: Token acquisition completed";
        if (m_ctx.token.isEmpty()) {
            qWarning() << "AccessHandler: Couldn't retrieve a valid token";
            emit accessTokenInvalid("Couldn't retrieve a valid token");
        }
        else {
            qInfo() << "AccessHandler: Successfully acquired token for user:" << m_ctx.acc;
            emit accessTokenValid(m_ctx.acc);
            if (!m_initialLogin)
                emit accessTokenReady(m_ctx.token); // Don't signal new token when starting up
            else
                m_initialLogin = false;
        }
    }
}

void AccessHandler::cleanupMsalResources(bool shutdownRuntime)
{
    if (m_asyncHandle) {
        MSALRUNTIME_CancelAsyncOperation(m_asyncHandle);
        MSALRUNTIME_ReleaseAsyncHandle(m_asyncHandle);
        m_asyncHandle = nullptr;
    }

    if (m_authParameters) {
        MSALRUNTIME_ReleaseAuthParameters(m_authParameters);
        m_authParameters = nullptr;
    }

    if (m_logHandle) {
        MSALRUNTIME_ReleaseLogCallbackHandle(m_logHandle);
        m_logHandle = nullptr;
    }

    if (m_ctx.account) {
        MSALRUNTIME_ReleaseAccount(m_ctx.account);
        m_ctx.account = nullptr;
    }

    if (shutdownRuntime && m_msalStarted) {
        MSALRUNTIME_Shutdown();
        m_msalStarted = false;
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
        qDebug() << "[Trace  ]" << logMessage;
        break;
    case Msalruntime_Log_Level_Debug:
        qDebug() << "[Debug  ]" << logMessage;
        break;
    case Msalruntime_Log_Level_Info:
        qInfo() << "[Info   ]" << logMessage;
        break;
    case Msalruntime_Log_Level_Warning:
        qWarning() << "[Warning]" << logMessage;
        break;
    case Msalruntime_Log_Level_Error:
        qWarning() << "[Error  ]" << logMessage;
        break;
    case Msalruntime_Log_Level_Fatal:
        qFatal() << "[Fatal  ]" << logMessage;
        break;
    }
}

// Read token/account details from MSAL result and store in shared callback context.
void AccessHandler::authCallback(MSALRUNTIME_AUTH_RESULT_HANDLE authResult, void *callbackData)
{
    auto *ctx = static_cast<DiscoverContext *>(callbackData);
    if (!ctx)
        return;

    if (!authResult) {
        ctx->called.store(true, std::memory_order_release);
        return;
    }

    MSALRUNTIME_ACCOUNT_HANDLE account = nullptr;

    MSALRUNTIME_ERROR_HANDLE error = MSALRUNTIME_GetAccount(authResult, &account);
    if (error)
        MSALRUNTIME_ReleaseError(error);
    if (!account) {
        ctx->called.store(true, std::memory_order_release);
        MSALRUNTIME_ReleaseAuthResult(authResult);
        return;
    }
    //printAccount(account);
    //printAuthResult(authResult);

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
    //QJsonValue expiryTime = jsonObj.value("access_token_expiry_time");
    QJsonValue success = jsonObj.value("is_successful");

    std::string additionalFields;
    MSAL_GET_STRING(account, MSALRUNTIME_GetAdditionalFieldsJson, additionalFields);
    jsonDoc = QJsonDocument::fromJson(QByteArray::fromStdString(additionalFields));
    jsonObj = jsonDoc.object();
    QJsonValue expTime = jsonObj.value("exp");

    if (success.toString() == "true") {
        //ctx->expiryTime = QDateTime::fromString(expiryTime.toString(), Qt::ISODateWithMs);
        ctx->expiryTime = QDateTime::fromSecsSinceEpoch(expTime.toInt(), QTimeZone::LocalTime);
        ctx->token = QString::fromStdString(token);
    }
    //qDebug() << expTime.toInteger() << ctx->expiryTime.toLocalTime();
    // free memory
    MSALRUNTIME_ReleaseAccount(account);
    MSALRUNTIME_ReleaseAuthResult(authResult);
    ctx->called.store(true, std::memory_order_release);
}

// Select first discovered account and mark async callback as completed.
void AccessHandler::discoverCallback(MSALRUNTIME_DISCOVER_ACCOUNTS_RESULT_HANDLE discoverAccountsResult, void *callbackData)
{
    auto *ctx = static_cast<DiscoverContext *>(callbackData);
    if (!ctx)
        return;

    // fetch the first account available
    if (discoverAccountsResult) {
        MSALRUNTIME_ERROR_HANDLE error = MSALRUNTIME_GetDiscoverAccountsAt(discoverAccountsResult, 0, &ctx->account);
        if (error)
            MSALRUNTIME_ReleaseError(error);
    }

    // free up memory
    if (discoverAccountsResult)
        MSALRUNTIME_ReleaseDiscoverAccountsResult(discoverAccountsResult);

    // stop waiting for this async call
    ctx->called.store(true, std::memory_order_release);
}

QString AccessHandler::getToken()
{
    return m_ctx.token;
}
