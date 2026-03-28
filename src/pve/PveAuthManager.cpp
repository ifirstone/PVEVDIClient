#include "PveAuthManager.h"
#include <QDebug>

// PVE ticket 有效期为 2 小时，每 1.5 小时自动续期
static const int TICKET_REFRESH_INTERVAL_MS = 90 * 60 * 1000; // 90 分钟

PveAuthManager::PveAuthManager(PveApiClient *apiClient, QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_refreshTimer(new QTimer(this))
{
    connect(m_apiClient, &PveApiClient::loginSuccess,
            this, &PveAuthManager::onLoginSuccess);
    connect(m_apiClient, &PveApiClient::loginFailed,
            this, &PveAuthManager::onLoginFailed);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &PveAuthManager::onRefreshTicket);
}

void PveAuthManager::login(const QString &host, int port,
                            const QString &username, const QString &password,
                            const QString &realm)
{
    // 缓存信息用于续期
    m_host = host;
    m_port = port;
    m_username = username;
    m_password = password;
    m_realm = realm;

    m_apiClient->setServer(host, port);
    m_apiClient->login(username, password, realm);
}

void PveAuthManager::logout()
{
    m_refreshTimer->stop();
    m_apiClient->logout();
    m_password.clear();
    qDebug() << "PVE 会话已结束";
}

bool PveAuthManager::isAuthenticated() const
{
    return m_apiClient->isAuthenticated();
}

QString PveAuthManager::currentUsername() const
{
    return m_username;
}

QString PveAuthManager::currentServer() const
{
    return QString("%1:%2").arg(m_host).arg(m_port);
}

void PveAuthManager::onLoginSuccess(const QString &username)
{
    qDebug() << "认证成功，启动 ticket 自动续期，用户:" << username;

    // 启动自动续期定时器
    m_refreshTimer->start(TICKET_REFRESH_INTERVAL_MS);

    emit authenticationSuccess(username);
}

void PveAuthManager::onLoginFailed(const QString &errorMessage)
{
    m_refreshTimer->stop();
    emit authenticationFailed(errorMessage);
}

void PveAuthManager::onRefreshTicket()
{
    if (m_password.isEmpty()) {
        qWarning() << "无法续期 ticket：没有保存的凭据";
        m_refreshTimer->stop();
        emit sessionExpired();
        return;
    }

    qDebug() << "正在自动续期 PVE ticket...";
    m_apiClient->login(m_username, m_password, m_realm);
}
