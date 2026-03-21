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
#include "../pve/PveAuthManager.h"

// 全屏锁屏登录视图
// 显示背景图 + 居中登录框 + 底部操作按钮（设置/关机/重启）
class LoginView : public QWidget
{
    Q_OBJECT

public:
    explicit LoginView(ConfigManager *configManager, PveAuthManager *authManager, QWidget *parent = nullptr);

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

protected:
    // 绘制全屏壁纸背景
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();

    ConfigManager *m_configManager;
    PveAuthManager *m_authManager;

    // 登录框控件
    QWidget    *m_loginCard;
    QLineEdit  *m_editUsername;
    QLineEdit  *m_editPassword;
    QCheckBox  *m_chkAutoLogin;
    QPushButton *m_btnLogin;
    QLabel     *m_lblError;
    
    // 底部状态栏组件
    QLabel     *m_lblDateTime;
    QTimer     *m_timer;

    // 壁纸
    QPixmap m_background;
};

#endif // LOGINVIEW_H
