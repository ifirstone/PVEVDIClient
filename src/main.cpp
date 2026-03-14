#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 应用基本信息
    app.setApplicationName("PVEClient");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("PVEClient");

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
