#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QIntValidator>
#include <QMessageBox>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QNetworkProxy>
#include <QSignalBlocker>
#include <QDebug>
#include "../core/XfreerdpDetector.h"
#include "../core/EasyTierManager.h"
#include "../core/DebugLogger.h"

// ========== 控件公共样式（明亮，适合对话框） ==========
static const QString INPUT_STYLE = R"(
    QLineEdit {
        border: 1.5px solid #c8d4e8;
        border-radius: 7px;
        padding: 5px 10px;
        font-size: 13px;
        color: #1a2a4a;
        background-color: #f5f8ff;
        min-height: 32px;
    }
    QLineEdit:focus {
        border-color: #3b71ca;
        background-color: #ffffff;
    }
    QLineEdit:disabled {
        background-color: #eaedf2;
        color: #8899aa;
    }
)";

static const QString COMBO_STYLE = R"(
    QComboBox {
        border: 1.5px solid #c8d4e8;
        border-radius: 7px;
        padding: 5px 10px;
        font-size: 13px;
        color: #1a2a4a;
        background-color: #f5f8ff;
        min-height: 32px;
    }
    QComboBox:focus {
        border-color: #3b71ca;
    }
    QComboBox::drop-down {
        border: none;
        width: 24px;
    }
    QComboBox QAbstractItemView {
        border: 1px solid #c8d4e8;
        background-color: #ffffff;
        color: #1a2a4a;
        selection-background-color: #e8f0fe;
        outline: none;
    }
)";

static const QString CHECK_STYLE = R"(
    QCheckBox {
        font-size: 13px;
        color: #2a3a5a;
        spacing: 8px;
    }
    QCheckBox::indicator {
        width: 16px; height: 16px;
        border: 1.5px solid #c8d4e8;
        border-radius: 4px;
        background: #f5f8ff;
    }
    QCheckBox::indicator:checked {
        background: #3b71ca;
        border-color: #3b71ca;
    }
)";

// EasyTier P2P 按钮样式：绿色启用/灰色禁用
static const QString BTN_START_P2P_ENABLED = R"(
    QPushButton {
        background-color: #28a745;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #218838;
    }
    QPushButton:pressed {
        background-color: #1e7e34;
    }
)";

static const QString BTN_START_P2P_DISABLED = R"(
    QPushButton {
        background-color: #d3d3d3;
        color: #666666;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: bold;
    }
)";

// EasyTier P2P 停止按钮样式：红色启用/灰色禁用
static const QString BTN_STOP_P2P_ENABLED = R"(
    QPushButton {
        background-color: #dc3545;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #c82333;
    }
    QPushButton:pressed {
        background-color: #bd2130;
    }
)";

static const QString BTN_STOP_P2P_DISABLED = R"(
    QPushButton {
        background-color: #d3d3d3;
        color: #666666;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: bold;
    }
)";

SettingsDialog::SettingsDialog(ConfigManager *config, EasyTierManager *runtimeManager, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
    , m_easyTierRuntimeManager(runtimeManager)
{
    setWindowTitle("客户端设置");
    setMinimumWidth(1100);  // 三栏布局，增加宽度
    setMinimumHeight(520);  // 确保高度在一个合理的范围内
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    applyDialogStyle();
    setupUI();
    adjustSize();           // 根据内容自动调整尺寸
}

void SettingsDialog::applyDialogStyle()
{
    // 对话框本身用白色/浅色背景，避免继承全局深色样式
    setStyleSheet(R"(
        QDialog {
            background-color: #f0f4fa;
        }
        QLabel {
            background-color: transparent;
            color: #1a2a4a;
        }
        QGroupBox {
            font-size: 13px;
            font-weight: bold;
            color: #2a3a5a;
            border: 1px solid #cfd9ee;
            border-radius: 10px;
            margin-top: 16px;
            padding-top: 10px;
            background-color: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 14px;
            padding: 2px 10px;
            color: #1f4ea3;
            font-size: 12px;
            font-weight: 700;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #eef4ff, stop:1 #e2ecff);
            border: 1px solid #c9d8f4;
            border-radius: 9px;
        }
        QLabel {
            font-size: 13px;
            color: #2a3a5a;
        }
        QPushButton {
            border: 1px solid #c8d4e8;
            border-radius: 7px;
            padding: 6px 16px;
            font-size: 13px;
            color: #2a3a5a;
            background-color: #f0f4fa;
            min-height: 32px;
        }
        QPushButton:hover {
            background-color: #e0e8f8;
            border-color: #3b71ca;
        }
        QPushButton#primaryButton {
            background-color: #1a56db;
            color: white;
            border: none;
            font-weight: bold;
        }
        QPushButton#primaryButton:hover {
            background-color: #1e65e8;
        }
        QPushButton#testBtn {
            background-color: #f0f4fa;
            color: #3b71ca;
            border: 1px solid #3b71ca;
        }
        QPushButton#testBtn:hover {
            background-color: #e8f0fe;
        }
        QCheckBox {
            font-size: 13px;
            color: #1a2a4a;
            spacing: 8px;
            background: transparent;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1.5px solid #c8d4e8;
            border-radius: 4px;
            background: #f5f8ff;
        }
        QCheckBox::indicator:checked {
            background-color: #3b71ca;
            border-color: #3b71ca;
        }
    )");
}

void SettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(20, 18, 20, 12);

    // 中间主要内容区：三栏布局
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // 左侧栏：PVE 服务器 + 行为
    QVBoxLayout *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(14);

    // 中间栏：EasyTier P2P + RDP 外设
    QVBoxLayout *middleColumn = new QVBoxLayout();
    middleColumn->setSpacing(14);

    // 右侧栏：RDP 外设 + FreeRDP 设置
    QVBoxLayout *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(14);

    // ===== 服务器配置区 =====
    QGroupBox *serverGroup = new QGroupBox("PVE 服务器");
    QVBoxLayout *serverLayout = new QVBoxLayout(serverGroup);
    serverLayout->setSpacing(10);
    serverLayout->setContentsMargins(16, 16, 16, 14);

    // --- 第一行：服务器地址 ---
    QHBoxLayout *addrRow = new QHBoxLayout();
    QLabel *lblHost = new QLabel("内网/公网服务器地址:");
    lblHost->setFixedWidth(130);
    m_editHost = new QLineEdit();
    m_editHost->setPlaceholderText("例: pve.example.com / 公网地址 / EasyTier地址");
    m_editHost->setText(m_config->pveHost());
    m_editHost->setStyleSheet(INPUT_STYLE);
    addrRow->addWidget(lblHost);
    addrRow->addWidget(m_editHost);
    serverLayout->addLayout(addrRow);

    // --- 第二行：PVE 内网地址（可选）---
    QHBoxLayout *lanRow = new QHBoxLayout();
    QLabel *lblLanHost = new QLabel("P2P内网地址:<br><span style='color:#dc2626;'>（服务器P2P地址）</span>");
    lblLanHost->setWordWrap(true);
    lblLanHost->setFixedWidth(150);
    m_editLanHost = new QLineEdit();
    m_editLanHost->setPlaceholderText("可选：若启用P2P，优先用此地址访问PVE");
    m_editLanHost->setText(m_config->pveLanHost());
    m_editLanHost->setStyleSheet(INPUT_STYLE);
    lanRow->addWidget(lblLanHost);
    lanRow->addWidget(m_editLanHost);
    serverLayout->addLayout(lanRow);

    // --- 第三行：端口 + 测试按钮 + 结果 ---
    QHBoxLayout *portRow = new QHBoxLayout();
    QLabel *lblPort = new QLabel("端口号:");
    lblPort->setFixedWidth(130);
    m_editPort = new QLineEdit();
    m_editPort->setFixedWidth(90);
    m_editPort->setPlaceholderText("8006");
    m_editPort->setText(QString::number(m_config->pvePort()));
    m_editPort->setValidator(new QIntValidator(1, 65535, this));
    m_editPort->setStyleSheet(INPUT_STYLE);

    m_lblTestResult = new QLabel("");
    m_lblTestResult->setWordWrap(true);
    m_lblTestResult->setStyleSheet("font-size: 12px; color: #27ae60;");

    m_btnTest = new QPushButton("测试连接");
    m_btnTest->setObjectName("testBtn");
    m_btnTest->setCursor(Qt::PointingHandCursor);
    m_btnTest->setFixedWidth(90);
    connect(m_btnTest, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);

    portRow->addWidget(lblPort);
    portRow->addWidget(m_editPort);
    portRow->addWidget(m_btnTest);
    serverLayout->addLayout(portRow);

    // --- 第四行：是否使用P2P连接 ---
    m_chkUseEasyTier = new QCheckBox("使用 P2P 连接 (EasyTier)");
    m_chkUseEasyTier->setStyleSheet(CHECK_STYLE);
    m_chkUseEasyTier->setChecked(m_config->useEasyTier());
    connect(m_chkUseEasyTier, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            m_btnTest->click();
        }
    });
    serverLayout->addWidget(m_chkUseEasyTier);

    // --- 第五行：测试结果 ---
    serverLayout->addWidget(m_lblTestResult);

    leftColumn->addWidget(serverGroup);

    // ===== 行为设置区 =====
    QGroupBox *behaviorGroup = new QGroupBox("行为");
    QVBoxLayout *behaviorLayout = new QVBoxLayout(behaviorGroup);
    behaviorLayout->setSpacing(8);
    behaviorLayout->setContentsMargins(16, 16, 16, 14);

    m_chkKioskMode = new QCheckBox("Kiosk 模式（全屏锁定，禁止任务栏操作）");
    m_chkKioskMode->setChecked(m_config->kioskMode());
    m_chkKioskMode->setStyleSheet(CHECK_STYLE);
    behaviorLayout->addWidget(m_chkKioskMode);

    m_chkAutoConnect = new QCheckBox("启动时自动登录");
    m_chkAutoConnect->setChecked(m_config->autoLogin());
    m_chkAutoConnect->setStyleSheet(CHECK_STYLE);
    connect(m_chkAutoConnect, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            return;
        }

        QStringList missing;
        if (m_config->pveUsername().trimmed().isEmpty()) {
            missing << "用户名";
        }
        if (!m_config->rememberPassword()) {
            missing << "记住密码";
        }
        if (m_config->pvePassword().isEmpty()) {
            missing << "密码";
        }

        if (!missing.isEmpty()) {
            const QSignalBlocker blocker(m_chkAutoConnect);
            m_chkAutoConnect->setChecked(false);
            QMessageBox::warning(this, "自动登录条件不完整",
                                 QString("启用“启动时自动登录”前，请先补充：\n%1")
                                     .arg(missing.join("、")));
        }
    });
    behaviorLayout->addWidget(m_chkAutoConnect);

    m_chkDebugMode = new QCheckBox("调试模式（显示连接过程中的详细错误信息）");
    m_chkDebugMode->setChecked(m_config->debugMode());
    m_chkDebugMode->setStyleSheet(CHECK_STYLE);
    behaviorLayout->addWidget(m_chkDebugMode);

    leftColumn->addWidget(behaviorGroup);

    // ===== 品牌显示设置区 =====
    QGroupBox *brandingGroup = new QGroupBox("品牌显示");
    QFormLayout *brandingLayout = new QFormLayout(brandingGroup);
    brandingLayout->setLabelAlignment(Qt::AlignRight);
    brandingLayout->setSpacing(10);
    brandingLayout->setContentsMargins(16, 16, 16, 14);

    m_editLoginWelcomeText = new QLineEdit();
    m_editLoginWelcomeText->setStyleSheet(INPUT_STYLE);
    m_editLoginWelcomeText->setText(m_config->loginWelcomeText());
    m_editLoginWelcomeText->setPlaceholderText("登录页欢迎语，例如：欢迎使用云桌面");
    brandingLayout->addRow("登录欢迎语:", m_editLoginWelcomeText);

    m_editFooterBrandText = new QLineEdit();
    m_editFooterBrandText->setStyleSheet(INPUT_STYLE);
    m_editFooterBrandText->setText(m_config->footerBrandText());
    m_editFooterBrandText->setPlaceholderText("底部品牌文案，例如：XXXX科技有限公司");
    brandingLayout->addRow("底部品牌文案:", m_editFooterBrandText);

    leftColumn->addWidget(brandingGroup);

    // ===== EasyTier P2P 设置区 =====
    QGroupBox *easytierGroup = new QGroupBox("EasyTier P2P");
    QFormLayout *easytierLayout = new QFormLayout(easytierGroup);
    easytierLayout->setLabelAlignment(Qt::AlignRight);
    easytierLayout->setSpacing(10);
    easytierLayout->setContentsMargins(16, 16, 16, 14);

    m_lblEasyTierStatus = new QLabel("EasyTier P2P 状态: 未检测");
    m_lblEasyTierStatus->setWordWrap(true);
    m_lblEasyTierStatus->setStyleSheet("font-size: 12px; color: #6b7280;");
    easytierLayout->addRow("连接状态:", m_lblEasyTierStatus);

    // P2P 按钮已全部移到登录页面，不在设置页显示

    m_editEasyTierNetworkName = new QLineEdit();
    m_editEasyTierNetworkName->setStyleSheet(INPUT_STYLE);
    m_editEasyTierNetworkName->setText(m_config->easyTierNetworkName());
    easytierLayout->addRow("EasyTier 网络名:", m_editEasyTierNetworkName);

    m_editEasyTierNetworkSecret = new QLineEdit();
    m_editEasyTierNetworkSecret->setStyleSheet(INPUT_STYLE);
    m_editEasyTierNetworkSecret->setText(m_config->easyTierNetworkSecret());
    easytierLayout->addRow("EasyTier 密钥:", m_editEasyTierNetworkSecret);

    m_editEasyTierServerPeerIp = new QLineEdit();
    m_editEasyTierServerPeerIp->setStyleSheet(INPUT_STYLE);
    m_editEasyTierServerPeerIp->setText(m_config->easyTierServerPeerIp());
    m_editEasyTierServerPeerIp->setPlaceholderText("必填：PVE服务器在EasyTier中的P2P地址，例如 10.126.126.1");
    easytierLayout->addRow("服务器P2P地址:", m_editEasyTierServerPeerIp);

    m_editEasyTierBootstrapUrl = new QLineEdit();
    m_editEasyTierBootstrapUrl->setStyleSheet(INPUT_STYLE);
    m_editEasyTierBootstrapUrl->setText(m_config->easyTierBootstrapUrl());
    m_editEasyTierBootstrapUrl->setPlaceholderText("可选：轻量级引导节点 URL");
    easytierLayout->addRow("EasyTier 公共节点:", m_editEasyTierBootstrapUrl);

    m_editEasyTierExtraArgs = new QLineEdit();
    m_editEasyTierExtraArgs->setStyleSheet(INPUT_STYLE);
    m_editEasyTierExtraArgs->setText(m_config->easyTierExtraArgs());
    m_editEasyTierExtraArgs->setPlaceholderText("可选：额外命令参数，例如 --log-level debug");
    easytierLayout->addRow("EasyTier 额外参数:", m_editEasyTierExtraArgs);

    QLabel *hint = new QLabel("点击“启动P2P”后将启动 easytier-core 并持续尝试组网；点击“停止P2P”可随时终止进程。\n若组网成功，建议再勾选“使用 P2P 连接”。");
    hint->setWordWrap(true);
    hint->setStyleSheet("font-size: 12px; color: #5d6f8e;");
    easytierLayout->addRow(hint);

    middleColumn->addWidget(easytierGroup);

    contentLayout->addLayout(leftColumn, 1);
    contentLayout->addLayout(middleColumn, 1);

    // ===== RDP 外设重定向区 =====
    QGroupBox *rdpGroup = new QGroupBox("RDP 外设重定向");
    QVBoxLayout *rdpLayout = new QVBoxLayout(rdpGroup);
    rdpLayout->setSpacing(8);
    rdpLayout->setContentsMargins(16, 16, 16, 14);

    m_chkRdpSound = new QCheckBox("声音输出");
    m_chkRdpMic   = new QCheckBox("麦克风输入");
    m_chkRdpClipboard = new QCheckBox("剪贴板共享");
    m_chkRdpUsb   = new QCheckBox("USB 驱动器");
    m_chkRdpSmartcard = new QCheckBox("智能卡 / U-Key");
    m_chkRdpPrinter   = new QCheckBox("打印机");

    m_chkRdpSound->setChecked(m_config->rdpSound());
    m_chkRdpMic->setChecked(m_config->rdpMicrophone());
    m_chkRdpClipboard->setChecked(m_config->rdpClipboard());
    m_chkRdpUsb->setChecked(m_config->rdpUsbDrive());
    m_chkRdpSmartcard->setChecked(m_config->rdpSmartcard());
    m_chkRdpPrinter->setChecked(m_config->rdpPrinter());

    for (QCheckBox *chk : {m_chkRdpSound, m_chkRdpMic, m_chkRdpClipboard,
                            m_chkRdpUsb,   m_chkRdpSmartcard, m_chkRdpPrinter}) {
        chk->setStyleSheet(CHECK_STYLE);
        rdpLayout->addWidget(chk);
    }

    middleColumn->addWidget(rdpGroup);

    // ===== FreeRDP 高级设置区 =====
    QGroupBox *freerdpGroup = new QGroupBox("FreeRDP 设置");
    QGridLayout *freerdpLayout = new QGridLayout(freerdpGroup);
    freerdpLayout->setSpacing(12);
    freerdpLayout->setContentsMargins(16, 16, 16, 14);

    // 行 1
    m_comboRdpVersion = new QComboBox();
    m_comboRdpVersion->addItems({"2", "3", "Auto"});
    m_comboRdpVersion->setStyleSheet(COMBO_STYLE);
    m_comboRdpVersion->setCurrentText(m_config->rdpVersion() == 3 ? "3" : (m_config->rdpVersion() == 2 ? "2" : "Auto"));

    m_comboRdpCodec = new QComboBox();
    m_comboRdpCodec->addItems({"软件解码", "h264:420", "h264:444"});
    m_comboRdpCodec->setStyleSheet(COMBO_STYLE);
    m_comboRdpCodec->setCurrentText(m_config->rdpCodec());

    freerdpLayout->addWidget(new QLabel("Freerdp 版本"), 0, 0);
    freerdpLayout->addWidget(m_comboRdpVersion, 0, 1);
    freerdpLayout->addWidget(new QLabel("解码"), 0, 2);
    freerdpLayout->addWidget(m_comboRdpCodec, 0, 3);

    // 行 2
    m_comboRdpColorDepth = new QComboBox();
    m_comboRdpColorDepth->addItems({"16", "24", "32"});
    m_comboRdpColorDepth->setStyleSheet(COMBO_STYLE);
    m_comboRdpColorDepth->setCurrentText(QString::number(m_config->rdpColorDepth()));

    m_comboRdpScale = new QComboBox();
    m_comboRdpScale->addItems({"100%", "125%", "150%", "180%", "200%"});
    m_comboRdpScale->setStyleSheet(COMBO_STYLE);
    m_comboRdpScale->setCurrentText(m_config->rdpScale());

    freerdpLayout->addWidget(new QLabel("色深"), 1, 0);
    freerdpLayout->addWidget(m_comboRdpColorDepth, 1, 1);
    freerdpLayout->addWidget(new QLabel("缩放"), 1, 2);
    freerdpLayout->addWidget(m_comboRdpScale, 1, 3);

    // 行 3
    m_comboRdpNetwork = new QComboBox();
    m_comboRdpNetwork->addItems({"auto", "lan", "broadband", "modem"});
    m_comboRdpNetwork->setStyleSheet(COMBO_STYLE);
    m_comboRdpNetwork->setCurrentText(m_config->rdpNetwork());

    freerdpLayout->addWidget(new QLabel("网络速率"), 2, 0);
    freerdpLayout->addWidget(m_comboRdpNetwork, 2, 1);

    rightColumn->addWidget(freerdpGroup);
    rightColumn->addStretch(); // 右侧如果项少，底部留白

    contentLayout->addLayout(rightColumn, 1);
    mainLayout->addLayout(contentLayout);
    mainLayout->addSpacing(12);


    // ===== 底部按钮 =====
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *btnCancel = new QPushButton("取消");
    btnCancel->setFixedWidth(80);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(btnCancel);

    btnLayout->addSpacing(8);

    QPushButton *btnSave = new QPushButton("保  存");
    btnSave->setObjectName("primaryButton");
    btnSave->setDefault(true);
    btnSave->setFixedWidth(90);
    connect(btnSave, &QPushButton::clicked, this, &SettingsDialog::onAccepted);
    btnLayout->addWidget(btnSave);

    mainLayout->addLayout(btnLayout);

    // 初始化 EasyTier 控件状态并监听运行时状态变化
    if (m_easyTierRuntimeManager) {
        connect(m_easyTierRuntimeManager, &EasyTierManager::stateChanged,
                this, &SettingsDialog::onEasyTierStateChanged);
        connect(m_easyTierRuntimeManager, &EasyTierManager::linkModeUpdated,
                this, &SettingsDialog::onEasyTierLinkModeChanged);

        if (m_easyTierRuntimeManager->isRunning()) {
            onEasyTierStateChanged(m_easyTierRuntimeManager->state());
        } else {
            setEasyTierAvailability(false, "EasyTier P2P 状态: 已停止");
        }
    } else {
        setEasyTierAvailability(false, "EasyTier P2P 状态: 运行管理器未初始化");
    }
    updateEasyTierButtonState();
}

// ========== 槽函数 ==========

void SettingsDialog::onTestConnection()
{
    const QString publicHost = m_editHost->text().trimmed();
    const QString p2pHost = m_editLanHost->text().trimmed();
    QString host;
    QString hostSource;

    // 规则：未勾选 -> 内网/公网服务器地址；勾选且P2P可用 -> P2P内网地址
    if (m_chkUseEasyTier->isChecked()) {
        if (!m_easyTierAvailable) {
            qWarning().noquote() << "[ConnTest] 取消测试: 已勾选P2P, 但EasyTier不可用";
            m_lblTestResult->setStyleSheet("font-size: 12px; color: #c0392b;");
            m_lblTestResult->setText("X P2P网络当前不可用，请先启动P2P并等待组网成功。");
            return;
        }
        if (p2pHost.isEmpty()) {
            qWarning().noquote() << "[ConnTest] 取消测试: 已勾选P2P, 但P2P内网地址为空";
            m_lblTestResult->setStyleSheet("font-size: 12px; color: #c0392b;");
            m_lblTestResult->setText("X 已勾选P2P连接，请先填写P2P内网地址");
            return;
        }
        host = p2pHost;
        hostSource = "P2P内网地址";
    } else {
        host = publicHost;
        hostSource = "内网/公网服务器地址";
    }

    int port = m_editPort->text().toInt();
    if (host.isEmpty()) {
        qWarning().noquote() << "[ConnTest] 取消测试: 目标地址为空, 来源=" << hostSource;
        m_lblTestResult->setStyleSheet("font-size: 12px; color: #c0392b;");
        m_lblTestResult->setText("X 请先输入服务器地址");
        return;
    }
    if (port <= 0) port = 8006;

    const QString decisionReason = m_chkUseEasyTier->isChecked()
            ? "已勾选P2P且P2P可用，使用P2P内网地址"
            : "未勾选P2P，使用内网/公网服务器地址";
    qInfo().noquote() << QString("[ConnTest] 目标=%1:%2 来源=%3 原因=%4")
                         .arg(host)
                         .arg(port)
                         .arg(hostSource)
                         .arg(decisionReason);

    m_btnTest->setEnabled(false);
    m_btnTest->setText("测试中...");
    m_lblTestResult->setStyleSheet("font-size: 12px; color: #7f8c8d;");
    m_lblTestResult->setText(QString("正在测试 %1: %2:%3 ...").arg(hostSource, host).arg(port));

    // 使用 TCP Socket 方式验证连通性
    // 避免直接发 HTTPS 请求——全志盒子 OpenSSL 环境不稳定，SSL 握手会触发 SIGSEGV 崩溃
    QTcpSocket *sock = new QTcpSocket(this);
    sock->setProxy(QNetworkProxy::NoProxy);

    // 连接成功 -> 说明目标端口可达
    connect(sock, &QTcpSocket::connected, this, [this, sock, hostSource]() {
        m_btnTest->setEnabled(true);
        m_btnTest->setText("测试连接");
        m_lblTestResult->setStyleSheet("font-size: 12px; color: #27ae60; font-weight: bold;");
        m_lblTestResult->setText(QString("成功！%1 可达: %2:%3")
                                 .arg(hostSource, sock->peerName().isEmpty() ? sock->peerAddress().toString() : sock->peerName())
                                 .arg(sock->peerPort()));
        qInfo().noquote() << QString("[ConnTest] 成功 来源=%1 对端=%2:%3")
                             .arg(hostSource)
                             .arg(sock->peerAddress().toString())
                             .arg(sock->peerPort());
        sock->disconnectFromHost();
        sock->deleteLater();
    });

    // 连接失败 -> 报告错误原因
    connect(sock, &QAbstractSocket::errorOccurred, this, [this, sock](QAbstractSocket::SocketError) {
        m_btnTest->setEnabled(true);
        m_btnTest->setText("测试连接");
        m_lblTestResult->setStyleSheet("font-size: 12px; color: #c0392b; font-weight: bold;");
        m_lblTestResult->setText(QString("X 连接失败: %1").arg(sock->errorString()));
        qWarning().noquote() << QString("[ConnTest] 失败 错误=%1").arg(sock->errorString());
        sock->deleteLater();
    });

    sock->connectToHost(host, static_cast<quint16>(port));
}


void SettingsDialog::onAccepted()
{
    QString host = m_editHost->text().trimmed();
    QString lanHost = m_editLanHost->text().trimmed();
    int port     = m_editPort->text().toInt();
    if (port <= 0) port = 8006;

    if (!host.isEmpty()) {
        m_config->setPveServer(host, port, m_config->pveUsername());
    }
    m_config->setPveLanHost(lanHost);

    if (m_chkUseEasyTier->isChecked() && lanHost.isEmpty()) {
        QMessageBox::warning(this, "配置不完整", "已勾选“是否使用 P2P 连接”，请填写 P2P 内网地址。");
        return;
    }

    m_config->setKioskMode(m_chkKioskMode->isChecked());

    // 启用“启动时自动登录”前，要求已保存用户名、密码且勾选记住密码
    if (m_chkAutoConnect->isChecked()) {
        QStringList missing;
        if (m_config->pveUsername().trimmed().isEmpty()) {
            missing << "用户名";
        }
        if (!m_config->rememberPassword()) {
            missing << "记住密码";
        }
        if (m_config->pvePassword().isEmpty()) {
            missing << "密码";
        }
        if (!missing.isEmpty()) {
            QMessageBox::warning(this, "自动登录条件不完整",
                                 QString("启用“启动时自动登录”前，请先补充：\n%1")
                                     .arg(missing.join("、")));
            return;
        }
    }
    m_config->setAutoLogin(m_chkAutoConnect->isChecked());
    m_config->setAutoConnect(m_config->autoConnect(), m_config->autoConnectId(), m_config->autoConnectProtocol());
    m_config->setLoginWelcomeText(m_editLoginWelcomeText->text());
    m_config->setFooterBrandText(m_editFooterBrandText->text());
    m_config->setDebugMode(m_chkDebugMode->isChecked());

    // EasyTier 设置
    m_config->setUseEasyTier(m_chkUseEasyTier->isChecked());
    m_config->setEasyTierBinaryPath(QString());
    m_config->setEasyTierNetworkName(m_editEasyTierNetworkName->text().trimmed());
    m_config->setEasyTierNetworkSecret(m_editEasyTierNetworkSecret->text().trimmed());
    m_config->setEasyTierServerPeerIp(m_editEasyTierServerPeerIp->text().trimmed());
    m_config->setEasyTierBootstrapUrl(m_editEasyTierBootstrapUrl->text().trimmed());
    m_config->setEasyTierExtraArgs(m_editEasyTierExtraArgs->text().trimmed());

    if (m_chkUseEasyTier->isChecked()) {
        QString err;
        if (!validateEasyTierInputs(&err)) {
            QMessageBox::warning(this, "EasyTier 参数不完整", err);
            return;
        }
    }

    m_config->setRdpRedirection(
        m_chkRdpSound->isChecked(),
        m_chkRdpMic->isChecked(),
        m_chkRdpClipboard->isChecked(),
        m_chkRdpUsb->isChecked(),
        m_chkRdpSmartcard->isChecked(),
        m_chkRdpPrinter->isChecked()
    );

    int version = m_comboRdpVersion->currentText() == "Auto" ? 0 : m_comboRdpVersion->currentText().toInt();

    // ========== 关键：FreeRDP 版本验证 ==========
    if (!validateXfreerdpVersion(version)) {
        // 版本验证失败或用户取消，不保存配置，保持对话框打开
        return;
    }

    m_config->setRdpAdvanced(
        version,
        m_comboRdpCodec->currentText(),
        m_comboRdpColorDepth->currentText().toInt(),
        m_comboRdpNetwork->currentText(),
        m_comboRdpScale->currentText(),
        m_config->rdpUsermode()
    );
    m_config->save();

    accept();
}

bool SettingsDialog::validateXfreerdpVersion(int selectedVersion)
{
    XfreerdpDetector &detector = XfreerdpDetector::instance();

    // 刷新检测（以防系统中途安装了新版本）
    detector.refresh();

    // 如果选择 "Auto"（版本 0），直接通过验证
    if (selectedVersion == 0) {
        if (detector.getAvailableVersions().isEmpty()) {
            QMessageBox::warning(this, "缺少依赖",
                "未检测到系统中安装 FreeRDP（xfreerdp 或 xfreerdp3）。\n\n"
                "请先安装 FreeRDP：\n"
                "  Ubuntu/Debian: sudo apt install freerdp2 freerdp3\n"
                "  Fedora: sudo dnf install freerdp freerdp3");
            return false;
        }
        return true;  // Auto 模式下有任意版本就通过
    }

    // 检查指定版本是否存在
    if (selectedVersion == 2) {
        if (!detector.hasXfreerdp2()) {
            QStringList available = detector.getAvailableVersions();
            QString availableMsg = available.isEmpty() ? "无任何版本"
                                 : "检测到版本：" + available.join("、");

            QMessageBox::StandardButton choice = QMessageBox::warning(this,
                "未找到 FreeRDP 2.x",
                QString("您选择的 FreeRDP 2.x 版本未找到。\n"
                        "系统中：%1\n\n"
                        "确定要继续保存此配置吗？\n"
                        "连接时将尝试使用其他可用版本。").arg(availableMsg),
                QMessageBox::Ok | QMessageBox::Cancel);

            return choice == QMessageBox::Ok;
        }
        return true;  // 版本存在，通过验证
    }

    if (selectedVersion == 3) {
        if (!detector.hasXfreerdp3()) {
            QStringList available = detector.getAvailableVersions();
            QString availableMsg = available.isEmpty() ? "无任何版本"
                                 : "检测到版本：" + available.join("、");

            QMessageBox::StandardButton choice = QMessageBox::warning(this,
                "未找到 FreeRDP 3.x",
                QString("您选择的 FreeRDP 3.x 版本未找到。\n"
                        "系统中：%1\n\n"
                        "确定要继续保存此配置吗？\n"
                        "连接时将尝试使用其他可用版本。").arg(availableMsg),
                QMessageBox::Ok | QMessageBox::Cancel);

            return choice == QMessageBox::Ok;
        }
        return true;  // 版本存在，通过验证
    }

    return false;  // 非法的版本号
}

bool SettingsDialog::validateEasyTierInputs(QString *errorMessage) const
{
    if (m_editEasyTierBootstrapUrl->text().trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = "请填写 EasyTier 公共节点。";
        return false;
    }
    if (m_editEasyTierNetworkName->text().trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = "请填写 EasyTier 网络名。";
        return false;
    }
    if (m_editEasyTierNetworkSecret->text().trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = "请填写 EasyTier 网络密钥。";
        return false;
    }
    if (m_editEasyTierServerPeerIp->text().trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = "请填写 服务器P2P地址，用于从 easytier-cli peer 中精确匹配服务端。";
        return false;
    }
    return true;
}

void SettingsDialog::onEasyTierStateChanged(EasyTierState state)
{
    switch (state) {
    case EasyTierState::Stopped:
        setEasyTierAvailability(false, "EasyTier P2P 状态: 已停止");
        break;
    case EasyTierState::Starting:
        setEasyTierAvailability(false, "EasyTier P2P 状态: 正在打洞/组网中（可随时点击停止P2P）");
        break;
    case EasyTierState::Connected:
        if (m_easyTierLinkMode == "p2p") {
            setEasyTierAvailability(true, "EasyTier P2P 状态: 已连接（P2P直连）");
        } else if (!m_easyTierLinkMode.isEmpty()) {
            setEasyTierAvailability(true, QString("EasyTier P2P 状态: 已连接（中转:%1）").arg(m_easyTierLinkMode));
        } else {
            setEasyTierAvailability(true, "EasyTier P2P 状态: 已连接");
        }
        break;
    case EasyTierState::Disconnected:
        setEasyTierAvailability(false, "EasyTier P2P 状态: 运行中，等待组网成功（持续重试）");
        break;
    case EasyTierState::Error:
        setEasyTierAvailability(false, "EasyTier P2P 状态: 运行中但当前检查异常（持续重试中）");
        break;
    }

    updateEasyTierButtonState();
}

void SettingsDialog::onEasyTierLinkModeChanged(const QString &mode, bool targetPeerFound)
{
    m_easyTierLinkMode = mode.toLower();
    if (targetPeerFound) {
        onEasyTierStateChanged(EasyTierState::Connected);
    }
}

void SettingsDialog::setEasyTierAvailability(bool available, const QString &message)
{
    m_easyTierAvailable = available;
    
    // 允许换行符显示（QLabel 默认不显示换行）
    m_lblEasyTierStatus->setWordWrap(true);
    m_lblEasyTierStatus->setText(message);
    
    // 根据内容类型设置颜色
    QString color = available ? "#16a34a" : "#dc2626";  // 绿色 vs 红色
    
    if (message.contains("打洞中") || message.contains("正在") || message.contains("⏳")) {
        color = "#ea580c";  // 正在进行：橙色
    } else if (message.contains("诊断") || message.contains("检查")) {
        color = "#0284c7";  // 诊断中：蓝色
    } else if (message.contains("✅")) {
        color = "#16a34a";  // 成功：绿色
    } else if (message.contains("❌")) {
        color = "#dc2626";  // 失败：红色
    }
    
    m_lblEasyTierStatus->setStyleSheet(QString("font-size: 12px; color: %1; line-height: 1.5;").arg(color));
}

void SettingsDialog::updateEasyTierButtonState()
{
    // P2P 按钮已迁移到登录页，这里保留空实现以兼容既有调用点。
}
