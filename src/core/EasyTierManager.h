#ifndef EASYTIERMANAGER_H
#define EASYTIERMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

enum class EasyTierState {
    Stopped,
    Starting,
    Connected,
    Disconnected,
    Error
};

class EasyTierManager : public QObject
{
    Q_OBJECT

public:
    explicit EasyTierManager(QObject *parent = nullptr);
    ~EasyTierManager();

    bool start(const QString &networkName,
               const QString &networkSecret,
               const QString &bootstrapUrl);
    void stop();
    bool isRunning() const;
    EasyTierState state() const;
    QString localIp() const;

    void setBinaryPath(const QString &path);
    void setExtraArgs(const QString &args);
    void setTargetPeerIp(const QString &ip);
    
    // 逐步诊断检查：返回为空表示所有检查通过，不为空表示错误信息
    QString runDiagnosticChecks();

signals:
    void stateChanged(EasyTierState newState);
    void localIpChanged(const QString &ip);
    void logMessage(const QString &msg);
    void linkModeUpdated(const QString &mode, bool targetPeerFound);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onHealthCheckTimeout();

private:
    void updateState(EasyTierState nextState);
    void parseStatusFromLine(const QString &line);
    QString locateEasyTierExecutable() const;
    QString locateEasyTierCliExecutable(const QString &corePath) const;
    QString normalizeIp(const QString &ip) const;

private:
    QProcess *m_process = nullptr;
    QTimer *m_healthTimer = nullptr;
    EasyTierState m_state = EasyTierState::Stopped;
    QString m_localIp;
    QString m_binaryPath;
    QString m_extraArgs;
    QString m_cliPath;
    QString m_targetPeerIp;
    QString m_linkMode;
    int m_healthCheckCount = 0;  // 热身期计数：前10次检查（30秒）不报错，给NAT打洞充足时间
};

#endif // EASYTIERMANAGER_H