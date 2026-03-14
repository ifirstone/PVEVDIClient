#ifndef CONNECTIONEDITDIALOG_H
#define CONNECTIONEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>

// RDP 连接凭据对话框
// 当用户选择以 RDP 方式连接某个 VM 时弹出，用于输入 Windows 凭据和外设选项
class ConnectionEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionEditDialog(const QString &vmName, QWidget *parent = nullptr);

    // 获取输入值
    QString rdpHost() const;
    int rdpPort() const;
    QString username() const;
    QString password() const;
    QString domain() const;
    QString resolution() const;

    // 外设选项
    bool enableSound() const;
    bool enableMicrophone() const;
    bool enableClipboard() const;
    bool enableUSBDrive() const;
    bool enableSmartcard() const;
    bool enablePrinter() const;

    // 预填 RDP 地址（从 VM 的 agent 获取的 IP）
    void setRdpHost(const QString &host);

private:
    void setupUI();
    QString m_vmName;

    QLineEdit *m_editHost;
    QSpinBox *m_spinPort;
    QLineEdit *m_editUsername;
    QLineEdit *m_editPassword;
    QLineEdit *m_editDomain;
    QComboBox *m_comboResolution;

    QCheckBox *m_chkSound;
    QCheckBox *m_chkMicrophone;
    QCheckBox *m_chkClipboard;
    QCheckBox *m_chkUSB;
    QCheckBox *m_chkSmartcard;
    QCheckBox *m_chkPrinter;
};

#endif // CONNECTIONEDITDIALOG_H
