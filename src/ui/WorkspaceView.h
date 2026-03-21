#ifndef WORKSPACEVIEW_H
#define WORKSPACEVIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QJsonArray>
#include <QJsonObject>
#include <QPixmap>
#include <QTimer>
#include <QDateTime>

#include "../core/ConfigManager.h"
#include "../core/ConnectionManager.h"
#include "../pve/PveApiClient.h"

class VmCard;

// 云桌面资源工作台：登录成功后展示 VM 卡片网格
class WorkspaceView : public QWidget
{
    Q_OBJECT

public:
    explicit WorkspaceView(ConfigManager *configManager,
                           PveApiClient *apiClient,
                           ConnectionManager *connectionManager,
                           QWidget *parent = nullptr);

    // 认证成功，初始化工作台并拉取 VM 列表
    void onAuthenticated(const QString &username);

signals:
    void logoutRequested();

private slots:
    void onRefreshVMs();
    void onNodesReceived(const QJsonArray &nodes);
    void onVMsReceived(const QString &node, const QJsonArray &vms);
    void onApiError(const QString &error);
    void onVmIpReceived(const QString &node, int vmId, const QStringList &ips);

protected:
    void paintEvent(QPaintEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();
    void clearCards();
    void addVmCard(const QJsonObject &vmInfo);

    ConfigManager    *m_configManager;
    PveApiClient     *m_apiClient;
    ConnectionManager *m_connectionManager;

    QString  m_currentUsername;

    // 顶部信息栏
    QLabel  *m_lblUser;

    // VM 卡片网格容器
    QWidget  *m_cardsContainer;
    QScrollArea *m_scrollArea;

    // 存储当前加载的 VM 列表（node/vmid -> VmCard）
    QList<VmCard*> m_cards;

    QPixmap     m_background;
    QLabel      *m_lblDateTime;
    QTimer      *m_timer;
};

#endif // WORKSPACEVIEW_H
