#include "PveApiClient.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QUrlQuery>
#include <QDebug>

PveApiClient::PveApiClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // PVE 默认使用自签名证书，忽略 SSL 错误
    connect(m_networkManager, &QNetworkAccessManager::sslErrors,
            [](QNetworkReply *reply, const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        reply->ignoreSslErrors();
    });
}

void PveApiClient::setServer(const QString &host, int port)
{
    m_host = host;
    m_port = port;
    qDebug() << "PVE 服务器设置为:" << serverUrl();
}

QString PveApiClient::serverUrl() const
{
    return QString("https://%1:%2").arg(m_host).arg(m_port);
}

QString PveApiClient::buildUrl(const QString &path) const
{
    return serverUrl() + path;
}

bool PveApiClient::isAuthenticated() const
{
    return !m_ticket.isEmpty();
}

void PveApiClient::logout()
{
    m_ticket.clear();
    m_csrfToken.clear();
    m_username.clear();
    qDebug() << "已登出 PVE";
}

// ========== 认证 ==========

void PveApiClient::login(const QString &username, const QString &password,
                          const QString &realm)
{
    QString fullUsername = username;
    // 自动补充 realm 后缀
    if (!username.contains("@")) {
        fullUsername = username + "@" + realm;
    }

    QJsonObject data;
    data["username"] = fullUsername;
    data["password"] = password;

    QNetworkReply *reply = post("/api2/json/access/ticket", data);

    connect(reply, &QNetworkReply::finished, this, [this, reply, fullUsername]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString errMsg = QString("登录失败: %1").arg(reply->errorString());
            qWarning() << errMsg;
            emit loginFailed(errMsg);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject result = doc.object()["data"].toObject();

        m_ticket = result["ticket"].toString();
        m_csrfToken = result["CSRFPreventionToken"].toString();
        m_username = fullUsername;

        if (m_ticket.isEmpty()) {
            emit loginFailed("认证失败：未获取到有效 ticket");
            return;
        }

        qDebug() << "PVE 登录成功:" << m_username;
        emit loginSuccess(m_username);
    });
}

// ========== 节点与 VM 管理 ==========

void PveApiClient::fetchNodes()
{
    QNetworkReply *reply = get("/api2/json/nodes");

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit apiError(QString("获取节点列表失败: %1").arg(reply->errorString()));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray nodes = doc.object()["data"].toArray();
        qDebug() << "获取到" << nodes.size() << "个节点";
        emit nodesReceived(nodes);
    });
}

void PveApiClient::fetchVMs(const QString &node)
{
    QString path = QString("/api2/json/nodes/%1/qemu").arg(node);
    QNetworkReply *reply = get(path);

    connect(reply, &QNetworkReply::finished, this, [this, reply, node]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit apiError(QString("获取 VM 列表失败: %1").arg(reply->errorString()));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray vms = doc.object()["data"].toArray();
        qDebug() << "节点" << node << "共有" << vms.size() << "台虚拟机";
        emit vmsReceived(node, vms);
    });
}

void PveApiClient::fetchVMStatus(const QString &node, int vmId)
{
    QString path = QString("/api2/json/nodes/%1/qemu/%2/status/current")
                       .arg(node).arg(vmId);
    QNetworkReply *reply = get(path);

    connect(reply, &QNetworkReply::finished, this, [this, reply, node, vmId]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit apiError(QString("获取 VM 状态失败: %1").arg(reply->errorString()));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject status = doc.object()["data"].toObject();
        emit vmStatusReceived(node, vmId, status);
    });
}

void PveApiClient::startVM(const QString &node, int vmId)
{
    QString path = QString("/api2/json/nodes/%1/qemu/%2/status/start")
                       .arg(node).arg(vmId);
    QNetworkReply *reply = post(path);

    connect(reply, &QNetworkReply::finished, this, [this, reply, node, vmId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit vmActionFailed(node, vmId, reply->errorString());
        } else {
            emit vmActionCompleted(node, vmId, "start");
        }
    });
}

void PveApiClient::stopVM(const QString &node, int vmId)
{
    QString path = QString("/api2/json/nodes/%1/qemu/%2/status/stop")
                       .arg(node).arg(vmId);
    QNetworkReply *reply = post(path);

    connect(reply, &QNetworkReply::finished, this, [this, reply, node, vmId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit vmActionFailed(node, vmId, reply->errorString());
        } else {
            emit vmActionCompleted(node, vmId, "stop");
        }
    });
}

void PveApiClient::fetchVmIp(const QString &node, int vmId)
{
    // 调用 agent 获取网络接口信息
    QString path = QString("/api2/json/nodes/%1/qemu/%2/agent/network-get-interfaces")
                       .arg(node).arg(vmId);
    QNetworkReply *reply = get(path);

    connect(reply, &QNetworkReply::finished, this, [this, reply, node, vmId]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "无法获取 VM IP，可能是未安装 qemu-guest-agent:" << reply->errorString();
            emit vmIpReceived(node, vmId, QStringList()); // 返回空列表
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject result = doc.object()["data"].toObject();
        QJsonArray interfaces = result["result"].toArray();

        QStringList ips;

        for (const QJsonValue &ifaceVal : interfaces) {
            QJsonObject iface = ifaceVal.toObject();
            // 跳过 loopback
            if (iface["name"].toString() == "lo") continue;

            QJsonArray ipAddresses = iface["ip-addresses"].toArray();
            for (const QJsonValue &ipVal : ipAddresses) {
                QJsonObject ipInfo = ipVal.toObject();
                QString ip = ipInfo["ip-address"].toString();
                QString type = ipInfo["ip-address-type"].toString();

                // 优先收集 IPv4
                if (type == "ipv4" && ip != "127.0.0.1") {
                    ips << ip;
                }
            }
        }

        qDebug() << "解析出 VM" << vmId << "的 IP:" << ips;
        emit vmIpReceived(node, vmId, ips);
    });
}

// ========== SPICE Proxy ==========

void PveApiClient::requestSpiceProxy(const QString &node, int vmId)
{
    QString path = QString("/api2/json/nodes/%1/qemu/%2/spiceproxy")
                       .arg(node).arg(vmId);
    QNetworkReply *reply = post(path);

    connect(reply, &QNetworkReply::finished, this, [this, reply, node, vmId]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QString errorBody = QString::fromUtf8(reply->readAll());
            qWarning() << "获取 SPICE 配置失败, HTTP状态码:" 
                       << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                       << " 错误:" << reply->errorString()
                       << " 响应体:" << errorBody;
            emit apiError(QString("获取 SPICE 配置失败: %1").arg(reply->errorString()));
            return;
        }

        QByteArray rawData = reply->readAll();
        qDebug() << "SPICE Proxy 原始返回数据:" << rawData;

        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        QJsonObject data = doc.object()["data"].toObject();

        // 构建 .vv 文件内容
        QString vvContent;
        vvContent += "[virt-viewer]\n";
        // 必选字段
        vvContent += QString("type=%1\n").arg(data["type"].toString());
        vvContent += QString("host=%1\n").arg(data["host"].toString());
        vvContent += QString("port=%1\n").arg(data["port"].toInt());

        // 针对无法解析 PVE 内部域名 (如 Cloud.server.localhost) 的修复方案：
        // 强制把返回的主机名和代理地址中的主机部分替换为客户端实际连接 PVE 的真实 IP
        QString realHost = m_host;
        
        QString proxyVal = data["proxy"].toString();
        if (!proxyVal.isEmpty()) {
            // 简单的替换：把 http://xxx.localhost:3128 变成 http://192.168.0.3:3128
            // 匹配 http:// 和 :端口 之间的内容进行替换，或者简单粗暴一点，只要包含 localhost/内部名称就整个替换 host 部分
            QUrl proxyUrl(proxyVal);
            if (proxyUrl.isValid() && proxyUrl.host().endsWith("localhost", Qt::CaseInsensitive)) {
                proxyUrl.setHost(realHost);
                proxyVal = proxyUrl.toString();
            }
            // 防止 qt5 不支持 setHost 等，做个兜底字符串替换
            if (proxyVal.contains(".localhost")) {
                int start = proxyVal.indexOf("://") + 3;
                int end = proxyVal.lastIndexOf(":");
                if (start > 2 && end > start) {
                    proxyVal = "http://" + realHost + proxyVal.mid(end);
                }
            }
            data["proxy"] = proxyVal;
        }

        // 可选字段 (按 PVE API 返回格式直接映射)
        QStringList stringFields = {
            "password", "proxy", "tls-port", "title", "host-subject", 
            "toggle-fullscreen", "release-cursor", "secure-attention"
        };
        for (const QString &field : stringFields) {
            if (data.contains(field)) {
                // 将所有 JSON 值转换为字符串并去掉可能前后的双引号
                QString val = data[field].isString() ? data[field].toString() : QString::number(data[field].toInt());
                // 如果是对象或未识别类型，则转为 string 再写入
                if (val.isEmpty() && !data[field].isNull()) {
                    val = data[field].toVariant().toString();
                }
                vvContent += QString("%1=%2\n").arg(field).arg(val);
            }
        }

        // CA 证书单独处理：
        // PVE 返回的 CA 字符串内包含了显示的 "\n" 字面量，
        // 在写入 .vv（INI 格式）时，多行证书必须在一行内并用 "\n" 字符表示换行。
        // 所以我们不需要将 "\n" 替换为真实换行，原样或修剪后写入即可。
        if (data.contains("ca")) {
            QString ca = data["ca"].toString();
            // 如果解析出有多余的双引号，去掉它
            if (ca.startsWith("\"") && ca.endsWith("\"")) {
                ca = ca.mid(1, ca.length() - 2);
            }
            // 确保没有多余空白
            ca = ca.trimmed();
            // 由于上面从 json 解析 toString 后，字面 "\n" 可能会变成真正的换行符
            // 我们需要重新将其安全转换为字面量 "\n" 以便单行写入 .vv 文件
            ca.replace("\n", "\\n"); 
            vvContent += QString("ca=%1\n").arg(ca);
        }

        // 启用 USB 重定向
        vvContent += "usb-filter=-1,-1,-1,-1,0\n";
        // 暂时关闭自动删除以便调试
        vvContent += "delete-this-file=0\n";

        qDebug() << "获取 SPICE 配置成功，VM:" << vmId;
        emit spiceProxyReceived(node, vmId, vvContent);
    });
}

void PveApiClient::requestVncProxy(const QString &node, int vmId)
{
    QString path = QString("/api2/json/nodes/%1/qemu/%2/vncproxy")
                       .arg(node).arg(vmId);
    QNetworkReply *reply = post(path);

    connect(reply, &QNetworkReply::finished, this, [this, reply, node, vmId]() {
        reply->deleteLater();
        Q_UNUSED(node);
        Q_UNUSED(vmId);
        // VNC proxy 暂不实现完整逻辑，预留接口
        if (reply->error() != QNetworkReply::NoError) {
            emit apiError(QString("获取 VNC 配置失败: %1").arg(reply->errorString()));
        }
    });
}

// ========== HTTP 方法 ==========

QNetworkReply* PveApiClient::get(const QString &path)
{
    QNetworkRequest request(QUrl(buildUrl(path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 添加认证 Cookie
    if (!m_ticket.isEmpty()) {
        request.setRawHeader("Cookie", QString("PVEAuthCookie=%1").arg(m_ticket).toUtf8());
    }

    // 忽略 SSL 自签名证书
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    return m_networkManager->get(request);
}

QNetworkReply* PveApiClient::post(const QString &path, const QJsonObject &data)
{
    QNetworkRequest request(QUrl(buildUrl(path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 添加认证信息
    if (!m_ticket.isEmpty()) {
        request.setRawHeader("Cookie", QString("PVEAuthCookie=%1").arg(m_ticket).toUtf8());
        request.setRawHeader("CSRFPreventionToken", m_csrfToken.toUtf8());
    }

    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);

    QJsonDocument doc(data);
    return m_networkManager->post(request, doc.toJson());
}
