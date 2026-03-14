#include "ProcessRunner.h"
#include <QDebug>

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started,
            this, &ProcessRunner::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &ProcessRunner::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &ProcessRunner::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &ProcessRunner::onReadyReadStandardError);
}

ProcessRunner::~ProcessRunner()
{
    stop();
}

bool ProcessRunner::start(const QString &program, const QStringList &arguments)
{
    if (isRunning()) {
        qWarning() << "进程已在运行中，请先停止当前进程";
        return false;
    }

    m_program = program;
    m_arguments = arguments;
    m_stdOut.clear();
    m_stdErr.clear();

    qDebug() << "启动进程:" << commandLine();
    m_process->start(program, arguments);

    return true;
}

void ProcessRunner::stop()
{
    if (!isRunning()) return;

    qDebug() << "正在停止进程...";
    m_process->terminate();

    // 等待 3 秒，如果还没退出则强制杀死
    if (!m_process->waitForFinished(3000)) {
        qWarning() << "进程未能正常退出，强制终止";
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool ProcessRunner::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

int ProcessRunner::exitCode() const
{
    return m_process->exitCode();
}

QString ProcessRunner::standardOutput() const
{
    return m_stdOut;
}

QString ProcessRunner::standardError() const
{
    return m_stdErr;
}

QString ProcessRunner::commandLine() const
{
    QStringList parts;
    parts << m_program;
    parts << m_arguments;
    return parts.join(" ");
}

void ProcessRunner::onProcessStarted()
{
    qDebug() << "进程已启动:" << m_program;
    emit started();
}

void ProcessRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString status = (exitStatus == QProcess::NormalExit) ? "正常退出" : "异常退出";
    qDebug() << "进程已结束:" << status << "退出码:" << exitCode;
    emit finished(exitCode, exitStatus);
}

void ProcessRunner::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "进程启动失败，请检查程序路径是否正确";
        break;
    case QProcess::Crashed:
        errorMsg = "进程异常崩溃";
        break;
    case QProcess::Timedout:
        errorMsg = "进程操作超时";
        break;
    case QProcess::WriteError:
        errorMsg = "向进程写入数据时出错";
        break;
    case QProcess::ReadError:
        errorMsg = "从进程读取数据时出错";
        break;
    default:
        errorMsg = "未知进程错误";
        break;
    }
    qWarning() << "进程错误:" << errorMsg;
    emit errorOccurred(errorMsg);
}

void ProcessRunner::onReadyReadStandardOutput()
{
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    m_stdOut += output;
    emit outputReady(output);
}

void ProcessRunner::onReadyReadStandardError()
{
    QString output = QString::fromUtf8(m_process->readAllStandardError());
    m_stdErr += output;
}
