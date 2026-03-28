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
        m_pveLanHost = server["lan_host"].toString();
        m_pvePort = server["port"].toInt(8006);
        m_pveUsername = server["username"].toString();
        m_rememberPassword = server["remember_password"].toBool(false);
        if (m_rememberPassword) {
            // 密码以 Base64 简单编码存储，避免明文直接暴露
            m_pvePassword = QString::fromUtf8(QByteArray::fromBase64(server["password"].toString().toUtf8()));
        }
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
        m_autoLogin = settings["auto_login"].toBool(false);
        m_autoConnect = settings["auto_connect"].toBool(false);
        m_autoConnectId = settings["auto_connect_id"].toString();
        m_autoConnectProtocol = settings["auto_connect_protocol"].toString("RDP").toUpper();
        m_loginWelcomeText = settings["login_welcome_text"].toString("欢迎使用云桌面").trimmed();
        m_footerBrandText = settings["footer_brand_text"].toString("PVE Thin Client").trimmed();
        if (m_loginWelcomeText.isEmpty()) {
            m_loginWelcomeText = "欢迎使用云桌面";
        }
        if (m_footerBrandText.isEmpty()) {
            m_footerBrandText = "PVE Thin Client";
        }
        m_debugMode = settings["debug_mode"].toBool(false);
    }

    // 解析 RDP 高级配置
    if (root.contains("rdp_advanced")) {
        QJsonObject rdpAdv = root["rdp_advanced"].toObject();
        m_rdpVersion = rdpAdv["version"].toInt(3);
        m_rdpCodec = rdpAdv["codec"].toString("h264:420");
        m_rdpColorDepth = rdpAdv["color_depth"].toInt(32);
        m_rdpNetwork = rdpAdv["network"].toString("auto");
        m_rdpScale = rdpAdv["scale"].toString("100%");
        m_rdpUsermode = rdpAdv["usermode"].toBool(false);
    }

    // 解析 RDP 重定向配置
    if (root.contains("rdp_redirection")) {
        QJsonObject rdpRedir = root["rdp_redirection"].toObject();
        m_rdpSound = rdpRedir["sound"].toBool(true);
        m_rdpMicrophone = rdpRedir["microphone"].toBool(false);
        m_rdpClipboard = rdpRedir["clipboard"].toBool(true);
        m_rdpUsbDrive = rdpRedir["usb_drive"].toBool(false);
        m_rdpSmartcard = rdpRedir["smartcard"].toBool(false);
        m_rdpPrinter = rdpRedir["printer"].toBool(false);
    }

    // 解析 EasyTier P2P 设置
    if (root.contains("easytier")) {
        QJsonObject easyTier = root["easytier"].toObject();
        m_useEasyTier = easyTier["use_easy_tier"].toBool(false);
        m_easyTierBinaryPath = easyTier["binary_path"].toString();
        m_enableTieredMode = easyTier["tiered_mode"].toBool(false);
        m_easyTierNetworkName = easyTier["network_name"].toString("pve-easytier");
        m_easyTierNetworkSecret = easyTier["network_secret"].toString("pve-easytier-secret");
        m_easyTierServerPeerIp = easyTier["server_peer_ip"].toString();
        m_easyTierBootstrapUrl = easyTier["bootstrap_url"].toString();
        m_easyTierExtraArgs = easyTier["extra_args"].toString();
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
    server["lan_host"] = m_pveLanHost;
    server["port"] = m_pvePort;
    server["username"] = m_pveUsername;
    server["remember_password"] = m_rememberPassword;
    if (m_rememberPassword && !m_pvePassword.isEmpty()) {
        server["password"] = QString::fromUtf8(m_pvePassword.toUtf8().toBase64());
    }
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
    settings["auto_login"] = m_autoLogin;
    settings["auto_connect"] = m_autoConnect;
    settings["auto_connect_id"] = m_autoConnectId;
    settings["auto_connect_protocol"] = m_autoConnectProtocol;
    settings["login_welcome_text"] = m_loginWelcomeText;
    settings["footer_brand_text"] = m_footerBrandText;
    settings["debug_mode"] = m_debugMode;
    root["settings"] = settings;

    // RDP 高级配置
    QJsonObject rdpAdv;
    rdpAdv["version"] = m_rdpVersion;
    rdpAdv["codec"] = m_rdpCodec;
    rdpAdv["color_depth"] = m_rdpColorDepth;
    rdpAdv["network"] = m_rdpNetwork;
    rdpAdv["scale"] = m_rdpScale;
    rdpAdv["usermode"] = m_rdpUsermode;
    root["rdp_advanced"] = rdpAdv;

    // RDP 重定向配置
    QJsonObject rdpRedir;
    rdpRedir["sound"] = m_rdpSound;
    rdpRedir["microphone"] = m_rdpMicrophone;
    rdpRedir["clipboard"] = m_rdpClipboard;
    rdpRedir["usb_drive"] = m_rdpUsbDrive;
    rdpRedir["smartcard"] = m_rdpSmartcard;
    rdpRedir["printer"] = m_rdpPrinter;
    root["rdp_redirection"] = rdpRedir;

    // EasyTier P2P 设置
    QJsonObject easyTier;
    easyTier["use_easy_tier"] = m_useEasyTier;
    easyTier["binary_path"] = m_easyTierBinaryPath;
    easyTier["tiered_mode"] = m_enableTieredMode;
    easyTier["network_name"] = m_easyTierNetworkName;
    easyTier["network_secret"] = m_easyTierNetworkSecret;
    easyTier["server_peer_ip"] = m_easyTierServerPeerIp;
    easyTier["bootstrap_url"] = m_easyTierBootstrapUrl;
    easyTier["extra_args"] = m_easyTierExtraArgs;
    root["easytier"] = easyTier;

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
QString ConfigManager::pveLanHost() const { return m_pveLanHost; }
QString ConfigManager::effectivePveHost() const
{
    if (m_useEasyTier) {
        return m_pveLanHost.trimmed();
    }
    return m_pveHost;
}
int ConfigManager::pvePort() const { return m_pvePort; }
QString ConfigManager::pveUsername() const { return m_pveUsername; }
QString ConfigManager::pvePassword() const { return m_pvePassword; }
bool ConfigManager::rememberPassword() const { return m_rememberPassword; }

void ConfigManager::setPveServer(const QString &host, int port, const QString &username)
{
    m_pveHost = host;
    m_pvePort = port;
    m_pveUsername = username;
}

void ConfigManager::setPveLanHost(const QString &host)
{
    m_pveLanHost = host.trimmed();
}

void ConfigManager::setPvePassword(const QString &password)
{
    m_pvePassword = password;
}

void ConfigManager::setRememberPassword(bool enabled)
{
    m_rememberPassword = enabled;
    if (!enabled) {
        m_pvePassword.clear(); // 取消勾选时清除已保存的密码
    }
}

// 全局设置 getter
QString ConfigManager::language() const { return m_language; }
QString ConfigManager::theme() const { return m_theme; }
bool ConfigManager::kioskMode() const { return m_kioskMode; }
bool ConfigManager::autoLogin() const { return m_autoLogin; }
bool ConfigManager::autoConnect() const { return m_autoConnect; }
QString ConfigManager::autoConnectId() const { return m_autoConnectId; }
QString ConfigManager::autoConnectProtocol() const { return m_autoConnectProtocol; }
QString ConfigManager::loginWelcomeText() const { return m_loginWelcomeText; }
QString ConfigManager::footerBrandText() const { return m_footerBrandText; }

// 全局设置 setter
void ConfigManager::setLanguage(const QString &lang) { m_language = lang; }
void ConfigManager::setTheme(const QString &theme) { m_theme = theme; }
void ConfigManager::setKioskMode(bool enabled) { m_kioskMode = enabled; }
void ConfigManager::setAutoLogin(bool enabled) { m_autoLogin = enabled; }
void ConfigManager::setAutoConnect(bool enabled, const QString &connectionId, const QString &protocol)
{
    m_autoConnect = enabled;
    if (!enabled) {
        m_autoConnectId.clear();
        m_autoConnectProtocol = "RDP";
        return;
    }

    m_autoConnectId = connectionId;
    if (!protocol.trimmed().isEmpty()) {
        m_autoConnectProtocol = protocol.trimmed().toUpper();
    }
}

void ConfigManager::setLoginWelcomeText(const QString &text)
{
    const QString t = text.trimmed();
    m_loginWelcomeText = t.isEmpty() ? QString("欢迎使用云桌面") : t;
}

void ConfigManager::setFooterBrandText(const QString &text)
{
    const QString t = text.trimmed();
    m_footerBrandText = t.isEmpty() ? QString("PVE Thin Client") : t;
}

// 调试模式 getter/setter
bool ConfigManager::debugMode() const { return m_debugMode; }
void ConfigManager::setDebugMode(bool enabled) { m_debugMode = enabled; }

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
}

// RDP 高级性能与编解码设置
int ConfigManager::rdpVersion() const { return m_rdpVersion; }
QString ConfigManager::rdpCodec() const { return m_rdpCodec; }
int ConfigManager::rdpColorDepth() const { return m_rdpColorDepth; }
QString ConfigManager::rdpNetwork() const { return m_rdpNetwork; }
QString ConfigManager::rdpScale() const { return m_rdpScale; }
bool ConfigManager::rdpUsermode() const { return m_rdpUsermode; }

void ConfigManager::setRdpAdvanced(int version, const QString &codec, int colorDepth,
                                   const QString &network, const QString &scale, bool usermode)
{
    m_rdpVersion = version;
    m_rdpCodec = codec;
    m_rdpColorDepth = colorDepth;
    m_rdpNetwork = network;
    m_rdpScale = scale;
    m_rdpUsermode = usermode;
}

bool ConfigManager::useEasyTier() const { return m_useEasyTier; }
QString ConfigManager::easyTierBinaryPath() const { return m_easyTierBinaryPath; }
bool ConfigManager::enableTieredMode() const { return m_enableTieredMode; }
QString ConfigManager::easyTierNetworkName() const { return m_easyTierNetworkName; }
QString ConfigManager::easyTierNetworkSecret() const { return m_easyTierNetworkSecret; }
QString ConfigManager::easyTierServerPeerIp() const { return m_easyTierServerPeerIp; }
QString ConfigManager::easyTierBootstrapUrl() const { return m_easyTierBootstrapUrl; }
QString ConfigManager::easyTierExtraArgs() const { return m_easyTierExtraArgs; }

void ConfigManager::setUseEasyTier(bool enabled) { m_useEasyTier = enabled; }
void ConfigManager::setEasyTierBinaryPath(const QString &path) { m_easyTierBinaryPath = path; }
void ConfigManager::setEnableTieredMode(bool enabled) { m_enableTieredMode = enabled; }
void ConfigManager::setEasyTierNetworkName(const QString &name) { m_easyTierNetworkName = name; }
void ConfigManager::setEasyTierNetworkSecret(const QString &secret) { m_easyTierNetworkSecret = secret; }
void ConfigManager::setEasyTierServerPeerIp(const QString &ip) { m_easyTierServerPeerIp = ip.trimmed(); }
void ConfigManager::setEasyTierBootstrapUrl(const QString &url) { m_easyTierBootstrapUrl = url; }
void ConfigManager::setEasyTierExtraArgs(const QString &args) { m_easyTierExtraArgs = args; }
