#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include "ConnectionInfo.h"
#include "ConfigManager.h"
#include "protocol/AbstractLauncher.h"
#include "protocol/RdpLauncher.h"
#include "protocol/SpiceLauncher.h"
#include "pve/PveApiClient.h"

// 连接管理器：统一管理连接的创建、启动和断开
class ConnectionManager : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionManager(ConfigManager *config,
                                PveApiClient *apiClient,
                                QObject *parent = nullptr);

    // 启动连接
    void connectTo(const ConnectionInfo &info);

    // 断开当前连接
    void disconnectCurrent();

    // 当前是否有活动连接
    bool isConnected() const;

    // 获取当前连接信息
    ConnectionInfo currentConnection() const;

    // 获取启动器生成的命令行（用于调试）
    QString currentCommandLine() const;

signals:
    void connectionStarted(const ConnectionInfo &info);
    void connectionEnded(const ConnectionInfo &info, int exitCode);
    void connectionError(const ConnectionInfo &info, const QString &errorMessage);

private slots:
    void onConnected();
    void onDisconnected(int exitCode);
    void onError(const QString &errorMessage);
    void onSpiceProxyReceived(const QString &node, int vmId, const QString &vvContent);

private:
    ConfigManager *m_config;
    PveApiClient *m_apiClient;
    RdpLauncher *m_rdpLauncher;
    SpiceLauncher *m_spiceLauncher;
    AbstractLauncher *m_currentLauncher = nullptr;
    ConnectionInfo m_currentConnection;
};

#endif // CONNECTIONMANAGER_H
