#include "RdpLauncher.h"
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>

RdpLauncher::RdpLauncher(QObject *parent)
    : AbstractLauncher(parent)
{
}

bool RdpLauncher::launch(const ConnectionInfo &info)
{
    QString program = xfreerdpPath();
    QStringList args = buildArguments(info);

    if (program.isEmpty()) {
        emit connectionError("未找到 xfreerdp 客户端程序，请确认系统已安装 freerdp 客户端组件");
        return false;
    }

    qDebug() << "启动 RDP 连接:" << program << args;
    return m_runner->start(program, args);
}

QStringList RdpLauncher::buildArguments(const ConnectionInfo &info) const
{
    QStringList args;

    // 目标主机
    QString host = info.rdpHost.isEmpty() ? info.serverHost : info.rdpHost;
    if (info.rdpPort != 3389) {
        args << QString("/v:%1:%2").arg(host).arg(info.rdpPort);
    } else {
        args << QString("/v:%1").arg(host);
    }

    // 认证信息
    if (!info.username.isEmpty()) {
        args << QString("/u:%1").arg(info.username);
    } else {
        // 为了实现空密直连并在远程画面输入密码，我们需要随意提供一个假账号，
        // 否则 freeRDP 必定在终端抛出提示符索要系统本地用户名对应的密码并引起阻塞。
        args << "/u:GuestUser";
    }

    if (!info.password.isEmpty()) {
        args << QString("/p:%1").arg(info.password);
    } else {
        // Windows 默认不允许使用空密码进行 RDP 登录，为了让画面弹出来要求重新输入，传一个假密码
        args << "/p:DefaultRDPPassword01!";
    }
    if (!info.domain.isEmpty()) {
        args << QString("/d:%1").arg(info.domain);
    }

    // 分辨率
    if (info.resolution == "fullscreen") {
        args << "/f";
    } else if (!info.resolution.isEmpty()) {
        args << QString("/size:%1").arg(info.resolution);
    }

    // 动态分辨率（跟随窗口大小变化）
    if (info.dynamicResolution) {
        args << "/dynamic-resolution";
    }

    // 声音
    if (info.enableSound) {
#ifdef Q_OS_WIN
        args << "/sound:sys:winmm";
#else
        args << "/sound:sys:pulse";
#endif
    }

    // 麦克风
    if (info.enableMicrophone) {
#ifdef Q_OS_WIN
        args << "/microphone:sys:winmm";
#else
        args << "/microphone:sys:pulse";
#endif
    }

    // 剪贴板
    if (info.enableClipboard) {
        args << "+clipboard";
    }

    // USB 驱动器重定向
    if (info.enableUSBDrive) {
#ifdef Q_OS_WIN
        // Windows: 不支持 /drive 方式，跳过
#else
        args << "/drive:usb,/run/media";
#endif
    }

    // 智能卡 (U-Key)
    if (info.enableSmartcard) {
        args << "/smartcard";
    }

    // 打印机
    if (info.enablePrinter) {
        args << "/printer";
    }

    // 证书处理：FreeRDP 3 建议用 /cert:tofu, 兼容旧版用 /cert:ignore
    args << "/cert:tofu" << "/cert:ignore";

    // 强制使用 TLS 安全层（绕过 NLA），这样即使密码为空/错误也能先弹出远程画面
    args << "/sec:tls";

    // 压缩与色彩优化降低卡顿
    args << "/compression-level:2" << "/bpp:32" << "+fonts";

    // 额外自定义参数
    if (!info.extraArgs.isEmpty()) {
        args << info.extraArgs.split(" ", QString::SkipEmptyParts);
    }

    return args;
}

QString RdpLauncher::buildCommandLine(const ConnectionInfo &info) const
{
    QString program = xfreerdpPath();
    if (program.isEmpty()) program = "xfreerdp";

    QStringList args = buildArguments(info);

    // 隐藏密码显示
    QStringList displayArgs;
    for (const QString &arg : args) {
        if (arg.startsWith("/p:")) {
            displayArgs << "/p:***";
        } else {
            displayArgs << arg;
        }
    }

    return program + " " + displayArgs.join(" ");
}

QString RdpLauncher::xfreerdpPath() const
{
    // 尝试在系统 PATH 中按优先级查找 xfreerdp3 或 xfreerdp
    QString path = QStandardPaths::findExecutable("xfreerdp3");
    if (!path.isEmpty()) return path;
    
    path = QStandardPaths::findExecutable("xfreerdp");
    if (!path.isEmpty()) return path;

    // 尝试常见安装路径
    QStringList candidates = {
        "/usr/bin/xfreerdp3",
        "/usr/bin/xfreerdp",
        "/usr/local/bin/xfreerdp",
#ifdef Q_OS_WIN
        "C:/Program Files/FreeRDP/bin/xfreerdp.exe",
#endif
    };

    for (const QString &candidate : candidates) {
        if (QFile::exists(candidate)) return candidate;
    }

    return QString(); // 未找到
}
