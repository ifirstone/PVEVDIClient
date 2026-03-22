#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIntValidator>
#include <QMessageBox>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QDebug>

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

SettingsDialog::SettingsDialog(ConfigManager *config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
{
    setWindowTitle("客户端设置");
    setMinimumWidth(780);   // 双栏布局，增加宽度以降低高度
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
            border: 1px solid #d4dae8;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 8px;
            background-color: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 6px;
            color: #3b71ca;
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

    // 中间主要内容区：左右分栏
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // 左侧栏：基础连接与通用设置
    QVBoxLayout *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(14);

    // 右侧栏：RDP 外设与性能微调
    QVBoxLayout *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(14);

    // ===== 服务器配置区 =====
    QGroupBox *serverGroup = new QGroupBox("PVE 服务器");
    QVBoxLayout *serverLayout = new QVBoxLayout(serverGroup);
    serverLayout->setSpacing(10);
    serverLayout->setContentsMargins(16, 16, 16, 14);

    // --- 第一行：服务器地址 ---
    QHBoxLayout *addrRow = new QHBoxLayout();
    QLabel *lblHost = new QLabel("服务器地址:");
    lblHost->setFixedWidth(80);
    m_editHost = new QLineEdit();
    m_editHost->setPlaceholderText("例: 192.168.1.100 或 pve.local");
    m_editHost->setText(m_config->pveHost());
    m_editHost->setStyleSheet(INPUT_STYLE);
    addrRow->addWidget(lblHost);
    addrRow->addWidget(m_editHost);
    serverLayout->addLayout(addrRow);

    // --- 第二行：端口 + 测试按钮 + 结果 ---
    QHBoxLayout *portRow = new QHBoxLayout();
    QLabel *lblPort = new QLabel("端口号:");
    lblPort->setFixedWidth(80);
    m_editPort = new QLineEdit();
    m_editPort->setFixedWidth(90);
    m_editPort->setPlaceholderText("8006");
    m_editPort->setText(QString::number(m_config->pvePort()));
    m_editPort->setValidator(new QIntValidator(1, 65535, this));
    m_editPort->setStyleSheet(INPUT_STYLE);

    m_lblTestResult = new QLabel("");
    m_lblTestResult->setStyleSheet("font-size: 12px; color: #27ae60;");

    m_btnTest = new QPushButton("测试连接");
    m_btnTest->setObjectName("testBtn");
    m_btnTest->setCursor(Qt::PointingHandCursor);
    m_btnTest->setFixedWidth(90);
    connect(m_btnTest, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);

    portRow->addWidget(lblPort);
    portRow->addWidget(m_editPort);
    portRow->addWidget(m_lblTestResult, 1);
    portRow->addWidget(m_btnTest);
    serverLayout->addLayout(portRow);

    leftColumn->addWidget(serverGroup);

    // ===== 外观设置区 =====
    QGroupBox *appearGroup = new QGroupBox("外观");
    QFormLayout *appearLayout = new QFormLayout(appearGroup);
    appearLayout->setLabelAlignment(Qt::AlignRight);
    appearLayout->setSpacing(10);
    appearLayout->setContentsMargins(16, 16, 16, 14);

    m_comboTheme = new QComboBox();
    m_comboTheme->addItem("深色主题", "dark");
    m_comboTheme->addItem("浅色主题", "light");
    m_comboTheme->setStyleSheet(COMBO_STYLE);
    int themeIdx = m_comboTheme->findData(m_config->theme());
    if (themeIdx >= 0) m_comboTheme->setCurrentIndex(themeIdx);
    appearLayout->addRow("主题:", m_comboTheme);

    m_comboLanguage = new QComboBox();
    m_comboLanguage->addItem("简体中文", "zh_CN");
    m_comboLanguage->addItem("English",  "en_US");
    m_comboLanguage->setStyleSheet(COMBO_STYLE);
    int langIdx = m_comboLanguage->findData(m_config->language());
    if (langIdx >= 0) m_comboLanguage->setCurrentIndex(langIdx);
    appearLayout->addRow("语言:", m_comboLanguage);

    leftColumn->addWidget(appearGroup);

    // ===== 行为设置区 =====
    QGroupBox *behaviorGroup = new QGroupBox("行为");
    QVBoxLayout *behaviorLayout = new QVBoxLayout(behaviorGroup);
    behaviorLayout->setSpacing(8);
    behaviorLayout->setContentsMargins(16, 16, 16, 14);

    m_chkKioskMode = new QCheckBox("Kiosk 模式（全屏锁定，禁止任务栏操作）");
    m_chkKioskMode->setChecked(m_config->kioskMode());
    m_chkKioskMode->setStyleSheet(CHECK_STYLE);
    behaviorLayout->addWidget(m_chkKioskMode);

    m_chkAutoConnect = new QCheckBox("启动时自动连接上次使用的虚拟机");
    m_chkAutoConnect->setChecked(m_config->autoConnect());
    m_chkAutoConnect->setStyleSheet(CHECK_STYLE);
    behaviorLayout->addWidget(m_chkAutoConnect);

    m_chkDebugMode = new QCheckBox("调试模式（显示连接过程中的详细错误信息）");
    m_chkDebugMode->setChecked(m_config->debugMode());
    m_chkDebugMode->setStyleSheet(CHECK_STYLE);
    behaviorLayout->addWidget(m_chkDebugMode);

    leftColumn->addWidget(behaviorGroup);

    contentLayout->addLayout(leftColumn, 1);

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

    rightColumn->addWidget(rdpGroup);

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

    m_chkRdpUsermode = new QCheckBox("usermode");
    m_chkRdpUsermode->setStyleSheet(CHECK_STYLE);
    m_chkRdpUsermode->setChecked(m_config->rdpUsermode());

    freerdpLayout->addWidget(new QLabel("网络速率"), 2, 0);
    freerdpLayout->addWidget(m_comboRdpNetwork, 2, 1);
    freerdpLayout->addWidget(m_chkRdpUsermode, 2, 2, 1, 2);

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
}

// ========== 槽函数 ==========

void SettingsDialog::onTestConnection()
{
    QString host = m_editHost->text().trimmed();
    int port = m_editPort->text().toInt();
    if (host.isEmpty()) {
        m_lblTestResult->setStyleSheet("font-size: 12px; color: #c0392b;");
        m_lblTestResult->setText("X 请先输入服务器地址");
        return;
    }
    if (port <= 0) port = 8006;

    m_btnTest->setEnabled(false);
    m_btnTest->setText("测试中...");
    m_lblTestResult->setStyleSheet("font-size: 12px; color: #7f8c8d;");
    m_lblTestResult->setText("正在连接...");

    // 使用 TCP Socket 方式验证连通性
    // 避免直接发 HTTPS 请求——全志盒子 OpenSSL 环境不稳定，SSL 握手会触发 SIGSEGV 崩溃
    QTcpSocket *sock = new QTcpSocket(this);

    // 连接成功 -> 说明目标端口可达
    connect(sock, &QTcpSocket::connected, this, [this, sock]() {
        m_btnTest->setEnabled(true);
        m_btnTest->setText("测试连接");
        m_lblTestResult->setStyleSheet("font-size: 12px; color: #27ae60; font-weight: bold;");
        m_lblTestResult->setText("成功！PVE 服务器端口可达");
        sock->disconnectFromHost();
        sock->deleteLater();
    });

    // 连接失败 -> 报告错误原因
    connect(sock, &QAbstractSocket::errorOccurred, this, [this, sock](QAbstractSocket::SocketError) {
        m_btnTest->setEnabled(true);
        m_btnTest->setText("测试连接");
        m_lblTestResult->setStyleSheet("font-size: 12px; color: #c0392b; font-weight: bold;");
        m_lblTestResult->setText("X 连接失败: " + sock->errorString());
        sock->deleteLater();
    });

    sock->connectToHost(host, static_cast<quint16>(port));
}


void SettingsDialog::onAccepted()
{
    QString host = m_editHost->text().trimmed();
    int port     = m_editPort->text().toInt();
    if (port <= 0) port = 8006;

    if (!host.isEmpty()) {
        m_config->setPveServer(host, port, m_config->pveUsername());
    }

    m_config->setTheme(m_comboTheme->currentData().toString());
    m_config->setLanguage(m_comboLanguage->currentData().toString());
    m_config->setKioskMode(m_chkKioskMode->isChecked());
    m_config->setAutoConnect(m_chkAutoConnect->isChecked());
    m_config->setDebugMode(m_chkDebugMode->isChecked());
    m_config->setRdpRedirection(
        m_chkRdpSound->isChecked(),
        m_chkRdpMic->isChecked(),
        m_chkRdpClipboard->isChecked(),
        m_chkRdpUsb->isChecked(),
        m_chkRdpSmartcard->isChecked(),
        m_chkRdpPrinter->isChecked()
    );

    int version = m_comboRdpVersion->currentText() == "Auto" ? 0 : m_comboRdpVersion->currentText().toInt();
    m_config->setRdpAdvanced(
        version,
        m_comboRdpCodec->currentText(),
        m_comboRdpColorDepth->currentText().toInt(),
        m_comboRdpNetwork->currentText(),
        m_comboRdpScale->currentText(),
        m_chkRdpUsermode->isChecked()
    );
    m_config->save();

    accept();
}
