#ifndef PVEAUTHMANAGER_H
#define PVEAUTHMANAGER_H

#include <QObject>
#include <QTimer>
#include "PveApiClient.h"

// PVE 认证管理器
// 管理 PVE ticket 的生命周期，自动续期，保存/恢复凭据
class PveAuthManager : public QObject
{
    Q_OBJECT

public:
    explicit PveAuthManager(PveApiClient *apiClient, QObject *parent = nullptr);

    // 使用用户名密码登录
    void login(const QString &host, int port,
               const QString &username, const QString &password,
               const QString &realm = "pam");

    // 登出
    void logout();

    // 当前是否已认证
    bool isAuthenticated() const;

    // 当前登录的用户名
    QString currentUsername() const;

    // 当前连接的服务器
    QString currentServer() const;

signals:
    void authenticationSuccess(const QString &username);
    void authenticationFailed(const QString &errorMessage);
    void sessionExpired();

private slots:
    void onLoginSuccess(const QString &username);
    void onLoginFailed(const QString &errorMessage);
    void onRefreshTicket();

private:
    PveApiClient *m_apiClient;
    QTimer *m_refreshTimer; // ticket 续期定时器

    // 缓存登录信息用于续期
    QString m_host;
    int m_port = 8006;
    QString m_username;
    QString m_password;
    QString m_realm;
};

#endif // PVEAUTHMANAGER_H
