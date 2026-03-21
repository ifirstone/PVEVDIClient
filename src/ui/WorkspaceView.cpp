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

    m_lblUser = new QLabel("");
    m_lblUser->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px; margin-left: 15px;");

    topLayout->addWidget(lblIcon);
    topLayout->addWidget(m_lblUser);
    topLayout->addStretch();

    // 调试按钮
    QPushButton *btnDebug = new QPushButton("运行日志");
    btnDebug->setCursor(Qt::PointingHandCursor);
    btnDebug->setStyleSheet(
        "QPushButton { color: white; font-size: 14px; background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.2); border-radius: 6px; padding: 6px 14px; margin-right: 15px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.2); }"
    );
    connect(btnDebug, &QPushButton::clicked, this, [](){
        DebugLogger::instance().show();
        DebugLogger::instance().raise();
    });

    // 右侧：用户注销按钮
    QPushButton *btnLogout = new QPushButton("注销");
    btnLogout->setCursor(Qt::PointingHandCursor);
    btnLogout->setStyleSheet(
        "QPushButton { color: white; font-size: 14px; background: transparent; border: none; }"
        "QPushButton:pressed { background: rgba(255,255,255,0.2) }"
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

    QLabel *lblClientInfo = new QLabel(QString("主机名: %1\t本地地址: %2").arg(QHostInfo::localHostName(), localIp));
    QLabel *lblVersion = new QLabel("开源技术驱动 v3.0");
    m_lblDateTime = new QLabel(QDateTime::currentDateTime().toString("MM-dd HH:mm:ss"));

    statusBarLayout->addWidget(lblClientInfo);
    statusBarLayout->addStretch();
    statusBarLayout->addWidget(lblVersion);
    statusBarLayout->addStretch();
    statusBarLayout->addWidget(m_lblDateTime);

    rootLayout->addWidget(statusBarWidget);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (m_lblDateTime) m_lblDateTime->setText(QDateTime::currentDateTime().toString("MM-dd HH:mm:ss"));
    });
    m_timer->start(1000);
}

void WorkspaceView::onAuthenticated(const QString &username)
{
    m_currentUsername = username;
    m_lblUser->setText(username);
    onRefreshVMs();
}

void WorkspaceView::onRefreshVMs()
{
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
