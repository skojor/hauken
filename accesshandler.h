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
 * AccessHandler takes care of logging in the user using Microsofts own MSAL library.
 * Neccessary config values are updated via Config class signal (updSettings).
 * Initial login is handled automagically. getToken()
 * returns a valid token, or blank QString if invalid.
 *
 * Call login() before asking for a token, to be sure it is valid!
 *
 * Signals when requesting token, when valid (including login name), and
 * eventually invalid if timeout or any error occurs.
 *
 * MSVC / Qt code mix, using QTimer and states to handle async operations
 *
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
    void updSettings();
    void login();
    QString getToken();

signals:
    void reqAccessToken();
    void accessTokenReady(QString);
    void accessTokenValid(QString);
    void accessTokenInvalid(QString);
    void settingsInvalid(QString);

private:
    void stateHandler();
    static void loggerCallback(const os_char *logMessage, const MSALRUNTIME_LOG_LEVEL logLevel, void *callbackData);
    static void authCallback(MSALRUNTIME_AUTH_RESULT_HANDLE authResult, void *callbackData);
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

    // Config cache
    bool m_loginEnabled = false;
    bool m_initialLogin = true;
};
#endif // ACCESSHANDLER_H
