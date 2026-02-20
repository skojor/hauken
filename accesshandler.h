#ifndef ACCESSHANDLER_H
#define ACCESSHANDLER_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QJsonArray>

#include <MSALRuntime.h>
#include <windows.h>
#include <utils.hpp>
#include <MSALRuntimeLogging.h>
#include <QSharedPointer>
#include "config.h"

#define TIMEOUT_MS 15000

/*
 * AccessHandler manages OAuth authentication/token retrieval using MSAL Runtime.
 *
 * Typical flow:
 *  1. updSettings() caches OAuth settings from Config and reacts to enable/disable.
 *  2. login() initializes MSAL and starts a small state machine.
 *  3. stateHandler() drives async account discovery + silent token acquisition.
 *  4. getToken() returns the latest cached token (or empty QString if unavailable).
 *
 * The class is signal-driven and intended to be used from Qt UI code:
 *  - reqAccessToken() when a token refresh starts.
 *  - accessTokenValid()/accessTokenReady() when authentication succeeds.
 *  - accessTokenInvalid()/settingsInvalid() on timeout/config/auth issues.
 */

enum STATE_HANDLER {
    IDLE,
    DISCOVER_ACCOUNT,
    DISCOVER_FIRST,
    ACQUIRE_TOKEN,
    FINISHED
};

typedef struct discover_s {
    bool called;
    MSALRUNTIME_ACCOUNT_HANDLE account;
    QDateTime expiryTime;
    QString token;
    QString acc;
} discover_t;

class AccessHandler : public QObject
{
    Q_OBJECT
public:
    explicit AccessHandler(QObject *parent = nullptr,
                           QSharedPointer<Config> c = nullptr);
    // Refresh cached OAuth settings from Config.
    // If OAuth was enabled, login is triggered automatically.
    void updSettings();
    // Start a login/token-refresh attempt if OAuth and required parameters are valid.
    void login();
    // Return latest cached access token. Caller should ensure login() has been run.
    QString getToken();

signals:
    void reqAccessToken();
    void accessTokenReady(QString);
    void accessTokenValid(QString);
    void accessTokenInvalid(QString);
    void settingsInvalid(QString);

private:
    // Polling state machine that waits for MSAL async callbacks and advances flow.
    void stateHandler();
    // Optional MSAL logger callback (currently not registered by default).
    static void loggerCallback(const os_char *logMessage, const MSALRUNTIME_LOG_LEVEL logLevel, void *callbackData);
    // Callback for silent token acquisition. Populates token/user info into context.
    static void authCallback(MSALRUNTIME_AUTH_RESULT_HANDLE authResult, void *callbackData);
    // Callback for account discovery. Captures first account and marks context as ready.
    static void discoverCallback(MSALRUNTIME_DISCOVER_ACCOUNTS_RESULT_HANDLE discoverAccountsResult, void *callbackData);

    MSALRUNTIME_LOG_CALLBACK_HANDLE m_logHandle = nullptr;
    MSALRUNTIME_AUTH_PARAMETERS_HANDLE m_authParameters = nullptr;
    MSALRUNTIME_ASYNC_HANDLE m_asyncHandle = nullptr;
    MSALRUNTIME_ACCOUNT_HANDLE m_account = nullptr;

    std::wstring m_appId;
    std::wstring m_authority;
    std::wstring m_scope;
    QSharedPointer<Config> m_config;
    STATE_HANDLER m_state = IDLE;
    std::wstring m_correlationId;
    discover_t m_ctx;
    QTimer *m_stateTimer = new QTimer;
    QTimer *m_timeoutTimer = new QTimer;
    QTimer *m_debugTimer = new QTimer;

    // Cached setting values/state to control when login should run.
    bool m_loginEnabled = false;
    bool m_initialLogin = true;
};
#endif // ACCESSHANDLER_H
