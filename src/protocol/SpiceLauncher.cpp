#include "SpiceLauncher.h"
#include <QDebug>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>

SpiceLauncher::SpiceLauncher(QObject *parent)
    : AbstractLauncher(parent)
{
}

bool SpiceLauncher::launch(const ConnectionInfo &info)
{
    QString program = remoteViewerPath();
    QStringList args = buildArguments(info);

    if (program.isEmpty()) {
#ifdef Q_OS_WIN
        QString cmd = buildCommandLine(info);
        qDebug() << "[模拟模式] SPICE 命令:" << cmd;
        emit connectionError(
            QString("Windows 开发模式 - 生成的命令:\n\n%1\n\n"
                    "在 Linux 环境中会实际执行此命令。").arg(cmd));
        return false;
#else
        emit connectionError("未找到 remote-viewer，请确认已安装 virt-viewer 包");
        return false;
#endif
    }

    qDebug() << "启动 SPICE 连接:" << program << args;
    return m_runner->start(program, args);
}

bool SpiceLauncher::launchWithVvFile(const QString &vvContent, bool fullscreen)
{
    QString program = remoteViewerPath();
    if (program.isEmpty()) {
#ifdef Q_OS_WIN
        emit connectionError("未找到 remote-viewer.exe，请确认已安装 VirtViewer，或将其添加到系统 PATH 中");
#else
        emit connectionError("未找到 remote-viewer，请确认已安装 virt-viewer 包");
#endif
        return false;
    }

    // 写入临时 .vv 文件
    QString vvPath = writeVvFile(vvContent);
    if (vvPath.isEmpty()) {
        emit connectionError("无法创建临时 SPICE 配置文件");
        return false;
    }

    QStringList args;

    // 在 Windows 下，remote-viewer 可能会对绝对路径解析出错，
    // 转为原生的 Windows 路径格式，或者前面加上 file:///
#ifdef Q_OS_WIN
    args << QDir::toNativeSeparators(vvPath);
#else
    args << vvPath;
#endif

    if (fullscreen) {
        args << "--full-screen";
    }

    qDebug() << "启动 SPICE 连接 (vv 文件):" << program << args;
    return m_runner->start(program, args);
}

QStringList SpiceLauncher::buildArguments(const ConnectionInfo &info) const
{
    QStringList args;

    // 直接连接方式：spice://host:port
    QString host = info.rdpHost.isEmpty() ? info.serverHost : info.rdpHost;
    int port = (info.rdpPort == 3389) ? 5900 : info.rdpPort; // SPICE 默认端口 5900

    args << QString("spice://%1:%2").arg(host).arg(port);

    // 全屏
    if (info.resolution == "fullscreen") {
        args << "--full-screen";
    }

    return args;
}

QString SpiceLauncher::buildCommandLine(const ConnectionInfo &info) const
{
    QString program = remoteViewerPath();
    if (program.isEmpty()) program = "remote-viewer";

    QStringList args = buildArguments(info);
    return program + " " + args.join(" ");
}

QString SpiceLauncher::remoteViewerPath() const
{
    // 尝试在系统 PATH 中查找
    QString path = QStandardPaths::findExecutable("remote-viewer");
    if (!path.isEmpty()) return path;

    // 尝试常见安装路径
    QStringList candidates = {
        "/usr/bin/remote-viewer",
        "/usr/local/bin/remote-viewer",
#ifdef Q_OS_WIN
        "C:/Program Files/VirtViewer/bin/remote-viewer.exe",
        "C:/Program Files/VirtViewer v11.0-256/bin/remote-viewer.exe",
        "D:/Program Files/VirtViewer/bin/remote-viewer.exe",
#endif
    };

    for (const QString &candidate : candidates) {
        if (QFile::exists(candidate)) return candidate;
    }

    return QString();
}

QString SpiceLauncher::writeVvFile(const QString &content) const
{
    // 在临时目录创建 .vv 文件
    QString tempDir = QDir::tempPath();
    QString filePath = QDir(tempDir).filePath("pve-spice.vv");

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法创建临时 .vv 文件:" << filePath;
        return QString();
    }

    file.write(content.toUtf8());
    file.close();

    qDebug() << "已写入 SPICE 配置文件:" << filePath;
    return filePath;
}
