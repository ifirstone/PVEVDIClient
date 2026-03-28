#ifndef CONNECTIONINFO_H
#define CONNECTIONINFO_H

#include <QString>
#include <QUuid>
#include <QJsonObject>

// 协议类型枚举
enum class Protocol {
    RDP,
    SPICE
};

// 连接信息数据模型
struct ConnectionInfo {
    QString id;           // UUID 唯一标识
    QString name;         // 显示名称
    QString serverHost;   // PVE 服务器地址
    int serverPort = 8006;// PVE API 端口
    QString node;         // PVE 节点名
    int vmId = 0;         // 虚拟机 ID

    Protocol protocol = Protocol::RDP;

    // RDP 连接参数
    QString rdpHost;      // RDP 目标地址（可与 PVE 不同）
    int rdpPort = 3389;   // RDP 端口
    QString username;
    QString password;     // 注意：生产环境应加密存储
    QString domain;       // Windows 域
    QString resolution = "fullscreen"; // "1920x1080" 或 "fullscreen"
    bool dynamicResolution = true;

    // 通用外设选项
    bool enableSound = true;
    bool enableMicrophone = false;
    bool enableClipboard = true;
    bool enableUSBDrive = false;
    bool enableSmartcard = false;
    bool enablePrinter = false;

    // 额外自定义参数
    QString extraArgs;

    // RDP 高级性能与编解码设置 (从全局 Config 继承或独立设置)
    int rdpVersion = 0; // 0: Auto, 2: v2, 3: v3
    QString rdpCodec = "h264:420";
    int rdpColorDepth = 32;
    QString rdpNetwork = "auto";
    QString rdpScale = "100%";
    bool rdpUsermode = false;

    // 生成新的连接 ID
    static QString generateId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // 序列化为 JSON
    QJsonObject toJson() const;

    // 从 JSON 反序列化
    static ConnectionInfo fromJson(const QJsonObject &json);

    // 协议枚举转字符串
    static QString protocolToString(Protocol p);
    static Protocol stringToProtocol(const QString &s);
};

#endif // CONNECTIONINFO_H
