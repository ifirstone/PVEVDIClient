#include "DebugLogger.h"
#include <QDateTime>
#include <QMetaObject>

DebugLogger& DebugLogger::instance() {
    static DebugLogger inst;
    return inst;
}

DebugLogger::DebugLogger(QWidget *parent) : QDialog(parent) {
    setWindowTitle("PVEClient 实时调试与交互控制台");
    // 强制此窗口在全屏模式或 Linux WM 下悬浮于其他窗口之上
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    resize(800, 500);
    
    // 设置深色调的黑客风格代码窗
    setStyleSheet("QDialog { background-color: #0d1117; }");
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setStyleSheet("background: #0d1117; color: #00ff00; font-family: 'Consolas', monospace; font-size: 13px; border: 1px solid #30363d; padding: 5px;");
    layout->addWidget(m_textEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    QPushButton *btnClear = new QPushButton("清空日志", this);
    btnClear->setStyleSheet("QPushButton { color: white; background: #238636; border: none; border-radius: 4px; padding: 6px 14px; margin-top: 5px; } QPushButton:hover { background: #2ea043; }");
    connect(btnClear, &QPushButton::clicked, m_textEdit, &QTextEdit::clear);
    
    btnLayout->addWidget(btnClear);
    layout->addLayout(btnLayout);
}

void DebugLogger::appendLog(QtMsgType type, const QString &msg) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString typeStr;
    QString color;
    switch(type) {
        case QtDebugMsg: typeStr = "[DBG]"; color = "#8b949e"; break;
        case QtInfoMsg:  typeStr = "[INF]"; color = "#58a6ff"; break;
        case QtWarningMsg: typeStr = "[WRN]"; color = "#f2cc60"; break;
        case QtCriticalMsg: typeStr = "[ERR]"; color = "#f85149"; break;
        case QtFatalMsg: typeStr = "[FATAL]"; color = "#ff7b72"; break;
    }
    QString html = QString("<pre style='margin:1px;'><font color=\"%1\">%2 %3: %4</font></pre>").arg(color, timeStr, typeStr, msg.toHtmlEscaped());
    m_textEdit->append(html);
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    // 依然保留真实的控制台输出以便在原生终端下查看
    QByteArray localMsg = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", localMsg.constData());
    
    // 跨线程安全地追加文本到单例界面的富文本框
    QMetaObject::invokeMethod(&DebugLogger::instance(), "appendLog", Qt::QueuedConnection,
                              Q_ARG(QtMsgType, type), Q_ARG(QString, msg));
}
