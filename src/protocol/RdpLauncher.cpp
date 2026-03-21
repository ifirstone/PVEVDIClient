#include "RdpLauncher.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QMessageBox>

RdpLauncher::RdpLauncher(QObject *parent)
    : AbstractLauncher(parent)
{
}

bool RdpLauncher::launch(const ConnectionInfo &info)
{
    QString program = xfreerdpPath();
    
    // 如果设置中强制指定了版本，则覆盖自动检测的结果
    if (info.rdpVersion == 2) {
        program = QStandardPaths::findExecutable("xfreerdp");
        if (program.isEmpty()) program = "/usr/bin/xfreerdp";
    } else if (info.rdpVersion == 3) {
        program = QStandardPaths::findExecutable("xfreerdp3");
        if (program.isEmpty()) program = "/usr/bin/xfreerdp3";
    }

    QStringList args = buildArguments(info);

    if (program.isEmpty()) {
        emit connectionError("未找到 xfreerdp 客户端程序，请确认系统已安装 freerdp 客户端组件");
        return false;
    }

    // 如果启用了 USB 驱动器重定向，在启动前先同步一次，然后启动定时轮询
    if (info.enableUSBDrive) {
        syncUsbMounts();
        // 启动后台定时器，每 3 秒刷新一次 USB 设备软链接
        if (!m_usbWatchTimer) {
            m_usbWatchTimer = new QTimer(this);
            connect(m_usbWatchTimer, &QTimer::timeout, this, [](){
                RdpLauncher::syncUsbMounts();
            });
        }
        m_usbWatchTimer->start(3000);
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
    // 【安全核心】严禁映射 /media、/run/media、/mnt 等系统级挂载点！
    // 因为瘦客户端通常以 root 运行，这些路径下会包含系统盘本身，
    // 一旦映射给远端虚拟机，用户可通过 Windows 资源管理器直接删除瘦客户端的系统文件导致崩溃！
    // 正确方案：创建一个专用的安全空白隔离目录，仅将该目录映射给 RDP 会话。
    // 用户插入的 USB 设备需要手动挂载到此目录下才会在远端可见。
    if (info.enableUSBDrive) {
#ifdef Q_OS_WIN
        // Windows 环境下不支持 freerdp /drive 简易映射，忽略
#else
        // 创建专用的安全沙箱共享目录（每次连接时确保存在）
        QString safeShareDir = "/tmp/pxvdi_usb_share";
        QDir shareDir(safeShareDir);
        if (!shareDir.exists()) {
            shareDir.mkpath(".");
        }
        args << "/drive:usb," + safeShareDir;
        qDebug() << "USB 重定向安全沙箱目录:" << safeShareDir
                 << "(请将 USB 设备挂载到此目录下以在远端可见)";
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
    bool isFreeRDP3 = exePath.contains("xfreerdp3") || info.rdpVersion == 3;

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
    args << "/network:" + info.rdpNetwork;   // 用户自定义或自动网络速率
    args << "+auto-reconnect" << "/auto-reconnect-max-retries:3"; // 闪断自动重连
    
    // 关闭终端乱刷日志提升 I/O 性能
    args << "/log-level:OFF";

    // 色深与缩放
    args << QString("/bpp:%1").arg(info.rdpColorDepth);
    if (!info.rdpScale.isEmpty() && info.rdpScale != "100%") {
        QString scaleVal = info.rdpScale;
        scaleVal.remove("%");
        args << "/scale:" + scaleVal;
    }

    // 编解码降阶保护（救命参数：针对 ARM 盒子没硬件 VAAPI 的情况）
    if (info.rdpCodec == "软件解码") {
        // 禁用 H264，强制退回到 RFX 模式，虽然肉眼可见刷新，但 CPU 不会爆表
        if (isFreeRDP3) {
            args << "/gfx:rfx"; 
        } else {
            args << "-gfx-h264";
        }
    } else if (info.rdpCodec == "h264:444") {
        if (isFreeRDP3) args << "/gfx:avc444";
        else args << "/gfx-h264:avc444";
    }

    if (isFreeRDP3) {
        // FreeRDP 3.x 全新语法 (冒号替代旧版的加号或短划线)
        args << "/multitouch" << "/gestures";
        args << "/cache:codec:rfx" << "/gfx:progressive" << "/cache:bitmap";
    } else {
        // FreeRDP 2.x 向下兼容语法
        args << "+multitouch" << "+gestures";
        args << "/codec-cache:rfx" << "+gfx-progressive" << "+bitmap-cache";
    }

    if (info.rdpUsermode) {
        args << "/pcb:usermode"; // 某些场景下的特定用户模式标识
    }

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

// ========== USB 安全沙箱同步器 ==========
// 通过 lsblk 的 HOTPLUG 属性精确识别可移动 USB 设备，
// 只将真正的 USB 设备挂载点软链接到沙箱目录中，
// 彻底隔离系统盘，防止越权访问。
void RdpLauncher::syncUsbMounts()
{
#ifndef Q_OS_WIN
    QString sandboxDir = USB_SANDBOX_DIR;
    QDir sandbox(sandboxDir);
    if (!sandbox.exists()) {
        sandbox.mkpath(".");
    }
    
    // 第一步：清理沙箱目录中所有旧的软链接
    QFileInfoList existing = sandbox.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : existing) {
        if (fi.isSymLink()) {
            QFile::remove(fi.absoluteFilePath());
        }
    }
    
    // 第二步：使用 lsblk 获取所有块设备信息（JSON 格式）
    // 输出字段：NAME, MOUNTPOINT, HOTPLUG, TYPE, LABEL
    // HOTPLUG=1 表示可移动设备（USB 等）
    // TYPE=part 表示分区
    // LABEL 是文件系统卷标（即 U 盘取名）
    QProcess lsblk;
    lsblk.start("lsblk", QStringList() << "-J" << "-o" << "NAME,MOUNTPOINT,HOTPLUG,TYPE,LABEL");
    if (!lsblk.waitForFinished(3000)) {
        qDebug() << "USB 探测器: lsblk 执行超时";
        return;
    }
    
    QByteArray output = lsblk.readAllStandardOutput();
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(output, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        qDebug() << "USB 探测器: lsblk JSON 解析失败:" << parseErr.errorString();
        return;
    }
    
    // 第三步：遍历所有块设备，筛选出 HOTPLUG=1 且有挂载点的分区
    QJsonArray devices = doc.object().value("blockdevices").toArray();
    int linkedCount = 0;
    
    // 递归函数：扫描设备及其子分区
    std::function<void(const QJsonObject&, bool)> scanDevice;
    scanDevice = [&](const QJsonObject &dev, bool parentIsHotplug) {
        bool isHotplug = dev.value("hotplug").toString() == "1" || 
                         dev.value("hotplug").toBool() == true ||
                         parentIsHotplug;
        QString mountpoint = dev.value("mountpoint").toString();
        QString type = dev.value("type").toString();
        QString name = dev.value("name").toString();
        QString label = dev.value("label").toString();
        
        // 只处理可移动设备的分区，且必须有有效的挂载点
        if (isHotplug && type == "part" && !mountpoint.isEmpty() && mountpoint != "/") {
            // 安全检查：确保不是挂载在根目录或关键系统路径上
            if (mountpoint.startsWith("/boot") || 
                mountpoint.startsWith("/usr") || 
                mountpoint.startsWith("/var") ||
                mountpoint.startsWith("/etc") ||
                mountpoint.startsWith("/home") ||
                mountpoint == "/") {
                qDebug() << "USB 探测器: 跳过系统路径" << mountpoint;
                return;
            }
            
            // 优先使用 U 盘卷标名称，如果没有卷标则使用挂载点目录名，最后使用设备名
            QString displayName = label;
            if (displayName.isEmpty()) {
                // 取挂载点的最后一级目录名作为显示名
                displayName = QFileInfo(mountpoint).fileName();
            }
            if (displayName.isEmpty()) {
                displayName = name;
            }
            
            // 创建软链接: /tmp/pxvdi_usb_share/<卷标名> -> <实际挂载点>
            QString linkPath = sandboxDir + "/" + displayName;
            // 如果同名链接已存在（比如两个同名U盘），附加设备名区分
            if (QFile::exists(linkPath)) {
                linkPath = sandboxDir + "/" + displayName + "_" + name;
            }
            if (!QFile::exists(linkPath)) {
                QFile::link(mountpoint, linkPath);
                qDebug() << "USB 探测器: 链接 USB 设备" << displayName << "->" << mountpoint;
                linkedCount++;
            }
        }
        
        // 递归扫描子设备（分区）
        QJsonArray children = dev.value("children").toArray();
        for (const QJsonValue &child : children) {
            scanDevice(child.toObject(), isHotplug);
        }
    };
    
    for (const QJsonValue &dev : devices) {
        scanDevice(dev.toObject(), false);
    }
    
    if (linkedCount > 0) {
        qDebug() << "USB 探测器: 已同步" << linkedCount << "个 USB 设备到沙箱目录";
    }
#endif
}
