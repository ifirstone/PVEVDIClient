#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "core/ConfigManager.h"

// 全局设置对话框 —— 服务器配置 + 运行行为 + 外观选项
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(ConfigManager *config, QWidget *parent = nullptr);

private slots:
    void onTestConnection();  // 测试服务器连通性
    void onAccepted();

private:
    void setupUI();
    void applyDialogStyle();  // 对话框自身的浅色样式

    ConfigManager *m_config;

    // 服务器设置
    QLineEdit   *m_editHost;
    QLineEdit   *m_editPort;
    QPushButton *m_btnTest;
    QLabel      *m_lblTestResult;

    // 外观设置
    QComboBox *m_comboTheme;
    QComboBox *m_comboLanguage;

    // 行为设置
    QCheckBox *m_chkKioskMode;
    QCheckBox *m_chkAutoConnect;
    QCheckBox *m_chkDebugMode;

    // RDP 外设重定向
    QCheckBox *m_chkRdpSound;
    QCheckBox *m_chkRdpMic;
    QCheckBox *m_chkRdpClipboard;
    QCheckBox *m_chkRdpUsb;
    QCheckBox *m_chkRdpSmartcard;
    QCheckBox *m_chkRdpPrinter;
};

#endif // SETTINGSDIALOG_H
