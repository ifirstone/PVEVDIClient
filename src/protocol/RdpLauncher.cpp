#include "RdpLauncher.h"
#include <QDebug>
#include <QFile>
#include <QDir>
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
    }
    
    if (!info.password.isEmpty()) {
        args << QString("/p:%1").arg(info.password);
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
        QDir runMedia("/run/media");
        QDir media("/media");
        QDir mnt("/mnt");
        if (runMedia.exists()) {
            args << "/drive:usb,/run/media";
        } else if (media.exists()) {
            args << "/drive:usb,/media";
        } else if (mnt.exists()) {
            args << "/drive:usb,/mnt";
        } else {
            // 如果系统过于精简连上述挂载点都没有，必须指定一个存在的文件夹，否则 freerdp 会因为目录不存在报错终止连接。
            args << "/drive:usb,/tmp";
        }
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

    // 判断 FreeRDP 大版本分支以使用不同兼容参数
    QString exePath = xfreerdpPath();
    bool isFreeRDP3 = exePath.contains("xfreerdp3");

    if (isFreeRDP3) {
        // FreeRDP 3.x 专属安全与证书参数
        args << "/cert:tofu" << "/cert:ignore";
        args << "/sec:tls"; // 强制用 TLS 绕过最新版的验证检测
    } else {
        // FreeRDP 2.x 向下兼容参数
        args << "/cert-ignore"; // 2.x 旧版经典忽略证书格式
        args << "-sec-nla";     // 2.x 中用显式关闭 NLA 以便顺利弹出 Windows 登录窗口
        // 关键修复：Debian 12 使用了严格的 OpenSSL 3.0，导致 Windows RDP 默认自签名证书或旧加密套件被强行阻断 (Error 104 Connection reset)
        args << "/tls-seclevel:0";
    }

    // 在 ARM 等低配盒子上，强制 32 位色宽和 v2 级压缩会造成 CPU 软解严重卡顿 (一帧一帧显现)。
    // 这里去除硬编码，直接使用 FreeRDP 默认的最底层自适应 GDI 渲染，恢复极致丝滑。
    // args << "/compression-level:2" << "/bpp:32" << "+fonts";

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
