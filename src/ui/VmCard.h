#ifndef VMCARD_H
#define VMCARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QJsonObject>

#include "../core/ConfigManager.h"
#include "../core/ConnectionManager.h"
#include "../core/ConnectionInfo.h"
#include "../pve/PveApiClient.h"

// 单个虚拟机卡片控件
// 展示: OS图标 | VM名称/ID/状态 | 底部操作区（协议选择/电源管理/快照/连接）
class VmCard : public QWidget
{
    Q_OBJECT

public:
    explicit VmCard(const QJsonObject &vmInfo,
                    ConfigManager *configManager,
                    ConnectionManager *connectionManager,
                    PveApiClient *apiClient,
                    QWidget *parent = nullptr);

    // 供 WorkspaceView 转发 IP 结果
    void onIpReceived(const QStringList &ips);

    QString node() const;
    int vmId() const;

private slots:
    void onConnectClicked();
    void onPowerAction(const QString &action);
    void onConnectionStarted(const ConnectionInfo &info);
    void onConnectionEnded(const ConnectionInfo &info, int exitCode);
    void onConnectionError(const ConnectionInfo &info, const QString &error);

private:
    void setupUI();
    QString osIcon() const;    // 根据 VM 名称/配置推断操作系统图标
    void updateStatusStyle();
    void doRdpConnect();       // 直接发起 RDP 连接（不弹窗）

    QJsonObject        m_vmInfo;
    ConfigManager     *m_configManager;
    ConnectionManager *m_connectionManager;
    PveApiClient      *m_apiClient;

    // RDP 获取到的 IP
    QString m_rdpIp;
    bool    m_waitingForIp = false;

    // 控件
    QLabel      *m_lblOsIcon;
    QLabel      *m_lblName;
    QLabel      *m_lblStatus;
    QLabel      *m_lblIp;          // 显示 VM 的 IP 地址（获取后更新）
    QComboBox   *m_cmbProtocol;
    QLineEdit   *m_editRdpPort;    // 选 RDP 时显示端口输入框（默认3389）
    QComboBox   *m_cmbPower;
    QPushButton *m_btnConnect;
    QPushButton *m_btnSnapshot;
};

#endif // VMCARD_H
