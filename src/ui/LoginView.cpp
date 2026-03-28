#include "LoginView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QFont>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QHostInfo>
#include "../core/DebugLogger.h"
#include "../core/EasyTierManager.h"

LoginView::LoginView(ConfigManager *configManager, PveAuthManager *authManager, EasyTierManager *easyTierManager, QWidget *parent)
    : QWidget(parent)
    , m_configManager(configManager)
    , m_authManager(authManager)
    , m_easyTierManager(easyTierManager)
{
    // 加载壁纸背景
    m_background = QPixmap(":/icons/Wallpaper.jpg");
    if (m_background.isNull()) {
        // 若没有壁纸图片，用纯色渐变兜底
        qDebug() << "未找到壁纸，将使用默认背景色";
    }

    setupUI();

    // 连接认证信号
    connect(m_authManager, &PveAuthManager::authenticationSuccess,
            this, &LoginView::onAuthSuccess);
    connect(m_authManager, &PveAuthManager::authenticationFailed,
            this, &LoginView::onAuthFailed);
    
    // 连接 EasyTier P2P 信号
    if (m_easyTierManager) {
        connect(m_easyTierManager, &EasyTierManager::stateChanged,
                this, &LoginView::onEasyTierStateChanged);
    }

    connect(m_configManager, &ConfigManager::configChanged,
            this, &LoginView::refreshBrandingTexts);
}

void LoginView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    if (!m_background.isNull()) {
        // 按比例拉伸平铺满全屏
        painter.drawPixmap(rect(), m_background.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        // 默认蓝灰渐变背景
        QLinearGradient gradient(0, 0, width(), height());
        gradient.setColorAt(0.0, QColor(15, 25, 50));
        gradient.setColorAt(1.0, QColor(25, 40, 80));
        painter.fillRect(rect(), gradient);
    }

    // 加一层半透明暗色蒙版让登录卡片更突出
    painter.fillRect(rect(), QColor(0, 0, 0, 90));
}

void LoginView::setupUI()
{
    // 主布局：全屏，登录卡片居中
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- 顶部弹性空间 ----
    rootLayout->addStretch(1);

    // ---- 居中的登录卡片 ----
    m_loginCard = new QWidget(this);
    m_loginCard->setObjectName("loginCard");
    m_loginCard->setFixedWidth(520);  // 扩大宽度以容纳 P2P 控制

    // 登录卡片样式（圆角白色半透明毛玻璃效果）
    m_loginCard->setStyleSheet(
        "#loginCard {"
        "  background-color: rgba(255,255,255,0.95);"
        "  border-radius: 18px;"
        "  border: 1px solid rgba(200,212,232,0.6);"
        "}"
        "#loginCard QWidget { background: transparent; }"
        "#loginCard QLabel {"
        "  color: #1a2a4a;"
        "  background: transparent;"
        "}"
        "#loginCard QCheckBox {"
        "  color: #3a5080;"
        "  font-size: 13px;"
        "  background: transparent;"
        "}"
        "#loginCard QCheckBox::indicator {"
        "  width: 15px; height: 15px;"
        "  border: 1.5px solid #c8d4e8;"
        "  border-radius: 3px;"
        "  background: #f5f8ff;"
        "}"
        "#loginCard QCheckBox::indicator:checked {"
        "  background-color: #3b71ca;"
        "  border-color: #3b71ca;"
        "}"
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(m_loginCard);
    cardLayout->setContentsMargins(40, 40, 40, 36);
    cardLayout->setSpacing(16);

    // ---- 标题图标区（跨平台 SVG 矢量图，不依赖 emoji 字体）----
    QLabel *lblIcon = new QLabel();
    QPixmap iconPix(":/icons/cloud_desktop.svg");
    if (!iconPix.isNull()) {
        lblIcon->setPixmap(iconPix.scaled(64, 54, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        lblIcon->setText("☁"); // 降级方案
        lblIcon->setStyleSheet("font-size: 48px; color: #3b71ca;");
    }
    lblIcon->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(lblIcon);

    m_lblTitle = new QLabel();
    m_lblTitle->setAlignment(Qt::AlignCenter);
    m_lblTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #1a2a4a; margin-bottom: 4px;");
    cardLayout->addWidget(m_lblTitle);

    QLabel *lblSubtitle = new QLabel("请输入 PVE 账户登录");
    lblSubtitle->setAlignment(Qt::AlignCenter);
    lblSubtitle->setStyleSheet("font-size: 13px; color: #5a6a8a; margin-bottom: 12px;");
    cardLayout->addWidget(lblSubtitle);

    // ---- 用户名输入框 ----
    m_editUsername = new QLineEdit();
    m_editUsername->setObjectName("loginInput");
    m_editUsername->setPlaceholderText("用户名  (例: root@pam 或 user@pve)");
    m_editUsername->setMinimumHeight(44);
    m_editUsername->setStyleSheet(
        "#loginInput {"
        "  border: 1.5px solid #c8d4e8;"
        "  border-radius: 10px;"
        "  padding: 6px 14px;"
        "  font-size: 14px;"
        "  color: #1a2a4a;"
        "  background-color: #f5f8ff;"
        "}"
        "#loginInput:focus {"
        "  border-color: #3b71ca;"
        "  background-color: #ffffff;"
        "}"
    );
    // 预填保存的用户名
    if (!m_configManager->pveUsername().isEmpty()) {
        m_editUsername->setText(m_configManager->pveUsername());
    }
    cardLayout->addWidget(m_editUsername);

    // ---- 密码输入框 ----
    m_editPassword = new QLineEdit();
    m_editPassword->setObjectName("loginInput");
    m_editPassword->setPlaceholderText("密码");
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setMinimumHeight(44);
    m_editPassword->setStyleSheet(m_editUsername->styleSheet());
    // 按回车即登录
    connect(m_editPassword, &QLineEdit::returnPressed, this, &LoginView::onLoginClicked);
    cardLayout->addWidget(m_editPassword);

    // ---- 记住密码 + P2P 控制 ----
    QHBoxLayout *loginOptionsLayout = new QHBoxLayout();
    m_chkAutoLogin = new QCheckBox("记住密码");
    m_chkAutoLogin->setStyleSheet("color: #3a5080; font-size: 13px;");
    
    // 恢复上次的"记住密码"状态
    m_chkAutoLogin->setChecked(m_configManager->rememberPassword());
    
    // 如果上次勾选了记住密码，自动回填已保存的密码
    if (m_configManager->rememberPassword() && !m_configManager->pvePassword().isEmpty()) {
        m_editPassword->setText(m_configManager->pvePassword());
    }
    
    loginOptionsLayout->addWidget(m_chkAutoLogin);
    loginOptionsLayout->addStretch();
    
    // P2P 控制按钮区
    m_btnStartP2p = new QPushButton("启动P2P");
    m_btnStartP2p->setCursor(Qt::PointingHandCursor);
    m_btnStartP2p->setFixedHeight(32);
    m_btnStartP2p->setStyleSheet(
        "QPushButton { background-color: #28a745; color: white; border: none; border-radius: 6px; "
        "padding: 6px 12px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #218838; }"
        "QPushButton:pressed { background-color: #1e7e34; }"
        "QPushButton:disabled { background-color: #d3d3d3; color: #666666; }"
    );
    connect(m_btnStartP2p, &QPushButton::clicked, this, &LoginView::onStartP2pClicked);
    
    m_btnStopP2p = new QPushButton("停止P2P");
    m_btnStopP2p->setCursor(Qt::PointingHandCursor);
    m_btnStopP2p->setFixedHeight(32);
    m_btnStopP2p->setEnabled(false);
    m_btnStopP2p->setStyleSheet(
        "QPushButton { background-color: #d3d3d3; color: #666666; border: none; border-radius: 6px; "
        "padding: 6px 12px; font-size: 12px; font-weight: bold; }"
        "QPushButton:enabled { background-color: #dc3545; color: white; }"
        "QPushButton:enabled:hover { background-color: #c82333; }"
        "QPushButton:enabled:pressed { background-color: #bd2130; }"
    );
    connect(m_btnStopP2p, &QPushButton::clicked, this, [this](){
        if (m_easyTierManager) {
            m_easyTierManager->stop();
        }
    });
    
    m_btnP2pLog = new QPushButton("日志");
    m_btnP2pLog->setCursor(Qt::PointingHandCursor);
    m_btnP2pLog->setFixedHeight(32);
    m_btnP2pLog->setStyleSheet(
        "QPushButton { background-color: #6c757d; color: white; border: none; border-radius: 6px; "
        "padding: 6px 10px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #5a6268; }"
        "QPushButton:pressed { background-color: #4e555b; }"
    );
    connect(m_btnP2pLog, &QPushButton::clicked, this, [this](){
        DebugLogger::instance().showLogger();
    });
    
    loginOptionsLayout->addWidget(m_btnStartP2p);
    loginOptionsLayout->addWidget(m_btnStopP2p);
    loginOptionsLayout->addWidget(m_btnP2pLog);
    
    cardLayout->addLayout(loginOptionsLayout);
    
    // P2P 连接状态显示（放在启动按钮下一行）
    m_lblP2pStatus = new QLabel("EasyTier P2P 状态：未启用");
    m_lblP2pStatus->setStyleSheet("color: #9ca3af; font-size: 12px; margin-top: 2px; margin-bottom: 6px;");
    m_lblP2pStatus->setAlignment(Qt::AlignRight);
    cardLayout->addWidget(m_lblP2pStatus);

    // ---- 错误提示 ----
    m_lblError = new QLabel();
    m_lblError->setAlignment(Qt::AlignCenter);
    m_lblError->setStyleSheet("color: #e53935; font-size: 13px;");
    m_lblError->setWordWrap(true);
    m_lblError->setVisible(false);
    cardLayout->addWidget(m_lblError);

    // ---- 登录按钮 ----
    m_btnLogin = new QPushButton("登  录");
    m_btnLogin->setObjectName("btnLogin");
    m_btnLogin->setMinimumHeight(46);
    m_btnLogin->setCursor(Qt::PointingHandCursor);
    m_btnLogin->setStyleSheet(
        "#btnLogin {"
        "  background-color: #1a56db;"
        "  color: white;"
        "  font-size: 15px;"
        "  font-weight: bold;"
        "  border-radius: 10px;"
        "  border: none;"
        "}"
        "#btnLogin:hover {"
        "  background-color: #1e65e8;"
        "}"
        "#btnLogin:pressed {"
        "  background-color: #1645b8;"
        "}"
        "#btnLogin:disabled {"
        "  background-color: #aab4cc;"
        "}"
    );
    connect(m_btnLogin, &QPushButton::clicked, this, &LoginView::onLoginClicked);
    cardLayout->addWidget(m_btnLogin);

    // ---- 底部操作栏（设置/关机/重启），采用实心色彩填充风格 ----
    QHBoxLayout *bottomBarLayout = new QHBoxLayout();
    bottomBarLayout->setSpacing(12);
    bottomBarLayout->setContentsMargins(0, 16, 0, 0);

    // 设置 - 蓝色
    QPushButton *btnSettings = new QPushButton("设置");
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setStyleSheet(
        "QPushButton { color: white; background: #3b82f6; border: none; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 7px 16px; }"
        "QPushButton:hover { background: #60a5fa; }"
        "QPushButton:pressed { background: #2563eb; }"
    );

    // 关机 - 红色
    QPushButton *btnShutdown = new QPushButton("关机");
    btnShutdown->setCursor(Qt::PointingHandCursor);
    btnShutdown->setStyleSheet(
        "QPushButton { color: white; background: #ef4444; border: none; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 7px 16px; }"
        "QPushButton:hover { background: #f87171; }"
        "QPushButton:pressed { background: #dc2626; }"
    );

    // 重启 - 橘黄色
    QPushButton *btnReboot = new QPushButton("重启");
    btnReboot->setCursor(Qt::PointingHandCursor);
    btnReboot->setStyleSheet(
        "QPushButton { color: white; background: #f59e0b; border: none; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 7px 16px; }"
        "QPushButton:hover { background: #fcd34d; }"
        "QPushButton:pressed { background: #d97706; }"
    );

    connect(btnSettings, &QPushButton::clicked, this, [this]() {
        // 打开设置前先同步登录页当前输入，确保设置页校验使用最新信息
        m_configManager->setPveServer(m_configManager->pveHost(),
                                      m_configManager->pvePort(),
                                      m_editUsername->text().trimmed());
        m_configManager->setRememberPassword(m_chkAutoLogin->isChecked());
        if (m_chkAutoLogin->isChecked()) {
            m_configManager->setPvePassword(m_editPassword->text());
        }
        m_configManager->save();
        emit settingsRequested();
    });
    connect(btnShutdown, &QPushButton::clicked, this, &LoginView::onShutdown);
    connect(btnReboot,   &QPushButton::clicked, this, &LoginView::onReboot);

    bottomBarLayout->addWidget(btnSettings);
    bottomBarLayout->addStretch();
    bottomBarLayout->addWidget(btnShutdown);
    bottomBarLayout->addWidget(btnReboot);

    cardLayout->addLayout(bottomBarLayout);

    // 把卡片在父布局中居中
    QHBoxLayout *centerRow = new QHBoxLayout();
    centerRow->addStretch();
    centerRow->addWidget(m_loginCard);
    centerRow->addStretch();
    rootLayout->addLayout(centerRow);

    // ---- 底部软垫 ----
    rootLayout->addStretch(1);

    // ---- 全屏底部系统状态指示栏 ----
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

    // 抓取本机内网 IP
    QString localIp = "127.0.0.1";
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
            QString ipStr = address.toString();
            // 剔除虚拟网卡 IP
            if (!ipStr.startsWith("169.254") && !ipStr.startsWith("172.17") && !ipStr.startsWith("172.18")) {
                localIp = ipStr;
                break;
            }
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

    refreshBrandingTexts();

    rootLayout->addWidget(statusBarWidget);

    // 启动系统级时间刷新器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LoginView::updateTime);
    m_timer->start(1000);
}

// ========== 槽函数 ==========

void LoginView::onLoginClicked()
{
    m_lblError->setVisible(false);

    QString username = m_editUsername->text().trimmed();
    QString password = m_editPassword->text();

    if (username.isEmpty()) {
        m_lblError->setText("请输入用户名");
        m_lblError->setVisible(true);
        return;
    }
    if (password.isEmpty()) {
        m_lblError->setText("请输入密码");
        m_lblError->setVisible(true);
        return;
    }

    QString host = m_configManager->effectivePveHost();
    if (host.isEmpty()) {
        m_lblError->setText("请先在设置中配置 PVE 服务器地址");
        m_lblError->setVisible(true);
        emit settingsRequested();
        return;
    }

    // 解析用户名格式：user@realm，realm 不写则默认 pam
    QString realm = "pam";
    QString user = username;
    int atIdx = username.lastIndexOf('@');
    if (atIdx >= 0) {
        user  = username.left(atIdx);
        realm = username.mid(atIdx + 1);
    }

    m_btnLogin->setEnabled(false);
    m_btnLogin->setText("登录中...");

    qDebug() << "开始登录，服务器:" << host << "用户:" << username;
    m_authManager->login(host, m_configManager->pvePort(), username, password, realm);
}

void LoginView::onAuthSuccess(const QString &username)
{
    if (m_autoLoginInProgress && m_configManager->debugMode()) {
        qInfo().noquote() << QString("[AutoLogin] success user=%1").arg(username);
    }

    m_hasLoggedInThisSession = true;
    m_autoLoginInProgress = false;

    m_btnLogin->setEnabled(true);
    m_btnLogin->setText("登  录");

    // 保存用户名到配置
    m_configManager->setPveServer(m_configManager->pveHost(),
                                  m_configManager->pvePort(),
                                  username);
    
    // 处理"记住密码"逻辑
    m_configManager->setRememberPassword(m_chkAutoLogin->isChecked());
    if (m_chkAutoLogin->isChecked()) {
        m_configManager->setPvePassword(m_editPassword->text());
    }
    
    m_configManager->save();

    // 发射信号通知 MainWindow 切换到工作台
    emit loginSuccess(username, "");
}

void LoginView::onAuthFailed(const QString &error)
{
    if (m_autoLoginInProgress) {
        if (m_configManager->debugMode()) {
            qWarning().noquote() << QString("[AutoLogin] attempt=%1 failed, error=%2")
                                    .arg(m_autoLoginAttemptCount)
                                    .arg(error);
        }

        if (m_autoLoginAttemptCount < 3) {
            if (m_configManager->debugMode()) {
                qInfo().noquote() << QString("[AutoLogin] scheduling retry nextAttempt=%1")
                                      .arg(m_autoLoginAttemptCount + 1);
            }
            QTimer::singleShot(1800, this, [this]() {
                performAutoLoginAttempt();
            });
            return;
        }

        m_autoLoginInProgress = false;
        m_btnLogin->setEnabled(true);
        m_btnLogin->setText("登  录");
        m_lblError->setVisible(false);
        if (m_configManager->debugMode()) {
            qWarning().noquote() << "[AutoLogin] aborted after 3 failed attempts";
        }
        return;
    }

    m_btnLogin->setEnabled(true);
    m_btnLogin->setText("登  录");
    m_lblError->setText("登录失败：" + error);
    m_lblError->setVisible(true);
}




void LoginView::onShutdown()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("关机确认");
    msgBox.setText("确定要关机吗？");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #ffffff; border-radius: 8px; }"
        "QLabel { color: #1a2a4a; font-size: 14px; font-weight: bold; min-height: 40px; }"
    );
    
    QPushButton *yesBtn = static_cast<QPushButton*>(msgBox.button(QMessageBox::Yes));
    QPushButton *noBtn  = static_cast<QPushButton*>(msgBox.button(QMessageBox::No));
    if (yesBtn) {
        yesBtn->setIcon(QIcon()); // 强制移除 Linux 可能附加的系统级勾选图标
        yesBtn->setText("确定");
        yesBtn->setCursor(Qt::PointingHandCursor);
        yesBtn->setStyleSheet(
            "QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; min-width: 60px; }"
            "QPushButton:hover { background-color: #f87171; }"
            "QPushButton:pressed { background-color: #dc2626; }"
        );
    }
    if (noBtn) {
        noBtn->setIcon(QIcon()); // 强制移除 Linux 可能附加的系统级取消图标
        noBtn->setText("取消");
        noBtn->setCursor(Qt::PointingHandCursor);
        noBtn->setStyleSheet(
            "QPushButton { background-color: #10b981; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; min-width: 60px; }"
            "QPushButton:hover { background-color: #34d399; }"
            "QPushButton:pressed { background-color: #059669; }"
        );
    }

    if (msgBox.exec() == QMessageBox::Yes) {
#ifdef Q_OS_LINUX
        // 使用现代 systemctl 命令，配合 polkit，普通桌面用户可直接执行
        QProcess::startDetached("systemctl", QStringList() << "poweroff");
#else
        // Windows 下调用关机命令
        QProcess::startDetached("shutdown", QStringList() << "/s" << "/t" << "0");
#endif
    }
}

void LoginView::onReboot()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("重启确认");
    msgBox.setText("确定要重启吗？");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #ffffff; border-radius: 8px; }"
        "QLabel { color: #1a2a4a; font-size: 14px; font-weight: bold; min-height: 40px; }"
    );
    
    QPushButton *yesBtn = static_cast<QPushButton*>(msgBox.button(QMessageBox::Yes));
    QPushButton *noBtn  = static_cast<QPushButton*>(msgBox.button(QMessageBox::No));
    if (yesBtn) {
        yesBtn->setIcon(QIcon()); // 强制移除 Linux 可能附加的系统级勾选图标
        yesBtn->setText("确定");
        yesBtn->setCursor(Qt::PointingHandCursor);
        yesBtn->setStyleSheet(
            "QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; min-width: 60px; }"
            "QPushButton:hover { background-color: #f87171; }"
            "QPushButton:pressed { background-color: #dc2626; }"
        );
    }
    if (noBtn) {
        noBtn->setIcon(QIcon()); // 强制移除 Linux 可能附加的系统级取消图标
        noBtn->setText("取消");
        noBtn->setCursor(Qt::PointingHandCursor);
        noBtn->setStyleSheet(
            "QPushButton { background-color: #10b981; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; min-width: 60px; }"
            "QPushButton:hover { background-color: #34d399; }"
            "QPushButton:pressed { background-color: #059669; }"
        );
    }

    if (msgBox.exec() == QMessageBox::Yes) {
#ifdef Q_OS_LINUX
        // 使用现代 systemctl 命令，配合 polkit，普通桌面用户可直接执行
        QProcess::startDetached("systemctl", QStringList() << "reboot");
#else
        // Windows 下调用重启命令
        QProcess::startDetached("shutdown", QStringList() << "/r" << "/t" << "0");
#endif
    }
}

void LoginView::updateTime()
{
    if (m_lblDateTime) {
        m_lblDateTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    }
}

void LoginView::onStartP2pClicked()
{
    if (!m_easyTierManager) {
        QMessageBox::warning(this, "错误", "EasyTier 管理器未初始化");
        return;
    }

    // 检查必需的参数
    QString networkName = m_configManager->easyTierNetworkName().trimmed();
    QString networkSecret = m_configManager->easyTierNetworkSecret().trimmed();
    QString serverPeerIp = m_configManager->easyTierServerPeerIp().trimmed();
    QString bootstrapUrl = m_configManager->easyTierBootstrapUrl().trimmed();

    QStringList emptyFields;
    if (networkName.isEmpty()) {
        emptyFields << "网络名";
    }
    if (networkSecret.isEmpty()) {
        emptyFields << "密钥";
    }
    if (serverPeerIp.isEmpty()) {
        emptyFields << "服务器P2P地址";
    }
    if (bootstrapUrl.isEmpty()) {
        emptyFields << "公共节点";
    }

    if (!emptyFields.isEmpty()) {
        QString missingParams = emptyFields.join("、");
        QMessageBox::warning(this, "参数不完整", 
            QString("请先在设置中配置以下参数：\n%1").arg(missingParams));
        emit settingsRequested();
        return;
    }

    // 参数校验通过，启动 P2P
    m_easyTierManager->start(networkName, networkSecret, bootstrapUrl);
}

void LoginView::onStopP2pClicked()
{
    if (m_easyTierManager) {
        m_easyTierManager->stop();
    }
}

void LoginView::onEasyTierStateChanged(EasyTierState state)
{
    // 根据 P2P 运行状态更新按钮状态
    bool isRunning = (state != EasyTierState::Stopped);
    
    if (m_btnStartP2p) {
        m_btnStartP2p->setEnabled(!isRunning);
    }
    if (m_btnStopP2p) {
        m_btnStopP2p->setEnabled(isRunning);
    }
    
    // 更新状态标签
    if (m_lblP2pStatus) {
        QString statusText;
        QString statusColor;
        switch (state) {
        case EasyTierState::Stopped:
            statusText = "EasyTier P2P 状态：未启用";
            statusColor = "#9ca3af";
            break;
        case EasyTierState::Starting:
            statusText = "EasyTier P2P 状态(时间需要1~5分钟)：启动中...";
            statusColor = "#f59e0b";
            break;
        case EasyTierState::Connected:
            statusText = "EasyTier P2P 状态：已连接";
            statusColor = "#22c55e";
            break;
        case EasyTierState::Disconnected:
            statusText = "EasyTier P2P 状态：未连接";
            statusColor = "#ef4444";
            break;
        case EasyTierState::Error:
            statusText = "EasyTier P2P 状态(时间需要1~5分钟)：错误";
            statusColor = "#ef4444";
            break;
        }
        m_lblP2pStatus->setText(statusText);
        m_lblP2pStatus->setStyleSheet(QString("color: %1; font-size: 12px; margin-top: 2px; margin-bottom: 6px;").arg(statusColor));
    }
}

void LoginView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_lblError->setVisible(false);
    
    // 每次视图重新显示时（包括注销后切换回来），回填已记住的密码
    if (m_configManager->rememberPassword() && !m_configManager->pvePassword().isEmpty()) {
        if (m_editPassword->text().isEmpty()) {
            m_editPassword->setText(m_configManager->pvePassword());
        }
    }
    
    // 同步 checkbox 状态
    m_chkAutoLogin->setChecked(m_configManager->rememberPassword());

    // 应用启动后的首次显示才尝试自动登录；注销返回登录页不触发
    if (!m_autoLoginTriedOnStartup && !m_hasLoggedInThisSession) {
        m_autoLoginTriedOnStartup = true;
        QTimer::singleShot(600, this, [this]() {
            startAutoLoginFlow();
        });
    }
}

void LoginView::refreshBrandingTexts()
{
    if (m_lblTitle) {
        m_lblTitle->setText(m_configManager->loginWelcomeText());
    }
    if (m_lblFooterBrand) {
        m_lblFooterBrand->setText(m_configManager->footerBrandText());
    }
}

void LoginView::startAutoLoginFlow()
{
    if (!m_configManager->autoLogin()) {
        if (m_configManager->debugMode()) {
            qInfo().noquote() << "[AutoLogin] skipped: auto_login disabled";
        }
        return;
    }

    // 启动自动登录需满足：已保存用户名、密码且勾选记住密码
    if (!m_configManager->rememberPassword()
        || m_configManager->pveUsername().trimmed().isEmpty()
        || m_configManager->pvePassword().isEmpty()) {
        m_lblError->setVisible(false);
        if (m_configManager->debugMode()) {
            qWarning().noquote() << "[AutoLogin] skipped: incomplete prerequisites (remember_password/username/password)";
        }
        return;
    }

    // 回填并启动后台自动登录
    m_editUsername->setText(m_configManager->pveUsername());
    m_editPassword->setText(m_configManager->pvePassword());
    m_chkAutoLogin->setChecked(true);

    m_autoLoginInProgress = true;
    m_autoLoginAttemptCount = 0;
    if (m_configManager->debugMode()) {
        qInfo().noquote() << "[AutoLogin] starting background auto-login flow";
    }
    performAutoLoginAttempt();
}

void LoginView::performAutoLoginAttempt()
{
    if (!m_autoLoginInProgress) {
        if (m_configManager->debugMode()) {
            qInfo().noquote() << "[AutoLogin] perform skipped: flow inactive";
        }
        return;
    }
    if (m_autoLoginAttemptCount >= 3) {
        if (m_configManager->debugMode()) {
            qWarning().noquote() << "[AutoLogin] perform skipped: attempts exhausted";
        }
        return;
    }

    m_autoLoginAttemptCount++;
    m_lblError->setVisible(false);

    const QString username = m_editUsername->text().trimmed();
    const QString password = m_editPassword->text();
    const QString host = m_configManager->effectivePveHost();

    if (username.isEmpty() || password.isEmpty() || host.isEmpty()) {
        m_autoLoginInProgress = false;
        m_btnLogin->setEnabled(true);
        m_btnLogin->setText("登  录");
        m_lblError->setVisible(false);
        if (m_configManager->debugMode()) {
            qWarning().noquote() << "[AutoLogin] aborted: runtime empty credentials/host";
        }
        return;
    }

    QString realm = "pam";
    int atIdx = username.lastIndexOf('@');
    if (atIdx >= 0) {
        realm = username.mid(atIdx + 1);
    }

    m_btnLogin->setEnabled(false);
    m_btnLogin->setText(QString("自动登录中(%1/3)...").arg(m_autoLoginAttemptCount));
    if (m_configManager->debugMode()) {
        qInfo().noquote() << QString("[AutoLogin] attempt=%1 host=%2 user=%3 realm=%4")
                              .arg(m_autoLoginAttemptCount)
                              .arg(host)
                              .arg(username)
                              .arg(realm);
    }
    m_authManager->login(host, m_configManager->pvePort(), username, password, realm);
}
