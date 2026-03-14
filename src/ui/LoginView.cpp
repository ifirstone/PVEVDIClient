#include "LoginView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QFont>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>

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

    // ---- 自动登录复选框 ----
    m_chkAutoLogin = new QCheckBox("自动登录");
    m_chkAutoLogin->setStyleSheet("color: #3a5080; font-size: 13px;");
    cardLayout->addWidget(m_chkAutoLogin);

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

    // ---- 底部操作栏（设置/关机/重启），彩色圆角按钮 ----
    QHBoxLayout *bottomBarLayout = new QHBoxLayout();
    bottomBarLayout->setSpacing(8);
    bottomBarLayout->setContentsMargins(0, 14, 0, 0);

    // 设置 - 蓝色
    QPushButton *btnSettings = new QPushButton("\u2699  \u8bbe\u7f6e");
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setStyleSheet(
        "QPushButton {"
        "  color: #1a56db; background: rgba(59,113,202,0.10);"
        "  border: 1.5px solid rgba(59,113,202,0.55); border-radius: 8px;"
        "  font-size: 12px; padding: 5px 14px;"
        "}"
        "QPushButton:hover { background: rgba(59,113,202,0.22); border-color: #3b71ca; }"
    );

    // 关机 - 红色
    QPushButton *btnShutdown = new QPushButton("\u23fb  \u5173\u673a");
    btnShutdown->setCursor(Qt::PointingHandCursor);
    btnShutdown->setStyleSheet(
        "QPushButton {"
        "  color: #c0392b; background: rgba(192,57,43,0.10);"
        "  border: 1.5px solid rgba(192,57,43,0.55); border-radius: 8px;"
        "  font-size: 12px; padding: 5px 14px;"
        "}"
        "QPushButton:hover { background: rgba(192,57,43,0.22); border-color: #c0392b; }"
    );

    // 重启 - 绿色
    QPushButton *btnReboot = new QPushButton("\u21ba  \u91cd\u542f");
    btnReboot->setCursor(Qt::PointingHandCursor);
    btnReboot->setStyleSheet(
        "QPushButton {"
        "  color: #1a7a40; background: rgba(33,145,80,0.10);"
        "  border: 1.5px solid rgba(33,145,80,0.55); border-radius: 8px;"
        "  font-size: 12px; padding: 5px 14px;"
        "}"
        "QPushButton:hover { background: rgba(33,145,80,0.22); border-color: #1a7a40; }"
    );

    connect(btnSettings, &QPushButton::clicked, this, &LoginView::settingsRequested);
    connect(btnShutdown, &QPushButton::clicked, this, &LoginView::onShutdown);
    connect(btnReboot,   &QPushButton::clicked, this, &LoginView::onReboot);

    bottomBarLayout->addWidget(btnSettings);
    bottomBarLayout->addStretch();
    bottomBarLayout->addWidget(btnShutdown);
    bottomBarLayout->addSpacing(4);
    bottomBarLayout->addWidget(btnReboot);

    cardLayout->addLayout(bottomBarLayout);

    // 把卡片在父布局中居中
    QHBoxLayout *centerRow = new QHBoxLayout();
    centerRow->addStretch();
    centerRow->addWidget(m_loginCard);
    centerRow->addStretch();
    rootLayout->addLayout(centerRow);

    // ---- 底部弹性空间 ----
    rootLayout->addStretch(1);
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
    m_editPassword->clear();

    // 保存用户名到配置
    m_configManager->setPveServer(m_configManager->pveHost(),
                                  m_configManager->pvePort(),
                                  username);
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
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "关机确认", "确定要关机吗？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply == QMessageBox::Yes) {
#ifdef Q_OS_LINUX
        QProcess::startDetached("shutdown", QStringList() << "-h" << "now");
#else
        // Windows 下调用关机命令（测试用）
        QProcess::startDetached("shutdown", QStringList() << "/s" << "/t" << "0");
#endif
    }
}

void LoginView::onReboot()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "重启确认", "确定要重启吗？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply == QMessageBox::Yes) {
#ifdef Q_OS_LINUX
        QProcess::startDetached("reboot", QStringList());
#else
        QProcess::startDetached("shutdown", QStringList() << "/r" << "/t" << "0");
#endif
    }
}
