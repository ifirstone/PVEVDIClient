#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "core/ConfigManager.h"
#include "core/EasyTierManager.h"
#include "core/DebugLogger.h"

class EasyTierManager;

// 全局设置对话框 —— 服务器配置 + 运行行为
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(ConfigManager *config, EasyTierManager *runtimeManager, QWidget *parent = nullptr);

private slots:
    void onTestConnection();  // 测试服务器连通性
    void onAccepted();
    void onEasyTierStateChanged(EasyTierState state);
    void onEasyTierLinkModeChanged(const QString &mode, bool targetPeerFound);

private:
    void setupUI();
    void applyDialogStyle();  // 对话框自身的浅色样式
    bool validateXfreerdpVersion(int selectedVersion);  // 验证选中的 xfreerdp 版本是否存在
    bool validateEasyTierInputs(QString *errorMessage = nullptr) const;
    void setEasyTierAvailability(bool available, const QString &message);
    void updateEasyTierButtonState();

    ConfigManager *m_config;

    // 服务器设置
    QLineEdit   *m_editHost;
    QLineEdit   *m_editLanHost;
    QLineEdit   *m_editPort;
    QPushButton *m_btnTest;
    QLabel      *m_lblTestResult;

    // 行为设置
    QCheckBox *m_chkKioskMode;
    QCheckBox *m_chkAutoConnect;
    QCheckBox *m_chkDebugMode;
    QLineEdit *m_editLoginWelcomeText;
    QLineEdit *m_editFooterBrandText;

    // RDP 外设重定向
    QCheckBox *m_chkRdpSound;
    QCheckBox *m_chkRdpMic;
    QCheckBox *m_chkRdpClipboard;
    QCheckBox *m_chkRdpUsb;
    QCheckBox *m_chkRdpSmartcard;
    QCheckBox *m_chkRdpPrinter;

    // RDP 高级设置
    QComboBox *m_comboRdpVersion;
    QComboBox *m_comboRdpCodec;
    QComboBox *m_comboRdpColorDepth;
    QComboBox *m_comboRdpNetwork;
    QComboBox *m_comboRdpScale;

    // EasyTier P2P 设置
    QCheckBox *m_chkUseEasyTier;
    QLineEdit *m_editEasyTierNetworkName;
    QLineEdit *m_editEasyTierNetworkSecret;
    QLineEdit *m_editEasyTierServerPeerIp;
    QLineEdit *m_editEasyTierBootstrapUrl;
    QLineEdit *m_editEasyTierExtraArgs;
    QLabel *m_lblEasyTierStatus;
    bool m_easyTierAvailable = false;
    QString m_easyTierLinkMode;
    EasyTierManager *m_easyTierRuntimeManager = nullptr;
};

#endif // SETTINGSDIALOG_H
