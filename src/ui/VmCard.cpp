#include "VmCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QIntValidator>
#include <QDebug>
#include <QPixmap>

VmCard::VmCard(const QJsonObject &vmInfo,
               ConfigManager *configManager,
               ConnectionManager *connectionManager,
               PveApiClient *apiClient,
               QWidget *parent)
    : QWidget(parent)
    , m_vmInfo(vmInfo)
    , m_configManager(configManager)
    , m_connectionManager(connectionManager)
    , m_apiClient(apiClient)
{
    setupUI();

    // 连接管理器信号
    connect(m_connectionManager, &ConnectionManager::connectionStarted,
            this, &VmCard::onConnectionStarted);
    connect(m_connectionManager, &ConnectionManager::connectionEnded,
            this, &VmCard::onConnectionEnded);
    connect(m_connectionManager, &ConnectionManager::connectionError,
            this, &VmCard::onConnectionError);
}

void VmCard::setupUI()
{
    QString name   = m_vmInfo["name"].toString("VM-%1").arg(m_vmInfo["vmid"].toInt());
    QString status = m_vmInfo["status"].toString();
    int     vmId   = m_vmInfo["vmid"].toInt();
    QString node   = m_vmInfo["node"].toString();

    // --- 卡片外观 ---
    setObjectName("vmCard");
    setFixedWidth(380);
    // QWidget 默认不绘制背景，必须设置此属性才能让 stylesheet 白色背景真正生效
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        // 卡片本身：白色圆角 + 柔和边框
        "#vmCard {"
        "  background-color: #ffffff;"
        "  border-radius: 12px;"
        "  border: 1px solid rgba(180,195,215,0.4);"
        "}"
        "#vmCard:hover {"
        "  border-color: #3b71ca;"
        "  background-color: #fcfdff;"
        "}"
        "#vmCard QWidget { background: transparent; }"
        "#vmCard QLabel  { color: #1a2a4a; background: transparent; }"
        "#vmCard QFrame  { background: transparent; }"
        "#vmCard QComboBox {"
        "  background: #f8faff;"
        "  color: #3a5080;"
        "  border: 1px solid #c8d4e8;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  height: 28px;"
        "  font-size: 12px;"
        "}"
        "#vmCard QComboBox:hover { border-color: #3b71ca; }"
        "#vmCard QComboBox QAbstractItemView {"
        "  background: #ffffff;"
        "  color: #1a2a4a;"
        "  selection-background-color: #e8f0fe;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    // --- 顶部：OS图标 + 名称+ID ---
    QHBoxLayout *headerRow = new QHBoxLayout();

    m_lblOsIcon = new QLabel();
    m_lblOsIcon->setAlignment(Qt::AlignCenter);
    m_lblOsIcon->setFixedSize(48, 48);
    // 从 Qt 资源系统中加载 SVG 图标，确保跨平台显示
    QPixmap osPix(osIcon());
    if (!osPix.isNull()) {
        m_lblOsIcon->setPixmap(osPix.scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QVBoxLayout *nameCol = new QVBoxLayout();
    m_lblName = new QLabel(name);
    m_lblName->setStyleSheet("font-size: 15px; font-weight: bold; color: #1a2a4a;");
    m_lblName->setWordWrap(true);
    QLabel *lblIdNode = new QLabel(QString("VM %1  [%2]").arg(vmId).arg(node));
    lblIdNode->setStyleSheet("font-size: 12px; color: #7a8aaa;");
    nameCol->addWidget(m_lblName);
    nameCol->addWidget(lblIdNode);

    headerRow->addWidget(m_lblOsIcon);
    headerRow->addLayout(nameCol);
    layout->addLayout(headerRow);

    // --- 状态标签与 IP 标签同行 ---
    QHBoxLayout *statusRow = new QHBoxLayout();
    m_lblStatus = new QLabel();
    updateStatusStyle();
    statusRow->addWidget(m_lblStatus);
    statusRow->addSpacing(10);

    m_lblIp = new QLabel(""); // 初始不显示文字也不占据明显空间
    m_lblIp->setStyleSheet("font-size: 11px; color: #8899aa;");
    m_lblIp->hide(); // 默认隐藏，等有 IP 或者有状态信息时再显示
    statusRow->addWidget(m_lblIp);
    statusRow->addStretch();

    layout->addLayout(statusRow);

    // --- 分割线 ---
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: rgba(0,0,0,0.08);");
    layout->addWidget(line);

    // --- 合并后的底部操作排 ---
    QHBoxLayout *bottomActRow = new QHBoxLayout();
    bottomActRow->setSpacing(8);

    auto makeCombo = [](const QString &prefix) -> QComboBox* { return new QComboBox(); };

    m_cmbProtocol = makeCombo("");
    m_cmbProtocol->addItem("SPICE");
    m_cmbProtocol->addItem("RDP");
    m_cmbProtocol->setFixedWidth(68);
    connect(m_cmbProtocol, &QComboBox::currentTextChanged, this, &VmCard::onProtocolChanged);

    m_cmbPower = makeCombo("");
    m_cmbPower->addItem("电源管理");
    m_cmbPower->addItem("开机");
    m_cmbPower->addItem("关机");
    m_cmbPower->addItem("重启");
    m_cmbPower->addItem("断电");
    m_cmbPower->setFixedWidth(86);
    connect(m_cmbPower, QOverload<int>::of(&QComboBox::activated), this, [this](int idx) {
        QStringList actions = {"", "start", "shutdown", "reset", "stop"};
        if (idx > 0 && idx < actions.size()) { onPowerAction(actions[idx]); }
    });

    // 隐藏的 RDP 端口列
    QLabel *lblPortLabel = new QLabel("端口");
    lblPortLabel->hide();
    lblPortLabel->setObjectName("lblRdpPort");
    m_editRdpPort = new QLineEdit("3389");
    m_editRdpPort->hide();

    // 如果是运行状态且有 qemu-agent，启动时异步获取 IP
    if (status == "running") {
        m_waitingForIp = true;
        m_apiClient->fetchVmIp(node, vmId);
    }

    m_btnSnapshot = new QPushButton("快照");
    m_btnSnapshot->setCursor(Qt::PointingHandCursor);
    m_btnSnapshot->setStyleSheet(
        "QPushButton { border: 1px solid #c8d4e8; border-radius: 6px; padding: 4px 10px; color: #556a8a; font-size: 12px; background: transparent; }"
        "QPushButton:hover { border-color: #3b71ca; color: #3b71ca; }"
    );
    connect(m_btnSnapshot, &QPushButton::clicked, this, [this](){ QMessageBox::information(this,"快照","快照管理..."); });

    m_btnConnect = new QPushButton("连接桌面");
    m_btnConnect->setCursor(Qt::PointingHandCursor);
    m_btnConnect->setStyleSheet(
        "QPushButton { border: none; border-radius: 6px; padding: 6px 16px; color: white; font-size: 13px; font-weight: bold; background-color: #1a56db; }"
        "QPushButton:hover { background-color: #1e65e8; }"
        "QPushButton:disabled { background-color: #aab4cc; }"
    );

    bool running = (status == "running");
    m_btnConnect->setEnabled(running);
    if (!running) m_btnConnect->setToolTip("虚拟机未运行");
    connect(m_btnConnect, &QPushButton::clicked, this, &VmCard::onConnectClicked);

    bottomActRow->addWidget(m_cmbProtocol);
    bottomActRow->addWidget(m_cmbPower);
    bottomActRow->addWidget(m_btnSnapshot);
    bottomActRow->addStretch();
    bottomActRow->addWidget(m_btnConnect);

    layout->addLayout(bottomActRow);
    
    // 隐藏行容器（适配 Protocol 切换）
    QWidget *portRowWidget = new QWidget();
    portRowWidget->setObjectName("portRowWidget");
    QHBoxLayout *prL = new QHBoxLayout(portRowWidget);
    prL->setContentsMargins(0,0,0,0);
    prL->addWidget(lblPortLabel);
    prL->addWidget(m_editRdpPort);
    prL->addStretch();
    layout->addWidget(portRowWidget);
    portRowWidget->hide();
}

// ========== 工具方法 ==========

QString VmCard::osIcon() const
{
    QString name = m_vmInfo["name"].toString().toLower();
    if (name.contains("win"))    return ":/icons/os_windows.svg";
    if (name.contains("ubuntu")) return ":/icons/os_linux.svg";
    if (name.contains("debian")) return ":/icons/os_linux.svg";
    if (name.contains("centos") || name.contains("rhel")) return ":/icons/os_redhat.svg";
    if (name.contains("mac"))    return ":/icons/os_macos.svg";
    return ":/icons/os_generic.svg";
}

void VmCard::updateStatusStyle()
{
    QString status = m_vmInfo["status"].toString();
    if (status == "running") {
        m_lblStatus->setText("● 运行中");
        m_lblStatus->setStyleSheet("color: #27ae60; font-size: 13px; font-weight: bold;");
    } else if (status == "stopped") {
        m_lblStatus->setText("○ 已关机");
        m_lblStatus->setStyleSheet("color: #7f8c8d; font-size: 13px;");
    } else {
        m_lblStatus->setText("◐ " + status);
        m_lblStatus->setStyleSheet("color: #e67e22; font-size: 13px;");
    }
}

QString VmCard::node()  const { return m_vmInfo["node"].toString(); }
int     VmCard::vmId()  const { return m_vmInfo["vmid"].toInt(); }

// ========== 槽函数 ==========

void VmCard::onConnectClicked()
{
    QString protocol = m_cmbProtocol->currentText();
    QString name = m_vmInfo["name"].toString();
    int     vid  = m_vmInfo["vmid"].toInt();
    QString nd   = m_vmInfo["node"].toString();

    if (protocol == "SPICE") {
        // SPICE 直接连接
        ConnectionInfo info;
        info.id         = ConnectionInfo::generateId();
        info.name       = name;
        info.serverHost = m_configManager->pveHost();
        info.serverPort = m_configManager->pvePort();
        info.node       = nd;
        info.vmId       = vid;
        info.protocol   = Protocol::SPICE;
        info.resolution = "fullscreen";
        m_connectionManager->connectTo(info);
    } else {
        // RDP
        if (m_rdpIp.isEmpty()) {
            // 还没获取到 IP，立即异步获取
            m_btnConnect->setEnabled(false);
            m_btnConnect->setText("⏳ 获取 IP...");
            m_waitingForIp = true;
            m_apiClient->fetchVmIp(nd, vid);
        } else {
            // 已有 IP，直接连接
            doRdpConnect();
        }
    }
}

void VmCard::onIpReceived(const QStringList &ips)
{
    bool wasWaiting = m_waitingForIp;
    m_waitingForIp = false;
    m_btnConnect->setEnabled(true);
    m_btnConnect->setText("\u25b6  \u8fde\u63a5\u684c\u9762");

    if (!ips.isEmpty()) {
        m_rdpIp = ips.first();
        // 更新卡片上的 IP 显示
        m_lblIp->setText(QString("• IP: %1").arg(m_rdpIp));
        m_lblIp->setStyleSheet("font-size: 11px; color: #2e86c1; font-weight: bold;");
        m_lblIp->show();
        // 如果是用户点连接触发的 IP 获取，自动建立连接
        if (wasWaiting && m_cmbProtocol->currentText() == "RDP") {
            doRdpConnect();
        }
    } else {
        m_rdpIp.clear();
        m_lblIp->setText("• IP: 未\u83b7取\uff08\u672a\u5b89\u88c5 agent\uff09");
        m_lblIp->setStyleSheet("font-size: 11px; color: #c0392b;");
        if (wasWaiting && m_cmbProtocol->currentText() == "RDP") {
            QMessageBox::warning(this, "\u63d0\u793a",
                "\u672a\u83b7\u53d6\u5230 IP\uff0c\u65e0\u6cd5\u8fde\u63a5 RDP\u3002\n"
                "\u8bf7\u786e\u8ba4 qemu-guest-agent \u5df2\u5b89\u88c5\u4e14 VM \u5df2\u542f\u52a8\u3002");
        }
    }
}

void VmCard::onPowerAction(const QString &action)
{
    int vmId     = m_vmInfo["vmid"].toInt();
    QString node = m_vmInfo["node"].toString();
    QString name = m_vmInfo["name"].toString();

    QString confirmMsg;
    if (action == "stop")  confirmMsg = QString("确定强制关闭 %1 吗？").arg(name);
    if (action == "reset") confirmMsg = QString("确定重启 %1 吗？").arg(name);

    if (!confirmMsg.isEmpty()) {
        if (QMessageBox::question(this, "确认操作", confirmMsg,
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
    }

    qDebug() << "电源操作:" << action << "VM:" << vmId << "节点:" << node;
    if (action == "start")    m_apiClient->startVM(node, vmId);
    else if (action == "shutdown" || action == "stop" || action == "reset")
        m_apiClient->stopVM(node, vmId);   // 后续可细化 stop/reset 指令

    m_cmbPower->setCurrentIndex(0);
}

void VmCard::onConnectionStarted(const ConnectionInfo &info)
{
    if (info.node == node() && info.vmId == vmId()) {
        m_btnConnect->setText("已连接");
        m_btnConnect->setEnabled(false);
    }
}

void VmCard::onConnectionEnded(const ConnectionInfo &info, int)
{
    if (info.node == node() && info.vmId == vmId()) {
        m_btnConnect->setText("▶  连接桌面");
        m_btnConnect->setEnabled(m_vmInfo["status"].toString() == "running");
    }
}

void VmCard::onConnectionError(const ConnectionInfo &info, const QString &error)
{
    if (info.node == node() && info.vmId == vmId()) {
        m_waitingForIp = false;
        m_btnConnect->setText("▶  连接桌面");
        m_btnConnect->setEnabled(m_vmInfo["status"].toString() == "running");

        // 仅在调试模式开启时，才弹出详细错误信息
        if (m_configManager->debugMode()) {
            QMessageBox::critical(parentWidget(), "连接错误", error);
        }
    }
}

// 直接用 m_rdpIp + 端口 + ConfigManager 外设设置 发起 RDP 连接（不弹窗）
void VmCard::doRdpConnect()
{
    if (m_rdpIp.isEmpty()) return;

    int rdpPort = m_editRdpPort->text().toInt();
    if (rdpPort <= 0) rdpPort = 3389;

    ConnectionInfo info;
    info.id         = ConnectionInfo::generateId();
    info.name       = m_vmInfo["name"].toString();
    info.serverHost = m_configManager->pveHost();
    info.serverPort = m_configManager->pvePort();
    info.node       = m_vmInfo["node"].toString();
    info.vmId       = m_vmInfo["vmid"].toInt();
    info.protocol   = Protocol::RDP;
    info.rdpHost    = m_rdpIp;
    info.rdpPort    = rdpPort;
    // 从全局设置读取外设重定向选项
    info.enableSound      = m_configManager->rdpSound();
    info.enableMicrophone = m_configManager->rdpMicrophone();
    info.enableClipboard  = m_configManager->rdpClipboard();
    info.enableUSBDrive   = m_configManager->rdpUsbDrive();
    info.enableSmartcard  = m_configManager->rdpSmartcard();
    info.enablePrinter    = m_configManager->rdpPrinter();
    info.resolution       = "fullscreen";

    m_connectionManager->connectTo(info);
}

// 切换协议时显隐 RDP 端口输入框
void VmCard::onProtocolChanged(const QString &protocol)
{
    bool isRdp = (protocol == "RDP");
    QWidget *portRowWidget = findChild<QWidget*>("portRowWidget");
    if (portRowWidget) portRowWidget->setVisible(isRdp);
    
    // IP 标签仅在 RDP 模式时才显示（并且有值时才显示）
    if (isRdp && !m_lblIp->text().isEmpty()) {
        m_lblIp->show();
    } else {
        m_lblIp->hide();
    }
}
