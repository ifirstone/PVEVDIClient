#include "WorkspaceView.h"
#include "VmCard.h"
#include "FlowLayout.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QHostInfo>
#include <QDateTime>
#include <QTimer>
#include "../core/DebugLogger.h"

WorkspaceView::WorkspaceView(ConfigManager *configManager,
                             PveApiClient *apiClient,
                             ConnectionManager *connectionManager,
                             QWidget *parent)
    : QWidget(parent)
    , m_configManager(configManager)
    , m_apiClient(apiClient)
    , m_connectionManager(connectionManager)
{
    m_background = QPixmap(":/icons/Wallpaper.jpg");
    setupUI();

    // 连接 API 信号
    connect(m_apiClient, &PveApiClient::nodesReceived,
            this, &WorkspaceView::onNodesReceived);
    connect(m_apiClient, &PveApiClient::vmsReceived,
            this, &WorkspaceView::onVMsReceived);
    connect(m_apiClient, &PveApiClient::apiError,
            this, &WorkspaceView::onApiError);
    connect(m_apiClient, &PveApiClient::vmIpReceived,
            this, &WorkspaceView::onVmIpReceived);

    connect(m_connectionManager, &ConnectionManager::easyTierStateChanged, this, [this](EasyTierState state){
        updateEasyTierIndicator(state);
    });

    connect(m_connectionManager, &ConnectionManager::easyTierLinkModeChanged,
            this, [this](const QString &mode, bool targetPeerFound) {
        Q_UNUSED(targetPeerFound)
        m_easyTierLinkMode = mode.toLower();
        updateEasyTierIndicator(EasyTierState::Connected);
    });

    connect(m_configManager, &ConfigManager::configChanged,
            this, &WorkspaceView::refreshFooterBrandText);
}

void WorkspaceView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!m_background.isNull()) {
        painter.drawPixmap(rect(), m_background.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        // 添加全局暗色半透明蒙版以弱化背景，凸显前景的虚拟机卡片
        painter.fillRect(rect(), QColor(0, 0, 0, 100));
    } else {
        painter.fillRect(rect(), QColor(25, 30, 45));
    }
}

void WorkspaceView::setupUI()
{
    // 根布局
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- 顶部玻璃态导航栏 ----
    QWidget *topBar = new QWidget();
    topBar->setObjectName("wsTopBar");
    topBar->setFixedHeight(64);
    topBar->setStyleSheet(
        "#wsTopBar { background-color: rgba(255,255,255,0.05); border-bottom: 1px solid rgba(255,255,255,0.15); }"
    );
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(36, 0, 36, 0);

    // 左侧：Logo
    QLabel *lblIcon = new QLabel("云桌面客户端 Cloud Desktop Client");
    lblIcon->setStyleSheet("color: white; font-size: 20px; font-weight: bold; letter-spacing: 1px; background: transparent;");

    // 用户信息区容器（改进外观）
    QWidget *userInfoContainer = new QWidget();
    QHBoxLayout *userInfoLayout = new QHBoxLayout(userInfoContainer);
    userInfoLayout->setContentsMargins(16, 8, 16, 8);
    userInfoLayout->setSpacing(12);
    userInfoContainer->setStyleSheet(
        "QWidget { background: rgba(59,113,202,0.15); border: 1px solid rgba(59,113,202,0.3); border-radius: 8px; }"
    );

    // 用户名标签（加大字体）
    m_lblUser = new QLabel("");
    m_lblUser->setStyleSheet(
        "color: rgba(255,255,255,1.0); font-size: 16px; font-weight: 600; "
        "background: transparent; padding: 2px 8px;"
    );
    userInfoLayout->addWidget(m_lblUser);

    // EasyTier 状态区
    m_lblEasyTierDot = new QLabel();
    m_lblEasyTierDot->setFixedSize(12, 12);
    m_lblEasyTierDot->setStyleSheet(
        "background-color: #6b7280; border-radius: 6px; "
        "border: 2px solid rgba(200,212,232,0.3);"
    );
    userInfoLayout->addWidget(m_lblEasyTierDot);

    m_lblEasyTier = new QLabel("EasyTier: 未启用");
    m_lblEasyTier->setStyleSheet(
        "color: rgba(150,200,255,0.95); font-size: 14px; font-weight: 500; "
        "background: transparent; padding: 2px 0;"
    );
    userInfoLayout->addWidget(m_lblEasyTier);
    userInfoLayout->addStretch();

    topLayout->addWidget(lblIcon);
    topLayout->addWidget(userInfoContainer, 0, Qt::AlignVCenter);

    topLayout->addStretch();

    // 调试按钮
    QPushButton *btnDebug = new QPushButton("运行日志");
    btnDebug->setCursor(Qt::PointingHandCursor);
    btnDebug->setStyleSheet(
        "QPushButton { color: white; font-size: 14px; background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.2); border-radius: 6px; padding: 6px 14px; margin-right: 15px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.2); }"
    );
    connect(btnDebug, &QPushButton::clicked, this, [](){
        DebugLogger::instance().showLogger();
    });

    // 右侧：用户注销按钮
    QPushButton *btnLogout = new QPushButton("注销");
    btnLogout->setCursor(Qt::PointingHandCursor);
    btnLogout->setStyleSheet(
        "QPushButton {"
        " color: #ffe4e6;"
        " font-size: 14px;"
        " font-weight: 600;"
        " background: rgba(220,38,38,0.16);"
        " border: 1px solid rgba(248,113,113,0.55);"
        " border-radius: 14px;"
        " padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        " background: rgba(239,68,68,0.28);"
        " border: 1px solid rgba(252,165,165,0.78);"
        " color: #fff1f2;"
        "}"
        "QPushButton:pressed {"
        " background: rgba(185,28,28,0.44);"
        " border: 1px solid rgba(252,165,165,0.9);"
        "}"
    );
    connect(btnLogout, &QPushButton::clicked, this, &WorkspaceView::logoutRequested);

    topLayout->addWidget(btnDebug);
    topLayout->addWidget(btnLogout);
    
    rootLayout->addWidget(topBar);

    // ---- 操作栏悬浮区（刷新按钮） ----
    QHBoxLayout *actionBarLayout = new QHBoxLayout();
    actionBarLayout->setContentsMargins(40, 20, 40, 0);
    
    QPushButton *btnRefresh = new QPushButton("刷新列表");
    btnRefresh->setFixedSize(100, 36);
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet(
        "QPushButton { background: rgba(0,0,0,0.6); color: white; border-radius: 18px; font-size: 13px; }"
        "QPushButton:hover { background: rgba(0,0,0,0.8); }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, &WorkspaceView::onRefreshVMs);
    
    actionBarLayout->addWidget(btnRefresh);
    actionBarLayout->addStretch();
    rootLayout->addLayout(actionBarLayout);

    // ---- VM 卡片滚动区 ----
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 6px; }"
        "QScrollBar::handle:vertical { background: rgba(255,255,255,0.3); border-radius: 3px; }"
    );

    m_cardsContainer = new QWidget();
    m_cardsContainer->setStyleSheet("background: transparent;");

    FlowLayout *flowLayout = new FlowLayout(m_cardsContainer, 40, 24, 24);
    m_cardsContainer->setLayout(flowLayout);

    m_scrollArea->setWidget(m_cardsContainer);
    rootLayout->addWidget(m_scrollArea);

    // ---- 底部系统状态栏 ----
    QHBoxLayout *statusBarLayout = new QHBoxLayout();
    statusBarLayout->setContentsMargins(24, 10, 24, 10);
    
    QWidget *statusBarWidget = new QWidget(this);
    statusBarWidget->setStyleSheet(
        "background-color: rgba(10, 15, 25, 180);"
        "color: rgba(255, 255, 255, 200);"
        "font-family: 'Segoe UI', 'PingFang SC', 'Microsoft YaHei', sans-serif;"
        "font-size: 13px;"
        "font-weight: 500;"
        "letter-spacing: 0.5px;"
    );
    statusBarWidget->setLayout(statusBarLayout);

    QString localIp = "127.0.0.1";
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
            QString ipStr = address.toString();
            if (!ipStr.startsWith("169") && !ipStr.startsWith("172.17")) { localIp = ipStr; break; }
        }
    }

    QLabel *lblClientInfo = new QLabel(QString("主机名: %1\t本机IP: %2").arg(QHostInfo::localHostName(), localIp));
    m_lblFooterBrand = new QLabel();
    m_lblDateTime = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    statusBarLayout->addWidget(lblClientInfo);
    statusBarLayout->addStretch();
    statusBarLayout->addWidget(m_lblFooterBrand);
    statusBarLayout->addStretch();
    statusBarLayout->addWidget(m_lblDateTime);

    refreshFooterBrandText();

    rootLayout->addWidget(statusBarWidget);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (m_lblDateTime) m_lblDateTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    m_timer->start(1000);
}

void WorkspaceView::onAuthenticated(const QString &username)
{
    m_currentUsername = username;
    m_lblUser->setText(username);

    // 仅在已记住密码场景下启用自动连接，避免未授权自动进入
    if (m_configManager->rememberPassword()
        && m_configManager->autoConnect()
        && !m_configManager->autoConnectId().isEmpty()) {
        m_pendingAutoConnectKey = m_configManager->autoConnectId();
    } else {
        m_pendingAutoConnectKey.clear();
    }
    m_autoConnectTriggered = false;

    if (m_configManager->useEasyTier()) {
        m_lblEasyTier->setText("EasyTier: 启动中...");
        m_connectionManager->refreshEasyTierRuntime();
    } else {
        updateEasyTierIndicator(EasyTierState::Stopped);
    }

    onRefreshVMs();
}

void WorkspaceView::updateEasyTierIndicator(EasyTierState state)
{
    QString dotColor = "#6b7280";
    QString textColor = "rgba(150,200,255,0.95)";
    
    switch (state) {
    case EasyTierState::Stopped:
        m_lblEasyTier->setText(m_configManager->useEasyTier() ? "EasyTier: 停止" : "EasyTier: 未启用");
        dotColor = "#9ca3af";
        textColor = "rgba(156,163,175,0.9)";
        break;
    case EasyTierState::Starting:
        m_lblEasyTier->setText("EasyTier: 启动中...");
        dotColor = "#f59e0b";
        textColor = "rgba(245,158,11,0.95)";
        break;
    case EasyTierState::Connected:
        if (m_easyTierLinkMode == "p2p") {
            m_lblEasyTier->setText("EasyTier: 已连接 (P2P直连)");
        } else if (!m_easyTierLinkMode.isEmpty()) {
            m_lblEasyTier->setText(QString("EasyTier: 已连接 (中转:%1)").arg(m_easyTierLinkMode));
        } else {
            m_lblEasyTier->setText("EasyTier: 已连接");
        }
        dotColor = "#22c55e";
        textColor = "rgba(34,197,94,0.95)";
        break;
    case EasyTierState::Disconnected:
        m_lblEasyTier->setText("EasyTier: 未连接");
        dotColor = "#ef4444";
        textColor = "rgba(239,68,68,0.95)";
        break;
    case EasyTierState::Error:
        m_lblEasyTier->setText("EasyTier: 错误");
        dotColor = "#ef4444";
        textColor = "rgba(239,68,68,0.95)";
        break;
    }

    // 更新指示点样式
    m_lblEasyTierDot->setStyleSheet(
        QString("background-color: %1; border-radius: 6px; border: 2px solid rgba(200,212,232,0.3);").arg(dotColor)
    );
    
    // 更新状态文字颜色
    m_lblEasyTier->setStyleSheet(
        QString("color: %1; font-size: 14px; font-weight: 500; background: transparent; padding: 2px 0;").arg(textColor)
    );
}

void WorkspaceView::onRefreshVMs()
{
    m_autoConnectTriggered = false;
    clearCards();
    m_apiClient->fetchNodes();
}

void WorkspaceView::onNodesReceived(const QJsonArray &nodes)
{
    for (const QJsonValue &nodeVal : nodes) {
        QString nodeName = nodeVal.toObject()["node"].toString();
        m_apiClient->fetchVMs(nodeName);
    }
}

void WorkspaceView::onVMsReceived(const QString &node, const QJsonArray &vms)
{
    qDebug() << "收到节点" << node << "的" << vms.size() << "台 VM";
    for (const QJsonValue &vmVal : vms) {
        QJsonObject vmInfo = vmVal.toObject();
        vmInfo["node"] = node;
        addVmCard(vmInfo);
    }
}

void WorkspaceView::addVmCard(const QJsonObject &vmInfo)
{
    VmCard *card = new VmCard(vmInfo, m_configManager, m_connectionManager, m_apiClient, m_cardsContainer);

    // FlowLayout 直接 addWidget，自动流式排列
    FlowLayout *flow = static_cast<FlowLayout*>(m_cardsContainer->layout());
    if (flow) {
        flow->addWidget(card);
    }
    m_cards.append(card);

    if (!m_autoConnectTriggered
        && !m_pendingAutoConnectKey.isEmpty()
        && card->autoConnectKey() == m_pendingAutoConnectKey) {
        m_autoConnectTriggered = true;
        QTimer::singleShot(300, card, [card]() {
            card->triggerAutoConnect();
        });
    }
}

void WorkspaceView::clearCards()
{
    for (VmCard *card : m_cards) {
        card->deleteLater();
    }
    m_cards.clear();
}

void WorkspaceView::onApiError(const QString &error)
{
    qWarning() << "WorkspaceView API error:" << error;
}

void WorkspaceView::onVmIpReceived(const QString &node, int vmId, const QStringList &ips)
{
    // 转发给对应的 VmCard 处理
    for (VmCard *card : m_cards) {
        if (card->node() == node && card->vmId() == vmId) {
            card->onIpReceived(ips);
            break;
        }
    }
}

void WorkspaceView::refreshFooterBrandText()
{
    if (m_lblFooterBrand) {
        m_lblFooterBrand->setText(m_configManager->footerBrandText());
    }
}
