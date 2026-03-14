#include "ConnectionInfo.h"
#include <QJsonObject>

QJsonObject ConnectionInfo::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["server_host"] = serverHost;
    obj["server_port"] = serverPort;
    obj["node"] = node;
    obj["vm_id"] = vmId;
    obj["protocol"] = protocolToString(protocol);

    // RDP 参数
    obj["rdp_host"] = rdpHost;
    obj["rdp_port"] = rdpPort;
    obj["username"] = username;
    // 密码不序列化到普通 JSON（安全考虑）
    obj["domain"] = domain;
    obj["resolution"] = resolution;
    obj["dynamic_resolution"] = dynamicResolution;

    // 外设选项
    obj["enable_sound"] = enableSound;
    obj["enable_microphone"] = enableMicrophone;
    obj["enable_clipboard"] = enableClipboard;
    obj["enable_usb_drive"] = enableUSBDrive;
    obj["enable_smartcard"] = enableSmartcard;
    obj["enable_printer"] = enablePrinter;

    obj["extra_args"] = extraArgs;

    return obj;
}

ConnectionInfo ConnectionInfo::fromJson(const QJsonObject &json)
{
    ConnectionInfo info;
    info.id = json["id"].toString();
    info.name = json["name"].toString();
    info.serverHost = json["server_host"].toString();
    info.serverPort = json["server_port"].toInt(8006);
    info.node = json["node"].toString();
    info.vmId = json["vm_id"].toInt(0);
    info.protocol = stringToProtocol(json["protocol"].toString());

    info.rdpHost = json["rdp_host"].toString();
    info.rdpPort = json["rdp_port"].toInt(3389);
    info.username = json["username"].toString();
    info.domain = json["domain"].toString();
    info.resolution = json["resolution"].toString("fullscreen");
    info.dynamicResolution = json["dynamic_resolution"].toBool(true);

    info.enableSound = json["enable_sound"].toBool(true);
    info.enableMicrophone = json["enable_microphone"].toBool(false);
    info.enableClipboard = json["enable_clipboard"].toBool(true);
    info.enableUSBDrive = json["enable_usb_drive"].toBool(false);
    info.enableSmartcard = json["enable_smartcard"].toBool(false);
    info.enablePrinter = json["enable_printer"].toBool(false);

    info.extraArgs = json["extra_args"].toString();

    return info;
}

QString ConnectionInfo::protocolToString(Protocol p)
{
    switch (p) {
    case Protocol::RDP:   return "rdp";
    case Protocol::SPICE: return "spice";
    default:              return "rdp";
    }
}

Protocol ConnectionInfo::stringToProtocol(const QString &s)
{
    if (s.toLower() == "spice") return Protocol::SPICE;
    return Protocol::RDP;
}
