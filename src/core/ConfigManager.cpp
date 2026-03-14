#include "ConfigManager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
}

QString ConfigManager::defaultConfigPath() const
{
    // 配置文件存放在用户数据目录
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("config.json");
}

QString ConfigManager::configFilePath() const
{
    return m_configFilePath.isEmpty() ? defaultConfigPath() : m_configFilePath;
}

bool ConfigManager::load(const QString &filePath)
{
    m_configFilePath = filePath.isEmpty() ? defaultConfigPath() : filePath;

    QFile file(m_configFilePath);
    if (!file.exists()) {
        qDebug() << "配置文件不存在，使用默认配置:" << m_configFilePath;
        return true; // 首次运行，使用默认值
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开配置文件:" << m_configFilePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "配置文件解析失败:" << error.errorString();
        return false;
    }

    QJsonObject root = doc.object();

    // 解析 PVE 服务器信息
    if (root.contains("pve_server")) {
        QJsonObject server = root["pve_server"].toObject();
        m_pveHost = server["host"].toString();
        m_pvePort = server["port"].toInt(8006);
        m_pveUsername = server["username"].toString();
    }

    // 解析连接列表
    m_connections.clear();
    QJsonArray connArray = root["connections"].toArray();
    for (const QJsonValue &val : connArray) {
        m_connections.append(ConnectionInfo::fromJson(val.toObject()));
    }

    // 解析全局设置
    if (root.contains("settings")) {
        QJsonObject settings = root["settings"].toObject();
        m_language = settings["language"].toString("zh_CN");
        m_theme = settings["theme"].toString("dark");
        m_kioskMode = settings["kiosk_mode"].toBool(false);
        m_autoConnect = settings["auto_connect"].toBool(false);
        m_autoConnectId = settings["auto_connect_id"].toString();
        m_debugMode = settings["debug_mode"].toBool(false);
    }

    qDebug() << "配置加载成功，共" << m_connections.size() << "个连接";
    return true;
}

bool ConfigManager::save(const QString &filePath)
{
    QString path = filePath.isEmpty() ? configFilePath() : filePath;

    // 确保目录存在
    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonObject root;
    root["version"] = 1;

    // PVE 服务器信息
    QJsonObject server;
    server["host"] = m_pveHost;
    server["port"] = m_pvePort;
    server["username"] = m_pveUsername;
    root["pve_server"] = server;

    // 连接列表
    QJsonArray connArray;
    for (const ConnectionInfo &conn : m_connections) {
        connArray.append(conn.toJson());
    }
    root["connections"] = connArray;

    // 全局设置
    QJsonObject settings;
    settings["language"] = m_language;
    settings["theme"] = m_theme;
    settings["kiosk_mode"] = m_kioskMode;
    settings["auto_connect"] = m_autoConnect;
    settings["auto_connect_id"] = m_autoConnectId;
    settings["debug_mode"] = m_debugMode;
    root["settings"] = settings;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法写入配置文件:" << path;
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "配置保存成功:" << path;
    emit configChanged();
    return true;
}

QList<ConnectionInfo> ConfigManager::connections() const
{
    return m_connections;
}

ConnectionInfo ConfigManager::connection(const QString &id) const
{
    for (const ConnectionInfo &conn : m_connections) {
        if (conn.id == id) return conn;
    }
    return ConnectionInfo();
}

void ConfigManager::addConnection(const ConnectionInfo &info)
{
    m_connections.append(info);
    save();
    emit connectionAdded(info);
}

void ConfigManager::updateConnection(const ConnectionInfo &info)
{
    for (int i = 0; i < m_connections.size(); ++i) {
        if (m_connections[i].id == info.id) {
            m_connections[i] = info;
            save();
            emit connectionUpdated(info);
            return;
        }
    }
}

void ConfigManager::removeConnection(const QString &id)
{
    for (int i = 0; i < m_connections.size(); ++i) {
        if (m_connections[i].id == id) {
            m_connections.removeAt(i);
            save();
            emit connectionRemoved(id);
            return;
        }
    }
}

// PVE 服务器设置
QString ConfigManager::pveHost() const { return m_pveHost; }
int ConfigManager::pvePort() const { return m_pvePort; }
QString ConfigManager::pveUsername() const { return m_pveUsername; }

void ConfigManager::setPveServer(const QString &host, int port, const QString &username)
{
    m_pveHost = host;
    m_pvePort = port;
    m_pveUsername = username;
    save();
}

// 全局设置 getter
QString ConfigManager::language() const { return m_language; }
QString ConfigManager::theme() const { return m_theme; }
bool ConfigManager::kioskMode() const { return m_kioskMode; }
bool ConfigManager::autoConnect() const { return m_autoConnect; }
QString ConfigManager::autoConnectId() const { return m_autoConnectId; }

// 全局设置 setter
void ConfigManager::setLanguage(const QString &lang) { m_language = lang; save(); }
void ConfigManager::setTheme(const QString &theme) { m_theme = theme; save(); }
void ConfigManager::setKioskMode(bool enabled) { m_kioskMode = enabled; save(); }
void ConfigManager::setAutoConnect(bool enabled, const QString &connectionId)
{
    m_autoConnect = enabled;
    m_autoConnectId = connectionId;
    save();
}

// 调试模式 getter/setter
bool ConfigManager::debugMode() const { return m_debugMode; }
void ConfigManager::setDebugMode(bool enabled) { m_debugMode = enabled; save(); }

// RDP 外设重定向 getter
bool ConfigManager::rdpSound()      const { return m_rdpSound; }
bool ConfigManager::rdpMicrophone() const { return m_rdpMicrophone; }
bool ConfigManager::rdpClipboard()  const { return m_rdpClipboard; }
bool ConfigManager::rdpUsbDrive()   const { return m_rdpUsbDrive; }
bool ConfigManager::rdpSmartcard()  const { return m_rdpSmartcard; }
bool ConfigManager::rdpPrinter()    const { return m_rdpPrinter; }

void ConfigManager::setRdpRedirection(bool sound, bool mic, bool clipboard,
                                       bool usb, bool smartcard, bool printer)
{
    m_rdpSound      = sound;
    m_rdpMicrophone = mic;
    m_rdpClipboard  = clipboard;
    m_rdpUsbDrive   = usb;
    m_rdpSmartcard  = smartcard;
    m_rdpPrinter    = printer;
    save();
}
