#ifndef LOGINVIEW_H
#define LOGINVIEW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

#include "../core/ConfigManager.h"
#include "../core/EasyTierManager.h"
#include "../pve/PveAuthManager.h"

// 全屏锁屏登录视图
// 显示背景图 + 居中登录框 + 底部操作按钮（设置/关机/重启）
class LoginView : public QWidget
{
    Q_OBJECT

public:
    explicit LoginView(ConfigManager *configManager, PveAuthManager *authManager, EasyTierManager *easyTierManager = nullptr, QWidget *parent = nullptr);

signals:
    void loginSuccess(const QString &username, const QString &node);
    void settingsRequested();

private slots:
    void onLoginClicked();
    void onAuthSuccess(const QString &username);
    void onAuthFailed(const QString &error);
    void onShutdown();
    void onReboot();
    void updateTime();
    void onStartP2pClicked();
    void onStopP2pClicked();
    void onEasyTierStateChanged(EasyTierState state);

protected:
    // 绘制全屏壁纸背景
    void paintEvent(QPaintEvent *event) override;
    // 注销后重新显示时回填密码
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    void startAutoLoginFlow();
    void performAutoLoginAttempt();
    void refreshBrandingTexts();

    ConfigManager *m_configManager;
    PveAuthManager *m_authManager;
    EasyTierManager *m_easyTierManager = nullptr;

    // 登录框控件
    QWidget    *m_loginCard;
    QLineEdit  *m_editUsername;
    QLineEdit  *m_editPassword;
    QCheckBox  *m_chkAutoLogin;
    QPushButton *m_btnLogin;
    QLabel     *m_lblError;
    QLabel     *m_lblTitle = nullptr;
    
    // EasyTier P2P 控制
    QPushButton *m_btnStartP2p = nullptr;
    QPushButton *m_btnStopP2p = nullptr;
    QPushButton *m_btnP2pLog = nullptr;
    QLabel *m_lblP2pStatus = nullptr;
    
    // 底部状态栏组件
    QLabel     *m_lblFooterBrand = nullptr;
    QLabel     *m_lblDateTime;
    QTimer     *m_timer;

    // 壁纸
    QPixmap m_background;

    // 启动时自动登录状态
    bool m_autoLoginTriedOnStartup = false;
    bool m_autoLoginInProgress = false;
    bool m_hasLoggedInThisSession = false;
    int  m_autoLoginAttemptCount = 0;
};

#endif // LOGINVIEW_H
