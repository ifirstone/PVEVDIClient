#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "core/DebugLogger.h"

int main(int argc, char *argv[])
{
    // 安装全局调试控制台日志钩子
    qInstallMessageHandler(customMessageHandler);
    
    QApplication a(argc, argv);

    // 兜底：应用正常退出时写入 user_exit，供外部守护计数。
    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&a]() {
        Q_UNUSED(a);
        QString stateDir = qEnvironmentVariable("PVECLIENT_STATE_DIR");
        if (stateDir.isEmpty()) {
            return;
        }
        QDir().mkpath(stateDir);
        QFile reasonFile(stateDir + "/exit_reason");
        if (reasonFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream out(&reasonFile);
            out << "user_exit";
            reasonFile.close();
        }
    });

    // 应用基本信息
    a.setApplicationName("PVEClient");
    a.setApplicationVersion("0.1.0");
    a.setOrganizationName("PVEClient");

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    return a.exec();
}
