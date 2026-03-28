#ifndef PROCESSRUNNER_H
#define PROCESSRUNNER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

// 子进程管理器：统一管理 xfreerdp / remote-viewer 等外部进程的启动、监控和停止
class ProcessRunner : public QObject
{
    Q_OBJECT

public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner();

    // 启动外部进程
    bool start(const QString &program, const QStringList &arguments);

    // 停止进程
    void stop();

    // 进程是否正在运行
    bool isRunning() const;

    // 获取进程退出码
    int exitCode() const;

    // 获取标准输出/错误输出
    QString standardOutput() const;
    QString standardError() const;

    // 获取实际执行的命令行（用于调试）
    QString commandLine() const;

signals:
    // 进程启动成功
    void started();

    // 进程结束
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

    // 进程出错
    void errorOccurred(const QString &errorMessage);

    // 标准输出有新数据
    void outputReady(const QString &output);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    QProcess *m_process = nullptr;
    QString m_program;
    QStringList m_arguments;
    QString m_stdOut;
    QString m_stdErr;
};

#endif // PROCESSRUNNER_H
