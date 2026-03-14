#include "MainWindow.h"
#include "LoginView.h"
#include "WorkspaceView.h"
#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 初始化核心组件
    m_configManager = new ConfigManager(this);
    m_apiClient = new PveApiClient(this);
    m_authManager = new PveAuthManager(m_apiClient, this);
    m_connectionManager = new ConnectionManager(m_configManager, m_apiClient, this);

    // 加载配置
    m_configManager->load();

    // 设置无边框 (Kiosk 模式基础)
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    // 构建界面
    setupUI();
    setupConnections();
    loadStyleSheet();

    setWindowTitle("PVE 云桌面客户端");
    resize(1280, 720);
    // showFullScreen() 可在正式部署时开启
}

MainWindow::~MainWindow()
{
}

// ========== UI 构建 ==========

void MainWindow::setupUI()
{
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // 页面 0: 全屏登录视图
    m_loginView = new LoginView(m_configManager, m_authManager, this);
    m_stackedWidget->addWidget(m_loginView);

    // 页面 1: 云桌面工作台
    m_workspaceView = new WorkspaceView(m_configManager, m_apiClient, m_connectionManager, this);
    m_stackedWidget->addWidget(m_workspaceView);

    // 默认显示登录页
    m_stackedWidget->setCurrentIndex(0);
}

void MainWindow::setupConnections()
{
    // LoginView 发出已登录信号
    connect(m_loginView, &LoginView::loginSuccess,
            this, &MainWindow::onLoginSuccess);
    connect(m_loginView, &LoginView::settingsRequested,
            this, &MainWindow::onShowSettings);

    // WorkspaceView 发出注销信号
    connect(m_workspaceView, &WorkspaceView::logoutRequested,
            this, &MainWindow::onLogout);
}

void MainWindow::loadStyleSheet()
{
    QFile styleFile(":/styles/dark_theme.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        this->setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
}

// ========== 页面路由槽函数 ==========

void MainWindow::onLoginSuccess(const QString &username, const QString &node)
{
    qDebug() << "主窗口：登录成功，跳转到工作台，用户:" << username;
    m_workspaceView->onAuthenticated(username);
    m_stackedWidget->setCurrentIndex(1);
}

void MainWindow::onLogout()
{
    qDebug() << "主窗口：用户注销，返回登录页";
    m_authManager->logout();
    m_stackedWidget->setCurrentIndex(0);
}

void MainWindow::onShowSettings()
{
    SettingsDialog dialog(m_configManager, this);
    dialog.exec();
}
