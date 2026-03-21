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
    
    // 为防止 xfreerdp 因为没拿到 /p 参数而在看不见的后台终端中挂起死锁等用户输入，
    // 我们强制下发 /p: (即使为空)，以此支持 Windows 的免密直接登录
    args << QString("/p:%1").arg(info.password);
    
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
        // Windows 环境下不支持 freerdp /drive 简易映射，忽略
#else
        // 缩窄权限范围限制在当前用户专有的 U 盘挂载点下，防范越权映射整个 /media（可能会导致越权访问并删除本机根目录文件）
        QString userStr = qgetenv("USER");
        if (userStr.isEmpty()) userStr = qgetenv("USERNAME");
        
        QDir userRunMedia("/run/media/" + userStr);
        QDir userMedia("/media/" + userStr);
        
        if (!userStr.isEmpty() && userRunMedia.exists()) {
            args << "/drive:usb," + userRunMedia.absolutePath();
        } else if (!userStr.isEmpty() && userMedia.exists()) {
            args << "/drive:usb," + userMedia.absolutePath();
        } else {
            // 退火防御策略：如果找不到专属挂载点，退而挂载当前登录用户的安全下载包路径，杜绝挂载全局 /tmp 或 /media
            QString safeDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
            if (safeDir.isEmpty()) safeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
            args << "/drive:usb," + safeDir;
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

    // ================= 核心渲染与画质控制（源自商用版黄金传参）=================
    // 动态分辨率支持（拖拽窗口大小或系统切屏时自适应）
    args << "/dynamic-resolution";
    // 强制使用硬件 GDI 渲染（大幅降低 ARM CPU 占用，避免全屏撕裂）
    args << "/gdi:hw";
    
    // 网络与连接容灾
    args << "/network:auto";                 // 自动适应网络带宽调整帧率
    args << "+auto-reconnect" << "/auto-reconnect-max-retries:3"; // 闪断自动重连
    
    // 关闭终端乱刷日志提升 I/O 性能
    args << "/log-level:OFF";

    if (isFreeRDP3) {
        // FreeRDP 3.x 全新语法 (冒号替代旧版的加号或短划线)
        args << "/multitouch" << "/gestures";
        args << "/cache:codec:rfx" << "/gfx:progressive" << "/cache:bitmap";
    } else {
        // FreeRDP 2.x 向下兼容语法
        args << "+multitouch" << "+gestures";
        args << "/codec-cache:rfx" << "+gfx-progressive" << "+bitmap-cache";
    }

    // // 在 ARM 等低配盒子上，强制 32 位色宽和 v2 级压缩会造成 CPU 软解严重卡顿 (一帧一帧显现)。
    // // 竞品使用了 /bpp:32 /compression-level:2 配合 /gfx:RFX 硬件解码。
    // // 如果我们的盒子不支持 RFX 硬解，使用这些参数会极其卡顿。为了安全起见，我们仅使用基础的 GDI:hw 和 cache，
    // // 或者我们可以把它们加上，让用户自己在连接设置里选择是否开启（当前留空使用智能默认值）。
    // // args << "/bpp:32" << "/compression-level:2";
    if (!info.extraArgs.isEmpty()) {
        args << info.extraArgs.split(" ", Qt::SkipEmptyParts);
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
