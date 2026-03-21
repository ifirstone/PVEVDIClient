#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QList>
#include <QJsonObject>
#include "ConnectionInfo.h"

// 配置管理器：负责连接配置的持久化读写 (JSON 文件)
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);

    // 加载/保存配置文件
    bool load(const QString &filePath = QString());
    bool save(const QString &filePath = QString());

    // 连接管理
    QList<ConnectionInfo> connections() const;
    ConnectionInfo connection(const QString &id) const;
    void addConnection(const ConnectionInfo &info);
    void updateConnection(const ConnectionInfo &info);
    void removeConnection(const QString &id);

    // PVE 服务器设置
    QString pveHost() const;
    int pvePort() const;
    QString pveUsername() const;
    QString pvePassword() const;
    bool rememberPassword() const;
    void setPveServer(const QString &host, int port, const QString &username);
    void setPvePassword(const QString &password);
    void setRememberPassword(bool enabled);

    // 全局设置
    QString language() const;
    QString theme() const;
    bool kioskMode() const;
    bool autoConnect() const;
    QString autoConnectId() const;

    void setLanguage(const QString &lang);
    void setTheme(const QString &theme);
    void setKioskMode(bool enabled);
    void setAutoConnect(bool enabled, const QString &connectionId = QString());

    // 调试模式（开启后显示连接过程中的详细错误弹窗）
    bool debugMode() const;
    void setDebugMode(bool enabled);

    // RDP 外设重定向设置
    bool rdpSound() const;
    bool rdpMicrophone() const;
    bool rdpClipboard() const;
    bool rdpUsbDrive() const;
    bool rdpSmartcard() const;
    bool rdpPrinter() const;
    void setRdpRedirection(bool sound, bool mic, bool clipboard,
                           bool usb, bool smartcard, bool printer);

    // 配置文件路径
    QString configFilePath() const;

signals:
    void configChanged();
    void connectionAdded(const ConnectionInfo &info);
    void connectionUpdated(const ConnectionInfo &info);
    void connectionRemoved(const QString &id);

private:
    // 获取默认配置文件路径
    QString defaultConfigPath() const;

    QString m_configFilePath;
    QList<ConnectionInfo> m_connections;

    // PVE 服务器信息
    QString m_pveHost;
    int m_pvePort = 8006;
    QString m_pveUsername;
    QString m_pvePassword;
    bool m_rememberPassword = false;

    // 全局设置
    QString m_language = "zh_CN";
    QString m_theme = "dark";
    bool m_kioskMode = false;
    bool m_autoConnect = false;
    QString m_autoConnectId;
    bool m_debugMode = false;

    // RDP 外设重定向设置（默认配置）
    bool m_rdpSound      = true;
    bool m_rdpMicrophone = false;
    bool m_rdpClipboard  = true;
    bool m_rdpUsbDrive   = true;
    bool m_rdpSmartcard  = false;
    bool m_rdpPrinter    = false;
};

#endif // CONFIGMANAGER_H
