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
    void appendLog(int type, const QString &msg);
    void showLogger();

private:
    explicit DebugLogger(QWidget *parent = nullptr);
    void trimLogIfNeeded();  // 内存管理：自动清理过旧的日志
    
    QTextEdit *m_textEdit;
    int m_lineCount = 0;
    static constexpr int MAX_LOG_LINES = 5000;  // 最多保留 5000 行日志（约 2-5MB）
};

// 拦截全局 qWarning, qDebug 输出
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

#endif // DEBUGLOGGER_H
