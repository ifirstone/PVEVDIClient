#include "ConnectionManager.h"
#include <QDebug>

ConnectionManager::ConnectionManager(ConfigManager *config,
                                       PveApiClient *apiClient,
                                       QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_apiClient(apiClient)
    , m_rdpLauncher(new RdpLauncher(this))
    , m_spiceLauncher(new SpiceLauncher(this))
{
    // RDP 启动器信号
    connect(m_rdpLauncher, &AbstractLauncher::connected,
            this, &ConnectionManager::onConnected);
    connect(m_rdpLauncher, &AbstractLauncher::disconnected,
            this, &ConnectionManager::onDisconnected);
    connect(m_rdpLauncher, &AbstractLauncher::connectionError,
            this, &ConnectionManager::onError);

    // SPICE 启动器信号
    connect(m_spiceLauncher, &AbstractLauncher::connected,
            this, &ConnectionManager::onConnected);
    connect(m_spiceLauncher, &AbstractLauncher::disconnected,
            this, &ConnectionManager::onDisconnected);
    connect(m_spiceLauncher, &AbstractLauncher::connectionError,
            this, &ConnectionManager::onError);

    // PVE SPICE proxy 回调
    connect(m_apiClient, &PveApiClient::spiceProxyReceived,
            this, &ConnectionManager::onSpiceProxyReceived);
            
    // 拦截 API 错误（如 SPICE 配置下发失败、网络断开等）并转给 UI 提示
    connect(m_apiClient, &PveApiClient::apiError, this, [this](const QString &err) {
        // 如果当前确实正打算建立连接却失败了，把错误以当前 ConnectionInfo 抛出去
        if (m_currentLauncher && !m_currentLauncher->isConnected()) {
            onError(err);
        }
    });
}

void ConnectionManager::connectTo(const ConnectionInfo &info)
{
    // 如果已有连接，先断开
    if (isConnected()) {
        qDebug() << "已有活动连接，先断开...";
        disconnectCurrent();
    }

    m_currentConnection = info;

    switch (info.protocol) {
    case Protocol::RDP:
        m_currentLauncher = m_rdpLauncher;
        qDebug() << "启动 RDP 连接到" << info.name;
        m_rdpLauncher->launch(info);
        break;

    case Protocol::SPICE:
        m_currentLauncher = m_spiceLauncher; // 必须在这里赋值，否则收到 proxy 时抛错
        // SPICE 需要先从 PVE API 获取 ticket
        if (m_apiClient->isAuthenticated() && info.vmId > 0 && !info.node.isEmpty()) {
            qDebug() << "向 PVE 请求 SPICE proxy，VM:" << info.vmId;
            m_apiClient->requestSpiceProxy(info.node, info.vmId);
        } else {
            // 直连模式（不通过 PVE API）
            qDebug() << "启动 SPICE 直连模式到" << info.name;
            m_spiceLauncher->launch(info);
        }
        break;
    }
}

void ConnectionManager::disconnectCurrent()
{
    if (m_currentLauncher && m_currentLauncher->isConnected()) {
        m_currentLauncher->disconnect();
    }
    m_currentLauncher = nullptr;
}

bool ConnectionManager::isConnected() const
{
    return m_currentLauncher && m_currentLauncher->isConnected();
}

ConnectionInfo ConnectionManager::currentConnection() const
{
    return m_currentConnection;
}

QString ConnectionManager::currentCommandLine() const
{
    if (m_currentLauncher) {
        return m_currentLauncher->buildCommandLine(m_currentConnection);
    }
    return QString();
}

void ConnectionManager::onConnected()
{
    qDebug() << "连接已建立:" << m_currentConnection.name;
    emit connectionStarted(m_currentConnection);
}

void ConnectionManager::onDisconnected(int exitCode)
{
    qDebug() << "连接已断开:" << m_currentConnection.name
             << "退出码:" << exitCode;
    emit connectionEnded(m_currentConnection, exitCode);
    m_currentLauncher = nullptr;
}

void ConnectionManager::onError(const QString &errorMessage)
{
    qWarning() << "连接错误:" << errorMessage;
    emit connectionError(m_currentConnection, errorMessage);
}

void ConnectionManager::onSpiceProxyReceived(const QString &node, int vmId,
                                               const QString &vvContent)
{
    // 确认是当前请求的 VM
    if (m_currentConnection.node == node && m_currentConnection.vmId == vmId) {
        bool fullscreen = (m_currentConnection.resolution == "fullscreen");
        m_spiceLauncher->launchWithVvFile(vvContent, fullscreen);
    }
}
