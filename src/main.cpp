#include <QApplication>
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "core/DebugLogger.h"

int main(int argc, char *argv[])
{
    // 安装全局调试控制台日志钩子
    qInstallMessageHandler(customMessageHandler);
    
    QApplication a(argc, argv);

    // 应用基本信息
    a.setApplicationName("PVEClient");
    a.setApplicationVersion("0.1.0");
    a.setOrganizationName("PVEClient");

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    return a.exec();
}
