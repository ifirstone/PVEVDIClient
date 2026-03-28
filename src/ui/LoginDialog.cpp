#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("登录 PVE 服务器");
    setMinimumWidth(400);
    setupUI();
}

void LoginDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel("连接到 Proxmox VE");
    titleLabel->setObjectName("dialogTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 服务器信息
    QGroupBox *serverGroup = new QGroupBox("服务器");
    QFormLayout *serverLayout = new QFormLayout(serverGroup);

    m_editHost = new QLineEdit();
    m_editHost->setPlaceholderText("pve.example.com 或 192.168.1.1");
    serverLayout->addRow("地址:", m_editHost);

    m_spinPort = new QSpinBox();
    m_spinPort->setRange(1, 65535);
    m_spinPort->setValue(8006);
    serverLayout->addRow("端口:", m_spinPort);

    mainLayout->addWidget(serverGroup);

    // 认证信息
    QGroupBox *authGroup = new QGroupBox("认证");
    QFormLayout *authLayout = new QFormLayout(authGroup);

    m_editUsername = new QLineEdit();
    m_editUsername->setPlaceholderText("root");
    authLayout->addRow("用户名:", m_editUsername);

    m_editPassword = new QLineEdit();
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setPlaceholderText("密码");
    authLayout->addRow("密码:", m_editPassword);

    m_comboRealm = new QComboBox();
    m_comboRealm->addItem("Linux PAM", "pam");
    m_comboRealm->addItem("PVE Authentication", "pve");
    authLayout->addRow("认证域:", m_comboRealm);

    mainLayout->addWidget(authGroup);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *btnCancel = new QPushButton("取消");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(btnCancel);

    QPushButton *btnLogin = new QPushButton("登录");
    btnLogin->setObjectName("primaryButton");
    btnLogin->setDefault(true);
    connect(btnLogin, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(btnLogin);

    mainLayout->addLayout(buttonLayout);

    // 回车键登录
    connect(m_editPassword, &QLineEdit::returnPressed, btnLogin, &QPushButton::click);
}

void LoginDialog::setServer(const QString &host, int port, const QString &username)
{
    m_editHost->setText(host);
    m_spinPort->setValue(port);
    m_editUsername->setText(username);
    // 光标聚焦到密码框
    m_editPassword->setFocus();
}

QString LoginDialog::host() const { return m_editHost->text().trimmed(); }
int LoginDialog::port() const { return m_spinPort->value(); }
QString LoginDialog::username() const { return m_editUsername->text().trimmed(); }
QString LoginDialog::password() const { return m_editPassword->text(); }
QString LoginDialog::realm() const { return m_comboRealm->currentData().toString(); }
