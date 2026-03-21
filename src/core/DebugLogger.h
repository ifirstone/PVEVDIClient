#ifndef DEBUGLOGGER_H
#define DEBUGLOGGER_H

#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>

// 全局内建的调试日志捕获穿透窗
class DebugLogger : public QDialog
{
    Q_OBJECT
public:
    static DebugLogger& instance();

public slots:
    void appendLog(QtMsgType type, const QString &msg);

private:
    explicit DebugLogger(QWidget *parent = nullptr);
    QTextEdit *m_textEdit;
};

// 拦截全局 qWarning, qDebug 输出
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

#endif // DEBUGLOGGER_H
