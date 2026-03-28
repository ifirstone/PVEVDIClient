#include "EasyTierManager.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace {
QString exeSuffix()
{
#ifdef Q_OS_WIN
    return ".exe";
#else
    return QString();
#endif
}
}

EasyTierManager::EasyTierManager(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_healthTimer(new QTimer(this))
{
    m_healthTimer->setInterval(3000);
    m_healthTimer->setSingleShot(false);
    connect(m_healthTimer, &QTimer::timeout,
            this, &EasyTierManager::onHealthCheckTimeout);

    connect(m_process, &QProcess::started, this, &EasyTierManager::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &EasyTierManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &EasyTierManager::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &EasyTierManager::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &EasyTierManager::onReadyReadStderr);
}

EasyTierManager::~EasyTierManager()
{
    stop();
}

QString EasyTierManager::locateEasyTierExecutable() const
{
    const QString suffix = exeSuffix();

    // 兼容旧配置：如果用户曾指定路径，仍允许优先使用
    if (!m_binaryPath.isEmpty()) {
        QFileInfo binInfo(m_binaryPath);
        if (binInfo.exists() && binInfo.isExecutable()) {
            return binInfo.absoluteFilePath();
        }
    }

    QString appDir = QCoreApplication::applicationDirPath();

    // 优先在发布目录/源码目录下的 resources/package 查找
    QStringList baseDirs;
    baseDirs << appDir
             << QDir(appDir).absoluteFilePath("resources/package")
             << QDir(appDir).absoluteFilePath("../resources/package")
             << QDir(appDir).absoluteFilePath("../../resources/package")
             << QDir(appDir).absoluteFilePath("../../../resources/package")
             << QDir(appDir).absoluteFilePath("package");

    QStringList candidates;
    for (const QString &dirPath : baseDirs) {
        QDir dir(dirPath);
        candidates << dir.absoluteFilePath("easytier-core" + suffix);
        candidates << dir.absoluteFilePath("easytier-core");
    }

    for (const QString &p : candidates) {
        if (QFileInfo::exists(p) && QFileInfo(p).isExecutable()) {
            return p;
        }
    }

    // 最后查 PATH
    QString path = QStandardPaths::findExecutable("easytier-core" + suffix);
    if (!path.isEmpty()) return path;
    path = QStandardPaths::findExecutable("easytier-core");
    if (!path.isEmpty()) return path;

    return QString();
}

QString EasyTierManager::locateEasyTierCliExecutable(const QString &corePath) const
{
    const QString suffix = exeSuffix();
    QFileInfo coreInfo(corePath);
    QStringList candidates;

    if (coreInfo.exists()) {
        QDir dir = coreInfo.absoluteDir();
        candidates << dir.absoluteFilePath("easytier-cli" + suffix)
                   << dir.absoluteFilePath("easytier-cli");
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QStringList baseDirs;
    baseDirs << appDir
             << QDir(appDir).absoluteFilePath("resources/package")
             << QDir(appDir).absoluteFilePath("../resources/package")
             << QDir(appDir).absoluteFilePath("../../resources/package")
             << QDir(appDir).absoluteFilePath("../../../resources/package")
             << QDir(appDir).absoluteFilePath("package");

    for (const QString &dirPath : baseDirs) {
        QDir dir(dirPath);
        candidates << dir.absoluteFilePath("easytier-cli" + suffix)
                   << dir.absoluteFilePath("easytier-cli");
    }

    for (const QString &p : candidates) {
        QFileInfo fi(p);
        if (fi.exists() && fi.isExecutable()) {
            return fi.absoluteFilePath();
        }
    }

    QString path = QStandardPaths::findExecutable("easytier-cli" + suffix);
    if (!path.isEmpty()) return path;
    path = QStandardPaths::findExecutable("easytier-cli");
    if (!path.isEmpty()) return path;

    return QString();
}

void EasyTierManager::setBinaryPath(const QString &path)
{
    m_binaryPath = path;
}

void EasyTierManager::setExtraArgs(const QString &args)
{
    m_extraArgs = args.trimmed();
}

void EasyTierManager::setTargetPeerIp(const QString &ip)
{
    m_targetPeerIp = normalizeIp(ip);
}

QString EasyTierManager::normalizeIp(const QString &ip) const
{
    QString v = ip.trimmed();
    int slash = v.indexOf('/');
    if (slash > 0) {
        v = v.left(slash);
    }
    return v;
}

QString EasyTierManager::runDiagnosticChecks()
{
    emit logMessage("=== 开始逐步诊断检查 ===");
    
    // ===== 第1步：查找 easytier-core =====
    emit logMessage("【检查 1/5】查找 easytier-core 二进制文件...");
    QString bin = locateEasyTierExecutable();
    if (bin.isEmpty()) {
        QString err = "❌ 未找到 easytier-core 可执行文件\n\n"
                     "已搜索以下位置：\n"
                     "• 应用程序目录\n"
                     "• resources/package\n"
                     "• 系统 PATH\n\n"
                     "解决方案：编译时检查是否正确复制了 bin/package/ 目录";
        emit logMessage(err);
        return err;
    }
    emit logMessage(QString("✓ 找到：%1").arg(bin));
    
    // ===== 第2步：检查文件权限 =====
    emit logMessage("【检查 2/5】检查 easytier-core 文件权限...");
    QFileInfo fi(bin);
    if (!fi.isExecutable()) {
        QString err = QString("❌ 文件没有执行权限：%1\n\n"
                             "解决方案：运行以下命令\n"
                             "chmod +x %1").arg(bin);
        emit logMessage(err);
        return err;
    }
    emit logMessage("✓ 执行权限正常");
    
    // ===== 第3步：检查文件完整性 =====
    emit logMessage("【检查 3/5】检查 easytier-core 文件完整性...");
    if (fi.size() == 0) {
        QString err = QString("❌ 二进制文件损坏（大小为 0 字节）：%1\n\n"
                             "解决方案：\n"
                             "1. 重新编译项目\n"
                             "2. 或手动从源码编译 easytier-core\n"
                             "3. 将编译好的二进制放到 resources/package/ 目录").arg(bin);
        emit logMessage(err);
        return err;
    }
    emit logMessage(QString("✓ 文件大小：%1 字节").arg(fi.size()));
    
    // ===== 第4步：查找 easytier-cli =====
    emit logMessage("【检查 4/5】查找 easytier-cli 二进制文件...");
    m_cliPath = locateEasyTierCliExecutable(bin);
    if (m_cliPath.isEmpty()) {
        QString err = "❌ 未找到 easytier-cli 可执行文件\n\n"
                     "这会导致对端检查功能无法工作\n\n"
                     "解决方案：确保 easytier-cli 与 easytier-core 在同一目录下";
        emit logMessage(err);
        return err;
    }
    emit logMessage(QString("✓ 找到：%1").arg(m_cliPath));
    
    // ===== 第5步：检查 easytier-cli 权限 =====
    emit logMessage("【检查 5/5】检查 easytier-cli 文件权限...");
    QFileInfo clipath(m_cliPath);
    if (!clipath.isExecutable()) {
        QString err = QString("❌ easytier-cli 没有执行权限：%1\n\n"
                             "解决方案：运行以下命令\n"
                             "chmod +x %1").arg(m_cliPath);
        emit logMessage(err);
        return err;
    }
    emit logMessage("✓ 执行权限正常");
    
    // ===== 诊断通过 =====
    emit logMessage("✅ 所有前置检查通过，现在可以启动 easytier-core");
    return QString();  // 空字符串表示检查通过
}

bool EasyTierManager::start(const QString &networkName,
                            const QString &networkSecret,
                            const QString &bootstrapUrl)
{
    if (isRunning()) {
        qWarning() << "EasyTier already running";
        return true;
    }

    QString bin = locateEasyTierExecutable();
    if (bin.isEmpty()) {
        updateState(EasyTierState::Error);
        QString errMsg = "未找到 easytier-core 可执行文件（已尝试 resources/package 与系统 PATH）";
        emit logMessage(errMsg);
        qCritical().noquote() << "[EasyTier]" << errMsg;
        return false;
    }
    
    // 前置检查：权限 + 文件完整性
    QFileInfo fi(bin);
    if (!fi.isExecutable()) {
        updateState(EasyTierState::Error);
        QString errMsg = QString("【权限问题】%1 没有执行权限\n请运行: chmod +x %1").arg(bin);
        emit logMessage(errMsg);
        qCritical().noquote() << "[EasyTier]" << errMsg;
        return false;
    }
    
    if (fi.size() == 0) {
        updateState(EasyTierState::Error);
        QString errMsg = QString("【二进制损坏】%1 文件大小为 0 字节（文件损坏）").arg(bin);
        emit logMessage(errMsg);
        qCritical().noquote() << "[EasyTier]" << errMsg;
        return false;
    }

    m_cliPath = locateEasyTierCliExecutable(bin);

    QStringList args;
    args << "-d";
    args << "--network-name" << networkName;
    args << "--network-secret" << networkSecret;
    if (!bootstrapUrl.isEmpty()) {
        args << "-p" << bootstrapUrl;
    }
    if (!m_extraArgs.isEmpty()) {
        args << m_extraArgs.split(' ', Qt::SkipEmptyParts);
    }

    emit logMessage(QString("⚙️ 启动进程：%1 %2").arg(bin, args.join(" ")));
    m_process->start(bin, args);
    
    // ===== 启动验证阶段：等待进程的初始输出 =====
    // easytier-core 如果有配置问题（网络、权限等），会立即输出错误信息并退出
    // 我们通过等待 1.5 秒来捕获这些初始错误
    emit logMessage("🔍 正在等待进程初始化（检查是否立即出错）...");
    QEventLoop waitLoop;
    QTimer startupTimer;
    startupTimer.setSingleShot(true);
    startupTimer.setInterval(1500);  // 等待 1.5 秒
    
    connect(&startupTimer, &QTimer::timeout, &waitLoop, &QEventLoop::quit);
    startupTimer.start();
    waitLoop.exec();
    
    if (!isRunning()) {
        // 进程在启动后立即退出，说明有严重错误（网络、权限、配置等）
        int exitCode = m_process->exitCode();
        QByteArray stderrBytes = m_process->readAllStandardError();
        QByteArray stdoutBytes = m_process->readAllStandardOutput();
        QString stderrText = QString::fromUtf8(stderrBytes);
        QString stdoutText = QString::fromUtf8(stdoutBytes);
        
        QString fullOutput = stderrText + stdoutText;
        
        if (!fullOutput.isEmpty()) {
            emit logMessage(QString("❌ 进程立即退出（exit code: %1）\n\n【进程输出】\n%2").arg(exitCode).arg(fullOutput));
        } else {
            emit logMessage(QString("❌ 进程启动失败，立即退出（exit code: %1）\n"
                                  "可能原因：\n"
                                  "• 网络配置错误（IP 冲突、网络不通）\n"
                                  "• 权限问题（需要 root 权限）\n"
                                  "• 系统依赖缺失\n"
                                  "• Bootstrap 服务器无法访问").arg(exitCode));
        }
        
        updateState(EasyTierState::Error);
        return false;
    }

    m_healthCheckCount = 0;  // 重置计数器，启动30秒热身期
    updateState(EasyTierState::Starting);
    m_healthTimer->start();

    emit logMessage(QString("✅ easytier-core 进程已启动（PID: %1）").arg(m_process->processId()));
    if (!m_cliPath.isEmpty()) {
        emit logMessage(QString("📊 使用 easytier-cli：%1").arg(m_cliPath));
    } else {
        emit logMessage("⚠️ 未找到 easytier-cli，将仅依赖进程输出推断状态");
    }
    return true;
}

void EasyTierManager::stop()
{
    if (!isRunning()) return;

    m_healthTimer->stop();
    m_process->terminate();
    if (!m_process->waitForFinished(2000)) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }

    updateState(EasyTierState::Stopped);
    emit logMessage("easytier-core 已停止");
}

bool EasyTierManager::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

EasyTierState EasyTierManager::state() const
{
    return m_state;
}

QString EasyTierManager::localIp() const
{
    return m_localIp;
}

void EasyTierManager::onProcessStarted()
{
    emit logMessage("easytier-core 进程已启动");
    updateState(EasyTierState::Starting);
}

void EasyTierManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    m_healthTimer->stop();
    QString info = QString("easytier-core 结束，exit=%1").arg(exitCode);
    emit logMessage(info);
    updateState(EasyTierState::Stopped);
}

void EasyTierManager::onProcessError(QProcess::ProcessError error)
{
    m_healthTimer->stop();
    
    // 将错误代码转换为可读的描述
    QString errorDesc;
    switch (error) {
    case QProcess::FailedToStart:
        errorDesc = "无法启动进程";
        break;
    case QProcess::Crashed:
        errorDesc = "进程崩溃";
        break;
    case QProcess::Timedout:
        errorDesc = "进程超时";
        break;
    case QProcess::WriteError:
        errorDesc = "写入错误";
        break;
    case QProcess::ReadError:
        errorDesc = "读取错误";
        break;
    default:
        errorDesc = "未知错误";
        break;
    }
    
    // 获取系统错误信息
    QString sysErr = m_process->errorString();
    
    // 执行诊断检查
    QString bin = locateEasyTierExecutable();
    QString diagnostics;
    
    if (bin.isEmpty()) {
        diagnostics = "【诊断】未找到 easytier-core 可执行文件";
    } else {
        QFileInfo fi(bin);
        if (!fi.exists()) {
            diagnostics = QString("【诊断】二进制文件不存在：%1").arg(bin);
        } else if (!fi.isExecutable()) {
            diagnostics = QString("【诊断】权限问题：%1 没有执行权限（请运行 chmod +x %1）").arg(bin);
        } else if (!fi.isReadable()) {
            diagnostics = QString("【诊断】无法读取：%1 没有读权限").arg(bin);
        } else if (fi.size() == 0) {
            diagnostics = QString("【诊断】二进制文件损坏：%1 大小为 0").arg(bin);
        } else {
            diagnostics = QString("【诊断】二进制文件路径：%1").arg(bin);
        }
    }
    
    QString fullErr = QString("easytier-core 启动失败 [%1]\n系统错误：%2\n%3")
                        .arg(errorDesc, sysErr, diagnostics);
    
    emit logMessage(fullErr);
    qCritical().noquote() << "[EasyTier]" << fullErr;
    
    updateState(EasyTierState::Error);
}

void EasyTierManager::onReadyReadStdout()
{
    QByteArray outputBytes = m_process->readAllStandardOutput();
    QString text = QString::fromUtf8(outputBytes);
    QString trimmed = text.trimmed();
    if (!trimmed.isEmpty()) {
        // 标准输出：通常是正常的状态信息
        emit logMessage(QString("[stdout] %1").arg(trimmed));
    }
    for (const QString &line : text.split("\n", QString::SkipEmptyParts)) {
        parseStatusFromLine(line);
    }
}

void EasyTierManager::onReadyReadStderr()
{
    QByteArray errorBytes = m_process->readAllStandardError();
    QString text = QString::fromUtf8(errorBytes);
    QString trimmed = text.trimmed();
    if (!trimmed.isEmpty()) {
        // 错误输出：标记为 stderr 并用警告色显示
        // 分析常见错误
        QString diagnosticMsg;
        if (trimmed.contains("Permission denied", Qt::CaseInsensitive) || 
            trimmed.contains("权限", Qt::CaseInsensitive)) {
            diagnosticMsg = "【网络权限错误】easytier-core 尝试绑定端口时被拒绝（可能需要 root 权限或端口被占用）";
        } else if (trimmed.contains("Connection refused", Qt::CaseInsensitive) || 
                   trimmed.contains("连接被拒", Qt::CaseInsensitive)) {
            diagnosticMsg = "【网络连接错误】无法连接到 Bootstrap 服务器（网络不通或服务器地址错误）";
        } else if (trimmed.contains("Network is unreachable", Qt::CaseInsensitive) || 
                   trimmed.contains("网络不可达", Qt::CaseInsensitive)) {
            diagnosticMsg = "【网络不可达】系统无法到达指定的网络（检查网络配置或 Bootstrap 地址）";
        } else if (trimmed.contains("Invalid argument", Qt::CaseInsensitive) || 
                   trimmed.contains("参数", Qt::CaseInsensitive)) {
            diagnosticMsg = "【配置错误】启动参数无效（检查网络名称、密钥或 Bootstrap 地址）";
        } else if (trimmed.contains("Address already in use", Qt::CaseInsensitive) || 
                   trimmed.contains("地址已被使用", Qt::CaseInsensitive)) {
            diagnosticMsg = "【端口占用】EasyTier 使用的端口已被其他程序占用";
        } else {
            diagnosticMsg = QString("❌ [stderr] %1").arg(trimmed);
        }
        emit logMessage(diagnosticMsg);
    }
    for (const QString &line : text.split("\n", QString::SkipEmptyParts)) {
        parseStatusFromLine(line);
    }
}

void EasyTierManager::onHealthCheckTimeout()
{
    m_healthCheckCount++;
    qInfo().noquote() << QString("[EasyTier] 健康检查 #%1 (热身期: %2/10)").arg(m_healthCheckCount).arg(m_healthCheckCount <= 10 ? "是" : "否");

    if (!isRunning()) {
        qWarning() << "[EasyTier] 进程不运行，状态改为 Disconnected";
        emit logMessage("【进程状态】easytier-core 进程已停止运行");
        updateState(EasyTierState::Disconnected);
        return;
    }

    // 强制检测状态：调用 easytier-cli peer
    if (m_cliPath.isEmpty()) {
        QString diagMsg = "【诊断】easytier-cli 路径为空，无法执行对端检查";
        qWarning().noquote() << "[EasyTier]" << diagMsg;
        emit logMessage(diagMsg);
        return;
    }
    
    // 检查 easytier-cli 是否存在且可执行
    QFileInfo clipath(m_cliPath);
    if (!clipath.exists()) {
        QString diagMsg = QString("【诊断】easytier-cli 不存在：%1").arg(m_cliPath);
        qWarning().noquote() << "[EasyTier]" << diagMsg;
        emit logMessage(diagMsg);
        return;
    }
    
    if (!clipath.isExecutable()) {
        QString diagMsg = QString("【权限问题】easytier-cli 无执行权限：%1\n请运行: chmod +x %1").arg(m_cliPath);
        qWarning().noquote() << "[EasyTier]" << diagMsg;
        emit logMessage(diagMsg);
        return;
    }

    QProcess statusProc;
    statusProc.start(m_cliPath, {"peer"});
    if (!statusProc.waitForFinished(3000)) {
        QString diagMsg = QString("【执行超时】easytier-cli peer 命令超时（3秒），检查 #%1\n可能原因："
                                    "1. easytier-core 尚未就绪或未监听本地 socket\n"
                                    "2. 网络问题导致命令阻塞").arg(m_healthCheckCount);
        qWarning().noquote() << "[EasyTier]" << diagMsg;
        emit logMessage(diagMsg);
        emit linkModeUpdated("timeout", false);
        return;
    }
    
    QString out = QString::fromUtf8(statusProc.readAllStandardOutput()).trimmed();
    QString err = QString::fromUtf8(statusProc.readAllStandardError()).trimmed();
    
    if (!err.isEmpty()) {
        QString diagMsg = QString("【CLI 错误】easytier-cli peer 返回错误：%1").arg(err);
        qWarning().noquote() << "[EasyTier]" << diagMsg;
        emit logMessage(diagMsg);
    }
    
    if (!out.isEmpty()) {
        emit logMessage(QString("easytier-cli peer: %1").arg(out));
    }

    bool anyPeerFound = false;
    bool targetFound = false;
    QString detectedMode;
    QString detectedIp;
    const QString targetIp = normalizeIp(m_targetPeerIp);

    const QStringList lines = out.split('\n', QString::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (!line.startsWith('|')) {
            continue;
        }
        if (line.contains("ipv4") || line.contains("---")) {
            continue;
        }

        QStringList cols = line.split('|', QString::SkipEmptyParts);
        for (QString &c : cols) {
            c = c.trimmed();
        }
        if (cols.size() < 3) {
            continue;
        }

        const QString peerIp = normalizeIp(cols.at(0));
        const QString costMode = cols.at(2).toLower();

        if (peerIp.isEmpty()) {
            continue;
        }

        anyPeerFound = true;

        if (targetIp.isEmpty()) {
            detectedIp = peerIp;
            detectedMode = costMode;
            targetFound = true;
            break;
        }

        if (peerIp == targetIp) {
            detectedIp = peerIp;
            detectedMode = costMode;
            targetFound = true;
            break;
        }
    }

    if (targetFound) {
        qInfo().noquote() << QString("[EasyTier] 在检查 #%1 发现目标P2P地址，模式: %2").arg(m_healthCheckCount).arg(detectedMode);
        updateState(EasyTierState::Connected);
        m_linkMode = detectedMode;
        if (!detectedIp.isEmpty() && detectedIp != m_localIp) {
            m_localIp = detectedIp;
            emit localIpChanged(m_localIp);
        }
        emit linkModeUpdated(m_linkMode, true);
    } else {
        // ===== 关键修改：热身期逻辑 =====
        // 前30秒（10次检查 × 3秒）不立即报错，给NAT打洞充足时间
        if (m_healthCheckCount <= 10) {
            // 在热身期内，即使找不到peer也保持Starting状态
            if (!anyPeerFound) {
                qDebug().noquote() << QString("[EasyTier] 热身期检查 #%1：未找到任何peer，保持Starting状态").arg(m_healthCheckCount);
                updateState(EasyTierState::Starting);
            } else {
                // 找到其他peer但没找到目标，保持Disconnected
                qDebug().noquote() << QString("[EasyTier] 热身期检查 #%1：找到其他peer但非目标IP，保持Disconnected").arg(m_healthCheckCount);
                updateState(EasyTierState::Disconnected);
            }
        } else {
            // 热身期结束后，如果还是找不到，才报告Disconnected或Error
            if (anyPeerFound) {
                QString diagMsg = QString("【诊断】检查 #%1：发现网络中有对端存在，但未能识别为目标P2P地址\n"
                                            "可能原因：1. 输入的 P2P 地址错误  2. 对端尚未上线  3. 网络隔离").arg(m_healthCheckCount);
                qWarning().noquote() << "[EasyTier]" << diagMsg;
                emit logMessage(diagMsg);
            } else {
                QString diagMsg = QString("【诊断】检查 #%1（热身期已过）：仍未检测到任何对端\n"
                                            "可能原因：1. easytier-core 未正确启动  2. 网络配置错误  3. bootstrapUrl 无效\n"
                                            "建议：检查 easytier-core 进程是否运行，查看系统日志").arg(m_healthCheckCount);
                qWarning().noquote() << "[EasyTier]" << diagMsg;
                emit logMessage(diagMsg);
            }
            updateState(anyPeerFound ? EasyTierState::Disconnected : EasyTierState::Error);
        }
        emit linkModeUpdated(anyPeerFound ? "relay-or-other" : "unavailable", false);
    }
}

void EasyTierManager::updateState(EasyTierState nextState)
{
    if (m_state == nextState) return;
    m_state = nextState;
    emit stateChanged(m_state);
}

void EasyTierManager::parseStatusFromLine(const QString &line)
{
    QString low = line.toLower();

    if (low.contains("joined network") || low.contains("connected")) {
        if (m_state != EasyTierState::Connected) {
            updateState(EasyTierState::Connected);
        }
    }

    // IP 解析示例： node ip: 10.15.198.3
    QRegExp ipRegex("([0-9]{1,3}(?:\\.[0-9]{1,3}){3})");
    if (ipRegex.indexIn(line) != -1) {
        QString ip = ipRegex.cap(1);
        if (ip != m_localIp) {
            m_localIp = ip;
            emit localIpChanged(ip);
        }
    }

    if (low.contains("error") || low.contains("failed")) {
        updateState(EasyTierState::Error);
    }
}
