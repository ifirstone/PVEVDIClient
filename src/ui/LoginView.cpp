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

LoginView::LoginView(ConfigManager *configManager, PveAuthManager *authManager, QWidget *parent)
    : QWidget(parent)
    , m_configManager(configManager)
    , m_authManager(authManager)
{
    // 加载壁纸背景
    m_background = QPixmap(":/images/wallpaper.jpg");
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
    m_loginCard->setFixedWidth(420);

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

    // ---- 标题区 ----
    QLabel *lblIcon = new QLabel("🖥");
    lblIcon->setAlignment(Qt::AlignCenter);
    lblIcon->setStyleSheet("font-size: 52px; color: #1e3a6e;");
    cardLayout->addWidget(lblIcon);

    QLabel *lblTitle = new QLabel("欢迎使用云桌面");
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #1a2a4a; margin-bottom: 4px;");
    cardLayout->addWidget(lblTitle);

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

    // ---- 记住密码 ----
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
    
    cardLayout->addLayout(loginOptionsLayout);

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
    QPushButton *btnSettings = new QPushButton("\u2699 设置");
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setStyleSheet(
        "QPushButton { color: white; background: #3b82f6; border: none; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 7px 16px; }"
        "QPushButton:hover { background: #60a5fa; }"
        "QPushButton:pressed { background: #2563eb; }"
    );

    // 关机 - 红色
    QPushButton *btnShutdown = new QPushButton("\u23fb 关机");
    btnShutdown->setCursor(Qt::PointingHandCursor);
    btnShutdown->setStyleSheet(
        "QPushButton { color: white; background: #ef4444; border: none; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 7px 16px; }"
        "QPushButton:hover { background: #f87171; }"
        "QPushButton:pressed { background: #dc2626; }"
    );

    // 重启 - 橘黄色
    QPushButton *btnReboot = new QPushButton("\u21ba 重启");
    btnReboot->setCursor(Qt::PointingHandCursor);
    btnReboot->setStyleSheet(
        "QPushButton { color: white; background: #f59e0b; border: none; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 7px 16px; }"
        "QPushButton:hover { background: #fcd34d; }"
        "QPushButton:pressed { background: #d97706; }"
    );

    connect(btnSettings, &QPushButton::clicked, this, &LoginView::settingsRequested);
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
    statusBarLayout->setContentsMargins(24, 6, 24, 6);

    QWidget *statusBarWidget = new QWidget(this);
    statusBarWidget->setStyleSheet("background-color: rgba(10, 15, 25, 140); color: rgba(255, 255, 255, 140); font-size: 11px;");
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

    QLabel *lblClientInfo = new QLabel(QString("主机名: %1\t本地地址: %2").arg(QHostInfo::localHostName(), localIp));
    QLabel *lblVersion = new QLabel("开源版 v1.0.0 PVE Thin Client");
    m_lblDateTime = new QLabel(QDateTime::currentDateTime().toString("MM-dd HH:mm:ss"));

    statusBarLayout->addWidget(lblClientInfo);
    statusBarLayout->addStretch();
    statusBarLayout->addWidget(lblVersion);
    statusBarLayout->addStretch();
    statusBarLayout->addWidget(m_lblDateTime);

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

    QString host = m_configManager->pveHost();
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
    
    m_editPassword->clear();
    m_configManager->save();

    // 发射信号通知 MainWindow 切换到工作台
    emit loginSuccess(username, "");
}

void LoginView::onAuthFailed(const QString &error)
{
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
        yesBtn->setText("确定");
        yesBtn->setCursor(Qt::PointingHandCursor);
        yesBtn->setStyleSheet(
            "QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; min-width: 60px; }"
            "QPushButton:hover { background-color: #f87171; }"
            "QPushButton:pressed { background-color: #dc2626; }"
        );
    }
    if (noBtn) {
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
        yesBtn->setText("确定");
        yesBtn->setCursor(Qt::PointingHandCursor);
        yesBtn->setStyleSheet(
            "QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; min-width: 60px; }"
            "QPushButton:hover { background-color: #f87171; }"
            "QPushButton:pressed { background-color: #dc2626; }"
        );
    }
    if (noBtn) {
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
        m_lblDateTime->setText(QDateTime::currentDateTime().toString("MM-dd HH:mm:ss"));
    }
}
