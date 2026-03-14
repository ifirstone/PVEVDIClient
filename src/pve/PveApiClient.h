#ifndef PVEAPICLIENT_H
#define PVEAPICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

// PVE REST API 客户端
// 负责与 Proxmox VE 服务器通信：认证、获取 VM 列表、获取 SPICE ticket 等
class PveApiClient : public QObject
{
    Q_OBJECT

public:
    explicit PveApiClient(QObject *parent = nullptr);

    // 设置服务器信息
    void setServer(const QString &host, int port = 8006);
    QString serverUrl() const;

    // 认证（获取 ticket 和 CSRFPreventionToken）
    void login(const QString &username, const QString &password,
               const QString &realm = "pam");

    // 获取节点列表
    void fetchNodes();

    // 获取指定节点的 VM 列表
    void fetchVMs(const QString &node);

    // 获取 VM 状态
    void fetchVMStatus(const QString &node, int vmId);

    // 启动/停止 VM
    void startVM(const QString &node, int vmId);
    void stopVM(const QString &node, int vmId);

    // 获取 SPICE 连接配置（返回 .vv 文件内容）
    void requestSpiceProxy(const QString &node, int vmId);

    // 获取 VNC ticket（用于 noVNC 或其他用途）
    void requestVncProxy(const QString &node, int vmId);

    // 获取 VM IP (利用 qemu-guest-agent)
    void fetchVmIp(const QString &node, int vmId);

    // 是否已认证
    bool isAuthenticated() const;

    // 登出
    void logout();

signals:
    // 认证结果
    void loginSuccess(const QString &username);
    void loginFailed(const QString &errorMessage);

    // 节点列表
    void nodesReceived(const QJsonArray &nodes);

    // VM 列表
    void vmsReceived(const QString &node, const QJsonArray &vms);

    // VM 状态
    void vmStatusReceived(const QString &node, int vmId, const QJsonObject &status);

    // VM IP (来自 qemu-guest-agent)
    void vmIpReceived(const QString &node, int vmId, const QStringList &ips);

    // VM 操作结果
    void vmActionCompleted(const QString &node, int vmId, const QString &action);
    void vmActionFailed(const QString &node, int vmId, const QString &error);

    // SPICE 配置
    void spiceProxyReceived(const QString &node, int vmId, const QString &vvContent);

    // 通用错误
    void apiError(const QString &errorMessage);

private:
    // 发送 GET/POST 请求
    QNetworkReply* get(const QString &path);
    QNetworkReply* post(const QString &path, const QJsonObject &data = QJsonObject());

    // 构建完整 API URL
    QString buildUrl(const QString &path) const;

    // 处理响应
    void handleReply(QNetworkReply *reply, std::function<void(const QJsonObject &)> callback);

    QNetworkAccessManager *m_networkManager;
    QString m_host;
    int m_port = 8006;

    // 认证信息
    QString m_ticket;
    QString m_csrfToken;
    QString m_username;
};

#endif // PVEAPICLIENT_H
