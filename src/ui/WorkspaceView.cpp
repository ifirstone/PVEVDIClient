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

WorkspaceView::WorkspaceView(ConfigManager *configManager,
                             PveApiClient *apiClient,
                             ConnectionManager *connectionManager,
                             QWidget *parent)
    : QWidget(parent)
    , m_configManager(configManager)
    , m_apiClient(apiClient)
    , m_connectionManager(connectionManager)
{
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

void WorkspaceView::setupUI()
{
    // 根布局
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- 顶部状态栏 ----
    QWidget *topBar = new QWidget();
    topBar->setObjectName("wsTopBar");
    topBar->setFixedHeight(54);
    topBar->setStyleSheet(
        "#wsTopBar { background-color: rgba(15,25,50,0.88); border-bottom: 1px solid rgba(255,255,255,0.12); }"
    );
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(24, 0, 24, 0);

    // 左侧：当前登录用户
    QLabel *lblAppName = new QLabel("☁ PVE 云桌面");
    lblAppName->setStyleSheet("color: #89b4fa; font-size: 16px; font-weight: bold;");

    m_lblUser = new QLabel("用户: --");
    m_lblUser->setStyleSheet("color: #a6b4ca; font-size: 13px; margin-left: 20px;");

    topLayout->addWidget(lblAppName);
    topLayout->addWidget(m_lblUser);
    topLayout->addStretch();

    // 右侧：刷新 & 注销
    QPushButton *btnRefresh = new QPushButton("↻  刷新列表");
    btnRefresh->setObjectName("topBarBtn");
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet(
        "#topBarBtn { color: #a6b4ca; border: 1px solid rgba(255,255,255,0.2); border-radius: 6px;"
        " padding: 4px 14px; font-size: 13px; background: transparent; }"
        "#topBarBtn:hover { color: #ffffff; border-color: rgba(255,255,255,0.5); }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, &WorkspaceView::onRefreshVMs);

    QPushButton *btnLogout = new QPushButton("⏏  注销");
    btnLogout->setObjectName("topBarBtn");
    btnLogout->setCursor(Qt::PointingHandCursor);
    btnLogout->setStyleSheet(btnRefresh->styleSheet());
    connect(btnLogout, &QPushButton::clicked, this, &WorkspaceView::logoutRequested);

    topLayout->addWidget(btnRefresh);
    topLayout->addSpacing(8);
    topLayout->addWidget(btnLogout);

    rootLayout->addWidget(topBar);

    // ---- VM 卡片滚动区 ----
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border: none; background-color: #13192b; }"
        "QScrollBar:vertical { background: #1e2740; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #3a4a6a; border-radius: 4px; min-height: 30px; }"
    );

    m_cardsContainer = new QWidget();
    m_cardsContainer->setAttribute(Qt::WA_StyledBackground, true);
    m_cardsContainer->setStyleSheet("background-color: #13192b;");

    // 使用 FlowLayout：卡片先横向排列，一行排满后自动换行
    FlowLayout *flowLayout = new FlowLayout(m_cardsContainer, 28, 18, 18);
    m_cardsContainer->setLayout(flowLayout);

    m_scrollArea->setWidget(m_cardsContainer);
    rootLayout->addWidget(m_scrollArea);

}

void WorkspaceView::onAuthenticated(const QString &username)
{
    m_currentUsername = username;
    m_lblUser->setText(QString("👤 %1").arg(username));
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
