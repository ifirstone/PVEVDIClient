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
    , m_easyTierManager(new EasyTierManager(this))
{
    // EasyTier 管理器信号
    connect(m_easyTierManager, &EasyTierManager::stateChanged, this, [this](EasyTierState state) {
        qDebug() << "EasyTier 状态:" << static_cast<int>(state);
        if (state == EasyTierState::Connected) {
            m_easytierFallback = false;
        }
        emit easyTierStateChanged(state);
    });

    connect(m_easyTierManager, &EasyTierManager::logMessage, this, [this](const QString &msg) {
        const QString trimmed = msg.trimmed();
        if (!trimmed.isEmpty()) {
            qInfo().noquote() << "[EasyTier]" << trimmed;
        }
    });

    connect(m_easyTierManager, &EasyTierManager::linkModeUpdated,
            this, &ConnectionManager::easyTierLinkModeChanged);

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

EasyTierManager* ConnectionManager::easyTierManager() const
{
    return m_easyTierManager;
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
    case Protocol::RDP: {
        m_currentLauncher = m_rdpLauncher;
        qDebug() << "启动 RDP 连接到" << info.name;

        // 先触发 EasyTier P2P 状态。在使用 EasyTier 时，若已启动则优先尝试 P2P 地址
        m_easytierFallback = false;
        if (m_config->useEasyTier()) {
            refreshEasyTierRuntime();

            if (m_easyTierManager->isRunning()) {
                QString p2pHost = m_easyTierManager->localIp();
                if (!p2pHost.isEmpty() && m_config->enableTieredMode()) {
                    qDebug() << "使用 EasyTier 本地 IP 作为 RDP 地址:" << p2pHost;
                    ConnectionInfo p2pInfo = info;
                    p2pInfo.rdpHost = p2pHost;
                    m_currentConnection = p2pInfo;
                    m_rdpLauncher->launch(p2pInfo);
                    return;
                }
            }
        }

        m_currentConnection = info;
        m_rdpLauncher->launch(info);
        break;
    }

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

void ConnectionManager::refreshEasyTierRuntime()
{
    if (!m_config->useEasyTier()) {
        if (m_easyTierManager->isRunning()) {
            m_easyTierManager->stop();
        }
        emit easyTierStateChanged(EasyTierState::Stopped);
        return;
    }

    m_easyTierManager->setExtraArgs(m_config->easyTierExtraArgs());
    m_easyTierManager->setTargetPeerIp(m_config->easyTierServerPeerIp());
    if (!m_easyTierManager->isRunning()) {
        const bool started = m_easyTierManager->start(
            m_config->easyTierNetworkName(),
            m_config->easyTierNetworkSecret(),
            m_config->easyTierBootstrapUrl());
        qInfo() << "EasyTier 预热启动:" << started;
    } else {
        emit easyTierStateChanged(m_easyTierManager->state());
    }
}

void ConnectionManager::stopEasyTierRuntime()
{
    if (m_easyTierManager->isRunning()) {
        qInfo() << "手动停止 EasyTier 运行时进程";
        m_easyTierManager->stop();
    }
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

    // RDP 失败后，若启用了 EasyTier 并且尚未回退过则尝试回退到 P2P 方式
    if (m_currentConnection.protocol == Protocol::RDP && m_config->useEasyTier() && !m_easytierFallback) {
        m_easytierFallback = true;

        if (!m_easyTierManager->isRunning()) {
            qDebug() << "EasyTier 未运行，尝试启动后回退";
            m_easyTierManager->setExtraArgs(m_config->easyTierExtraArgs());
            m_easyTierManager->setTargetPeerIp(m_config->easyTierServerPeerIp());
            m_easyTierManager->start(
                m_config->easyTierNetworkName(),
                m_config->easyTierNetworkSecret(),
                m_config->easyTierBootstrapUrl());
        }

        if (m_easyTierManager->isRunning()) {
            QString p2pHost = m_easyTierManager->localIp();
            if (!p2pHost.isEmpty() && p2pHost != m_currentConnection.rdpHost) {
                qDebug() << "RDP 失败，回退到 EasyTier P2P 地址:" << p2pHost;
                ConnectionInfo fallbackInfo = m_currentConnection;
                fallbackInfo.rdpHost = p2pHost;
                fallbackInfo.rdpPort = m_currentConnection.rdpPort;
                connectTo(fallbackInfo);
                return;
            }
        }

        qDebug() << "EasyTier 启动/回退失败，继续使用直连或其他通道";
    }

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
