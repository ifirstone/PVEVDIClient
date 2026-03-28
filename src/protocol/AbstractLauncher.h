#ifndef ABSTRACTLAUNCHER_H
#define ABSTRACTLAUNCHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "core/ConnectionInfo.h"
#include "core/ProcessRunner.h"

// 协议启动器抽象基类
class AbstractLauncher : public QObject
{
    Q_OBJECT

public:
    explicit AbstractLauncher(QObject *parent = nullptr)
        : QObject(parent)
        , m_runner(new ProcessRunner(this))
    {
        connect(m_runner, &ProcessRunner::started,
                this, &AbstractLauncher::connected);
        connect(m_runner, &ProcessRunner::finished,
                this, &AbstractLauncher::onProcessFinished);
        connect(m_runner, &ProcessRunner::errorOccurred,
                this, &AbstractLauncher::connectionError);
    }

    virtual ~AbstractLauncher() = default;

    // 启动连接
    virtual bool launch(const ConnectionInfo &info) = 0;

    // 断开连接
    virtual void disconnect() {
        if (m_runner) {
            m_runner->stop();
        }
    }

    // 是否已连接
    bool isConnected() const {
        return m_runner && m_runner->isRunning();
    }

    // 获取构建的命令行（用于调试/展示）
    virtual QString buildCommandLine(const ConnectionInfo &info) const = 0;

    // 获取协议名称
    virtual QString protocolName() const = 0;

signals:
    void connected();
    void disconnected(int exitCode);
    void connectionError(const QString &errorMessage);

protected:
    ProcessRunner *m_runner;

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode != 0 || exitStatus == QProcess::CrashExit) {
            QString errorMsg = m_runner->standardError().trimmed();
            if (errorMsg.isEmpty()) {
                errorMsg = QString("连接进程意外结束，退出码: %1").arg(exitCode);
            } else {
                // 如果有多行错误，只取最后几行关键错误
                QStringList lines = errorMsg.split('\n', Qt::SkipEmptyParts);
                if (lines.size() > 3) {
                    errorMsg = lines.mid(lines.size() - 3).join('\n');
                }
            }
            emit connectionError(errorMsg);
        }
        emit disconnected(exitCode);
    }
};

#endif // ABSTRACTLAUNCHER_H
