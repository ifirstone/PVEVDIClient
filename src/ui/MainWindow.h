#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

#include "core/ConfigManager.h"
#include "core/ConnectionManager.h"
#include "pve/PveApiClient.h"
#include "pve/PveAuthManager.h"

// 前置声明，避免循环包含
class LoginView;
class WorkspaceView;
class SettingsDialog;

// 主窗口 - 全屏 Kiosk 外壳，负责视图切换路由
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginSuccess(const QString &username, const QString &node);
    void onLogout();
    void onShowSettings();

private:
    void setupUI();
    void setupConnections();
    void loadStyleSheet();
    void updateStatusBar(const QString &message);

    // 核心业务组件
    ConfigManager     *m_configManager;
    PveApiClient      *m_apiClient;
    PveAuthManager    *m_authManager;
    ConnectionManager *m_connectionManager;

    // UI 视图路由
    QStackedWidget *m_stackedWidget;
    LoginView      *m_loginView;
    WorkspaceView  *m_workspaceView;
};

#endif // MAINWINDOW_H
