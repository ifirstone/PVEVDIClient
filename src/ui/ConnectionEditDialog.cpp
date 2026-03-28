#include "ConnectionEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

ConnectionEditDialog::ConnectionEditDialog(const QString &vmName, QWidget *parent)
    : QDialog(parent)
    , m_vmName(vmName)
{
    setWindowTitle(QString("RDP 连接 - %1").arg(vmName));
    setMinimumWidth(420);
    setupUI();
}

void ConnectionEditDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel(QString("通过 RDP 连接到 %1").arg(m_vmName));
    titleLabel->setObjectName("dialogTitle");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // RDP 目标
    QGroupBox *targetGroup = new QGroupBox("RDP 连接目标");
    QFormLayout *targetLayout = new QFormLayout(targetGroup);

    m_editHost = new QLineEdit();
    m_editHost->setPlaceholderText("虚拟机的 IP 地址");
    targetLayout->addRow("地址:", m_editHost);

    m_spinPort = new QSpinBox();
    m_spinPort->setRange(1, 65535);
    m_spinPort->setValue(3389);
    targetLayout->addRow("端口:", m_spinPort);

    mainLayout->addWidget(targetGroup);

    // Windows 认证
    QGroupBox *authGroup = new QGroupBox("Windows 登录凭据");
    QFormLayout *authLayout = new QFormLayout(authGroup);

    m_editUsername = new QLineEdit();
    m_editUsername->setPlaceholderText("Windows 用户名");
    authLayout->addRow("用户名:", m_editUsername);

    m_editPassword = new QLineEdit();
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setPlaceholderText("密码");
    authLayout->addRow("密码:", m_editPassword);

    m_editDomain = new QLineEdit();
    m_editDomain->setPlaceholderText("可选");
    authLayout->addRow("域:", m_editDomain);

    m_comboResolution = new QComboBox();
    m_comboResolution->addItem("全屏", "fullscreen");
    m_comboResolution->addItem("1920x1080", "1920x1080");
    m_comboResolution->addItem("1680x1050", "1680x1050");
    m_comboResolution->addItem("1440x900", "1440x900");
    m_comboResolution->addItem("1366x768", "1366x768");
    m_comboResolution->addItem("1280x720", "1280x720");
    authLayout->addRow("分辨率:", m_comboResolution);

    mainLayout->addWidget(authGroup);

    // 外设选项
    QGroupBox *deviceGroup = new QGroupBox("外设重定向");
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceGroup);
    deviceLayout->setSpacing(6);

    m_chkSound = new QCheckBox("声音输出");
    m_chkSound->setChecked(true);
    deviceLayout->addWidget(m_chkSound);

    m_chkMicrophone = new QCheckBox("麦克风输入");
    deviceLayout->addWidget(m_chkMicrophone);

    m_chkClipboard = new QCheckBox("剪贴板共享");
    m_chkClipboard->setChecked(true);
    deviceLayout->addWidget(m_chkClipboard);

    m_chkUSB = new QCheckBox("USB 驱动器");
    deviceLayout->addWidget(m_chkUSB);

    m_chkSmartcard = new QCheckBox("智能卡 / U-Key");
    deviceLayout->addWidget(m_chkSmartcard);

    m_chkPrinter = new QCheckBox("打印机");
    deviceLayout->addWidget(m_chkPrinter);

    mainLayout->addWidget(deviceGroup);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *btnCancel = new QPushButton("取消");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(btnCancel);

    QPushButton *btnConnect = new QPushButton("🖥  连接");
    btnConnect->setObjectName("primaryButton");
    btnConnect->setDefault(true);
    connect(btnConnect, &QPushButton::clicked, this, [this]() {
        if (m_editHost->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入虚拟机的 IP 地址");
            m_editHost->setFocus();
            return;
        }
        if (m_editUsername->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入 Windows 用户名");
            m_editUsername->setFocus();
            return;
        }
        accept();
    });
    buttonLayout->addWidget(btnConnect);

    mainLayout->addLayout(buttonLayout);

    // 回车键连接
    connect(m_editPassword, &QLineEdit::returnPressed, btnConnect, &QPushButton::click);
}

void ConnectionEditDialog::setRdpHost(const QString &host)
{
    m_editHost->setText(host);
    // 如果已有 IP，光标直接聚焦到用户名
    if (!host.isEmpty()) {
        m_editUsername->setFocus();
    }
}

QString ConnectionEditDialog::rdpHost() const { return m_editHost->text().trimmed(); }
int ConnectionEditDialog::rdpPort() const { return m_spinPort->value(); }
QString ConnectionEditDialog::username() const { return m_editUsername->text().trimmed(); }
QString ConnectionEditDialog::password() const { return m_editPassword->text(); }
QString ConnectionEditDialog::domain() const { return m_editDomain->text().trimmed(); }
QString ConnectionEditDialog::resolution() const { return m_comboResolution->currentData().toString(); }

bool ConnectionEditDialog::enableSound() const { return m_chkSound->isChecked(); }
bool ConnectionEditDialog::enableMicrophone() const { return m_chkMicrophone->isChecked(); }
bool ConnectionEditDialog::enableClipboard() const { return m_chkClipboard->isChecked(); }
bool ConnectionEditDialog::enableUSBDrive() const { return m_chkUSB->isChecked(); }
bool ConnectionEditDialog::enableSmartcard() const { return m_chkSmartcard->isChecked(); }
bool ConnectionEditDialog::enablePrinter() const { return m_chkPrinter->isChecked(); }
